#pragma once
#include <freetype/freetype.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <ft2build.h>

#include "Resource.h"

class Image;
class TextureAtlas;
class VertexArrayObject;

class McFont : public Resource {
   public:
    static const wchar_t UNKNOWN_CHAR = 63;  // ascii '?'

    struct GLYPH_METRICS {
        wchar_t character;

        unsigned int uvPixelsX;
        unsigned int uvPixelsY;
        unsigned int sizePixelsX;
        unsigned int sizePixelsY;

        int left;
        int top;
        int width;
        int rows;

        float advance_x;
    };

   public:
    McFont(std::string filepath, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);
    McFont(std::string filepath, std::vector<wchar_t> characters, int fontSize = 16, bool antialiasing = true,
           int fontDPI = 96);
    ~McFont() override { this->destroy(); }

    void drawString(Graphics *g, UString text);
    void drawTextureAtlas(Graphics *g);

    void setSize(int fontSize) { this->iFontSize = fontSize; }
    void setDPI(int dpi) { this->iFontDPI = dpi; }
    void setHeight(float height) { this->fHeight = height; }

    inline int getSize() const { return this->iFontSize; }
    inline int getDPI() const { return this->iFontDPI; }
    inline float getHeight() const { return this->fHeight; }  // precomputed average height (fast)

    float getStringWidth(UString text) const;
    float getStringHeight(UString text) const;

    const GLYPH_METRICS &getGlyphMetrics(wchar_t ch) const;
    bool hasGlyph(wchar_t ch) const;

    std::vector<UString> wrap(const UString &text, f64 max_width);

    // ILLEGAL:
    inline TextureAtlas *getTextureAtlas() const { return this->textureAtlas; }

   protected:
    void constructor(std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI);

    void init() override;
    void initAsync() override;
    void destroy() override;

    bool addGlyph(wchar_t ch);

    void addAtlasGlyphToVao(Graphics *g, wchar_t ch, float &advanceX, VertexArrayObject *vao);
    void renderFTGlyphToTextureAtlas(FT_Library library, FT_Face face, wchar_t ch, TextureAtlas *textureAtlas,
                                     bool antialiasing,
                                     std::unordered_map<wchar_t, McFont::GLYPH_METRICS> *glyphMetrics);

    int iFontSize;
    bool bAntialiasing;
    int iFontDPI;

    // glyphs
    TextureAtlas *textureAtlas;

    std::vector<wchar_t> vGlyphs;
    std::unordered_map<wchar_t, bool> vGlyphExistence;
    std::unordered_map<wchar_t, GLYPH_METRICS> vGlyphMetrics;

    float fHeight;

    GLYPH_METRICS errorGlyph;
};
