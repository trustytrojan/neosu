//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL GLSL implementation of Shader
//
// $NoKeywords: $glshader
//===============================================================================//

#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_OPENGL

class OpenGLShader : public Shader {
   public:
    OpenGLShader(std::string vertexShader, std::string fragmentShader, bool source);
    ~OpenGLShader() override { this->destroy(); }

    void enable() override;
    void disable() override;

    void setUniform1f(UString name, float value) override;
    void setUniform1fv(UString name, int count, float *values) override;
    void setUniform1i(UString name, int value) override;
    void setUniform2f(UString name, float x, float y) override;
    void setUniform2fv(UString name, int count, float *vectors) override;
    void setUniform3f(UString name, float x, float y, float z) override;
    void setUniform3fv(UString name, int count, float *vectors) override;
    void setUniform4f(UString name, float x, float y, float z, float w) override;
    void setUniformMatrix4fv(UString name, Matrix4 &matrix) override;
    void setUniformMatrix4fv(UString name, float *v) override;

    // ILLEGAL:
    int getAttribLocation(UString name);
    int getAndCacheUniformLocation(const UString &name);

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    bool compile(std::string vertexShader, std::string fragmentShader, bool source);
    int createShaderFromString(std::string shaderSource, int shaderType);
    int createShaderFromFile(std::string fileName, int shaderType);

    std::string sVsh, sFsh;

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
