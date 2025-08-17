#pragma once
// Copyright (c) 2016, PG, All rights reserved.
#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include <unordered_map>

class OpenGLShader final : public Shader {
   public:
    OpenGLShader(std::string vertexShader, std::string fragmentShader, bool source);
    ~OpenGLShader() override { destroy(); }

    void enable() override;
    void disable() override;

    void setUniform1f(const std::string_view &name, float value) override;
    void setUniform1fv(const std::string_view &name, int count, float *values) override;
    void setUniform1i(const std::string_view &name, int value) override;
    void setUniform2f(const std::string_view &name, float x, float y) override;
    void setUniform2fv(const std::string_view &name, int count, float *vectors) override;
    void setUniform3f(const std::string_view &name, float x, float y, float z) override;
    void setUniform3fv(const std::string_view &name, int count, float *vectors) override;
    void setUniform4f(const std::string_view &name, float x, float y, float z, float w) override;
    void setUniformMatrix4fv(const std::string_view &name, Matrix4 &matrix) override;
    void setUniformMatrix4fv(const std::string_view &name, float *v) override;

   protected:
    void init() override;
    void initAsync() override;
    void destroy() override;

   private:
    bool compile(const std::string &vertexShader, const std::string &fragmentShader, bool source);
    int createShaderFromString(const std::string &shaderSource, int shaderType);
    int createShaderFromFile(const std::string &fileName, int shaderType);

    int getAttribLocation(const std::string_view &name);
    int getAndCacheUniformLocation(const std::string_view &name);

    std::string sVsh;
    std::string sFsh;

    bool bSource;
    int iVertexShader;
    int iFragmentShader;
    int iProgram;

    int iProgramBackup;

    std::unordered_map<std::string_view, int> uniformLocationCache;
};

#endif

#endif
