//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of Image
//
// $NoKeywords: $glimg
//===============================================================================//

#ifndef OPENGLIMAGE_H
#define OPENGLIMAGE_H

#include "Image.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

class OpenGLImage : public Image {
   public:
    OpenGLImage(std::string filepath, bool mipmapped = false);
    OpenGLImage(int width, int height, bool mipmapped = false);
    ~OpenGLImage() override { this->destroy(); }

    void bind(unsigned int textureUnit = 0) override;
    void unbind() override;

    void setFilterMode(Graphics::FILTER_MODE filterMode) override;
    void setWrapMode(Graphics::WRAP_MODE wrapMode) override;

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    void handleGLErrors();

    unsigned int GLTexture;
    unsigned int iTextureUnitBackup;
};

#endif

#endif
