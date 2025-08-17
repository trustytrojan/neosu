#pragma once
// Copyright (c) 2012, PG, All rights reserved.
#include "Resource.h"

struct Matrix4;

class Shader : public Resource {
   public:
    Shader() : Resource() { ; }
    ~Shader() override { ; }

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual void setUniform1f(const std::string_view &name, float value) = 0;
    virtual void setUniform1fv(const std::string_view &name, int count, float *values) = 0;
    virtual void setUniform1i(const std::string_view &name, int value) = 0;
    virtual void setUniform2f(const std::string_view &name, float x, float y) = 0;
    virtual void setUniform2fv(const std::string_view &name, int count, float *vectors) = 0;
    virtual void setUniform3f(const std::string_view &name, float x, float y, float z) = 0;
    virtual void setUniform3fv(const std::string_view &name, int count, float *vectors) = 0;
    virtual void setUniform4f(const std::string_view &name, float x, float y, float z, float w) = 0;
    virtual void setUniformMatrix4fv(const std::string_view &name, Matrix4 &matrix) = 0;
    virtual void setUniformMatrix4fv(const std::string_view &name, float *v) = 0;

    // type inspection
    [[nodiscard]] Type getResType() const final { return SHADER; }

    Shader *asShader() final { return this; }
    [[nodiscard]] const Shader *asShader() const final { return this; }

   protected:
    void init() override = 0;
    void initAsync() override = 0;
    void destroy() override = 0;
};
