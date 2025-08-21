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
#include "WinDebloatDefs.h"
#include <shlobj.h>
#include <windows.h>
#endif

// constants for atlas generation and rendering
static constexpr const float ATLAS_OCCUPANCY_TARGET{0.75f};  // target atlas occupancy before resize
static constexpr const size_t MIN_ATLAS_SIZE{256};
static constexpr const size_t MAX_ATLAS_SIZE{4096};
static constexpr const wchar_t UNKNOWN_CHAR{L'?'};  // ASCII '?'

static constexpr const size_t VERTS_PER_VAO{Env::cfg(REND::GLES32) ? 6 : 4};

// static member definitions
FT_Library McFont::s_sharedFtLibrary = nullptr;
bool McFont::s_sharedFtLibraryInitialized = false;
std::vector<McFont::FallbackFont> McFont::s_sharedFallbackFonts;
bool McFont::s_sharedFallbacksInitialized = false;

McFont::McFont(std::string filepath, int fontSize, bool antialiasing, int fontDPI)
    : Resource(std::move(filepath)),
      m_vao((Env::cfg(REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
            Graphics::USAGE_TYPE::USAGE_DYNAMIC) {
    std::vector<wchar_t> characters;
    characters.reserve(96);  // reserve space for basic ASCII, load the rest as needed
    for(int i = 32; i < 128; i++) {
        characters.push_back(static_cast<wchar_t>(i));
    }
    constructor(characters, fontSize, antialiasing, fontDPI);
}

McFont::McFont(std::string filepath, const std::vector<wchar_t> &characters, int fontSize, bool antialiasing,
               int fontDPI)
    : Resource(std::move(filepath)),
      m_vao((Env::cfg(REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
            Graphics::USAGE_TYPE::USAGE_DYNAMIC) {
    constructor(characters, fontSize, antialiasing, fontDPI);
}

void McFont::constructor(const std::vector<wchar_t> &characters, int fontSize, bool antialiasing, int fontDPI) {
    m_iFontSize = fontSize;
    m_bAntialiasing = antialiasing;
    m_iFontDPI = fontDPI;
    m_textureAtlas = nullptr;
    m_fHeight = 1.0f;
    m_batchActive = false;
    m_batchQueue.totalVerts = 0;
    m_batchQueue.usedEntries = 0;

    // per-instance freetype initialization state
    m_ftFace = nullptr;
    m_bFreeTypeInitialized = false;
    m_bAtlasNeedsRebuild = false;

    // setup error glyph
    m_errorGlyph = {.character = UNKNOWN_CHAR,
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
    m_vGlyphs.reserve(characters.size());
    for(wchar_t ch : characters) {
        addGlyph(ch);
    }
}

void McFont::init() {
    debugLog("Loading font: {:s}\n", this->sFilePath);

    if(!initializeFreeType()) return;

    // setup the static fallbacks once
    initializeSharedFallbackFonts();

    // load metrics for all initial glyphs
    for(wchar_t ch : m_vGlyphs) {
        loadGlyphMetrics(ch);
    }

    // create atlas and render all glyphs
    if(!createAndPackAtlas(m_vGlyphs)) return;

    // precalculate average/max ASCII glyph height
    m_fHeight = 0.0f;
    for(int i = 32; i < 128; i++) {
        const int curHeight = getGlyphMetrics(static_cast<wchar_t>(i)).top;
        m_fHeight = std::max(m_fHeight, static_cast<float>(curHeight));
    }

    this->bReady = true;
}

void McFont::initAsync() { this->bAsyncReady = true; }

void McFont::destroy() {
    // only clean up per-instance resources (primary font face and atlas)
    // shared resources are cleaned up separately via cleanupSharedResources()

    if(m_bFreeTypeInitialized) {
        if(m_ftFace) {
            FT_Done_Face(m_ftFace);
            m_ftFace = nullptr;
        }
        m_bFreeTypeInitialized = false;
    }

    SAFE_DELETE(m_textureAtlas);
    m_vGlyphMetrics.clear();
    m_vPendingGlyphs.clear();
    m_fHeight = 1.0f;
    m_bAtlasNeedsRebuild = false;
}

bool McFont::addGlyph(wchar_t ch) {
    if(m_vGlyphExistence.find(ch) != m_vGlyphExistence.end() || ch < 32) return false;

    m_vGlyphs.push_back(ch);
    m_vGlyphExistence[ch] = true;
    return true;
}

bool McFont::loadGlyphDynamic(wchar_t ch) {
    if(!m_bFreeTypeInitialized || hasGlyph(ch)) return hasGlyph(ch);

    int fontIndex = 0;
    FT_Face targetFace = getFontFaceForGlyph(ch, fontIndex);

    if(!targetFace) {
        if(cv::r_debug_font_unicode.getBool()) {
            // character not supported by any available font
            const char *charRange = FontTypeMap::getCharacterRangeName(ch);
            if(charRange)
                debugLog("Font Warning: Character U+{:04X} ({:s}) not supported by any font\n", (unsigned int)ch,
                         charRange);
            else
                debugLog("Font Warning: Character U+{:04X} not supported by any font\n", (unsigned int)ch);
        }
        return false;
    }

    if(cv::r_debug_font_unicode.getBool() && fontIndex > 0)
        debugLog("Font Info: Using fallback font #{:d} for character U+{:04X}\n", fontIndex, (unsigned int)ch);

    // load glyph from the selected font face
    if(!loadGlyphFromFace(ch, targetFace, fontIndex)) return false;

    const auto &metrics = m_vGlyphMetrics[ch];

    // check if we need atlas space for non-empty glyphs
    if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
        const int requiredWidth = metrics.sizePixelsX + TextureAtlas::ATLAS_PADDING;
        const int requiredHeight = metrics.sizePixelsY + TextureAtlas::ATLAS_PADDING;

        if(!ensureAtlasSpace(requiredWidth, requiredHeight)) {
            // atlas is full, queue for rebuild
            m_vPendingGlyphs.push_back(ch);
            m_bAtlasNeedsRebuild = true;
            addGlyph(ch);
            return true;  // glyph metrics are stored, will be packed later
        }

        // try to pack into current atlas
        std::vector<TextureAtlas::PackRect> singleRect = {{.x = 0,
                                                           .y = 0,
                                                           .width = static_cast<int>(metrics.sizePixelsX),
                                                           .height = static_cast<int>(metrics.sizePixelsY),
                                                           .id = 0}};

        if(m_textureAtlas->packRects(singleRect)) {
            // successfully packed, render to atlas using the correct font face
            renderGlyphToAtlas(ch, singleRect[0].x, singleRect[0].y, targetFace);
        } else {
            // couldn't pack into current atlas, queue for rebuild
            m_vPendingGlyphs.push_back(ch);
            m_bAtlasNeedsRebuild = true;
        }
    }

    addGlyph(ch);
    return true;
}

FT_Face McFont::getFontFaceForGlyph(wchar_t ch, int &fontIndex) {
    // check primary font first
    fontIndex = 0;
    FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, ch);
    if(glyphIndex != 0) return m_ftFace;

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
                     m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO)) {
        debugLog("Font Error: Failed to load glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
        return false;
    }

    FT_Glyph glyph{};
    if(FT_Get_Glyph(face->glyph, &glyph)) {
        debugLog("Font Error: Failed to get glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
        return false;
    }

    FT_Glyph_To_Bitmap(&glyph, m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

    auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

    // store metrics for the character
    GLYPH_METRICS &metrics = m_vGlyphMetrics[ch];
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

void McFont::setFaceSize(FT_Face face) const {
    FT_Set_Char_Size(face, m_iFontSize * 64LL, m_iFontSize * 64LL, m_iFontDPI, m_iFontDPI);
}

bool McFont::ensureAtlasSpace(int requiredWidth, int requiredHeight) {
    if(!m_textureAtlas) return false;

    // simple heuristic: check if there's roughly enough space left
    // this is not perfect but avoids complex packing simulation
    const int atlasWidth = m_textureAtlas->getWidth();
    const int atlasHeight = m_textureAtlas->getHeight();
    const float occupiedRatio = static_cast<float>(m_vGlyphMetrics.size()) / 100.0f;  // rough estimation

    return (requiredWidth < atlasWidth / 4 && requiredHeight < atlasHeight / 4 && occupiedRatio < 0.8f);
}

void McFont::rebuildAtlas() {
    if(!m_bAtlasNeedsRebuild || m_vPendingGlyphs.empty()) return;

    // collect all glyphs that need to be in the atlas
    std::vector<wchar_t> allGlyphs;
    allGlyphs.reserve(m_vGlyphMetrics.size());

    // add existing glyphs with non-empty size
    for(const auto &[ch, metrics] : m_vGlyphMetrics) {
        if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
            allGlyphs.push_back(ch);
        }
    }

    // destroy old atlas
    SAFE_DELETE(m_textureAtlas);

    // create new atlas and render all glyphs
    if(!createAndPackAtlas(allGlyphs)) {
        debugLog("Font Error: Failed to rebuild atlas!\n");
        return;
    }

    m_vPendingGlyphs.clear();
    m_bAtlasNeedsRebuild = false;
}

bool McFont::initializeFreeType() {
    // initialize shared freetype library
    if(!initializeSharedFreeType()) return false;

    // load this font's primary face
    if(FT_New_Face(s_sharedFtLibrary, this->sFilePath.c_str(), 0, &m_ftFace)) {
        engine->showMessageError("Font Error", "Couldn't load font file!");
        return false;
    }

    if(FT_Select_Charmap(m_ftFace, ft_encoding_unicode)) {
        engine->showMessageError("Font Error", "FT_Select_Charmap() failed!");
        FT_Done_Face(m_ftFace);
        return false;
    }

    // set font size for this instance's primary face
    setFaceSize(m_ftFace);

    m_bFreeTypeInitialized = true;
    return true;
}

bool McFont::loadGlyphMetrics(wchar_t ch) {
    if(!m_bFreeTypeInitialized) return false;

    int fontIndex = 0;
    FT_Face face = getFontFaceForGlyph(ch, fontIndex);

    if(!face) return false;

    return loadGlyphFromFace(ch, face, fontIndex);
}

std::unique_ptr<Channel[]> McFont::unpackMonoBitmap(const FT_Bitmap &bitmap) {
    auto result = std::make_unique<Channel[]>(static_cast<size_t>(bitmap.rows) * bitmap.width);

    for(u32 y = 0; y < bitmap.rows; y++) {
        for(u32 byteIdx = 0; byteIdx < bitmap.pitch; byteIdx++) {
            const Channel byteValue = bitmap.buffer[y * bitmap.pitch + byteIdx];
            const u32 numBitsDone = byteIdx * 8;
            const u32 rowstart = y * bitmap.width + byteIdx * 8;

            const u32 bits = std::min(8u, bitmap.width - numBitsDone);

            for(u32 bitIdx = 0; bitIdx < bits; bitIdx++) {
                result[rowstart + bitIdx] = (byteValue & (1 << (7 - bitIdx))) ? 1 : 0;
            }
        }
    }

    return result;
}

std::unique_ptr<Color[]> McFont::createExpandedBitmapData(const FT_Bitmap &bitmap) {
    auto expandedData = std::make_unique<Color[]>(static_cast<size_t>(bitmap.width) * bitmap.rows);

    std::unique_ptr<Channel[]> monoBitmapUnpacked{nullptr};
    if(!m_bAntialiasing) monoBitmapUnpacked = unpackMonoBitmap(bitmap);

    for(u32 j = 0; j < bitmap.rows; j++) {
        for(u32 k = 0; k < bitmap.width; k++) {
            const sSz expandedIdx = (k + (bitmap.rows - j - 1) * bitmap.width);

            Channel alpha = m_bAntialiasing                                ? bitmap.buffer[k + bitmap.width * j]
                            : monoBitmapUnpacked[k + bitmap.width * j] > 0 ? 255
                                                                           : 0;
            expandedData[expandedIdx] = argb(alpha, 255, 255, 255);
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
    } else if(face != m_ftFace) {
        // make sure fallback face has the correct size for this font instance
        setFaceSize(face);
    }

    if(FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
        return;

    FT_Glyph glyph{};
    if(FT_Get_Glyph(face->glyph, &glyph)) return;

    FT_Glyph_To_Bitmap(&glyph, m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

    auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
    auto &bitmap = bitmapGlyph->bitmap;

    if(bitmap.width > 0 && bitmap.rows > 0) {
        auto expandedData = createExpandedBitmapData(bitmap);
        m_textureAtlas->putAt(x, y, bitmap.width, bitmap.rows, false, true, expandedData.get());

        // update metrics with atlas coordinates
        GLYPH_METRICS &metrics = m_vGlyphMetrics[ch];
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
        const auto &metrics = m_vGlyphMetrics[ch];
        if(metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0) {
            // add packrect
            packRects.push_back({.x = 0,
                                 .y = 0,
                                 .width = static_cast<int>(metrics.sizePixelsX),
                                 .height = static_cast<int>(metrics.sizePixelsY),
                                 .id = static_cast<int>(rectIndex)});
            rectsToChars.push_back(ch);
            rectIndex++;
        }
    }

    if(packRects.empty()) return true;

    // calculate optimal atlas size and create texture atlas
    const size_t atlasSize =
        TextureAtlas::calculateOptimalSize(packRects, ATLAS_OCCUPANCY_TARGET, MIN_ATLAS_SIZE, MAX_ATLAS_SIZE);

    resourceManager->requestNextLoadUnmanaged();
    m_textureAtlas = resourceManager->createTextureAtlas(static_cast<int>(atlasSize), static_cast<int>(atlasSize));

    // pack glyphs into atlas
    if(!m_textureAtlas->packRects(packRects)) {
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
    resourceManager->loadResource(m_textureAtlas);
    m_textureAtlas->getAtlasImage()->setFilterMode(m_bAntialiasing ? Graphics::FILTER_MODE::FILTER_MODE_LINEAR
                                                                   : Graphics::FILTER_MODE::FILTER_MODE_NONE);

    return true;
}

void McFont::buildGlyphGeometry(const GLYPH_METRICS &gm, const vec3 &basePos, float advanceX, size_t &vertexCount) {
    const float atlasWidth{static_cast<float>(m_textureAtlas->getAtlasImage()->getWidth())};
    const float atlasHeight{static_cast<float>(m_textureAtlas->getAtlasImage()->getHeight())};

    const float x{basePos.x + static_cast<float>(gm.left) + advanceX};
    const float y{basePos.y - static_cast<float>(gm.top - gm.rows)};
    const float z{basePos.z};
    const float sx{static_cast<float>(gm.width)};
    const float sy{static_cast<float>(-gm.rows)};

    const float texX{static_cast<float>(gm.uvPixelsX) / atlasWidth};
    const float texY{static_cast<float>(gm.uvPixelsY) / atlasHeight};
    const float texSizeX{static_cast<float>(gm.sizePixelsX) / atlasWidth};
    const float texSizeY{static_cast<float>(gm.sizePixelsY) / atlasHeight};

    // corners of the "quad"
    vec3 bottomLeft{x, y + sy, z};
    vec3 topLeft{x, y, z};
    vec3 topRight{x + sx, y, z};
    vec3 bottomRight{x + sx, y + sy, z};

    // texcoords
    vec2 texBottomLeft{texX, texY};
    vec2 texTopLeft{texX, texY + texSizeY};
    vec2 texTopRight{texX + texSizeX, texY + texSizeY};
    vec2 texBottomRight{texX + texSizeX, texY};

    const size_t idx{vertexCount};

    if constexpr(Env::cfg(REND::GLES32)) {
        // triangles (quads are slower for GL ES because they need to be converted to triangles at submit time)
        // first triangle (bottom-left, top-left, top-right)
        m_vertices[idx] = bottomLeft;
        m_vertices[idx + 1] = topLeft;
        m_vertices[idx + 2] = topRight;

        m_texcoords[idx] = texBottomLeft;
        m_texcoords[idx + 1] = texTopLeft;
        m_texcoords[idx + 2] = texTopRight;

        // second triangle (bottom-left, top-right, bottom-right)
        m_vertices[idx + 3] = bottomLeft;
        m_vertices[idx + 4] = topRight;
        m_vertices[idx + 5] = bottomRight;

        m_texcoords[idx + 3] = texBottomLeft;
        m_texcoords[idx + 4] = texTopRight;
        m_texcoords[idx + 5] = texBottomRight;
    } else {
        // quads
        m_vertices[idx] = bottomLeft;       // bottom-left
        m_vertices[idx + 1] = topLeft;      // top-left
        m_vertices[idx + 2] = topRight;     // top-right
        m_vertices[idx + 3] = bottomRight;  // bottom-right

        m_texcoords[idx] = texBottomLeft;
        m_texcoords[idx + 1] = texTopLeft;
        m_texcoords[idx + 2] = texTopRight;
        m_texcoords[idx + 3] = texBottomRight;
    }
    vertexCount += VERTS_PER_VAO;
}

void McFont::buildStringGeometry(const UString &text, size_t &vertexCount) {
    if(!this->bReady || text.length() == 0 || text.length() > cv::r_drawstring_max_string_length.getInt()) return;

    // check if atlas needs rebuilding before geometry generation
    if(m_bAtlasNeedsRebuild) rebuildAtlas();

    float advanceX = 0.0f;
    const size_t maxGlyphs =
        std::min(text.length(), (int)((double)(m_vertices.size() - vertexCount) / (double)VERTS_PER_VAO));

    for(int i = 0; i < maxGlyphs; i++) {
        const GLYPH_METRICS &gm = getGlyphMetrics(text[i]);
        buildGlyphGeometry(gm, vec3(), advanceX, vertexCount);
        advanceX += gm.advance_x;
    }
}

void McFont::drawString(const UString &text) {
    if(!this->bReady) return;

    const int maxNumGlyphs = cv::r_drawstring_max_string_length.getInt();
    if(text.length() == 0 || text.length() > maxNumGlyphs) return;

    m_vao.empty();

    const size_t totalVerts = text.length() * VERTS_PER_VAO;
    m_vao.reserve(totalVerts);
    m_vertices.resize(totalVerts);
    m_texcoords.resize(totalVerts);

    size_t vertexCount = 0;
    buildStringGeometry(text, vertexCount);

    for(size_t i = 0; i < vertexCount; i++) {
        m_vao.addVertex(m_vertices[i]);
        m_vao.addTexcoord(m_texcoords[i]);
    }

    m_textureAtlas->getAtlasImage()->bind();
    g->drawVAO(&m_vao);

    if(cv::r_debug_drawstring_unbind.getBool()) m_textureAtlas->getAtlasImage()->unbind();
}

void McFont::beginBatch() {
    m_batchActive = true;
    m_batchQueue.totalVerts = 0;
    m_batchQueue.usedEntries = 0;  // don't clear/reallocate, reuse the entries instead
}

void McFont::addToBatch(const UString &text, const vec3 &pos, Color color) {
    size_t verts{};
    if(!m_batchActive || (verts = text.length() * VERTS_PER_VAO) == 0) return;
    m_batchQueue.totalVerts += verts;

    if(m_batchQueue.usedEntries < m_batchQueue.entryList.size()) {
        // reuse existing entry
        BatchEntry &entry = m_batchQueue.entryList[m_batchQueue.usedEntries];
        entry.text = text;
        entry.pos = pos;
        entry.color = color;
    } else {
        // need to add new entry
        m_batchQueue.entryList.push_back({text, pos, color});
    }
    m_batchQueue.usedEntries++;
}

void McFont::flushBatch() {
    if(!m_batchActive || !m_batchQueue.totalVerts) {
        m_batchActive = false;
        return;
    }

    m_vertices.resize(m_batchQueue.totalVerts);
    m_texcoords.resize(m_batchQueue.totalVerts);
    m_vao.empty();
    m_vao.reserve(m_batchQueue.totalVerts);

    size_t currentVertex = 0;
    for(size_t i = 0; i < m_batchQueue.usedEntries; i++) {
        const auto &entry = m_batchQueue.entryList[i];
        const size_t stringStart = currentVertex;
        buildStringGeometry(entry.text, currentVertex);

        for(size_t j = stringStart; j < currentVertex; j++) {
            m_vertices[j] += entry.pos;
            m_vao.addVertex(m_vertices[j]);
            m_vao.addTexcoord(m_texcoords[j]);
            m_vao.addColor(entry.color);
        }
    }

    m_textureAtlas->getAtlasImage()->bind();

    g->drawVAO(&m_vao);

    if(cv::r_debug_drawstring_unbind.getBool()) m_textureAtlas->getAtlasImage()->unbind();

    m_batchActive = false;
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
    auto it = m_vGlyphMetrics.find(ch);
    if(it != m_vGlyphMetrics.end()) return it->second;

    // attempt dynamic loading for unicode characters
    if(const_cast<McFont *>(this)->loadGlyphDynamic(ch)) {
        it = m_vGlyphMetrics.find(ch);
        if(it != m_vGlyphMetrics.end()) return it->second;
    }

    // fallback to unknown character glyph
    it = m_vGlyphMetrics.find(UNKNOWN_CHAR);
    if(it != m_vGlyphMetrics.end()) return it->second;

    debugLog("Font Error: Missing default backup glyph (UNKNOWN_CHAR)?\n");
    return m_errorGlyph;
}

/*
########################################################################
## shared/static FreeType management stuff below
##########################################################################
*/

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
            if(cv::r_debug_font_unicode.getBool())
                debugLog("Font Info: Loaded bundled fallback font: {:s}\n", fontName);
        }
    }

    // then find likely system fonts
    if(!Env::cfg(OS::WASM) && cv::font_load_system.getBool()) discoverSystemFallbacks();

    s_sharedFallbacksInitialized = true;
    return true;
}

void McFont::discoverSystemFallbacks() {
#ifdef MCENGINE_PLATFORM_WINDOWS
    std::array<char, MAX_PATH> windir{};
    if(GetWindowsDirectoryA(windir.data(), MAX_PATH) <= 0) return;

    std::vector<std::string> systemFonts = {
        std::string{windir.data()} + "\\Fonts\\arial.ttf",
        std::string{windir.data()} + "\\Fonts\\msyh.ttc",      // Microsoft YaHei (Chinese)
        std::string{windir.data()} + "\\Fonts\\malgun.ttf",    // Malgun Gothic (Korean)
        std::string{windir.data()} + "\\Fonts\\meiryo.ttc",    // Meiryo (Japanese)
        std::string{windir.data()} + "\\Fonts\\seguiemj.ttf",  // Segoe UI Emoji
        std::string{windir.data()} + "\\Fonts\\seguisym.ttf"   // Segoe UI Symbol
    };
#elif defined(MCENGINE_PLATFORM_LINUX)
    // linux system fonts (common locations)
    std::vector<std::string> systemFonts = {"/usr/share/fonts/TTF/dejavu/DejaVuSans.ttf",
                                            "/usr/share/fonts/TTF/liberation/LiberationSans-Regular.ttf",
                                            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
                                            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                            "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
                                            "/usr/share/fonts/TTF/noto/NotoColorEmoji.ttf"};
#else  // TODO: loading WOFF fonts in wasm? idk
    std::vector<std::string> systemFonts;
    return;
#endif

    for(auto &fontPath : systemFonts) {
        if(File::existsCaseInsensitive(fontPath) == File::FILETYPE::FILE) {
            loadFallbackFont(fontPath.c_str(), true);
        }
    }
}

bool McFont::loadFallbackFont(const UString &fontPath, bool isSystemFont) {
    FT_Face face{};
    if(FT_New_Face(s_sharedFtLibrary, fontPath.toUtf8(), 0, &face)) {
        if(cv::r_debug_font_unicode.getBool()) debugLog("Font Warning: Failed to load fallback font: {:s}\n", fontPath);
        return false;
    }

    if(FT_Select_Charmap(face, ft_encoding_unicode)) {
        if(cv::r_debug_font_unicode.getBool())
            debugLog("Font Warning: Failed to select unicode charmap for fallback font: {:s}\n", fontPath);
        FT_Done_Face(face);
        return false;
    }

    // don't set font size here, will be set when the face is used by individual font instances
    s_sharedFallbackFonts.push_back(FallbackFont{fontPath, face, isSystemFont});
    return true;
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
