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
    virtual ~NullGraphicsInterface() { ; }

    // scene
    virtual void beginScene() { ; }
    virtual void endScene() { ; }

    // depth buffer
    virtual void clearDepthBuffer() { ; }

    // color
    virtual void setColor(Color color) { (void)color; }
    virtual void setAlpha(float alpha) { (void)alpha; }

    // 2d primitive drawing
    virtual void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)type;
        (void)pixels;
    }
    virtual void drawPixel(int x, int y) {
        (void)x;
        (void)y;
    }
    virtual void drawLine(int x1, int y1, int x2, int y2) {
        (void)x1;
        (void)y1;
        (void)x2;
        (void)y2;
    }
    virtual void drawLine(Vector2 pos1, Vector2 pos2) {
        (void)pos1;
        (void)pos2;
    }
    virtual void drawRect(int x, int y, int width, int height) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    virtual void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)top;
        (void)right;
        (void)bottom;
        (void)left;
    }

    virtual void fillRect(int x, int y, int width, int height) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    virtual void fillRoundedRect(int x, int y, int width, int height, int radius) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)radius;
    }
    virtual void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor,
                              Color bottomLeftColor, Color bottomRightColor) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        (void)topLeftColor;
        (void)topRightColor;
        (void)bottomLeftColor;
        (void)bottomRightColor;
    }

    virtual void drawQuad(int x, int y, int width, int height) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }
    virtual void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft,
                          Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) {
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
    virtual void drawImage(Image *image) { (void)image; }
    virtual void drawString(McFont *font, UString text);

    // 3d type drawing
    virtual void drawVAO(VertexArrayObject *vao) { (void)vao; }

    // DEPRECATED: 2d clipping
    virtual void setClipRect(McRect clipRect) { (void)clipRect; }
    virtual void pushClipRect(McRect clipRect) { (void)clipRect; }
    virtual void popClipRect() { ; }

    // stencil
    virtual void pushStencil() { ; }
    virtual void fillStencil(bool inside) { (void)inside; }
    virtual void popStencil() { ; }

    // renderer settings
    virtual void setClipping(bool enabled) { (void)enabled; }
    virtual void setAlphaTesting(bool enabled) { (void)enabled; }
    virtual void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) {
        (void)alphaFunc;
        (void)ref;
    }
    virtual void setBlending(bool enabled) { (void)enabled; }
    virtual void setBlendMode(BLEND_MODE blendMode) { (void)blendMode; }
    virtual void setDepthBuffer(bool enabled) { (void)enabled; }
    virtual void setCulling(bool culling) { (void)culling; }
    virtual void setVSync(bool vsync) { (void)vsync; }
    virtual void setAntialiasing(bool aa) { (void)aa; }
    virtual void setWireframe(bool enabled) { (void)enabled; }

    // renderer actions
    virtual void flush() { ; }
    virtual std::vector<unsigned char> getScreenshot() { return std::vector<unsigned char>(); }

    // renderer info
    virtual Vector2 getResolution() const { return m_vResolution; }
    virtual UString getVendor();
    virtual UString getModel();
    virtual UString getVersion();
    virtual int getVRAMTotal() { return -1; }
    virtual int getVRAMRemaining() { return -1; }

    // callbacks
    virtual void onResolutionChange(Vector2 newResolution) { m_vResolution = newResolution; }

    // factory
    virtual Image *createImage(std::string filePath, bool mipmapped);
    virtual Image *createImage(int width, int height, bool mipmapped);
    virtual RenderTarget *createRenderTarget(int x, int y, int width, int height,
                                             Graphics::MULTISAMPLE_TYPE multiSampleType);
    virtual Shader *createShaderFromFile(std::string vertexShaderFilePath, std::string fragmentShaderFilePath);
    virtual Shader *createShaderFromSource(std::string vertexShader, std::string fragmentShader);
    virtual VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                       bool keepInSystemMemory);

   protected:
    virtual void init() { ; }
    virtual void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) {
        (void)projectionMatrix;
        (void)worldMatrix;
    }

   private:
    // renderer
    Vector2 m_vResolution;
};

#endif
