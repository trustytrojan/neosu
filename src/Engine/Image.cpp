//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		image wrapper
//
// $NoKeywords: $img
//===============================================================================//

#include "Image.h"

#include <png.h>
#include <turbojpeg.h>
#include <zlib.h>

#include <csetjmp>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <utility>

#include "Engine.h"
#include "Environment.h"
#include "File.h"

namespace {
// this is complete bullshit and a bug in zlib-ng (probably, less likely libpng)
// need to prevent zlib from lazy-initializing the crc tables, otherwise data race galore
// literally causes insane lags/issues in completely unrelated places for async loading
std::mutex zlib_init_mutex;
std::atomic<bool> zlib_initialized{false};

void garbage_zlib() {
    if(zlib_initialized.load(std::memory_order_acquire)) return;
    std::scoped_lock lock(zlib_init_mutex);
    if(zlib_initialized.load(std::memory_order_relaxed)) return;
    uLong dummy_crc = crc32(0L, Z_NULL, 0);
    const char test_data[] = "shit";
    dummy_crc = crc32(dummy_crc, reinterpret_cast<const Bytef *>(test_data), 4);
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if(inflateInit(&strm) == Z_OK) inflateEnd(&strm);
    (void)dummy_crc;
    zlib_initialized.store(true, std::memory_order_release);
}

struct pngErrorManager {
    jmp_buf setjmp_buffer;
};

void pngErrorExit(png_structp png_ptr, png_const_charp error_msg) {
    debugLogF("PNG Error: {:s}\n", error_msg);
    auto *err = static_cast<pngErrorManager *>(png_get_error_ptr(png_ptr));
    longjmp(&err->setjmp_buffer[0], 1);
}

void pngWarning(png_structp, [[maybe_unused]] png_const_charp warning_msg) {
#ifdef _DEBUG
    debugLogF("PNG Warning: {:s}\n", warning_msg);
#endif
}

struct pngMemoryReader {
    const unsigned char *data;
    size_t size;
    size_t offset;
};

void pngReadFromMemory(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {
    auto *reader = static_cast<pngMemoryReader *>(png_get_io_ptr(png_ptr));

    if(reader->offset + byteCountToRead > reader->size) {
        png_error(png_ptr, "Read past end of data");
        return;
    }

    memcpy(outBytes, reader->data + reader->offset, byteCountToRead);
    reader->offset += byteCountToRead;
}
}  // namespace

bool Image::decodePNGFromMemory(const unsigned char *data, size_t size, std::vector<unsigned char> &outData,
                                int &outWidth, int &outHeight, int &outChannels) {
    garbage_zlib();
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        debugLogF("Image Error: png_create_read_struct failed\n");
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        debugLogF("Image Error: png_create_info_struct failed\n");
        return false;
    }

    pngErrorManager err;
    png_set_error_fn(png_ptr, &err, pngErrorExit, pngWarning);

    if(setjmp(&err.setjmp_buffer[0])) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return false;
    }

    // Set up memory reading
    pngMemoryReader reader;
    reader.data = data;
    reader.size = size;
    reader.offset = 0;
    png_set_read_fn(png_ptr, &reader, pngReadFromMemory);

    png_read_info(png_ptr, info_ptr);

    outWidth = static_cast<int>(png_get_image_width(png_ptr, info_ptr));
    outHeight = static_cast<int>(png_get_image_height(png_ptr, info_ptr));
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    png_byte interlace_type = png_get_interlace_type(png_ptr, info_ptr);

    // convert to RGBA if needed
    if(bit_depth == 16) png_set_strip_16(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);

    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);

    // these color types don't have alpha channel, so fill it with 0xff
    if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);

    // "Interlace handling should be turned on when using png_read_image"
    if(interlace_type != PNG_INTERLACE_NONE) png_set_interlace_handling(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    // after transformations, we should always have RGBA
    outChannels = 4;

    if(outWidth > 8192 || outHeight > 8192) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        debugLogF("Image Error: PNG image size is too big ({} x {})\n", outWidth, outHeight);
        return false;
    }

    // allocate memory for the image
    outData.resize(static_cast<long>(outWidth * outHeight) * outChannels);

    // read it
    auto *row_pointers = new png_bytep[outHeight];
    for(int y = 0; y < outHeight; y++) {
        row_pointers[y] = &outData[static_cast<long>(y * outWidth * outChannels)];
    }

    png_read_image(png_ptr, row_pointers);
    delete[] row_pointers;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return true;
}

