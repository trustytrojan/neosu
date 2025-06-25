//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty renderer, for debugging and new OS implementations
//
// $NoKeywords: $ni
//===============================================================================//

#ifndef NULLGRAPHICSINTERFACE_H
#define NULLGRAPHICSINTERFACE_H

#include "Graphics.h"

class NullGraphicsInterface : public Graphics {
   public:
    NullGraphicsInterface() : Graphics() { ; }
    ~NullGraphicsInterface() override { ; }

    // scene
    void beginScene() override { ; }
    void endScene() override { ; }

    // depth buffer
    void clearDepthBuffer() override { ; }

    // color
    void setColor(Color color) override { (void)color; }
    void setAlpha(float alpha) override { (void)alpha; }

    // 2d primitive drawing
    void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)type;
        (void)pixels;
    }
    void drawPixel(int x, int y) override {
        (void)x;
        (void)y;
    }
    void drawLine(int x1, int y1, int x2, int y2) override {
        (void)x1;
        (void)y1;
        (void)x2;
        (void)y2;
    }
    void drawLine(Vector2 pos1, Vector2 pos2) override {
        (void)pos1;
        (void)pos2;
    }
    void drawRect(int x, int y, int width, int height) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)top;
        (void)right;
        (void)bottom;
        (void)left;
    }

    void fillRect(int x, int y, int width, int height) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    void fillRoundedRect(int x, int y, int width, int height, int radius) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)radius;
    }
    void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor,
                              Color bottomLeftColor, Color bottomRightColor) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)topLeftColor;
        (void)topRightColor;
        (void)bottomLeftColor;
        (void)bottomRightColor;
    }

    void drawQuad(int x, int y, int width, int height) override {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft,
                          Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) override {
        (void)topLeft;
        (void)topRight;
        (void)bottomRight;
        (void)bottomLeft;
        (void)topLeftColor;
        (void)topRightColor;
        (void)bottomRightColor;
        (void)bottomLeftColor;
    }

    // 2d resource drawing
    void drawImage(Image *image, AnchorPoint anchor = AnchorPoint::CENTER) override { (void)image; }
    void drawString(McFont *font, UString text) override;

    // 3d type drawing
    void drawVAO(VertexArrayObject *vao) override { (void)vao; }

    // DEPRECATED: 2d clipping
    void setClipRect(McRect clipRect) override { (void)clipRect; }
    void pushClipRect(McRect clipRect) override { (void)clipRect; }
    void popClipRect() override { ; }

    // stencil
    void pushStencil() override { ; }
    void fillStencil(bool inside) override { (void)inside; }
    void popStencil() override { ; }

    // renderer settings
    void setClipping(bool enabled) override { (void)enabled; }
    void setAlphaTesting(bool enabled) override { (void)enabled; }
    void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) override {
        (void)alphaFunc;
        (void)ref;
    }
    void setBlending(bool enabled) override { (void)enabled; }
    void setBlendMode(BLEND_MODE blendMode) override { (void)blendMode; }
    void setDepthBuffer(bool enabled) override { (void)enabled; }
    void setCulling(bool culling) override { (void)culling; }
    void setVSync(bool vsync) override { (void)vsync; }
    void setAntialiasing(bool aa) override { (void)aa; }
    void setWireframe(bool enabled) override { (void)enabled; }

    // renderer actions
    void flush() override { ; }
    std::vector<unsigned char> getScreenshot() override { return std::vector<unsigned char>(); }

    // renderer info
    [[nodiscard]] Vector2 getResolution() const override { return this->vResolution; }
    UString getVendor() override;
    UString getModel() override;
    UString getVersion() override;
    int getVRAMTotal() override { return -1; }
    int getVRAMRemaining() override { return -1; }

    // callbacks
    void onResolutionChange(Vector2 newResolution) override { this->vResolution = newResolution; }

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
    void init() override { ; }
    void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override {
        (void)projectionMatrix;
        (void)worldMatrix;
    }

   private:
    // renderer
    Vector2 vResolution;
};

#endif
