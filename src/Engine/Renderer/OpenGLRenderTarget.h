//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of RenderTarget / render to texture
//
// $NoKeywords: $glrt
//===============================================================================//

#ifndef OPENGLRENDERTARGET_H
#define OPENGLRENDERTARGET_H

#include "RenderTarget.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

class OpenGLRenderTarget : public RenderTarget {
   public:
    OpenGLRenderTarget(int x, int y, int width, int height,
                       Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
    virtual ~OpenGLRenderTarget() { this->destroy(); }

    virtual void enable();
    virtual void disable();

    virtual void bind(unsigned int textureUnit = 0);
    virtual void unbind();

    // ILLEGAL:
    void blitResolveFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
    void blitFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
    inline unsigned int getFrameBuffer() const { return this->iFrameBuffer; }
    inline unsigned int getRenderTexture() const { return this->iRenderTexture; }
    inline unsigned int getResolveFrameBuffer() const { return this->iResolveFrameBuffer; }
    inline unsigned int getResolveTexture() const { return this->iResolveTexture; }

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy();

    unsigned int iFrameBuffer;
    unsigned int iRenderTexture;
    unsigned int iDepthBuffer;
    unsigned int iResolveFrameBuffer;
    unsigned int iResolveTexture;

    int iFrameBufferBackup;
    unsigned int iTextureUnitBackup;
    int iViewportBackup[4];
};

#endif

#endif
