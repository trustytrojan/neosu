#pragma once
#include "cbase.h"

class Shader;
class VertexArrayObject;

class SliderRenderer {
   public:
    static Shader *BLEND_SHADER;

    static float UNIT_CIRCLE_VAO_DIAMETER;

   public:
    static VertexArrayObject *generateVAO(const std::vector<Vector2> &points, float hitcircleDiameter,
                                          Vector3 translation = Vector3(0, 0, 0), bool skipOOBPoints = true);

    static void draw(Graphics *g, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints,
                     float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff,
                     float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0);
    static void draw(Graphics *g, VertexArrayObject *vao, const std::vector<Vector2> &alwaysPoints, Vector2 translation,
                     float scale, float hitcircleDiameter, float from = 0.0f, float to = 1.0f,
                     Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f, float alpha = 1.0f,
                     long sliderTimeForRainbow = 0, bool doEnableRenderTarget = true, bool doDisableRenderTarget = true,
                     bool doDrawSliderFrameBufferToScreen = true);
    static void drawMM(Graphics *g, const std::vector<Vector2> &points, float hitcircleDiameter, float from = 0.0f,
                       float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f,
                       float alpha = 1.0f, long sliderTimeForRainbow = 0);

   private:
    static void drawFillSliderBodyPeppy(Graphics *g, const std::vector<Vector2> &points, VertexArrayObject *circleMesh,
                                        float radius, int drawFromIndex, int drawUpToIndex, Shader *shader = NULL);
    static void drawFillSliderBodyMM(Graphics *g, const std::vector<Vector2> &points, float radius, int drawFromIndex,
                                     int drawUpToIndex);

    static void checkUpdateVars(float hitcircleDiameter);

    static void resetRenderTargetBoundingBox();

    // base mesh
    static float MESH_CENTER_HEIGHT;
    static int UNIT_CIRCLE_SUBDIVISIONS;
    static std::vector<float> UNIT_CIRCLE;
    static VertexArrayObject *UNIT_CIRCLE_VAO;
    static VertexArrayObject *UNIT_CIRCLE_VAO_BAKED;
    static VertexArrayObject *UNIT_CIRCLE_VAO_TRIANGLES;

    // tiny rendering optimization for RenderTarget
    static float fBoundingBoxMinX;
    static float fBoundingBoxMaxX;
    static float fBoundingBoxMinY;
    static float fBoundingBoxMaxY;
};
