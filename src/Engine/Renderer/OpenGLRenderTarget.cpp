//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of RenderTarget / render to texture
//
// $NoKeywords: $glrt
//===============================================================================//

#include "OpenGLRenderTarget.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

#include "ConVar.h"
#include "Engine.h"
#include "OpenGLHeaders.h"
#include "VertexArrayObject.h"

OpenGLRenderTarget::OpenGLRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
    : RenderTarget(x, y, width, height, multiSampleType) {
    this->iFrameBuffer = 0;
    this->iRenderTexture = 0;
    this->iDepthBuffer = 0;
    this->iResolveTexture = 0;
    this->iResolveFrameBuffer = 0;

    this->iFrameBufferBackup = 0;
    this->iTextureUnitBackup = 0;

    this->iViewportBackup[0] = 0;
    this->iViewportBackup[1] = 0;
    this->iViewportBackup[2] = 0;
    this->iViewportBackup[3] = 0;
}

void OpenGLRenderTarget::init() {
    debugLog("Building RenderTarget (%ix%i) ...\n", (int)this->vSize.x, (int)this->vSize.y);

    this->iFrameBuffer = 0;
    this->iRenderTexture = 0;
    this->iDepthBuffer = 0;
    this->iResolveTexture = 0;
    this->iResolveFrameBuffer = 0;

    int numMultiSamples = 2;
    switch(this->multiSampleType) {
        case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X: // spec: i guess 0x isn't desirable? seems like it's handled in "if (isMultiSampled())"
            break;
        case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_2X:
            numMultiSamples = 2;
            break;
        case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_4X:
            numMultiSamples = 4;
            break;
        case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_8X:
            numMultiSamples = 8;
            break;
        case Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_16X:
            numMultiSamples = 16;
            break;
    }

    // create framebuffer
    glGenFramebuffers(1, &this->iFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, this->iFrameBuffer);
    if(this->iFrameBuffer == 0) {
        engine->showMessageError("RenderTarget Error", "Couldn't glGenFramebuffers() or glBindFramebuffer()!");
        return;
    }

    // create depth buffer
    glGenRenderbuffers(1, &this->iDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, this->iDepthBuffer);
    if(this->iDepthBuffer == 0) {
        engine->showMessageError("RenderTarget Error", "Couldn't glGenRenderBuffers() or glBindRenderBuffer()!");
        return;
    }

    // fill depth buffer
#ifdef MCENGINE_FEATURE_OPENGL

    if(this->isMultiSampled())
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, numMultiSamples, GL_DEPTH_COMPONENT, (int)this->vSize.x,
                                         (int)this->vSize.y);
    else
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (int)this->vSize.x, (int)this->vSize.y);

#else

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, (int)this->vSize.x, (int)this->vSize.y);

#endif

    // set depth buffer as depth attachment on the framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->iDepthBuffer);

    // create texture
    glGenTextures(1, &this->iRenderTexture);

#ifdef MCENGINE_FEATURE_OPENGL

    glBindTexture(this->isMultiSampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, this->iRenderTexture);

#else

    glBindTexture(GL_TEXTURE_2D, this->iRenderTexture);

#endif

    if(this->iRenderTexture == 0) {
        engine->showMessageError("RenderTarget Error", "Couldn't glGenTextures() or glBindTexture()!");
        return;
    }

    // fill texture
#ifdef MCENGINE_FEATURE_OPENGL

    if(this->isMultiSampled())
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numMultiSamples, GL_RGBA8, (int)this->vSize.x,
                                (int)this->vSize.y,
                                true);  // use fixed sample locations
    else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)this->vSize.x, (int)this->vSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     NULL);

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);              // no mipmapping atm
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // disable texture wrap
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

#else

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)this->vSize.x, (int)this->vSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // disable texture wrap
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#endif

    // set render texture as color attachment0 on the framebuffer
#ifdef MCENGINE_FEATURE_OPENGL

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           this->isMultiSampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, this->iRenderTexture, 0);

#else

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->iRenderTexture, 0);

#endif

    // if multisampled, create resolve framebuffer/texture
#ifdef MCENGINE_FEATURE_OPENGL

    if(this->isMultiSampled()) {
        if(this->iResolveFrameBuffer == 0) {
            // create resolve framebuffer
            glGenFramebuffers(1, &this->iResolveFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, this->iResolveFrameBuffer);
            if(this->iResolveFrameBuffer == 0) {
                engine->showMessageError("RenderTarget Error",
                                         "Couldn't glGenFramebuffers() or glBindFramebuffer() multisampled!");
                return;
            }

            // create resolve texture
            glGenTextures(1, &this->iResolveTexture);
            glBindTexture(GL_TEXTURE_2D, this->iResolveTexture);
            if(this->iResolveTexture == 0) {
                engine->showMessageError("RenderTarget Error",
                                         "Couldn't glGenTextures() or glBindTexture() multisampled!");
                return;
            }

            // set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);  // no mips

            // fill texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)this->vSize.x, (int)this->vSize.y, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, NULL);

            // set resolve texture as color attachment0 on the resolve framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->iResolveTexture, 0);
        }
    }

