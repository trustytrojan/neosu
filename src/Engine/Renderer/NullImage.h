//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of Image
//
// $NoKeywords: $nimg
//===============================================================================//

#ifndef NULLIMAGE_H
#define NULLIMAGE_H

#include "Image.h"

#include <utility>

class NullImage : public Image {
   public:
    NullImage(std::string filePath, bool mipmapped = false, bool keepInSystemMemory = false) : Image(std::move(filePath), mipmapped, keepInSystemMemory) { ; }
    NullImage(int width, int height, bool mipmapped = false, bool keepInSystemMemory = false) : Image(width, height, mipmapped, keepInSystemMemory) { ; }
    ~NullImage() override { this->destroy(); }

    void bind(unsigned int textureUnit = 0) override { ; }
    void unbind() override { ; }

    void setFilterMode(Graphics::FILTER_MODE filterMode) override;
    void setWrapMode(Graphics::WRAP_MODE wrapMode) override;

   private:
    void init() override { this->bReady = true; }
    void initAsync() override { this->bAsyncReady = true; }
    void destroy() override { ; }
};

#endif
