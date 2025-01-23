#include "OpenGL3VertexArrayObject.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "Engine.h"
#include "OpenGL3Interface.h"
#include "OpenGLHeaders.h"

using namespace std;

OpenGL3VertexArrayObject::OpenGL3VertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                   bool keepInSystemMemory)
    : VertexArrayObject(primitive, usage, keepInSystemMemory) {
    this->iVAO = 0;
    this->iVertexBuffer = 0;
    this->iTexcoordBuffer = 0;

    this->iNumTexcoords = 0;
}

void OpenGL3VertexArrayObject::init() {
    if(this->vertices.size() < 2) return;

    OpenGL3Interface *g = (OpenGL3Interface *)engine->getGraphics();

    // backup vao
    int vaoBackup = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBackup);

    // create and bind a VAO to hold state for this model
    glGenVertexArrays(1, &this->iVAO);
    glBindVertexArray(this->iVAO);
    {
        // populate a vertex buffer
        glGenBuffers(1, &this->iVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, this->iVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * this->vertices.size(), &(this->vertices[0]),
                     usageToOpenGL(this->usage));

        // identify the components in the vertex buffer
        glEnableVertexAttribArray(g->getShaderGenericAttribPosition());
        glVertexAttribPointer(g->getShaderGenericAttribPosition(), 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (GLvoid *)0);

        // populate texcoord buffer
        if(this->texcoords.size() > 0 && this->texcoords[0].size() > 0) {
            this->iNumTexcoords = this->texcoords[0].size();

            glGenBuffers(1, &this->iTexcoordBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, this->iTexcoordBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * this->texcoords[0].size(), &(this->texcoords[0][0]),
                         usageToOpenGL(this->usage));

            // identify the components in the texcoord buffer
            glEnableVertexAttribArray(g->getShaderGenericAttribUV());
            glVertexAttribPointer(g->getShaderGenericAttribUV(), 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
        }
    }
    glBindVertexArray(vaoBackup);  // restore vao

    // free memory
    if(!this->bKeepInSystemMemory) this->clear();

    this->bReady = true;
}

void OpenGL3VertexArrayObject::initAsync() { this->bAsyncReady = true; }

void OpenGL3VertexArrayObject::destroy() {
    VertexArrayObject::destroy();

    if(this->iVAO > 0) {
        glDeleteBuffers(1, &this->iVertexBuffer);
        glDeleteBuffers(1, &this->iTexcoordBuffer);
        glDeleteVertexArrays(1, &this->iVAO);

        this->iVAO = 0;
        this->iVertexBuffer = 0;
        this->iTexcoordBuffer = 0;
    }
}

void OpenGL3VertexArrayObject::draw() {
    if(!this->bReady) {
        debugLog("WARNING: OpenGL3VertexArrayObject::draw() called, but was not ready!\n");
        return;
    }

    int start = clamp<int>(nearestMultipleUp((int)(this->iNumVertices * this->fDrawPercentFromPercent),
                                             this->iDrawPercentNearestMultiple),
                           0, this->iNumVertices);  // HACKHACK: osu sliders
    int end = clamp<int>(nearestMultipleDown((int)(this->iNumVertices * this->fDrawPercentToPercent),
                                             this->iDrawPercentNearestMultiple),
                         0, this->iNumVertices);  // HACKHACK: osu sliders

    if(start > end || std::abs(end - start) == 0) return;

    // backup vao
    int vaoBackup = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBackup);

    // bind and draw
    glBindVertexArray(this->iVAO);
    { glDrawArrays(primitiveToOpenGL(this->primitive), start, end - start); }
    glBindVertexArray(vaoBackup);  // restore vao
}

int OpenGL3VertexArrayObject::primitiveToOpenGL(Graphics::PRIMITIVE primitive) {
    switch(primitive) {
        case Graphics::PRIMITIVE::PRIMITIVE_LINES:
            return GL_LINES;
        case Graphics::PRIMITIVE::PRIMITIVE_LINE_STRIP:
            return GL_LINE_STRIP;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES:
            return GL_TRIANGLES;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case Graphics::PRIMITIVE::PRIMITIVE_QUADS:
            return GL_QUADS;
    }

    return GL_TRIANGLES;
}

unsigned int OpenGL3VertexArrayObject::usageToOpenGL(Graphics::USAGE_TYPE usage) {
    switch(usage) {
        case Graphics::USAGE_TYPE::USAGE_STATIC:
            return GL_STATIC_DRAW;
        case Graphics::USAGE_TYPE::USAGE_DYNAMIC:
            return GL_DYNAMIC_DRAW;
        case Graphics::USAGE_TYPE::USAGE_STREAM:
            return GL_STREAM_DRAW;
    }

    return GL_STATIC_DRAW;
}

#endif
