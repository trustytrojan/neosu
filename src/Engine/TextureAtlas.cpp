#include "TextureAtlas.h"

#include "Engine.h"
#include "ResourceManager.h"

using namespace std;

TextureAtlas::TextureAtlas(int width, int height) : Resource() {
    this->iWidth = width;
    this->iHeight = height;

    this->iPadding = 1;

    engine->getResourceManager()->requestNextLoadUnmanaged();
    this->atlasImage = engine->getResourceManager()->createImage(this->iWidth, this->iHeight);

    this->iCurX = this->iPadding;
    this->iCurY = this->iPadding;
    this->iMaxHeight = 0;
}

void TextureAtlas::init() {
    engine->getResourceManager()->loadResource(this->atlasImage);

    this->bReady = true;
}

void TextureAtlas::initAsync() { this->bAsyncReady = true; }

void TextureAtlas::destroy() { SAFE_DELETE(this->atlasImage); }

Vector2 TextureAtlas::put(int width, int height, bool flipHorizontal, bool flipVertical, Color *pixels) {
    if(width < 1 || height < 1) return Vector2();

    if(width > (this->iWidth - this->iPadding * 2) || height > (this->iHeight - this->iCurY - this->iPadding)) {
        debugLog("TextureAtlas::put( %i, %i ) WARNING: Out of bounds / impossible fit!\n", width, height);
        return Vector2();
    }
    if(this->atlasImage == NULL) {
        debugLog("TextureAtlas::put() ERROR: m_atlasImage == NULL!\n");
        return Vector2();
    }
    if(pixels == NULL) {
        debugLog("TextureAtlas::put() ERROR: pixels == NULL!\n");
        return Vector2();
    }

    // very simple packing logic: overflow down individual lines/rows, highest element in line determines max
    if(this->iCurX + width + this->iPadding > this->iWidth) {
        this->iCurX = this->iPadding;
        this->iCurY += this->iMaxHeight + this->iPadding;
        this->iMaxHeight = 0;
    }
    if(height > this->iMaxHeight) this->iMaxHeight = height;

    if(this->iCurY + height + this->iPadding > this->iHeight) {
        debugLog("TextureAtlas::put( %i, %i ) WARNING: Out of bounds / impossible fit!\n", width, height);
        return Vector2();
    }

    // insert pixels
    {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                int actualX = (flipHorizontal ? width - x - 1 : x);
                int actualY = (flipVertical ? height - y - 1 : y);

                this->atlasImage->setPixel(this->iCurX + x, this->iCurY + y, pixels[actualY * width + actualX]);
            }
        }

        // TODO: make this into an API
        // mirror border pixels
        if(this->iPadding > 1) {
            // left
            for(int y = -1; y < height + 1; y++) {
                const int x = 0;
                int actualX = (flipHorizontal ? width - x - 1 : x);
                int actualY = clamp<int>((flipVertical ? height - y - 1 : y), 0, height - 1);

                this->atlasImage->setPixel(this->iCurX + x - 1, this->iCurY + y, pixels[actualY * width + actualX]);
            }
            // right
            for(int y = -1; y < height + 1; y++) {
                const int x = width - 1;
                int actualX = (flipHorizontal ? width - x - 1 : x);
                int actualY = clamp<int>((flipVertical ? height - y - 1 : y), 0, height - 1);

                this->atlasImage->setPixel(this->iCurX + x + 1, this->iCurY + y, pixels[actualY * width + actualX]);
            }
            // top
            for(int x = -1; x < width + 1; x++) {
                const int y = 0;
                int actualX = clamp<int>((flipHorizontal ? width - x - 1 : x), 0, width - 1);
                int actualY = (flipVertical ? height - y - 1 : y);

                this->atlasImage->setPixel(this->iCurX + x, this->iCurY + y - 1, pixels[actualY * width + actualX]);
            }
            // bottom
            for(int x = -1; x < width + 1; x++) {
                const int y = height - 1;
                int actualX = clamp<int>((flipHorizontal ? width - x - 1 : x), 0, width - 1);
                int actualY = (flipVertical ? height - y - 1 : y);

                this->atlasImage->setPixel(this->iCurX + x, this->iCurY + y + 1, pixels[actualY * width + actualX]);
            }
        }
    }
    Vector2 pos = Vector2(this->iCurX, this->iCurY);

    // move offset for next element
    this->iCurX += width + this->iPadding;

    return pos;
}
