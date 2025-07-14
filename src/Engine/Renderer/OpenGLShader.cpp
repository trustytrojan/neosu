#include "OpenGLShader.h"

#include <utility>

#ifdef MCENGINE_FEATURE_OPENGL

#include "ConVar.h"
#include "Engine.h"
#include "OpenGLHeaders.h"

OpenGLShader::OpenGLShader(std::string vertexShader, std::string fragmentShader, bool source) : Shader() {
    this->sVsh = std::move(vertexShader);
    this->sFsh = std::move(fragmentShader);
    this->bSource = source;

    this->iProgram = 0;
    this->iVertexShader = 0;
    this->iFragmentShader = 0;

    this->iProgramBackup = 0;
}

void OpenGLShader::init() { this->bReady = this->compile(this->sVsh, this->sFsh, this->bSource); }

void OpenGLShader::initAsync() { this->bAsyncReady = true; }

void OpenGLShader::destroy() {
    if(this->iProgram != 0) glDeleteObjectARB(this->iProgram);
    if(this->iFragmentShader != 0) glDeleteObjectARB(this->iFragmentShader);
    if(this->iVertexShader != 0) glDeleteObjectARB(this->iVertexShader);

    this->iProgram = 0;
    this->iFragmentShader = 0;
    this->iVertexShader = 0;

    this->iProgramBackup = 0;

    this->uniformLocationCache.clear();
}

void OpenGLShader::enable() {
    if(!this->bReady) return;

    glGetIntegerv(GL_CURRENT_PROGRAM, &this->iProgramBackup);  // backup
    glUseProgramObjectARB(this->iProgram);
}

void OpenGLShader::disable() {
    if(!this->bReady) return;

    glUseProgramObjectARB(this->iProgramBackup);  // restore
}

void OpenGLShader::setUniform1f(UString name, float value) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform1fARB(id, value);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform1fv(UString name, int count, float *values) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform1fvARB(id, count, values);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform1i(UString name, int value) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform1iARB(id, value);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform2f(UString name, float value1, float value2) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform2fARB(id, value1, value2);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform2fv(UString name, int count, float *vectors) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform2fv(id, count, (float *)&vectors[0]);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform3f(UString name, float x, float y, float z) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform3fARB(id, x, y, z);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform3fv(UString name, int count, float *vectors) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform3fv(id, count, (float *)&vectors[0]);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniform4f(UString name, float x, float y, float z, float w) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniform4fARB(id, x, y, z, w);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniformMatrix4fv(UString name, Matrix4 &matrix) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniformMatrix4fv(id, 1, GL_FALSE, matrix.get());
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

void OpenGLShader::setUniformMatrix4fv(UString name, float *v) {
    if(!this->bReady) return;

    const int id = this->getAndCacheUniformLocation(name);
    if(id != -1)
        glUniformMatrix4fv(id, 1, GL_FALSE, v);
    else if(cv::debug_shaders.getBool())
        debugLog("OpenGLShader Warning: Can't find uniform %s\n", name.toUtf8());
}

int OpenGLShader::getAttribLocation(const UString& name) {
    if(!this->bReady) return -1;

    return glGetAttribLocation(this->iProgram, name.toUtf8());
}

int OpenGLShader::getAndCacheUniformLocation(const UString &name) {
    if(!this->bReady) return -1;

    this->sTempStringBuffer.reserve(name.lengthUtf8());
    this->sTempStringBuffer.assign(name.toUtf8(), name.lengthUtf8());

    const auto cachedValue = this->uniformLocationCache.find(this->sTempStringBuffer);
    const bool cached = (cachedValue != this->uniformLocationCache.end());

    const int id = (cached ? cachedValue->second : glGetUniformLocationARB(this->iProgram, name.toUtf8()));
    if(!cached && id != -1) this->uniformLocationCache[this->sTempStringBuffer] = id;

    return id;
}

