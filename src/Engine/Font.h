//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		freetype font wrapper with unicode support
//
// $NoKeywords: $fnt
//===============================================================================//

#pragma once
#ifndef FONT_H
#define FONT_H

#include "Resource.h"
#include "UString.h"
#include "VertexArrayObject.h"

#include <memory>
#include <unordered_map>
typedef struct FT_Bitmap_ FT_Bitmap;
typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;

class TextureAtlas;
class VertexArrayObject;

class McFont final : public Resource {
   public:
    McFont(std::string filepath, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);
    McFont(std::string filepath, const std::vector<wchar_t> &characters, int fontSize = 16, bool antialiasing = true,
           int fontDPI = 96);
    ~McFont() override;

    McFont &operator=(const McFont &) = delete;
    McFont &operator=(McFont &&) = delete;

    McFont(const McFont &) = delete;
    McFont(McFont &&) = delete;

    // called on engine shutdown to clean up freetype/shared fallback fonts
    static void cleanupSharedResources();

    void drawString(const UString &text);
    void beginBatch();
    void addToBatch(const UString &text, const vec3 &pos, Color color = 0xffffffff);
    void flushBatch();

    void setSize(int fontSize) { m_iFontSize = fontSize; }
    void setDPI(int dpi) { m_iFontDPI = dpi; }
    void setHeight(float height) { m_fHeight = height; }

    inline int getSize() const { return m_iFontSize; }
    inline int getDPI() const { return m_iFontDPI; }
    inline float getHeight() const { return m_fHeight; }  // precomputed average height (fast)

    float getStringWidth(const UString &text) const;
    float getStringHeight(const UString &text) const;
    std::vector<UString> wrap(const UString &text, f64 max_width) const;

   public:
    struct GLYPH_METRICS {
        wchar_t character;
        unsigned int uvPixelsX, uvPixelsY;
        unsigned int sizePixelsX, sizePixelsY;
        int left, top, width, rows;
        float advance_x;
        int fontIndex;  // which font this glyph came from (0 = primary, >0 = fallback)
    };
    // McKay would say: ILLEGAL
    const GLYPH_METRICS &getGlyphMetrics(wchar_t ch) const;

    // type inspection
    [[nodiscard]] Type getResType() const override { return FONT; }

    McFont *asFont() override { return this; }
    [[nodiscard]] const McFont *asFont() const override { return this; }

   protected:
    void constructor(const std::vector<wchar_t> &characters, int fontSize, bool antialiasing, int fontDPI);
    void init() override;
    void initAsync() override;
    void destroy() override;

   private:
    struct FallbackFont {
        UString fontPath;
        FT_Face face;
        bool isSystemFont;
    };

    struct BatchEntry {
        UString text;
        vec3 pos{0.f};
        Color color;
    };

    struct TextBatch {
        size_t totalVerts;
        size_t usedEntries;
        std::vector<BatchEntry> entryList;
    };

    forceinline bool hasGlyph(wchar_t ch) const { return m_vGlyphMetrics.find(ch) != m_vGlyphMetrics.end(); };
    bool addGlyph(wchar_t ch);
    bool loadGlyphDynamic(wchar_t ch);
    bool ensureAtlasSpace(int requiredWidth, int requiredHeight);
    void rebuildAtlas();

    // consolidated glyph processing methods
    bool initializeFreeType();
    bool loadGlyphMetrics(wchar_t ch);
    std::unique_ptr<Color[]> createExpandedBitmapData(const FT_Bitmap &bitmap);
    void renderGlyphToAtlas(wchar_t ch, int x, int y, FT_Face face = nullptr);
    bool createAndPackAtlas(const std::vector<wchar_t> &glyphs, bool isRebuild);

    // fallback font management
    FT_Face getFontFaceForGlyph(wchar_t ch, int &fontIndex);
    bool loadGlyphFromFace(wchar_t ch, FT_Face face, int fontIndex);

    void buildGlyphGeometry(const GLYPH_METRICS &gm, const vec3 &basePos, float advanceX, size_t &vertexCount);
    void buildStringGeometry(const UString &text, size_t &vertexCount);

    static std::unique_ptr<Channel[]> unpackMonoBitmap(const FT_Bitmap &bitmap);

    // shared freetype resources
    static FT_Library s_sharedFtLibrary;
    static bool s_sharedFtLibraryInitialized;
    static std::vector<FallbackFont> s_sharedFallbackFonts;
    static bool s_sharedFallbacksInitialized;

    // shared resource initialization
    static bool initializeSharedFreeType();
    static bool initializeSharedFallbackFonts();
    static void discoverSystemFallbacks();
    static bool loadFallbackFont(const UString &fontPath, bool isSystemFont = false);

    // helper to set font size on any face for this font instance
    void setFaceSize(FT_Face face) const;

    std::vector<wchar_t> m_vGlyphs;
    std::unordered_map<wchar_t, bool> m_vGlyphExistence;
    std::unordered_map<wchar_t, GLYPH_METRICS> m_vGlyphMetrics;

    VertexArrayObject m_vao;
    TextBatch m_batchQueue;
    std::vector<vec3> m_vertices;
    std::vector<vec2> m_texcoords;

    std::unique_ptr<TextureAtlas> m_textureAtlas{nullptr};

    // per-instance freetype resources (only primary font face)
    FT_Face m_ftFace;  // primary font face

    int m_iFontSize;
    int m_iFontDPI;
    float m_fHeight;
    GLYPH_METRICS m_errorGlyph;

    // atlas management
    std::vector<wchar_t> m_vPendingGlyphs;

    mutable bool m_bAtlasNeedsRebuild;

    bool m_batchActive;
    bool m_bFreeTypeInitialized;
    bool m_bAntialiasing;
};

#endif
