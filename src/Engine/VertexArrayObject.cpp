#include "VertexArrayObject.h"

#include "Engine.h"

using namespace std;

VertexArrayObject::VertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory)
    : Resource() {
    this->primitive = primitive;
    this->usage = usage;
    this->bKeepInSystemMemory = keepInSystemMemory;

    this->iNumVertices = 0;
    this->bHasTexcoords = false;

    this->iDrawRangeFromIndex = -1;
    this->iDrawRangeToIndex = -1;
    this->iDrawPercentNearestMultiple = 0;
    this->fDrawPercentFromPercent = 0.0f;
    this->fDrawPercentToPercent = 1.0f;
}

void VertexArrayObject::init() {
    // m_bReady may only be set in inheriting classes, if baking was successful
}

void VertexArrayObject::initAsync() { this->bAsyncReady = true; }

void VertexArrayObject::destroy() {
    this->clear();

    this->iNumVertices = 0;
    this->bHasTexcoords = false;
}

void VertexArrayObject::clear() {
    this->vertices = std::vector<Vector3>();
    for(size_t i = 0; i < this->texcoords.size(); i++) {
        this->texcoords[i] = std::vector<Vector2>();
    }
    this->texcoords = std::vector<std::vector<Vector2>>();
    this->normals = std::vector<Vector3>();
    this->colors = std::vector<Color>();

    this->partialUpdateVertexIndices = std::vector<int>();
    this->partialUpdateColorIndices = std::vector<int>();

    // NOTE: do NOT set m_iNumVertices to 0! (also don't change m_bHasTexcoords)
}

void VertexArrayObject::empty() {
    this->vertices.clear();
    for(size_t i = 0; i < this->texcoords.size(); i++) {
        this->texcoords[i].clear();
    }
    this->texcoords.clear();
    this->normals.clear();
    this->colors.clear();

    this->partialUpdateVertexIndices.clear();
    this->partialUpdateColorIndices.clear();

    // NOTE: do NOT set m_iNumVertices to 0! (also don't change m_bHasTexcoords)
}

void VertexArrayObject::addVertex(Vector2 v) {
    this->vertices.push_back(Vector3(v.x, v.y, 0));
    this->iNumVertices = this->vertices.size();
}

void VertexArrayObject::addVertex(float x, float y, float z) {
    this->vertices.push_back(Vector3(x, y, z));
    this->iNumVertices = this->vertices.size();
}

void VertexArrayObject::addVertex(Vector3 v) {
    this->vertices.push_back(v);
    this->iNumVertices = this->vertices.size();
}

void VertexArrayObject::addTexcoord(float u, float v, unsigned int textureUnit) {
    this->updateTexcoordArraySize(textureUnit);
    this->texcoords[textureUnit].push_back(Vector2(u, v));
    this->bHasTexcoords = true;
}

void VertexArrayObject::addTexcoord(Vector2 uv, unsigned int textureUnit) {
    this->updateTexcoordArraySize(textureUnit);
    this->texcoords[textureUnit].push_back(uv);
    this->bHasTexcoords = true;
}

void VertexArrayObject::addNormal(Vector3 normal) { this->normals.push_back(normal); }

void VertexArrayObject::addNormal(float x, float y, float z) { this->normals.push_back(Vector3(x, y, z)); }

void VertexArrayObject::addColor(Color color) { this->colors.push_back(color); }

void VertexArrayObject::setVertex(int index, Vector2 v) {
    if(index < 0 || index > (this->vertices.size() - 1)) return;

    this->vertices[index].x = v.x;
    this->vertices[index].y = v.y;

    this->partialUpdateVertexIndices.push_back(index);
}

void VertexArrayObject::setVertex(int index, Vector3 v) {
    if(index < 0 || index > (this->vertices.size() - 1)) return;

    this->vertices[index].x = v.x;
    this->vertices[index].y = v.y;
    this->vertices[index].z = v.z;

    this->partialUpdateVertexIndices.push_back(index);
}

void VertexArrayObject::setVertex(int index, float x, float y, float z) {
    if(index < 0 || index > (this->vertices.size() - 1)) return;

    this->vertices[index].x = x;
    this->vertices[index].y = y;
    this->vertices[index].z = z;

    this->partialUpdateVertexIndices.push_back(index);
}

void VertexArrayObject::setColor(int index, Color color) {
    if(index < 0 || index > (this->colors.size() - 1)) return;

    this->colors[index] = color;

    this->partialUpdateColorIndices.push_back(index);
}

void VertexArrayObject::setType(Graphics::PRIMITIVE primitive) { this->primitive = primitive; }

void VertexArrayObject::setDrawRange(int fromIndex, int toIndex) {
    this->iDrawRangeFromIndex = fromIndex;
    this->iDrawRangeToIndex = toIndex;
}

void VertexArrayObject::setDrawPercent(float fromPercent, float toPercent, int nearestMultiple) {
    this->fDrawPercentFromPercent = clamp<float>(fromPercent, 0.0f, 1.0f);
    this->fDrawPercentToPercent = clamp<float>(toPercent, 0.0f, 1.0f);
    this->iDrawPercentNearestMultiple = nearestMultiple;
}

void VertexArrayObject::updateTexcoordArraySize(unsigned int textureUnit) {
    while(this->texcoords.size() < (textureUnit + 1)) {
        std::vector<Vector2> emptyVector;
        this->texcoords.push_back(emptyVector);
    }
}

int VertexArrayObject::nearestMultipleUp(int number, int multiple) {
    if(multiple == 0) return number;

    int remainder = number % multiple;
    if(remainder == 0) return number;

    return number + multiple - remainder;
}

int VertexArrayObject::nearestMultipleDown(int number, int multiple) {
    if(multiple == 0) return number;

    int remainder = number % multiple;
    if(remainder == 0) return number;

    return number - remainder;
}