void Image::saveToImage(unsigned char *data, unsigned int width, unsigned int height, std::string filepath) {
    garbage_zlib();
    debugLogF("Saving image to {:s} ...\n", filepath);

    FILE *fp = fopen(filepath.c_str(), "wb");
    if(!fp) {
        debugLogF("PNG error: Could not open file {:s} for writing\n", filepath);
        engine->showMessageError("PNG Error", "Could not open file for writing");
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        fclose(fp);
        debugLogF("PNG error: png_create_write_struct failed\n");
        engine->showMessageError("PNG Error", "png_create_write_struct failed");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        debugLogF("PNG error: png_create_info_struct failed\n");
        engine->showMessageError("PNG Error", "png_create_info_struct failed");
        return;
    }

    if(setjmp(&png_jmpbuf(png_ptr)[0])) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        debugLogF("PNG error during write\n");
        engine->showMessageError("PNG Error", "Error during PNG write");
        return;
    }

    png_init_io(png_ptr, fp);

    // write header (8 bit colour depth, RGB)
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    // write image data
    auto row = new png_byte[3L * width];
    for(unsigned int y = 0; y < height; y++) {
        for(unsigned int x = 0; x < width; x++) {
            row[x * 3 + 0] = data[(y * width + x) * 3 + 0];
            row[x * 3 + 1] = data[(y * width + x) * 3 + 1];
            row[x * 3 + 2] = data[(y * width + x) * 3 + 2];
        }
        png_write_row(png_ptr, row);
    }
    delete[] row;

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

Image::Image(std::string filepath, bool mipmapped, bool keepInSystemMemory) : Resource(std::move(filepath)) {
    this->bMipmapped = mipmapped;
    this->bKeepInSystemMemory = keepInSystemMemory;

    this->type = Image::TYPE::TYPE_PNG;
    this->filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
    this->wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
    this->iNumChannels = 4;
    this->iWidth = 1;
    this->iHeight = 1;

    this->bHasAlphaChannel = true;
    this->bCreatedImage = false;
}

Image::Image(int width, int height, bool mipmapped, bool keepInSystemMemory) : Resource() {
    this->bMipmapped = mipmapped;
    this->bKeepInSystemMemory = keepInSystemMemory;

    this->type = Image::TYPE::TYPE_RGBA;
    this->filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
    this->wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
    this->iNumChannels = 4;
    this->iWidth = width;
    this->iHeight = height;

    this->bHasAlphaChannel = true;
    this->bCreatedImage = true;

    // reserve and fill with pink pixels
    this->rawImage.resize(static_cast<long>(this->iWidth * this->iHeight * this->iNumChannels));
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
    bool alreadyLoaded = this->rawImage.size() > 0;

    // if it isn't a created image (created within the engine), load it from the corresponding file
    if(!this->bCreatedImage) {
        if(alreadyLoaded)  // has already been loaded (or loading it again after setPixel(s))
            return true;

        if(!env->fileExists(this->sFilePath)) {
            debugLogF("Image Error: Couldn't find file {:s}\n", this->sFilePath);
            return false;
        }

        if(this->bInterrupted)  // cancellation point
            return false;

        // load entire file
        File file(this->sFilePath);
        if(!file.canRead()) {
            debugLogF("Image Error: Couldn't canRead() file {:s}\n", this->sFilePath);
            return false;
        }
        if(file.getFileSize() < 4) {
            debugLogF("Image Error: FileSize is < 4 in file {:s}\n", this->sFilePath);
            return false;
        }

        if(this->bInterrupted)  // cancellation point
            return false;

        const char *data = file.readFile();
        if(data == NULL) {
            debugLogF("Image Error: Couldn't readFile() file {:s}\n", this->sFilePath);
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
            type = Image::TYPE::TYPE_JPG;

            // decode jpeg
            tjhandle tjInstance = tj3Init(TJINIT_DECOMPRESS);
            if(!tjInstance) {
                debugLogF("Image Error: tj3Init failed in file {:s}\n", this->sFilePath);
                return false;
            }

            if(tj3DecompressHeader(tjInstance, (unsigned char *)data, file.getFileSize()) < 0) {
                debugLogF("Image Error: tj3DecompressHeader failed: {:s} in file {:s}\n", tj3GetErrorStr(tjInstance),
                          this->sFilePath);
                tj3Destroy(tjInstance);
                return false;
            }

            if(this->bInterrupted)  // cancellation point
            {
                tj3Destroy(tjInstance);
                return false;
            }

            this->iWidth = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
            this->iHeight = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
            this->iNumChannels = 4;  // always convert to RGBA for consistency with PNG
            this->bHasAlphaChannel = true;

            if(this->iWidth > 8192 || this->iHeight > 8192) {
                debugLogF("Image Error: JPEG image size is too big ({} x {}) in file {:s}\n", this->iWidth,
                          this->iHeight, this->sFilePath);
                tj3Destroy(tjInstance);
                return false;
            }

            if(this->bInterrupted)  // cancellation point
            {
                tj3Destroy(tjInstance);
                return false;
            }

            // preallocate
            this->rawImage.resize(static_cast<long>(this->iWidth * this->iHeight * this->iNumChannels));

            // decompress directly to RGBA
            if(tj3Decompress8(tjInstance, (unsigned char *)data, file.getFileSize(), &this->rawImage[0], 0, TJPF_RGBA) <
               0) {
                debugLogF("Image Error: tj3Decompress8 failed: {:s} in file {:s}\n", tj3GetErrorStr(tjInstance),
                          this->sFilePath);
                tj3Destroy(tjInstance);
                return false;
            }

            tj3Destroy(tjInstance);
        } else if(isPNG) {
            this->type = Image::TYPE::TYPE_PNG;

            // decode png using libpng
            if(!decodePNGFromMemory((const unsigned char *)data, file.getFileSize(), this->rawImage, this->iWidth,
                                    this->iHeight, this->iNumChannels)) {
                debugLogF("Image Error: PNG decoding failed in file {:s}\n", this->sFilePath);
                return false;
            }

            this->bHasAlphaChannel = true;
        } else {
            debugLogF("Image Error: Neither PNG nor JPEG in file {:s}\n", this->sFilePath);
            return false;
        }
    }

    if(this->bInterrupted)  // cancellation point
        return false;

    // error checking

    // size sanity check
    if(this->rawImage.size() < static_cast<long>(this->iWidth * this->iHeight * this->iNumChannels)) {
        debugLogF("Image Error: Loaded image has only {}/{} bytes in file {:s}\n", (unsigned long)this->rawImage.size(),
                  this->iWidth * this->iHeight * this->iNumChannels, this->sFilePath);
        // engine->showMessageError("Image Error", UString::format("Loaded image has only %i/%i bytes in file %s",
        // rawImage.size(), iWidth*iHeight*iNumChannels, this->sFilePath));
        return false;
    }

    // supported channels sanity check
    if(this->iNumChannels != 4 && this->iNumChannels != 3 && this->iNumChannels != 1) {
        debugLogF("Image Error: Unsupported number of color channels ({}) in file {:s}\n", this->iNumChannels,
                  this->sFilePath);
        // engine->showMessageError("Image Error", UString::format("Unsupported number of color channels (%i) in file
        // %s", iNumChannels, this->sFilePath));
        return false;
    }

    // optimization: ignore completely transparent images (don't render) (only PNGs can have them, obviously)
    if(!alreadyLoaded && (type == Image::TYPE::TYPE_PNG) &&
       canHaveTransparency(this->rawImage.data(), this->rawImage.size()) && isCompletelyTransparent()) {
        if(!this->bInterrupted) debugLogF("Image: Ignoring empty transparent image {:s}\n", this->sFilePath);
        return false;
    }

    return true;
}

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

    return argb(a, r, g, b);
}

