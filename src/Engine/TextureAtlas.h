//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		container for dynamically merging multiple images into one
//
// $NoKeywords: $imgtxat
//===============================================================================//

#ifndef TEXTUREATLAS_H
#define TEXTUREATLAS_H

#include "Resource.h"

class Image;

class TextureAtlas : public Resource {
   public:
    TextureAtlas(int width = 512, int height = 512);
    ~TextureAtlas() override { this->destroy(); }

    Vector2 put(int width, int height, Color *pixels) { return this->put(width, height, false, false, pixels); }
    Vector2 put(int width, int height, bool flipHorizontal, bool flipVertical, Color *pixels);

    void setPadding(int padding) { this->iPadding = padding; }

    [[nodiscard]] inline int getWidth() const { return this->iWidth; }
    [[nodiscard]] inline int getHeight() const { return this->iHeight; }
    [[nodiscard]] inline Image *getAtlasImage() const { return this->atlasImage; }

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    int iPadding;

    int iWidth;
    int iHeight;

    Image *atlasImage;

    int iCurX;
    int iCurY;
    int iMaxHeight;
};

#endif
