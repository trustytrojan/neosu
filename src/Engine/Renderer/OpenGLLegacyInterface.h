//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		raw legacy opengl graphics interface
//
// $NoKeywords: $lgli
//===============================================================================//

#ifndef LEGACYOPENGLINTERFACE_H
#define LEGACYOPENGLINTERFACE_H

#include "cbase.h"

#ifdef MCENGINE_FEATURE_OPENGL

class Image;

class OpenGLLegacyInterface : public Graphics {
   public:
    OpenGLLegacyInterface();
    ~OpenGLLegacyInterface() override;

    // scene
    void beginScene() override;
    void endScene() override;

    // depth buffer
    void clearDepthBuffer() override;

    // color
    void setColor(Color color) override;
    void setAlpha(float alpha) override;

    // 2d primitive drawing
    void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) override;
    void drawPixel(int x, int y) override;
    void drawLine(int x1, int y1, int x2, int y2) override;
    void drawLine(Vector2 pos1, Vector2 pos2) override;
    void drawRect(int x, int y, int width, int height) override;
    void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) override;

    void fillRect(int x, int y, int width, int height) override;
    void fillRoundedRect(int x, int y, int width, int height, int radius) override;
    void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor,
                              Color bottomLeftColor, Color bottomRightColor) override;

    void drawQuad(int x, int y, int width, int height) override;
    void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft,
                          Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) override;

    // 2d resource drawing
    void drawImage(Image *image, AnchorPoint anchor = AnchorPoint::CENTER) override;
    void drawString(McFont *font, UString text) override;

    // 3d type drawing
    void drawVAO(VertexArrayObject *vao) override;

    // DEPRECATED: 2d clipping
    void setClipRect(McRect clipRect) override;
    void pushClipRect(McRect clipRect) override;
    void popClipRect() override;

    // stencil
    void pushStencil() override;
    void fillStencil(bool inside) override;
    void popStencil() override;

    // renderer settings
    void setClipping(bool enabled) override;
    void setAlphaTesting(bool enabled) override;
    void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) override;
    void setBlending(bool enabled) override;
    void setBlendMode(BLEND_MODE blendMode) override;
    void setDepthBuffer(bool enabled) override;
    void setCulling(bool culling) override;
    void setAntialiasing(bool aa) override;
    void setWireframe(bool enabled) override;

    // renderer actions
    void flush() override;
    std::vector<unsigned char> getScreenshot() override;

    // renderer info
    [[nodiscard]] Vector2 getResolution() const override { return this->vResolution; }
    UString getVendor() override;
    UString getModel() override;
    UString getVersion() override;
    int getVRAMTotal() override;
    int getVRAMRemaining() override;

    // callbacks
    void onResolutionChange(Vector2 newResolution) override;

    // factory
    Image *createImage(std::string filePath, bool mipmapped) override;
    Image *createImage(int width, int height, bool mipmapped) override;
    RenderTarget *createRenderTarget(int x, int y, int width, int height,
                                             Graphics::MULTISAMPLE_TYPE multiSampleType) override;
    Shader *createShaderFromFile(std::string vertexShaderFilePath, std::string fragmentShaderFilePath) override;
    Shader *createShaderFromSource(std::string vertexShader, std::string fragmentShader) override;
    VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                       bool keepInSystemMemory) override;

   protected:
    void init() override;
    void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override;

   private:
    static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
    static int compareFuncToOpenGL(Graphics::COMPARE_FUNC compareFunc);

    void handleGLErrors();

    // renderer
    bool bInScene;
    Vector2 vResolution;

    // persistent vars
    bool bAntiAliasing;
    Color color;
    float fZ;
    float fClearZ;

    // clipping
    std::stack<McRect> clipRectStack;
};

#endif

#endif
