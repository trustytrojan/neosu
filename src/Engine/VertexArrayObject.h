//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		modern opengl style mesh wrapper (vertices, texcoords, etc.)
//
// $NoKeywords: $vao
//===============================================================================//

#ifndef VERTEXARRAYOBJECT_H
#define VERTEXARRAYOBJECT_H

#include "Resource.h"
#include "Graphics.h"

class VertexArrayObject : public Resource {
   public:
    VertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES,
                      Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);
    ~VertexArrayObject() override { ; }

    // TODO: fix the naming schema. clear = empty = just empty the containers, but not necessarily release memory
    void clear();
    void empty();

    void addVertex(Vector2 v);
    void addVertex(Vector3 v);
    void addVertex(float x, float y, float z = 0);

    void addTexcoord(Vector2 uv, unsigned int textureUnit = 0);
    void addTexcoord(float u, float v, unsigned int textureUnit = 0);

    void addNormal(Vector3 normal);
    void addNormal(float x, float y, float z);

    void addColor(Color color);

    void setVertex(int index, Vector2 v);
    void setVertex(int index, Vector3 v);
    void setVertex(int index, float x, float y, float z = 0);

    void setColor(int index, Color color);

    void setType(Graphics::PRIMITIVE primitive);
    void setDrawRange(int fromIndex, int toIndex);
    void setDrawPercent(float fromPercent = 0.0f, float toPercent = 1.0f, int nearestMultiple = 0);  // DEPRECATED

    [[nodiscard]] inline Graphics::PRIMITIVE getPrimitive() const { return this->primitive; }
    [[nodiscard]] inline Graphics::USAGE_TYPE getUsage() const { return this->usage; }

    [[nodiscard]] const std::vector<Vector3> &getVertices() const { return this->vertices; }
    [[nodiscard]] const std::vector<std::vector<Vector2>> &getTexcoords() const { return this->texcoords; }
    [[nodiscard]] const std::vector<Vector3> &getNormals() const { return this->normals; }
    [[nodiscard]] const std::vector<Color> &getColors() const { return this->colors; }

    [[nodiscard]] inline unsigned int getNumVertices() const { return this->iNumVertices; }
    [[nodiscard]] inline bool hasTexcoords() const { return this->bHasTexcoords; }

    virtual void draw() { assert(false); }  // implementation dependent (gl/dx11/etc.)

    // type inspection
    [[nodiscard]] Type getResType() const final { return VAO; }

    VertexArrayObject *asVAO() final { return this; }
    [[nodiscard]] const VertexArrayObject *asVAO() const final { return this; }

   protected:
    static int nearestMultipleUp(int number, int multiple);
    static int nearestMultipleDown(int number, int multiple);

    void init() override;
    void initAsync() override;
    void destroy() override;

    void updateTexcoordArraySize(unsigned int textureUnit);

    Graphics::PRIMITIVE primitive;
    Graphics::USAGE_TYPE usage;
    bool bKeepInSystemMemory;

    std::vector<Vector3> vertices;
    std::vector<std::vector<Vector2>> texcoords;
    std::vector<Vector3> normals;
    std::vector<Color> colors;

    unsigned int iNumVertices;
    bool bHasTexcoords;

    std::vector<int> partialUpdateVertexIndices;
    std::vector<int> partialUpdateColorIndices;

    // custom
    int iDrawRangeFromIndex;
    int iDrawRangeToIndex;
    int iDrawPercentNearestMultiple;
    float fDrawPercentFromPercent;
    float fDrawPercentToPercent;
};

#endif
