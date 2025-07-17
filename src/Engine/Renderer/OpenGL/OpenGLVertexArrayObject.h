//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		OpenGL baking support for vao
//
// $NoKeywords: $glvao
//===============================================================================//

#pragma once
#ifndef OPENGLVERTEXARRAYOBJECT_H
#define OPENGLVERTEXARRAYOBJECT_H

#include "VertexArrayObject.h"

#ifdef MCENGINE_FEATURE_OPENGL

class OpenGLVertexArrayObject final : public VertexArrayObject
{
public:
	OpenGLVertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);
	~OpenGLVertexArrayObject() override {destroy();}

	OpenGLVertexArrayObject &operator=(const OpenGLVertexArrayObject &) = delete;
	OpenGLVertexArrayObject &operator=(OpenGLVertexArrayObject &&) = delete;

	OpenGLVertexArrayObject(const OpenGLVertexArrayObject &) = delete;
	OpenGLVertexArrayObject(OpenGLVertexArrayObject &&) = delete;

	void draw() override;

private:
	static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
	static unsigned int usageToOpenGL(Graphics::USAGE_TYPE usage);

	void init() override;
	void initAsync() override;
	void destroy() override;

	static inline Color ARGBtoABGR(Color color) {return ((color & 0xff000000) >> 0) | ((color & 0x00ff0000) >> 16) | ((color & 0x0000ff00) << 0) | ((color & 0x000000ff) << 16);}

	unsigned int iVertexBuffer;
	unsigned int iTexcoordBuffer;
	unsigned int iColorBuffer;
	unsigned int iNormalBuffer;

	unsigned int iNumTexcoords;
	unsigned int iNumColors;
	unsigned int iNumNormals;

	unsigned int iVertexArray;
};

#endif

#endif
