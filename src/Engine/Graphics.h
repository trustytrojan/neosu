#pragma once

#include "types.h"
// ^ import order matters

#include <stack>
#include <vector>

#include "Color.h"
#include "Matrices.h"
#include "Rect.h"
#include "Vectors.h"

class ConVar;
class UString;

class Image;
class McFont;
class Shader;
class RenderTarget;
class VertexArrayObject;

class Graphics {
   public:
    enum class PRIMITIVE : uint8_t {
        PRIMITIVE_LINES,
        PRIMITIVE_LINE_STRIP,
        PRIMITIVE_TRIANGLES,
        PRIMITIVE_TRIANGLE_FAN,
        PRIMITIVE_TRIANGLE_STRIP,
        PRIMITIVE_QUADS
    };

    enum class USAGE_TYPE : uint8_t { USAGE_STATIC, USAGE_DYNAMIC, USAGE_STREAM };

    enum class DRAWPIXELS_TYPE : uint8_t { DRAWPIXELS_UBYTE, DRAWPIXELS_FLOAT };

    enum class MULTISAMPLE_TYPE : uint8_t { MULTISAMPLE_0X, MULTISAMPLE_2X, MULTISAMPLE_4X, MULTISAMPLE_8X, MULTISAMPLE_16X };

    enum class WRAP_MODE : uint8_t { WRAP_MODE_CLAMP, WRAP_MODE_REPEAT };

    enum class FILTER_MODE : uint8_t { FILTER_MODE_NONE, FILTER_MODE_LINEAR, FILTER_MODE_MIPMAP };

    enum class BLEND_MODE : uint8_t {
        BLEND_MODE_ALPHA,         // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) (default)
        BLEND_MODE_ADDITIVE,      // glBlendFunc(GL_SRC_ALPHA, GL_ONE)
        BLEND_MODE_PREMUL_ALPHA,  // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                  // GL_ONE_MINUS_SRC_ALPHA)
        BLEND_MODE_PREMUL_COLOR   // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
    };

    enum class COMPARE_FUNC : uint8_t {
        COMPARE_FUNC_NEVER,
        COMPARE_FUNC_LESS,
        COMPARE_FUNC_EQUAL,
        COMPARE_FUNC_LESSEQUAL,
        COMPARE_FUNC_GREATER,
        COMPARE_FUNC_NOTEQUAL,
        COMPARE_FUNC_GREATEREQUAL,
        COMPARE_FUNC_ALWAYS
    };

   public:
    friend class Engine;

    Graphics();
    virtual ~Graphics() {}

    // scene
    virtual void beginScene() = 0;
    virtual void endScene() = 0;

    // depth buffer
    virtual void clearDepthBuffer() = 0;

    // color
    virtual void setColor(Color color) = 0;
    virtual void setAlpha(float alpha) = 0;