void Image::setPixel(int x, int y, Color color) {
    const int indexBegin = this->iNumChannels * y * this->iWidth + this->iNumChannels * x;
    const int indexEnd = this->iNumChannels * y * this->iWidth + this->iNumChannels * x + this->iNumChannels;

    if(this->rawImage.size() < 1 || x < 0 || y < 0 || indexEnd < 0 || indexEnd > this->rawImage.size()) return;

    this->rawImage[indexBegin + 0] = color.R();
    if(this->iNumChannels > 1) this->rawImage[indexBegin + 1] = color.G();
    if(this->iNumChannels > 2) this->rawImage[indexBegin + 2] = color.B();
    if(this->iNumChannels > 3) this->rawImage[indexBegin + 3] = color.A();
}

void Image::setPixels(const char *data, size_t size, TYPE type) {
    if(data == NULL) return;

    // TODO: implement remaining types
    switch(type) {
        case TYPE::TYPE_PNG: {
            if(!decodePNGFromMemory((const unsigned char *)data, size, this->rawImage, this->iWidth, this->iHeight,
                                    this->iNumChannels)) {
                debugLogF("Image Error: PNG decoding failed in setPixels\n");
            }
        } break;

        default:
            debugLogF("Image Error: Format not yet implemented\n");
            break;
    }
}

void Image::setPixels(const std::vector<unsigned char> &pixels) {
    if(pixels.size() < static_cast<long>(this->iWidth * this->iHeight * this->iNumChannels)) {
        debugLogF("Image Error: setPixels() supplied array is too small!\n");
        return;
    }

    rawImage = pixels;
}

// internal
bool Image::canHaveTransparency(const unsigned char *data, size_t size) {
    if(size < 33)  // not enough data for IHDR, so just assume true
        return true;

    // PNG IHDR chunk starts at offset 16 (8 bytes signature + 8 bytes chunk header)
    // color type is at offset 25 (16 + 4 width + 4 height + 1 bit depth)
    if(size > 25) {
        unsigned char colorType = data[25];
        return colorType != 2;  // RGB without alpha
    }

    return true;  // unknown format? just assume true
}

bool Image::isCompletelyTransparent() const {
    if(rawImage.empty() || this->iNumChannels < 4 || !this->bHasAlphaChannel) return false;

    const size_t alphaOffset = 3;
    const size_t stride = this->iNumChannels;
    const size_t totalPixels = this->iWidth * this->iHeight;

    for(size_t i = 0; i < totalPixels; ++i) {
        if(this->bInterrupted)  // cancellation point
            return false;

        // check alpha channel directly
        if(this->rawImage[i * stride + alphaOffset] > 0) return false;  // non-transparent pixel
    }

    return true;  // all pixels are transparent
}
