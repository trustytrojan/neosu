#pragma once
#include "Resource.h"

class Image : public Resource {
   public:
    static void saveToImage(unsigned char *data, unsigned int width, unsigned int height, std::string filepath);

    enum class TYPE { TYPE_RGBA, TYPE_PNG, TYPE_JPG };

   public:
    Image(std::string filepath, bool mipmapped = false);
    Image(int width, int height, bool mipmapped = false);
    ~Image() override { ; }

    virtual void bind(unsigned int textureUnit = 0) = 0;
    virtual void unbind() = 0;

    virtual void setFilterMode(Graphics::FILTER_MODE filterMode);
    virtual void setWrapMode(Graphics::WRAP_MODE wrapMode);

    void setPixel(int x, int y, Color color);
    void setPixels(const char *data, size_t size, TYPE type);
    void setPixels(const std::vector<unsigned char> &pixels);

    Color getPixel(int x, int y) const;

    inline Image::TYPE getType() const { return this->type; }
    inline int getNumChannels() const { return this->iNumChannels; }
    inline int getWidth() const { return this->iWidth; }
    inline int getHeight() const { return this->iHeight; }
    inline Vector2 getSize() const { return Vector2(this->iWidth, this->iHeight); }

    inline bool hasAlphaChannel() const { return this->bHasAlphaChannel; }

    bool is_2x;

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

    std::vector<unsigned char> rawImage;
};