    // 2d primitive drawing
    virtual void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type,
                            const void *pixels) = 0;
    virtual void drawPixel(int x, int y) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
    virtual void drawLine(Vector2 pos1, Vector2 pos2) = 0;
    virtual void drawRect(int x, int y, int width, int height) = 0;
    virtual void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) = 0;

    virtual void fillRect(int x, int y, int width, int height) = 0;
    virtual void fillRoundedRect(int x, int y, int width, int height, int radius) = 0;
    virtual void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor,
                              Color bottomLeftColor, Color bottomRightColor) = 0;

    virtual void drawQuad(int x, int y, int width, int height) = 0;
    virtual void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft,
                          Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) = 0;

    // 2d resource drawing
    virtual void drawImage(Image *image, AnchorPoint anchor = AnchorPoint::CENTER) = 0;
    virtual void drawString(McFont *font, UString text) = 0;

    // 3d type drawing
    virtual void drawVAO(VertexArrayObject *vao) = 0;

    // DEPRECATED: 2d clipping
    virtual void setClipRect(McRect clipRect) = 0;
    virtual void pushClipRect(McRect clipRect) = 0;
    virtual void popClipRect() = 0;

    // stencil buffer
    virtual void pushStencil() = 0;
    virtual void fillStencil(bool inside) = 0;
    virtual void popStencil() = 0;

    // renderer settings
    virtual void setClipping(bool enabled) = 0;
    virtual void setAlphaTesting(bool enabled) = 0;
    virtual void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) = 0;
    virtual void setBlending(bool enabled) = 0;
    virtual void setBlendMode(BLEND_MODE blendMode) = 0;
    virtual void setDepthBuffer(bool enabled) = 0;
    virtual void setCulling(bool enabled) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setAntialiasing(bool enabled) = 0;
    virtual void setWireframe(bool enabled) = 0;

    // renderer actions
    virtual void flush() = 0;
    virtual std::vector<unsigned char> getScreenshot() = 0;

    // renderer info
    [[nodiscard]] virtual Vector2 getResolution() const = 0;
    virtual UString getVendor() = 0;
    virtual UString getModel() = 0;
    virtual UString getVersion() = 0;
    virtual int getVRAMTotal() = 0;
    virtual int getVRAMRemaining() = 0;

    // callbacks
    virtual void onResolutionChange(Vector2 newResolution) = 0;

    // factory
	virtual Image *createImage(std::string filePath, bool mipmapped, bool keepInSystemMemory) = 0;
	virtual Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) = 0;
    virtual RenderTarget *createRenderTarget(int x, int y, int width, int height,
                                             Graphics::MULTISAMPLE_TYPE multiSampleType) = 0;
    virtual Shader *createShaderFromFile(std::string vertexShaderFilePath, std::string fragmentShaderFilePath) = 0;
    virtual Shader *createShaderFromSource(std::string vertexShader, std::string fragmentShader) = 0;
    virtual VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                       bool keepInSystemMemory) = 0;

   public:
    // provided core functions (api independent)

    // matrices & transforms
    void pushTransform();
    void popTransform();

    // 2D
    // TODO: rename these to translate2D() etc.
    void translate(float x, float y, float z = 0);
    void translate(Vector2 translation) { this->translate(translation.x, translation.y); }
    void translate(Vector3 translation) { this->translate(translation.x, translation.y, translation.z); }
    void rotate(float deg, float x = 0, float y = 0, float z = 1);
    void rotate(float deg, Vector3 axis) { this->rotate(deg, axis.x, axis.y, axis.z); }
    void scale(float x, float y, float z = 1);
    void scale(Vector2 scaling) { this->scale(scaling.x, scaling.y, 1); }
    void scale(Vector3 scaling) { this->scale(scaling.x, scaling.y, scaling.z); }

    // 3D
    void translate3D(float x, float y, float z);
    void translate3D(Vector3 translation) { this->translate3D(translation.x, translation.y, translation.z); }
    void rotate3D(float deg, float x, float y, float z);
    void rotate3D(float deg, Vector3 axis) { this->rotate3D(deg, axis.x, axis.y, axis.z); }
    void setWorldMatrix(Matrix4 &worldMatrix);
    void setWorldMatrixMul(Matrix4 &worldMatrix);
    void setProjectionMatrix(Matrix4 &projectionMatrix);

    Matrix4 getWorldMatrix();
    Matrix4 getProjectionMatrix();

    // 3d gui scenes
    void push3DScene(McRect region);
    void pop3DScene();
    void translate3DScene(float x, float y, float z = 0);
    void rotate3DScene(float rotx, float roty, float rotz);
    void offset3DScene(float x, float y, float z = 0);

   protected:
    virtual void init() = 0;  // must be called after the OS implementation constructor
    virtual void onTransformUpdate(
        Matrix4 &projectionMatrix,
        Matrix4 &worldMatrix) = 0;  // called if matrices have changed and need to be (re-)applied/uploaded

    void updateTransform(bool force = false);
    void checkStackLeaks();

    // transforms
    bool bTransformUpToDate;
    std::stack<Matrix4> worldTransformStack;
    std::stack<Matrix4> projectionTransformStack;

    // 3d gui scenes
    bool bIs3dScene;
    std::stack<bool> scene_stack;
    McRect scene_region;
    Vector3 v3dSceneOffset;
    Matrix4 scene_world_matrix;
    Matrix4 scene_projection_matrix;
};
