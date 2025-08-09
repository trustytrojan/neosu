// Copyright (c) 2016, PG, All rights reserved.
#include "OpenGLImage.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

#include <utility>
#include <algorithm>

#include "ResourceManager.h"
#include "Environment.h"
#include "Engine.h"
#include "ConVar.h"
#include "File.h"

#include "OpenGLHeaders.h"

OpenGLImage::OpenGLImage(std::string filepath, bool mipmapped, bool keepInSystemMemory)
    : Image(std::move(filepath), mipmapped, keepInSystemMemory) {
    this->GLTexture = 0;
    this->iTextureUnitBackup = 0;
}

OpenGLImage::OpenGLImage(i32 width, i32 height, bool mipmapped, bool keepInSystemMemory)
    : Image(width, height, mipmapped, keepInSystemMemory) {
    this->GLTexture = 0;
    this->iTextureUnitBackup = 0;
}

void OpenGLImage::init() {
    if((this->GLTexture != 0 && !this->bKeepInSystemMemory) || !(this->bAsyncReady.load()))
        return;  // only load if we are not already loaded

    // create texture object
    if(this->GLTexture == 0) {
        // DEPRECATED LEGACY (1)
        if constexpr(Env::cfg(REND::GL)) glEnable(GL_TEXTURE_2D);

        // create texture and bind
        glGenTextures(1, &this->GLTexture);
        glBindTexture(GL_TEXTURE_2D, this->GLTexture);

        // set texture filtering mode (mipmapping is disabled by default)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->bMipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // texture wrapping, defaults to clamp
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // upload to gpu
    {
        glBindTexture(GL_TEXTURE_2D, this->GLTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->iWidth, this->iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     &this->rawImage[0]);
        if(this->bMipmapped) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    if(this->rawImage.empty()) {
        auto GLerror = glGetError();
        debugLog("OpenGL Image Error: {} on file {:s}!\n", GLerror, this->sFilePath.c_str());
        engine->showMessageError("Image Error",
                                 UString::format("OpenGL Image error %i on file %s", GLerror, this->sFilePath.c_str()));
        return;
    }

    // free memory
    if(!this->bKeepInSystemMemory) this->rawImage.clear();

    this->bReady = true;

    if(this->filterMode != Graphics::FILTER_MODE::FILTER_MODE_LINEAR) setFilterMode(this->filterMode);

    if(this->wrapMode != Graphics::WRAP_MODE::WRAP_MODE_CLAMP) setWrapMode(this->wrapMode);
}

void OpenGLImage::initAsync() {
    if(this->GLTexture != 0) return;  // only load if we are not already loaded

    if(!this->bCreatedImage) {
        if(cv::debug_rm.getBool()) debugLog("Resource Manager: Loading {:s}\n", this->sFilePath.c_str());

        this->bAsyncReady = loadRawImage();
    }
}

void OpenGLImage::destroy() {
    if(this->GLTexture != 0) {
        glDeleteTextures(1, &this->GLTexture);
        this->GLTexture = 0;
    }

    this->rawImage.clear();
}

void OpenGLImage::bind(unsigned int textureUnit) {
    if(!this->bReady) return;

    this->iTextureUnitBackup = textureUnit;

    // switch texture units before enabling+binding
    glActiveTexture(GL_TEXTURE0 + textureUnit);

    // set texture
    glBindTexture(GL_TEXTURE_2D, this->GLTexture);

    // DEPRECATED LEGACY (2)
    if constexpr(Env::cfg(REND::GL)) glEnable(GL_TEXTURE_2D);
}

void OpenGLImage::unbind() {
    if(!this->bReady) return;

    // restore texture unit (just in case) and set to no texture
    glActiveTexture(GL_TEXTURE0 + this->iTextureUnitBackup);
    glBindTexture(GL_TEXTURE_2D, 0);

    // restore default texture unit
    if(this->iTextureUnitBackup != 0) glActiveTexture(GL_TEXTURE0);
}

void OpenGLImage::setFilterMode(Graphics::FILTER_MODE filterMode) {
    Image::setFilterMode(filterMode);
    if(!this->bReady) return;

    bind();
    {
        switch(filterMode) {
            case Graphics::FILTER_MODE::FILTER_MODE_NONE:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            case Graphics::FILTER_MODE::FILTER_MODE_LINEAR:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            case Graphics::FILTER_MODE::FILTER_MODE_MIPMAP:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
        }
    }
    unbind();
}

void OpenGLImage::setWrapMode(Graphics::WRAP_MODE wrapMode) {
    Image::setWrapMode(wrapMode);
    if(!this->bReady) return;

    bind();
    {
        switch(wrapMode) {
            case Graphics::WRAP_MODE::WRAP_MODE_CLAMP:  // NOTE: there is also GL_CLAMP, which works a bit differently
                                                        // concerning the border color
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case Graphics::WRAP_MODE::WRAP_MODE_REPEAT:
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
        }
    }
    unbind();
}

void OpenGLImage::handleGLErrors() {
    // no
}

#endif
