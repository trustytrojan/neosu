//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		image wrapper
//
// $NoKeywords: $img
//===============================================================================//

#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#include "Resource.h"

class Image : public Resource {
   public:
    static void saveToImage(unsigned char *data, unsigned int width, unsigned int height, std::string filepath);

    enum class TYPE : uint8_t { TYPE_RGBA, TYPE_PNG, TYPE_JPG };

   public:
    Image(std::string filepath, bool mipmapped = false, bool keepInSystemMemory = false);
    Image(int width, int height, bool mipmapped = false, bool keepInSystemMemory = false);

    virtual void bind(unsigned int textureUnit = 0) = 0;
    virtual void unbind() = 0;

    virtual void setFilterMode(Graphics::FILTER_MODE filterMode);
    virtual void setWrapMode(Graphics::WRAP_MODE wrapMode);

    void setPixel(int x, int y, Color color);
    void setPixels(const char *data, size_t size, TYPE type);
    void setPixels(const std::vector<unsigned char> &pixels);

    [[nodiscard]] Color getPixel(int x, int y) const;

    [[nodiscard]] inline Image::TYPE getType() const { return type; }
    [[nodiscard]] inline int getNumChannels() const { return iNumChannels; }
    [[nodiscard]] inline int getWidth() const { return iWidth; }
    [[nodiscard]] inline int getHeight() const { return iHeight; }
    [[nodiscard]] inline Vector2 getSize() const { return Vector2{static_cast<float>(iWidth), static_cast<float>(iHeight)}; }

    [[nodiscard]] inline bool hasAlphaChannel() const { return bHasAlphaChannel; }

    std::atomic<bool> is_2x{false};

    // type inspection
    [[nodiscard]] Type getResType() const final { return IMAGE; }

    Image *asImage() final { return this; }
    [[nodiscard]] const Image *asImage() const final { return this; }

   protected:
    void init() override = 0;
    void initAsync() override = 0;
    void destroy() override = 0;

    bool loadRawImage();

    Image::TYPE type;
    Graphics::FILTER_MODE filterMode;
    Graphics::WRAP_MODE wrapMode;

    int iNumChannels;
    int iWidth;
    int iHeight;

    bool bHasAlphaChannel;
    bool bMipmapped;
    bool bCreatedImage;
    bool bKeepInSystemMemory;

    std::vector<unsigned char> rawImage;

   private:
    [[nodiscard]] bool isCompletelyTransparent() const;
    static bool canHaveTransparency(const unsigned char *data, size_t size);

    static bool decodePNGFromMemory(const unsigned char *data, size_t size, std::vector<unsigned char> &outData,
                                    int &outWidth, int &outHeight, int &outChannels);
};

#endif