#endif

    // error checking
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        engine->showMessageError("RenderTarget Error",
                                 UString::format("!GL_FRAMEBUFFER_COMPLETE, size = (%ix%i), multisampled = %i",
                                                 (int)this->vSize.x, (int)this->vSize.y, (int)this->isMultiSampled()));
        return;
    }

    // reset bound texture and framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_bReady = true;
}

void OpenGLRenderTarget::initAsync() { m_bAsyncReady = true; }

void OpenGLRenderTarget::destroy() {
    if(this->iResolveTexture != 0) glDeleteTextures(1, &this->iResolveTexture);
    if(this->iResolveFrameBuffer != 0) glDeleteFramebuffers(1, &this->iResolveFrameBuffer);
    if(this->iRenderTexture != 0) glDeleteTextures(1, &this->iRenderTexture);
    if(this->iDepthBuffer != 0) glDeleteRenderbuffers(1, &this->iDepthBuffer);
    if(this->iFrameBuffer != 0) glDeleteFramebuffers(1, &this->iFrameBuffer);

    this->iFrameBuffer = 0;
    this->iRenderTexture = 0;
    this->iDepthBuffer = 0;
    this->iResolveTexture = 0;
    this->iResolveFrameBuffer = 0;
}

void OpenGLRenderTarget::enable() {
    if(!m_bReady) return;

    // bind framebuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &this->iFrameBufferBackup);  // backup
    glBindFramebuffer(GL_FRAMEBUFFER, this->iFrameBuffer);

    // set new viewport
    glGetIntegerv(GL_VIEWPORT, this->iViewportBackup);  // backup
    glViewport(-this->vPos.x, (this->vPos.y - g->getResolution().y) + this->vSize.y,
               g->getResolution().x, g->getResolution().y);

    // clear
    if(cv_debug_rt.getBool())
        glClearColor(0.0f, 0.5f, 0.0f, 0.5f);
    else
        glClearColor(COLOR_GET_Rf(this->clearColor), COLOR_GET_Gf(this->clearColor), COLOR_GET_Bf(this->clearColor),
                     COLOR_GET_Af(this->clearColor));

    if(this->bClearColorOnDraw || this->bClearDepthOnDraw)
        glClear((this->bClearColorOnDraw ? GL_COLOR_BUFFER_BIT : 0) |
                (this->bClearDepthOnDraw ? GL_DEPTH_BUFFER_BIT : 0));
}

void OpenGLRenderTarget::disable() {
    if(!m_bReady) return;

        // if multisampled, blit content for multisampling into resolve texture
#ifdef MCENGINE_FEATURE_OPENGL

    if(this->isMultiSampled()) {
        // HACKHACK: force disable antialiasing
        g->setAntialiasing(false);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->iFrameBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->iResolveFrameBuffer);

        // for multisampled, the sizes MUST be the same! you can't blit from multisampled into non-multisampled or
        // different size
        glBlitFramebuffer(0, 0, (int)this->vSize.x, (int)this->vSize.y, 0, 0, (int)this->vSize.x, (int)this->vSize.y,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

#endif

    // restore viewport
    glViewport(this->iViewportBackup[0], this->iViewportBackup[1], this->iViewportBackup[2], this->iViewportBackup[3]);

    // restore framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, this->iFrameBufferBackup);
}

void OpenGLRenderTarget::bind(unsigned int textureUnit) {
    if(!m_bReady) return;

    this->iTextureUnitBackup = textureUnit;

    // switch texture units before enabling+binding
    glActiveTexture(GL_TEXTURE0 + textureUnit);

    // set texture
    glBindTexture(GL_TEXTURE_2D, this->isMultiSampled() ? this->iResolveTexture : this->iRenderTexture);

    // needed for legacy support (OpenGLLegacyInterface)
    // DEPRECATED LEGACY
    glEnable(GL_TEXTURE_2D);
    glGetError();  // clear gl error state
}

void OpenGLRenderTarget::unbind() {
    if(!m_bReady) return;

    // restore texture unit (just in case) and set to no texture
    glActiveTexture(GL_TEXTURE0 + this->iTextureUnitBackup);
    glBindTexture(GL_TEXTURE_2D, 0);

    // restore default texture unit
    if(this->iTextureUnitBackup != 0) glActiveTexture(GL_TEXTURE0);
}

void OpenGLRenderTarget::blitResolveFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt) {
#ifdef MCENGINE_FEATURE_OPENGL

    if(this->isMultiSampled()) {
        // HACKHACK: force disable antialiasing
        g->setAntialiasing(false);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->iResolveFrameBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->getFrameBuffer());

        glBlitFramebuffer(0, 0, (int)this->vSize.x, (int)this->vSize.y, 0, 0, (int)rt->getWidth(), (int)rt->getHeight(),
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

#endif
}

void OpenGLRenderTarget::blitFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt) {
#ifdef MCENGINE_FEATURE_OPENGL

    // HACKHACK: force disable antialiasing
    g->setAntialiasing(false);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->iFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->getFrameBuffer());

    glBlitFramebuffer(0, 0, (int)this->vSize.x, (int)this->vSize.y, 0, 0, (int)rt->getWidth(), (int)rt->getHeight(),
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#endif
}

#endif
