//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		image wrapper
//
// $NoKeywords: $img
//===============================================================================//

#include "Image.h"

#include <setjmp.h>

#include "Engine.h"
#include "Environment.h"
#include "File.h"
#include "ResourceManager.h"
#include "jpeglib.h"
#include "lodepng.h"

struct jpegErrorManager {
    // "public" fields
    struct jpeg_error_mgr pub;

    // for returning to the caller
    jmp_buf setjmp_buffer;
};

void jpegErrorExit(j_common_ptr cinfo) {
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];

    jpegErrorManager *err = (jpegErrorManager *)cinfo->err;

    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
    jpegLastErrorMsg[JMSG_LENGTH_MAX - 1] = '\0';

    printf("JPEG Error: %s", jpegLastErrorMsg);

    longjmp(err->setjmp_buffer, 1);
}

void Image::saveToImage(unsigned char *data, unsigned int width, unsigned int height, std::string filepath) {
    debugLog("Saving image to %s ...\n", filepath.c_str());

    const unsigned error = lodepng::encode(filepath.c_str(), data, width, height, LodePNGColorType::LCT_RGB, 8);
    if(error) {
        debugLog("PNG error %i on file %s", error, filepath.c_str());
        UString errorMessage = UString::format("PNG error %i on file ", error);
        errorMessage.append(filepath.c_str());
        engine->showMessageError(errorMessage, lodepng_error_text(error));
        return;
    }
}

Image::Image(std::string filepath, bool mipmapped) : Resource(filepath) {
    this->bMipmapped = mipmapped;

    this->type = Image::TYPE::TYPE_PNG;
    this->filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
    this->wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
    this->iNumChannels = 4;
    this->iWidth = 1;
    this->iHeight = 1;

    this->bHasAlphaChannel = true;
    this->bCreatedImage = false;
}

Image::Image(int width, int height, bool mipmapped) : Resource() {
    this->bMipmapped = mipmapped;

    this->type = Image::TYPE::TYPE_RGBA;
    this->filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
    this->wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
    this->iNumChannels = 4;
    this->iWidth = width;
    this->iHeight = height;

    this->bHasAlphaChannel = true;
    this->bCreatedImage = true;

    // reserve and fill with pink pixels
    this->rawImage.resize(this->iWidth * this->iHeight * this->iNumChannels);
    for(int i = 0; i < this->iWidth * this->iHeight; i++) {
        this->rawImage.push_back(255);
        this->rawImage.push_back(0);
        this->rawImage.push_back(255);
        this->rawImage.push_back(255);
    }

    // special case: filled rawimage is always already async ready
    this->bAsyncReady = true;
}

