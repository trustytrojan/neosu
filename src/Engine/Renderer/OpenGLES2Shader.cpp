//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		OpenGLES2 GLSL implementation of Shader
//
// $NoKeywords: $gles2shader
//===============================================================================//

#include "OpenGLES2Shader.h"

#ifdef MCENGINE_FEATURE_OPENGLES

#include "ConVar.h"
#include "Engine.h"
#include "OpenGLHeaders.h"

OpenGLES2Shader::OpenGLES2Shader(std::string vertexShader, std::string fragmentShader, bool source) : Shader() {
    m_sVsh = vertexShader;
    m_sFsh = fragmentShader;
    m_bSource = source;

    m_iProgram = 0;
    m_iVertexShader = 0;
    m_iFragmentShader = 0;

    m_iProgramBackup = 0;
}

void OpenGLES2Shader::init() { this->bReady = compile(this->sVsh, this->sFsh, this->bSource); }

void OpenGLES2Shader::initAsync() { this->bAsyncReady = true; }

void OpenGLES2Shader::destroy() {
    if(this->iProgram != 0) glDeleteProgram(this->iProgram);
    if(this->iFragmentShader != 0) glDeleteShader(this->iFragmentShader);
    if(this->iVertexShader != 0) glDeleteShader(this->iVertexShader);

    m_iProgram = 0;
    m_iFragmentShader = 0;
    m_iVertexShader = 0;

    m_iProgramBackup = 0;
}

void OpenGLES2Shader::enable() {
    if(!this->bReady) return;

    glGetIntegerv(GL_CURRENT_PROGRAM, &m_iProgramBackup);  // backup
    glUseProgram(this->iProgram);
}

void OpenGLES2Shader::disable() {
    if(!this->bReady) return;

    glUseProgram(this->iProgramBackup);  // restore
}

void OpenGLES2Shader::setUniform1f(UString name, float value) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform1f(id, value);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform1fv(UString name, int count, float *values) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform1fv(id, count, values);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform1i(UString name, int value) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform1i(id, value);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform2f(UString name, float value1, float value2) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform2f(id, value1, value2);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform2fv(UString name, int count, float *vectors) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform2fv(id, count, (float *)&vectors[0]);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform3f(UString name, float x, float y, float z) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform3f(id, x, y, z);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform3fv(UString name, int count, float *vectors) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform3fv(id, count, (float *)&vectors[0]);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniform4f(UString name, float x, float y, float z, float w) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniform4f(id, x, y, z, w);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniformMatrix4fv(UString name, Matrix4 &matrix) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniformMatrix4fv(id, 1, GL_FALSE, matrix.get());
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLES2Shader::setUniformMatrix4fv(UString name, float *v) {
    if(!this->bReady) return;
    int id = glGetUniformLocation(this->iProgram, name.toUtf8());
    if(id != -1)
        glUniformMatrix4fv(id, 1, GL_FALSE, v);
    else if(debug_shaders->getBool())
        debugLog("OpenGLES2Shader Warning: Can't find uniform %s\n", name.toUtf8());
}

int OpenGLES2Shader::getAttribLocation(UString name) {
    if(!this->bReady) return -1;
    return glGetAttribLocation(this->iProgram, name.toUtf8());
}

bool OpenGLES2Shader::isActive() {
    int currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    return (this->bReady && currentProgram == m_iProgram);
}

bool OpenGLES2Shader::compile(std::string vertexShader, std::string fragmentShader, bool source) {
    // load & compile shaders
    debugLog("OpenGLES2Shader: Compiling %s ...\n", (source ? "vertex source" : vertexShader.c_str()));
    m_iVertexShader = source ? createShaderFromString(vertexShader, GL_VERTEX_SHADER)
                             : createShaderFromFile(vertexShader, GL_VERTEX_SHADER);
    debugLog("OpenGLES2Shader: Compiling %s ...\n", (source ? "fragment source" : fragmentShader.c_str()));
    m_iFragmentShader = source ? createShaderFromString(fragmentShader, GL_FRAGMENT_SHADER)
                               : createShaderFromFile(fragmentShader, GL_FRAGMENT_SHADER);

    if(this->iVertexShader == 0 || m_iFragmentShader == 0) {
        engine->showMessageError("OpenGLES2Shader Error", "Couldn't createShader()");
        return false;
    }

    // create program
    m_iProgram = (int)glCreateProgram();
    if(this->iProgram == 0) {
        engine->showMessageError("OpenGLES2Shader Error", "Couldn't glCreateProgram()");
        return false;
    }

    // attach
    glAttachShader(this->iProgram, this->iVertexShader);
    glAttachShader(this->iProgram, this->iFragmentShader);

    // link
    glLinkProgram(this->iProgram);

    GLint ret = GL_FALSE;
    glGetProgramiv(this->iProgram, GL_LINK_STATUS, &ret);
    if(ret == GL_FALSE) {
        engine->showMessageError("OpenGLES2Shader Error", "Couldn't glLinkProgram()");
        return false;
    }

    // validate
    ret = GL_FALSE;
    glValidateProgram(this->iProgram);
    glGetProgramiv(this->iProgram, GL_VALIDATE_STATUS, &ret);
    if(ret == GL_FALSE) {
        engine->showMessageError("OpenGLES2Shader Error", "Couldn't glValidateProgram()");
        return false;
    }

    return true;
}

int OpenGLES2Shader::createShaderFromString(std::string shaderSource, int shaderType) {
    const GLint shader = glCreateShader(shaderType);

    if(shader == 0) {
        engine->showMessageError("OpenGLES2Shader Error", "Couldn't glCreateShader()");
        return 0;
    }

    // compile shader
    const char *shaderSourceChar = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourceChar, NULL);
    glCompileShader(shader);

    GLint ret = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
    if(ret == GL_FALSE) {
        debugLog("------------------OpenGLES2Shader Compile Error------------------\n");

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);
        char *errorLog = new char[ret];
        {
            glGetShaderInfoLog(shader, ret, &ret, errorLog);
            debugLog(errorLog);
        }
        delete[] errorLog;

        debugLog("-----------------------------------------------------------------\n");

        engine->showMessageError("OpenGLES2Shader Error", "Couldn't glShaderSource() or glCompileShader()");
        return 0;
    }

    return shader;
}

int OpenGLES2Shader::createShaderFromFile(std::string fileName, int shaderType) {
    // load file
    std::ifstream inFile(fileName.c_str());
    if(!inFile) {
        engine->showMessageError("OpenGLES2Shader Error", fileName);
        return 0;
    }
    std::string line;
    std::string shaderSource;
    int linecount = 0;
    while(inFile.good()) {
        std::getline(inFile, line);
        shaderSource += line + "\n\0";
        linecount++;
    }
    shaderSource += "\n\0";
    inFile.close();

    std::string shaderSourcePtr = shaderSource;

    return createShaderFromString(shaderSourcePtr, shaderType);
}

#endif