bool OpenGLShader::compile(const std::string& vertexShader, const std::string& fragmentShader, bool source) {
    // load & compile shaders
    debugLog("OpenGLShader: Compiling %s ...\n", (source ? "vertex source" : vertexShader.c_str()));
    this->iVertexShader = source ? this->createShaderFromString(vertexShader, GL_VERTEX_SHADER_ARB)
                             : this->createShaderFromFile(vertexShader, GL_VERTEX_SHADER_ARB);
    debugLog("OpenGLShader: Compiling %s ...\n", (source ? "fragment source" : fragmentShader.c_str()));
    this->iFragmentShader = source ? this->createShaderFromString(fragmentShader, GL_FRAGMENT_SHADER_ARB)
                               : this->createShaderFromFile(fragmentShader, GL_FRAGMENT_SHADER_ARB);

    if(this->iVertexShader == 0 || this->iFragmentShader == 0) {
        engine->showMessageError("OpenGLShader Error", "Couldn't createShader()");
        return false;
    }

    // create program
    this->iProgram = glCreateProgramObjectARB();
    if(this->iProgram == 0) {
        engine->showMessageError("OpenGLShader Error", "Couldn't glCreateProgramObjectARB()");
        return false;
    }

    // attach
    glAttachObjectARB(this->iProgram, this->iVertexShader);
    glAttachObjectARB(this->iProgram, this->iFragmentShader);

    // link
    glLinkProgramARB(this->iProgram);

    int returnValue = GL_TRUE;
    glGetObjectParameterivARB(this->iProgram, GL_OBJECT_LINK_STATUS_ARB, &returnValue);
    if(returnValue == GL_FALSE) {
        engine->showMessageError("OpenGLShader Error", "Couldn't glLinkProgramARB()");
        return false;
    }

    // validate
    glValidateProgramARB(this->iProgram);
    returnValue = GL_TRUE;
    glGetObjectParameterivARB(this->iProgram, GL_OBJECT_VALIDATE_STATUS_ARB, &returnValue);
    if(returnValue == GL_FALSE) {
        engine->showMessageError("OpenGLShader Error", "Couldn't glValidateProgramARB()");
        return false;
    }

    return true;
}

int OpenGLShader::createShaderFromString(const std::string& shaderSource, int shaderType) {
    const int shader = glCreateShaderObjectARB(shaderType);

    if(shader == 0) {
        engine->showMessageError("OpenGLShader Error", "Couldn't glCreateShaderObjectARB()");
        return 0;
    }

    // compile shader
    const char *shaderSourceChar = shaderSource.c_str();
    glShaderSourceARB(shader, 1, &shaderSourceChar, NULL);
    glCompileShaderARB(shader);

    int returnValue = GL_TRUE;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &returnValue);

    if(returnValue == GL_FALSE) {
        debugLog("------------------OpenGLShader Compile Error------------------\n");

        glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &returnValue);
        char *errorLog = new char[returnValue];
        {
            glGetInfoLogARB(shader, returnValue, &returnValue, errorLog);
            debugLog(errorLog);
        }
        delete[] errorLog;

        debugLog("--------------------------------------------------------------\n");

        engine->showMessageError("OpenGLShader Error", "Couldn't glShaderSourceARB() or glCompileShaderARB()");
        return 0;
    }

    return shader;
}

int OpenGLShader::createShaderFromFile(const std::string& fileName, int shaderType) {
    // load file
    std::ifstream inFile(fileName.c_str());
    if(!inFile) {
        engine->showMessageError("OpenGLShader Error", fileName.c_str());
        return 0;
    }
    std::string line;
    std::string shaderSource;
    while(inFile.good()) {
        std::getline(inFile, line);
        shaderSource += line + "\n\0";
    }
    shaderSource += "\n\0";
    inFile.close();

    std::string shaderSourcePtr = shaderSource;

    return this->createShaderFromString(shaderSourcePtr, shaderType);
}

#endif