bool Image::loadRawImage() {
    // if it isn't a created image (created within the engine), load it from the corresponding file
    if(!this->bCreatedImage) {
        if(this->rawImage.size() > 0)  // has already been loaded (or loading it again after setPixel(s))
            return true;

        if(!env->fileExists(this->sFilePath)) {
            printf("Image Error: Couldn't find file %s\n", this->sFilePath.c_str());
            return false;
        }

        if(this->bInterrupted)  // cancellation point
            return false;

        // load entire file
        File file(this->sFilePath);
        if(!file.canRead()) {
            printf("Image Error: Couldn't canRead() file %s\n", this->sFilePath.c_str());
            return false;
        }
        if(file.getFileSize() < 4) {
            printf("Image Error: FileSize is < 4 in file %s\n", this->sFilePath.c_str());
            return false;
        }

        if(this->bInterrupted)  // cancellation point
            return false;

        const u8 *data = file.readFile();
        if(data == NULL) {
            printf("Image Error: Couldn't readFile() file %s\n", this->sFilePath.c_str());
            return false;
        }

        if(this->bInterrupted)  // cancellation point
            return false;

        // determine file type by magic number (png/jpg)
        bool isJPEG = false;
        bool isPNG = false;
        {
            const int numBytes = 4;

            unsigned char buf[numBytes];

            for(int i = 0; i < numBytes; i++) {
                buf[i] = (unsigned char)data[i];
            }

            if(buf[0] == 0xff && buf[1] == 0xD8 && buf[2] == 0xff)  // 0xFFD8FF
                isJPEG = true;
            else if(buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47)  // 0x89504E47 (%PNG)
                isPNG = true;
        }

        // depending on the type, load either jpeg or png
        if(isJPEG) {
            this->type = Image::TYPE::TYPE_JPG;

            this->bHasAlphaChannel = false;

            // decode jpeg
            jpegErrorManager err;
            jpeg_decompress_struct cinfo;

            jpeg_create_decompress(&cinfo);
            cinfo.err = jpeg_std_error(&err.pub);
            err.pub.error_exit = jpegErrorExit;
            if(setjmp(err.setjmp_buffer)) {
                jpeg_destroy_decompress(&cinfo);
                printf("Image Error: JPEG error (see above) in file %s\n", this->sFilePath.c_str());
                return false;
            }

            jpeg_mem_src(&cinfo, (unsigned char *)data, file.getFileSize());
#ifdef __APPLE__
            const int headerRes =
                jpeg_read_header(&cinfo, boolean::TRUE);  // HACKHACK: wtf is this boolean enum here suddenly required?
#else
            const int headerRes = jpeg_read_header(&cinfo, TRUE);
#endif
            if(headerRes != JPEG_HEADER_OK) {
                jpeg_destroy_decompress(&cinfo);
                printf("Image Error: JPEG read_header() error %i in file %s\n", headerRes, this->sFilePath.c_str());
                return false;
            }

            this->iWidth = cinfo.image_width;
            this->iHeight = cinfo.image_height;
            this->iNumChannels = cinfo.num_components;

            // NOTE: color spaces which require color profiles are not supported (e.g. J_COLOR_SPACE::JCS_YCCK)

            if(this->iNumChannels == 4) this->bHasAlphaChannel = true;

            if(this->iWidth > 8192 || this->iHeight > 8192) {
                jpeg_destroy_decompress(&cinfo);
                printf("Image Error: JPEG image size is too big (%i x %i) in file %s\n", this->iWidth, this->iHeight,
                       this->sFilePath.c_str());
                return false;
            }

            // preallocate
            this->rawImage.resize(this->iWidth * this->iHeight * this->iNumChannels);

            // extract each scanline of the image
            jpeg_start_decompress(&cinfo);
            JSAMPROW j;
            for(int y = 0; y < this->iHeight; y++) {
                if(this->bInterrupted)  // cancellation point
                {
                    jpeg_destroy_decompress(&cinfo);
                    return false;
                }

                j = (&this->rawImage[0] + (y * this->iWidth * this->iNumChannels));
                jpeg_read_scanlines(&cinfo, &j, 1);
            }

            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
        } else if(isPNG) {
            this->type = Image::TYPE::TYPE_PNG;

            unsigned int width = 0;  // yes, these are here on purpose
            unsigned int height = 0;

            const unsigned error =
                lodepng::decode(this->rawImage, width, height, (const unsigned char *)data, file.getFileSize());

            this->iWidth = width;
            this->iHeight = height;

            if(error) {
                printf("Image Error: PNG error %i (%s) in file %s\n", error, lodepng_error_text(error),
                       this->sFilePath.c_str());
                return false;
            }
        } else {
            printf("Image Error: Neither PNG nor JPEG in file %s\n", this->sFilePath.c_str());
            return false;
        }
    }

    if(this->bInterrupted)  // cancellation point
        return false;

    // error checking

    // size sanity check
    if(this->rawImage.size() < (this->iWidth * this->iHeight * this->iNumChannels)) {
        printf("Image Error: Loaded image has only %lu/%i bytes in file %s\n", (unsigned long)this->rawImage.size(),
               this->iWidth * this->iHeight * this->iNumChannels, this->sFilePath.c_str());
        // engine->showMessageError("Image Error", UString::format("Loaded image has only %i/%i bytes in file %s",
        // m_rawImage.size(), this->iWidth*m_iHeight*m_iNumChannels, this->sFilePath.c_str()));
        return false;
    }

    // supported channels sanity check
    if(this->iNumChannels != 4 && this->iNumChannels != 3 && this->iNumChannels != 1) {
        printf("Image Error: Unsupported number of color channels (%i) in file %s", this->iNumChannels,
               this->sFilePath.c_str());
        // engine->showMessageError("Image Error", UString::format("Unsupported number of color channels (%i) in file
        // %s", this->iNumChannels, this->sFilePath.c_str()));
        return false;
    }

    // optimization: ignore completely transparent images (don't render)
    bool foundNonTransparentPixel = false;
    for(int x = 0; x < this->iWidth; x++) {
        if(this->bInterrupted)  // cancellation point
            return false;

        for(int y = 0; y < this->iHeight; y++) {
            if(COLOR_GET_Ai(getPixel(x, y)) > 0) {
                foundNonTransparentPixel = true;
                break;
            }
        }

        if(foundNonTransparentPixel) break;
    }
    if(!foundNonTransparentPixel) {
        printf("Image: Ignoring empty transparent image %s\n", this->sFilePath.c_str());
        return false;
    }

    return true;
}

