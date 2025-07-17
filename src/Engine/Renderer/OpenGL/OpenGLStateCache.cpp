//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGL state cache for reducing synchronization points (WIP)
//
// $NoKeywords: $glsc
//===============================================================================//

#include "OpenGLStateCache.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

#include "ConVar.h"
#include "OpenGLHeaders.h"

OpenGLStateCache *OpenGLStateCache::s_instance = nullptr;

OpenGLStateCache::OpenGLStateCache()
{
	this->iCurrentProgram = 0;
	this->iCurrentFramebuffer = 0;
	this->iViewport[0] = 0;
	this->iViewport[1] = 0;
	this->iViewport[2] = 0;
	this->iViewport[3] = 0;
	this->bInitialized = false;
}

OpenGLStateCache &OpenGLStateCache::getInstance()
{
	if (s_instance == nullptr)
		s_instance = new OpenGLStateCache();

	return *s_instance;
}

void OpenGLStateCache::initialize()
{
	if (this->bInitialized)
		return;

	// one-time initialization of cache from actual GL state
	refresh();
	this->bInitialized = true;
}

void OpenGLStateCache::refresh()
{
	// only do the expensive query when necessary
	glGetIntegerv(GL_CURRENT_PROGRAM, &this->iCurrentProgram);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &this->iCurrentFramebuffer);
	glGetIntegerv(GL_VIEWPORT, this->iViewport);
}

void OpenGLStateCache::setCurrentProgram(int program)
{
	this->iCurrentProgram = program;
}

int OpenGLStateCache::getCurrentProgram() const
{
	return this->iCurrentProgram;
}

void OpenGLStateCache::setCurrentFramebuffer(int framebuffer)
{
	this->iCurrentFramebuffer = framebuffer;
}

int OpenGLStateCache::getCurrentFramebuffer() const
{
	return this->iCurrentFramebuffer;
}

void OpenGLStateCache::setCurrentViewport(int x, int y, int width, int height)
{
	this->iViewport[0] = x;
	this->iViewport[1] = y;
	this->iViewport[2] = width;
	this->iViewport[3] = height;
}

void OpenGLStateCache::getCurrentViewport(int &x, int &y, int &width, int &height)
{
	x = this->iViewport[0];
	y = this->iViewport[1];
	width = this->iViewport[2];
	height = this->iViewport[3];
}

#endif
