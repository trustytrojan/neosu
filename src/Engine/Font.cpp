//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		freetype font wrapper with unicode support
//
// $NoKeywords: $fnt
//===============================================================================//

#include "Font.h"

#include <freetype/freetype.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <ft2build.h>

#include <algorithm>
#include <utility>

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "FontTypeMap.h"
#include "ResourceManager.h"
#include "TextureAtlas.h"

// TODO: use fontconfig on linux?
#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

// constants for atlas generation and rendering
static constexpr float ATLAS_OCCUPANCY_TARGET = 0.75f;  // target atlas occupancy before resize
static constexpr size_t MIN_ATLAS_SIZE = 256;
static constexpr size_t MAX_ATLAS_SIZE = 4096;
static constexpr wchar_t UNKNOWN_CHAR = '?';  // ASCII '?'

static constexpr auto VERTS_PER_VAO = Env::cfg(REND::GLES2 | REND::GLES32) ? 6 : 4;

// static member definitions
FT_Library McFont::s_sharedFtLibrary = nullptr;
bool McFont::s_sharedFtLibraryInitialized = false;
std::vector<McFont::FallbackFont> McFont::s_sharedFallbackFonts;
bool McFont::s_sharedFallbacksInitialized = false;

McFont::McFont(const UString &filepath, int fontSize, bool antialiasing, int fontDPI)
    : Resource(filepath.toUtf8()),
      vao((Env::cfg(REND::GLES2 | REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES
                                                : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
          Graphics::USAGE_TYPE::USAGE_DYNAMIC) {
    std::vector<wchar_t> characters;
    characters.reserve(96);  // reserve space for basic ASCII, load the rest as needed
    for(int i = 32; i < 128; i++) {
        characters.push_back(static_cast<wchar_t>(i));
    }
    constructor(characters, fontSize, antialiasing, fontDPI);
}

McFont::McFont(const UString &filepath, const std::vector<wchar_t> &characters, int fontSize, bool antialiasing,
               int fontDPI)
    : Resource(filepath.toUtf8()),
      vao((Env::cfg(REND::GLES2 | REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES
                                                : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
          Graphics::USAGE_TYPE::USAGE_DYNAMIC) {
    constructor(characters, fontSize, antialiasing, fontDPI);
}

void McFont::constructor(const std::vector<wchar_t> &characters, int fontSize, bool antialiasing, int fontDPI) {
    this->iFontSize = fontSize;
    this->bAntialiasing = antialiasing;
    this->iFontDPI = fontDPI;
    this->textureAtlas = nullptr;
    this->fHeight = 1.0f;
    this->batchActive = false;
    this->batchQueue.totalVerts = 0;
    this->batchQueue.usedEntries = 0;

    // per-instance freetype initialization state
    this->ftFace = nullptr;
    this->bFreeTypeInitialized = false;
    this->bAtlasNeedsRebuild = false;

    // setup error glyph
    this->errorGlyph = {.character = UNKNOWN_CHAR,
                        .uvPixelsX = 10,
                        .uvPixelsY = 1,
                        .sizePixelsX = 1,
                        .sizePixelsY = 0,
                        .left = 0,
                        .top = 10,
                        .width = 10,
                        .rows = 1,
                        .advance_x = 0,
                        .fontIndex = 0};

    // pre-allocate space for initial glyphs
    this->vGlyphs.reserve(characters.size());
    for(wchar_t ch : characters) {
        addGlyph(ch);
    }
}

void McFont::init() {
    debugLogF("Resource Manager: Loading {:s}\n", this->sFilePath);

    if(!initializeFreeType()) return;

    // setup the static fallbacks once
    initializeSharedFallbackFonts();

    // load metrics for all initial glyphs
    for(wchar_t ch : this->vGlyphs) {
        loadGlyphMetrics(ch);
    }

    // create atlas and render all glyphs
    if(!createAndPackAtlas(this->vGlyphs)) return;

    // precalculate average/max ASCII glyph height
    this->fHeight = 0.0f;
    for(int i = 32; i < 128; i++) {
        const int curHeight = getGlyphMetrics(static_cast<wchar_t>(i)).top;
        this->fHeight = std::max(this->fHeight, static_cast<float>(curHeight));
    }

    this->bReady = true;
}

bool McFont::loadGlyphDynamic(wchar_t ch) {
    if(!this->bFreeTypeInitialized || hasGlyph(ch)) return hasGlyph(ch);

    int fontIndex = 0;
    FT_Face targetFace = getFontFaceForGlyph(ch, fontIndex);

    if(!targetFace) {
        if(cv_r_debug_font_unicode.getBool()) {
            // character not supported by any available font
            const char *charRange = FontTypeMap::getCharacterRangeName(ch);
            if(charRange)
                debugLogF("Font Warning: Character U+{:04X} ({:s}) not supported by any font\n", (unsigned int)ch,
                          charRange);
            else
                debugLogF("Font Warning: Character U+{:04X} not supported by any font\n", (unsigned int)ch);
        }
        return false;
    }

    if(cv_r_debug_font_unicode.getBool() && fontIndex > 0)
        debugLogF("Font Info: Using fallback font #{:d} for character U+{:04X}\n", fontIndex, (unsigned int)ch);

    // load glyph from the selected font face
    if(!loadGlyphFromFace(ch, targetFace, fontIndex)) return false;

    const auto &metrics = this->vGlyphMetrics[ch];

    // check if we need atlas space for non-empty glyphs
    if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
        const int padding = cv_r_debug_font_atlas_padding.getInt();
        const int requiredWidth = metrics.sizePixelsX + padding;
        const int requiredHeight = metrics.sizePixelsY + padding;

        if(!ensureAtlasSpace(requiredWidth, requiredHeight)) {
            // atlas is full, queue for rebuild
            this->vPendingGlyphs.push_back(ch);
            this->bAtlasNeedsRebuild = true;
            addGlyph(ch);
            return true;  // glyph metrics are stored, will be packed later
        }

        // try to pack into current atlas
        std::vector<TextureAtlas::PackRect> singleRect = {{.x = 0,
                                                           .y = 0,
                                                           .width = static_cast<int>(metrics.sizePixelsX),
                                                           .height = static_cast<int>(metrics.sizePixelsY),
                                                           .id = 0}};

        if(this->textureAtlas->packRects(singleRect)) {
            // successfully packed, render to atlas using the correct font face
            renderGlyphToAtlas(ch, singleRect[0].x, singleRect[0].y, targetFace);
        } else {
            // couldn't pack into current atlas, queue for rebuild
            this->vPendingGlyphs.push_back(ch);
            this->bAtlasNeedsRebuild = true;
        }
    }

    addGlyph(ch);
    return true;
}

FT_Face McFont::getFontFaceForGlyph(wchar_t ch, int &fontIndex) {
    // check primary font first
    fontIndex = 0;
    FT_UInt glyphIndex = FT_Get_Char_Index(this->ftFace, ch);
    if(glyphIndex != 0) return this->ftFace;

    // search through shared fallback fonts if initialized
    if(s_sharedFallbacksInitialized) {
        for(size_t i = 0; i < s_sharedFallbackFonts.size(); ++i) {
            glyphIndex = FT_Get_Char_Index(s_sharedFallbackFonts[i].face, ch);
            if(glyphIndex != 0) {
                fontIndex = static_cast<int>(i + 1);  // offset by 1 since 0 is primary font
                // set the appropriate size for this font instance
                setFaceSize(s_sharedFallbackFonts[i].face);
                return s_sharedFallbackFonts[i].face;
            }
        }
    }

    return nullptr;  // character not found in any font
}

bool McFont::loadGlyphFromFace(wchar_t ch, FT_Face face, int fontIndex) {
    if(FT_Load_Glyph(face, FT_Get_Char_Index(face, ch),
                     this->bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO)) {
        debugLogF("Font Error: Failed to load glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
        return false;
    }

    FT_Glyph glyph{};
    if(FT_Get_Glyph(face->glyph, &glyph)) {
        debugLogF("Font Error: Failed to get glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
        return false;
    }

    FT_Glyph_To_Bitmap(&glyph, this->bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

    auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

    // store metrics for the character
    GLYPH_METRICS &metrics = this->vGlyphMetrics[ch];
    metrics.character = ch;
    metrics.left = bitmapGlyph->left;
    metrics.top = bitmapGlyph->top;
    metrics.width = bitmapGlyph->bitmap.width;
    metrics.rows = bitmapGlyph->bitmap.rows;
    metrics.advance_x = static_cast<float>(face->glyph->advance.x >> 6);
    metrics.fontIndex = fontIndex;

    // initialize texture coordinates (will be updated when rendered to atlas)
    metrics.uvPixelsX = 0;
    metrics.uvPixelsY = 0;
    metrics.sizePixelsX = bitmapGlyph->bitmap.width;
    metrics.sizePixelsY = bitmapGlyph->bitmap.rows;

    FT_Done_Glyph(glyph);
    return true;
}

bool McFont::initializeSharedFreeType() {
    if(s_sharedFtLibraryInitialized) return true;

    if(FT_Init_FreeType(&s_sharedFtLibrary)) {
        engine->showMessageError("Font Error", "FT_Init_FreeType() failed!");
        return false;
    }

    s_sharedFtLibraryInitialized = true;
    return true;
}

bool McFont::initializeSharedFallbackFonts() {
    if(s_sharedFallbacksInitialized) return true;

    // make sure shared freetype library is initialized first
    if(!initializeSharedFreeType()) return false;

    // check all bundled fonts first
    std::vector<std::string> bundledFallbacks = env->getFilesInFolder(ResourceManager::PATH_DEFAULT_FONTS);

    for(const auto &fontName : bundledFallbacks) {
        if(loadFallbackFont(UString{fontName}, false)) {
            if(cv_r_debug_font_unicode.getBool())
                debugLogF("Font Info: Loaded bundled fallback font: {:s}\n", fontName);
        }
    }

    // then find likely system fonts
    if(!Env::cfg(OS::WASM) && cv_font_load_system.getBool()) discoverSystemFallbacks();

    s_sharedFallbacksInitialized = true;
    return true;
}

void McFont::discoverSystemFallbacks() {
#ifdef MCENGINE_PLATFORM_WINDOWS
    wchar_t windir[MAX_PATH];
    if(GetWindowsDirectoryW(windir, MAX_PATH) <= 0) return;

    std::vector<UString> systemFonts = {
        UString{windir} + "\\Fonts\\arial.ttf",    UString{windir} + "\\Fonts\\msyh.ttc",  // Microsoft YaHei (Chinese)
        UString{windir} + "\\Fonts\\malgun.ttf",                                           // Malgun Gothic (Korean)
        UString{windir} + "\\Fonts\\meiryo.ttc",                                           // Meiryo (Japanese)
        UString{windir} + "\\Fonts\\seguiemj.ttf",                                         // Segoe UI Emoji
        UString{windir} + "\\Fonts\\seguisym.ttf"                                          // Segoe UI Symbol
    };
#elif defined(MCENGINE_PLATFORM_LINUX)
    // linux system fonts (common locations)
    std::vector<UString> systemFonts = {"/usr/share/fonts/TTF/dejavu/DejaVuSans.ttf",
                                        "/usr/share/fonts/TTF/liberation/LiberationSans-Regular.ttf",
                                        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
                                        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
                                        "/usr/share/fonts/TTF/noto/NotoColorEmoji.ttf"};
#else  // TODO: loading WOFF fonts in wasm? idk
    std::vector<UString> systemFonts;
    return;
#endif

    for(const auto &fontPath : systemFonts) {
        if(File::exists(fontPath.toUtf8()) == File::FILETYPE::FILE) {
            loadFallbackFont(fontPath, true);
        }
    }
}

bool McFont::loadFallbackFont(const UString &fontPath, bool isSystemFont) {
    FT_Face face{};
    if(FT_New_Face(s_sharedFtLibrary, fontPath.toUtf8(), 0, &face)) {
        if(cv_r_debug_font_unicode.getBool()) debugLogF("Font Warning: Failed to load fallback font: {:s}\n", fontPath);
        return false;
    }

    if(FT_Select_Charmap(face, ft_encoding_unicode)) {
        if(cv_r_debug_font_unicode.getBool())
            debugLogF("Font Warning: Failed to select unicode charmap for fallback font: {:s}\n", fontPath);
        FT_Done_Face(face);
        return false;
    }

    // don't set font size here, will be set when the face is used by individual font instances
    s_sharedFallbackFonts.push_back({fontPath, face, isSystemFont});
    return true;
}

void McFont::setFaceSize(FT_Face face) const {
    FT_Set_Char_Size(face, this->iFontSize * 64, this->iFontSize * 64, this->iFontDPI, this->iFontDPI);
}

bool McFont::ensureAtlasSpace(int requiredWidth, int requiredHeight) {
    if(!this->textureAtlas) return false;

    // simple heuristic: check if there's roughly enough space left
    // this is not perfect but avoids complex packing simulation
    const int atlasWidth = this->textureAtlas->getWidth();
    const int atlasHeight = this->textureAtlas->getHeight();
    const float occupiedRatio = static_cast<float>(this->vGlyphMetrics.size()) / 100.0f;  // rough estimation

    return (requiredWidth < atlasWidth / 4 && requiredHeight < atlasHeight / 4 && occupiedRatio < 0.8f);
}

void McFont::rebuildAtlas() {
    if(!this->bAtlasNeedsRebuild || this->vPendingGlyphs.empty()) return;

    // collect all glyphs that need to be in the atlas
    std::vector<wchar_t> allGlyphs;
    allGlyphs.reserve(this->vGlyphMetrics.size());

    // add existing glyphs with non-empty size
    for(const auto &[ch, metrics] : this->vGlyphMetrics) {
        if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
            allGlyphs.push_back(ch);
        }
    }

    // destroy old atlas
    SAFE_DELETE(this->textureAtlas);

    // create new atlas and render all glyphs
    if(!createAndPackAtlas(allGlyphs)) {
        debugLogF("Font Error: Failed to rebuild atlas!\n");
        return;
    }

    this->vPendingGlyphs.clear();
    this->bAtlasNeedsRebuild = false;
}

bool McFont::initializeFreeType() {
    // initialize shared freetype library
    if(!initializeSharedFreeType()) return false;

    // load this font's primary face
    if(FT_New_Face(s_sharedFtLibrary, this->sFilePath.c_str(), 0, &this->ftFace)) {
        engine->showMessageError("Font Error", "Couldn't load font file!");
        return false;
    }

    if(FT_Select_Charmap(this->ftFace, ft_encoding_unicode)) {
        engine->showMessageError("Font Error", "FT_Select_Charmap() failed!");
        FT_Done_Face(this->ftFace);
        return false;
    }

    // set font size for this instance's primary face
    setFaceSize(this->ftFace);

    this->bFreeTypeInitialized = true;
    return true;
}

bool McFont::loadGlyphMetrics(wchar_t ch) {
    if(!this->bFreeTypeInitialized) return false;

    int fontIndex = 0;
    FT_Face face = getFontFaceForGlyph(ch, fontIndex);

    if(!face) return false;

    return loadGlyphFromFace(ch, face, fontIndex);
}

std::unique_ptr<Color[]> McFont::createExpandedBitmapData(const FT_Bitmap &bitmap) {
    std::unique_ptr<Color[]> expandedData(new Color[bitmap.width * bitmap.rows]);

    std::unique_ptr<Channel[]> monoBitmapUnpacked;
    if(!this->bAntialiasing) monoBitmapUnpacked.reset(unpackMonoBitmap(bitmap));

    for(unsigned int j = 0; j < bitmap.rows; j++) {
        for(unsigned int k = 0; k < bitmap.width; k++) {
            const size_t expandedIdx = (k + (bitmap.rows - j - 1) * bitmap.width);

            Channel alpha = this->bAntialiasing                            ? bitmap.buffer[k + bitmap.width * j]
                            : monoBitmapUnpacked[k + bitmap.width * j] > 0 ? 255
                                                                           : 0;
            expandedData[expandedIdx] = Color(alpha, 0xff, 0xff, 0xff);  // ARGB
        }
    }

    return expandedData;
}

void McFont::renderGlyphToAtlas(wchar_t ch, int x, int y, FT_Face face) {
    if(!face) {
        // fall back to getting the face again if not provided
        int fontIndex = 0;
        face = getFontFaceForGlyph(ch, fontIndex);
        if(!face) return;
    } else if(face != this->ftFace) {
        // make sure fallback face has the correct size for this font instance
        setFaceSize(face);
    }

    if(FT_Load_Glyph(face, FT_Get_Char_Index(face, ch),
                     this->bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
        return;

    FT_Glyph glyph{};
    if(FT_Get_Glyph(face->glyph, &glyph)) return;

    FT_Glyph_To_Bitmap(&glyph, this->bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

    auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
    auto &bitmap = bitmapGlyph->bitmap;

    if(bitmap.width > 0 && bitmap.rows > 0) {
        auto expandedData = createExpandedBitmapData(bitmap);
        this->textureAtlas->putAt(x, y, bitmap.width, bitmap.rows, false, true, expandedData.get());

        // update metrics with atlas coordinates
        GLYPH_METRICS &metrics = this->vGlyphMetrics[ch];
        metrics.uvPixelsX = static_cast<unsigned int>(x);
        metrics.uvPixelsY = static_cast<unsigned int>(y);
    }

    FT_Done_Glyph(glyph);
}

bool McFont::createAndPackAtlas(const std::vector<wchar_t> &glyphs) {
    if(glyphs.empty()) return true;

    // prepare packing rectangles
    std::vector<TextureAtlas::PackRect> packRects;
    std::vector<wchar_t> rectsToChars;
    packRects.reserve(glyphs.size());
    rectsToChars.reserve(glyphs.size());

    size_t rectIndex = 0;
    for(wchar_t ch : glyphs) {
        const auto &metrics = this->vGlyphMetrics[ch];
        if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
            packRects.push_back({0, 0, static_cast<int>(metrics.sizePixelsX), static_cast<int>(metrics.sizePixelsY),
                                 static_cast<int>(rectIndex)});
            rectsToChars.push_back(ch);
            rectIndex++;
        }
    }

    if(packRects.empty()) return true;

    // calculate optimal atlas size and create texture atlas
    const int padding = cv_r_debug_font_atlas_padding.getInt();
    const size_t atlasSize =
        TextureAtlas::calculateOptimalSize(packRects, ATLAS_OCCUPANCY_TARGET, padding, MIN_ATLAS_SIZE, MAX_ATLAS_SIZE);

    resourceManager->requestNextLoadUnmanaged();
    this->textureAtlas = resourceManager->createTextureAtlas(atlasSize, atlasSize);
    this->textureAtlas->setPadding(padding);

    // pack glyphs into atlas
    if(!this->textureAtlas->packRects(packRects)) {
        engine->showMessageError("Font Error", "Failed to pack glyphs into atlas!");
        return false;
    }

    // render all packed glyphs to atlas
    for(const auto &rect : packRects) {
        const wchar_t ch = rectsToChars[rect.id];

        // get the correct font face for this glyph
        int fontIndex = 0;
        FT_Face face = getFontFaceForGlyph(ch, fontIndex);

        renderGlyphToAtlas(ch, rect.x, rect.y, face);
    }

    // finalize atlas texture
    resourceManager->loadResource(this->textureAtlas);
    this->textureAtlas->getAtlasImage()->setFilterMode(this->bAntialiasing ? Graphics::FILTER_MODE::FILTER_MODE_LINEAR
                                                                           : Graphics::FILTER_MODE::FILTER_MODE_NONE);

    return true;
}

void McFont::buildGlyphGeometry(const GLYPH_METRICS &gm, const Vector3 &basePos, float advanceX, size_t &vertexCount) {
    const float atlasWidth = static_cast<float>(this->textureAtlas->getAtlasImage()->getWidth());
    const float atlasHeight = static_cast<float>(this->textureAtlas->getAtlasImage()->getHeight());

    const float x = basePos.x + gm.left + advanceX;
    const float y = basePos.y - (gm.top - gm.rows);
    const float z = basePos.z;
    const float sx = gm.width;
    const float sy = -gm.rows;

    const float texX = static_cast<float>(gm.uvPixelsX) / atlasWidth;
    const float texY = static_cast<float>(gm.uvPixelsY) / atlasHeight;
    const float texSizeX = static_cast<float>(gm.sizePixelsX) / atlasWidth;
    const float texSizeY = static_cast<float>(gm.sizePixelsY) / atlasHeight;

    // corners of the "quad"
    Vector3 bottomLeft = Vector3(x, y + sy, z);
    Vector3 topLeft = Vector3(x, y, z);
    Vector3 topRight = Vector3(x + sx, y, z);
    Vector3 bottomRight = Vector3(x + sx, y + sy, z);

    // texcoords
    Vector2 texBottomLeft = Vector2(texX, texY);
    Vector2 texTopLeft = Vector2(texX, texY + texSizeY);
    Vector2 texTopRight = Vector2(texX + texSizeX, texY + texSizeY);
    Vector2 texBottomRight = Vector2(texX + texSizeX, texY);

    const size_t idx = vertexCount;

    if constexpr(Env::cfg(REND::GLES2 | REND::GLES32)) {
        // first triangle (bottom-left, top-left, top-right)
        this->vertices[idx] = bottomLeft;
        this->vertices[idx + 1] = topLeft;
        this->vertices[idx + 2] = topRight;

        this->texcoords[idx] = texBottomLeft;
        this->texcoords[idx + 1] = texTopLeft;
        this->texcoords[idx + 2] = texTopRight;

        // second triangle (bottom-left, top-right, bottom-right)
        this->vertices[idx + 3] = bottomLeft;
        this->vertices[idx + 4] = topRight;
        this->vertices[idx + 5] = bottomRight;

        this->texcoords[idx + 3] = texBottomLeft;
        this->texcoords[idx + 4] = texTopRight;
        this->texcoords[idx + 5] = texBottomRight;
    } else {
        this->vertices[idx] = bottomLeft;       // bottom-left
        this->vertices[idx + 1] = topLeft;      // top-left
        this->vertices[idx + 2] = topRight;     // top-right
        this->vertices[idx + 3] = bottomRight;  // bottom-right

        this->texcoords[idx] = texBottomLeft;
        this->texcoords[idx + 1] = texTopLeft;
        this->texcoords[idx + 2] = texTopRight;
        this->texcoords[idx + 3] = texBottomRight;
    }
    vertexCount += VERTS_PER_VAO;
}

void McFont::buildStringGeometry(const UString &text, size_t &vertexCount) {
    if(!this->bReady || text.length() == 0 || text.length() > cv_r_drawstring_max_string_length.getInt()) return;

    // check if atlas needs rebuilding before geometry generation
    if(this->bAtlasNeedsRebuild) rebuildAtlas();

    float advanceX = 0.0f;
    const size_t maxGlyphs = std::min(text.length(), (int)(this->vertices.size() - vertexCount) / VERTS_PER_VAO);

    for(size_t i = 0; i < maxGlyphs; i++) {
        const GLYPH_METRICS &gm = getGlyphMetrics(text[i]);
        buildGlyphGeometry(gm, Vector3(), advanceX, vertexCount);
        advanceX += gm.advance_x;
    }
}

void McFont::drawString(const UString &text) {
    if(!this->bReady) return;

    const int maxNumGlyphs = cv_r_drawstring_max_string_length.getInt();
    if(text.length() == 0 || text.length() > maxNumGlyphs) return;

    this->vao.empty();

    const size_t totalVerts = text.length() * VERTS_PER_VAO;
    this->vertices.resize(totalVerts);
    this->texcoords.resize(totalVerts);

    size_t vertexCount = 0;
    buildStringGeometry(text, vertexCount);

    for(size_t i = 0; i < vertexCount; i++) {
        this->vao.addVertex(this->vertices[i]);
        this->vao.addTexcoord(this->texcoords[i]);
    }

    this->textureAtlas->getAtlasImage()->bind();
    g->drawVAO(&this->vao);

    if(cv_r_debug_drawstring_unbind.getBool()) this->textureAtlas->getAtlasImage()->unbind();
}

void McFont::beginBatch() {
    this->batchActive = true;
    this->batchQueue.totalVerts = 0;
    this->batchQueue.usedEntries = 0;  // don't clear/reallocate, reuse the entries instead
}

void McFont::addToBatch(const UString &text, const Vector3 &pos, Color color) {
    size_t verts{};
    if(!this->batchActive || (verts = text.length() * VERTS_PER_VAO) == 0) return;
    this->batchQueue.totalVerts += verts;

    if(this->batchQueue.usedEntries < this->batchQueue.entryList.size()) {
        // reuse existing entry
        BatchEntry &entry = this->batchQueue.entryList[this->batchQueue.usedEntries];
        entry.text = text;
        entry.pos = pos;
        entry.color = color;
    } else {
        // need to add new entry
        this->batchQueue.entryList.push_back({text, pos, color});
    }
    this->batchQueue.usedEntries++;
}

void McFont::flushBatch() {
    if(!this->batchActive || !this->batchQueue.totalVerts) {
        this->batchActive = false;
        return;
    }

    this->vertices.resize(this->batchQueue.totalVerts);
    this->texcoords.resize(this->batchQueue.totalVerts);
    this->vao.empty();

    size_t currentVertex = 0;
    for(size_t i = 0; i < this->batchQueue.usedEntries; i++) {
        const auto &entry = this->batchQueue.entryList[i];
        const size_t stringStart = currentVertex;
        buildStringGeometry(entry.text, currentVertex);

        for(size_t j = stringStart; j < currentVertex; j++) {
            this->vertices[j] += entry.pos;
            this->vao.addVertex(this->vertices[j]);
            this->vao.addTexcoord(this->texcoords[j]);
            this->vao.addColor(entry.color);
        }
    }

    this->textureAtlas->getAtlasImage()->bind();

    g->drawVAO(&this->vao);

    if(cv_r_debug_drawstring_unbind.getBool()) this->textureAtlas->getAtlasImage()->unbind();

    this->batchActive = false;
}

float McFont::getStringWidth(const UString &text) const {
    if(!this->bReady) return 1.0f;

    float width = 0.0f;
    for(int i = 0; i < text.length(); i++) {
        width += getGlyphMetrics(text[i]).advance_x;
    }
    return width;
}

float McFont::getStringHeight(const UString &text) const {
    if(!this->bReady) return 1.0f;

    float height = 0.0f;
    for(int i = 0; i < text.length(); i++) {
        height = std::max(height, static_cast<float>(getGlyphMetrics(text[i]).top));
    }
    return height;
}

std::vector<UString> McFont::wrap(const UString &text, f64 max_width) const {
    std::vector<UString> lines;
    lines.emplace_back();

    UString word = "";
    u32 line = 0;
    f64 line_width = 0.0;
    f64 word_width = 0.0;
    for(int i = 0; i < text.length(); i++) {
        if(text[i] == '\n') {
            lines[line].append(word);
            lines.emplace_back();
            line++;
            line_width = 0.0;
            word = "";
            word_width = 0.0;
            continue;
        }

        f32 char_width = getGlyphMetrics(text[i]).advance_x;

        if(text[i] == ' ') {
            lines[line].append(word);
            line_width += word_width;
            word = "";
            word_width = 0.0;

            if(line_width + char_width > max_width) {
                // Ignore spaces at the end of a line
                lines.emplace_back();
                line++;
                line_width = 0.0;
            } else if(line_width > 0.0) {
                lines[line].append(' ');
                line_width += char_width;
            }
        } else {
            if(word_width + char_width > max_width) {
                // Split word onto new line
                lines[line].append(word);
                lines.emplace_back();
                line++;
                line_width = 0.0;
                word = "";
                word_width = 0.0;
            } else if(line_width + word_width + char_width > max_width) {
                // Wrap word on new line
                lines.emplace_back();
                line++;
                line_width = 0.0;
                word.append(text[i]);
                word_width += char_width;
            } else {
                // Add character to word
                word.append(text[i]);
                word_width += char_width;
            }
        }
    }

    // Don't forget! ;)
    lines[line].append(word);

    return lines;
}

const McFont::GLYPH_METRICS &McFont::getGlyphMetrics(wchar_t ch) const {
    auto it = this->vGlyphMetrics.find(ch);
    if(it != this->vGlyphMetrics.end()) return it->second;

    // attempt dynamic loading for unicode characters
    if(const_cast<McFont *>(this)->loadGlyphDynamic(ch)) {
        it = this->vGlyphMetrics.find(ch);
        if(it != this->vGlyphMetrics.end()) return it->second;
    }

    // fallback to unknown character glyph
    it = this->vGlyphMetrics.find(UNKNOWN_CHAR);
    if(it != this->vGlyphMetrics.end()) return it->second;

    debugLogF("Font Error: Missing default backup glyph (UNKNOWN_CHAR)?\n");
    return this->errorGlyph;
}

void McFont::initAsync() { this->bAsyncReady = true; }

void McFont::destroy() {
    // only clean up per-instance resources (primary font face and atlas)
    // shared resources are cleaned up separately via cleanupSharedResources()

    if(this->bFreeTypeInitialized) {
        if(this->ftFace) {
            FT_Done_Face(this->ftFace);
            this->ftFace = nullptr;
        }
        this->bFreeTypeInitialized = false;
    }

    SAFE_DELETE(this->textureAtlas);
    this->vGlyphMetrics.clear();
    this->vPendingGlyphs.clear();
    this->fHeight = 1.0f;
    this->bAtlasNeedsRebuild = false;
}

void McFont::cleanupSharedResources() {
    // clean up shared fallback fonts
    for(auto &fallbackFont : s_sharedFallbackFonts) {
        if(fallbackFont.face) FT_Done_Face(fallbackFont.face);
    }
    s_sharedFallbackFonts.clear();
    s_sharedFallbacksInitialized = false;

    // clean up shared freetype library
    if(s_sharedFtLibraryInitialized) {
        if(s_sharedFtLibrary) {
            FT_Done_FreeType(s_sharedFtLibrary);
            s_sharedFtLibrary = nullptr;
        }
        s_sharedFtLibraryInitialized = false;
    }
}

bool McFont::addGlyph(wchar_t ch) {
    if(this->vGlyphExistence.find(ch) != this->vGlyphExistence.end() || ch < 32) return false;

    this->vGlyphs.push_back(ch);
    this->vGlyphExistence[ch] = true;
    return true;
}

Channel *McFont::unpackMonoBitmap(const FT_Bitmap &bitmap) {
    auto result = new Channel[bitmap.rows * bitmap.width];

    for(int y = 0; std::cmp_less(y, bitmap.rows); y++) {
        for(int byteIdx = 0; byteIdx < bitmap.pitch; byteIdx++) {
            const unsigned char byteValue = bitmap.buffer[y * bitmap.pitch + byteIdx];
            const int numBitsDone = byteIdx * 8;
            const int rowstart = y * bitmap.width + byteIdx * 8;

            const int bits = std::min(8, static_cast<int>(bitmap.width - numBitsDone));

            for(int bitIdx = 0; bitIdx < bits; bitIdx++) {
                result[rowstart + bitIdx] = (byteValue & (1 << (7 - bitIdx))) ? 1 : 0;
            }
        }
    }

    return result;
}