void Image::setFilterMode(Graphics::FILTER_MODE filterMode) { this->filterMode = filterMode; }

void Image::setWrapMode(Graphics::WRAP_MODE wrapMode) { this->wrapMode = wrapMode; }

Color Image::getPixel(int x, int y) const {
    const int indexBegin = this->iNumChannels * y * this->iWidth + this->iNumChannels * x;
    const int indexEnd = this->iNumChannels * y * this->iWidth + this->iNumChannels * x + this->iNumChannels;

    if(this->rawImage.size() < 1 || x < 0 || y < 0 || indexEnd < 0 || indexEnd > this->rawImage.size())
        return 0xffffff00;

    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 0;
    unsigned char a = 255;

    r = this->rawImage[indexBegin + 0];
    if(this->iNumChannels > 1) {
        g = this->rawImage[indexBegin + 1];
        b = this->rawImage[indexBegin + 2];

        if(this->iNumChannels > 3)
            a = this->rawImage[indexBegin + 3];
        else
            a = 255;
    } else {
        g = r;
        b = r;
        a = r;
    }

    return COLOR(a, r, g, b);
}

void Image::setPixel(int x, int y, Color color) {
    const int indexBegin = this->iNumChannels * y * this->iWidth + this->iNumChannels * x;
    const int indexEnd = this->iNumChannels * y * this->iWidth + this->iNumChannels * x + this->iNumChannels;

    if(this->rawImage.size() < 1 || x < 0 || y < 0 || indexEnd < 0 || indexEnd > this->rawImage.size()) return;

    this->rawImage[indexBegin + 0] = COLOR_GET_Ri(color);
    if(this->iNumChannels > 1) this->rawImage[indexBegin + 1] = COLOR_GET_Gi(color);
    if(this->iNumChannels > 2) this->rawImage[indexBegin + 2] = COLOR_GET_Bi(color);
    if(this->iNumChannels > 3) this->rawImage[indexBegin + 3] = COLOR_GET_Ai(color);
}

void Image::setPixels(const char *data, size_t size, TYPE type) {
    if(data == NULL) return;

    // TODO: implement remaining types
    switch(type) {
        case TYPE::TYPE_PNG: {
            unsigned int width = 0;  // yes, these are here on purpose
            unsigned int height = 0;

            const unsigned error = lodepng::decode(this->rawImage, width, height, (const unsigned char *)data, size);

            this->iWidth = width;
            this->iHeight = height;

            if(error)
                printf("Image Error: PNG error %i (%s) in file %s\n", error, lodepng_error_text(error),
                       this->sFilePath.c_str());
        } break;

        default:
            debugLog("Image Error: Format not yet implemented\n");
            break;
    }
}

void Image::setPixels(const std::vector<unsigned char> &pixels) {
    if(pixels.size() < (this->iWidth * this->iHeight * this->iNumChannels)) {
        debugLog("Image Error: setPixels() supplied array is too small!\n");
        return;
    }

    this->rawImage = pixels;
}
