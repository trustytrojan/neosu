//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of RenderTarget / render to texture
//
// $NoKeywords: $glrt
//===============================================================================//

#pragma once
#ifndef OPENGLRENDERTARGET_H
#define OPENGLRENDERTARGET_H

#include "RenderTarget.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

class OpenGLRenderTarget final : public RenderTarget
{
public:
	OpenGLRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
	~OpenGLRenderTarget() override {destroy();}

	void enable() override;
	void disable() override;

	void bind(unsigned int textureUnit = 0) override;
	void unbind() override;

	// ILLEGAL:
	void blitResolveFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
	void blitFrameBufferIntoFrameBuffer(OpenGLRenderTarget *rt);
	[[nodiscard]] inline unsigned int getFrameBuffer() const {return this->iFrameBuffer;}
	[[nodiscard]] inline unsigned int getRenderTexture() const {return this->iRenderTexture;}
	[[nodiscard]] inline unsigned int getResolveFrameBuffer() const {return this->iResolveFrameBuffer;}
	[[nodiscard]] inline unsigned int getResolveTexture() const {return this->iResolveTexture;}

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

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
