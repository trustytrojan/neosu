//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of Shader
//
// $NoKeywords: $nshader
//===============================================================================//

#ifndef NULLSHADER_H
#define NULLSHADER_H

#include "Shader.h"

class NullShader : public Shader
{
public:
	NullShader(UString vertexShader, UString fragmentShader, bool source) : Shader() {;}
	virtual ~NullShader() {destroy();}

	virtual void enable() {;}
	virtual void disable() {;}

	virtual void setUniform1f(UString name, float value) {
		(void)name; (void)value;
	}
	virtual void setUniform1fv(UString name, int count, float *values) {
		(void)name; (void)count; (void)values;
	}
	virtual void setUniform1i(UString name, int value) {
		(void)name; (void)value;
	}
	virtual void setUniform2f(UString name, float x, float y) {
		(void)name; (void)x; (void)y;
	}
	virtual void setUniform2fv(UString name, int count, float *vectors) {
		(void)name; (void)count; (void)vectors;
	}
	virtual void setUniform3f(UString name, float x, float y, float z) {
		(void)name; (void)x; (void)y; (void)z;
	}
	virtual void setUniform3fv(UString name, int count, float *vectors) {
		(void)name; (void)count; (void)vectors;
	}
	virtual void setUniform4f(UString name, float x, float y, float z, float w) {
		(void)name; (void)x; (void)y; (void)z; (void)w;
	}
	virtual void setUniformMatrix4fv(UString name, Matrix4 &matrix) {
		(void)name; (void)matrix;
	}
	virtual void setUniformMatrix4fv(UString name, float *v) {
		(void)name; (void)v;
	}

private:
	virtual void init() {m_bReady = true;}
	virtual void initAsync() {m_bAsyncReady = true;}
	virtual void destroy() {;}
};

#endif
