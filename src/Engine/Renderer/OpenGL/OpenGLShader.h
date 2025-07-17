//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL GLSL implementation of Shader
//
// $NoKeywords: $glshader
//===============================================================================//

#pragma once
#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_OPENGL

class OpenGLShader final : public Shader {
   public:
    OpenGLShader(std::string vertexShader, std::string fragmentShader, bool source);
    ~OpenGLShader() override { destroy(); }

    void enable() override;
    void disable() override;

    void setUniform1f(const UString &name, float value) override;
    void setUniform1fv(const UString &name, int count, float *values) override;
    void setUniform1i(const UString &name, int value) override;
    void setUniform2f(const UString &name, float x, float y) override;
    void setUniform2fv(const UString &name, int count, float *vectors) override;
    void setUniform3f(const UString &name, float x, float y, float z) override;
    void setUniform3fv(const UString &name, int count, float *vectors) override;
    void setUniform4f(const UString &name, float x, float y, float z, float w) override;
    void setUniformMatrix4fv(const UString &name, Matrix4 &matrix) override;
    void setUniformMatrix4fv(const UString &name, float *v) override;

    // ILLEGAL:
    int getAttribLocation(const UString &name);
    int getAndCacheUniformLocation(const UString &name);

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

   private:
    bool compile(const std::string &vertexShader, const std::string &fragmentShader, bool source);
    int createShaderFromString(const std::string &shaderSource, int shaderType);
    int createShaderFromFile(const std::string &fileName, int shaderType);

   private:
    std::string sVsh;
    std::string sFsh;

    bool bSource;
    int iVertexShader;
    int iFragmentShader;
    int iProgram;

    int iProgramBackup;

    std::unordered_map<std::string, int> uniformLocationCache;
    std::string sTempStringBuffer;
};

#endif

#endif
