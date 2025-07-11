#pragma once
#include <list>

#include "cbase.h"

class Camera;
class ConVar;
class Image;
class Shader;
class VertexArrayObject;

class ModFPoSu {
   public:
    static constexpr const float SIZEDIV3D = 1.0f / 512.0f;  // 1.0f / (float)GameRules::OSU_COORD_WIDTH

   private:
    static constexpr const int SUBDIVISIONS = 4;

   public:
    ModFPoSu();
    ~ModFPoSu();

    void draw();
    void update();

    void onKeyDown(KeyboardEvent &key);
    void onKeyUp(KeyboardEvent &key);

    [[nodiscard]] inline const Camera *getCamera() const { return this->camera; }

    [[nodiscard]] inline float getEdgeDistance() const { return this->fEdgeDistance; }
    [[nodiscard]] inline bool isCrosshairIntersectingScreen() const { return this->bCrosshairIntersectsScreen; }
    [[nodiscard]] float get3DPlayfieldScale() const;

    [[nodiscard]] inline Shader *getHitcircleShader() const { return this->hitcircleShader; }

   private:
    void handleZoomedChange();
    void noclipMove();

    void setMousePosCompensated(Vector2 newMousePos);
    Vector2 intersectRayMesh(Vector3 pos, Vector3 dir);
    Vector3 calculateUnProjectedVector(Vector2 pos);

    void makePlayfield();
    void makeBackgroundCube();

    void onCurvedChange(const UString &oldValue, const UString &newValue);
    void onDistanceChange(const UString &oldValue, const UString &newValue);
    void onNoclipChange(const UString &oldValue, const UString &newValue);

   private:
    struct VertexPair {
        Vector3 a;
        Vector3 b;
        float textureCoordinate;
        Vector3 normal;

        VertexPair(Vector3 a, Vector3 b, float tc) : a(a), b(b), textureCoordinate(tc) { ; }
    };

   private:
    static float subdivide(std::list<VertexPair> &meshList, const std::list<VertexPair>::iterator &begin,
                           const std::list<VertexPair>::iterator &end, int n, float edgeDistance);
    static Vector3 normalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3);

   private:
    VertexArrayObject *vao;
    VertexArrayObject *vaoCube;

    std::list<VertexPair> meshList;
    float fCircumLength;

    Matrix4 modelMatrix;
    Camera *camera;
    Vector3 vPrevNoclipCameraPos;
    bool bKeyLeftDown;
    bool bKeyUpDown;
    bool bKeyRightDown;
    bool bKeyDownDown;
    bool bKeySpaceDown;
    bool bKeySpaceUpDown;
    Vector3 vVelocity;
    bool bZoomKeyDown;
    bool bZoomed;
    float fZoomFOVAnimPercent;

    float fEdgeDistance;
    bool bCrosshairIntersectsScreen;

    Shader *hitcircleShader;
};
