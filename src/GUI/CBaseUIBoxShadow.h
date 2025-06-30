#pragma once
#include "CBaseUIElement.h"

class Shader;
class GaussianBlur;
class RenderTarget;
class GaussianBlurKernel;

class CBaseUIBoxShadow : public CBaseUIElement {
   public:
    CBaseUIBoxShadow(Color color = argb(0, 0, 0, 0), float radius = 0, float xPos = 0, float yPos = 0, float xSize = 0,
                     float ySize = 0, UString name = "");
    ~CBaseUIBoxShadow() override;

    void draw() override;
    void renderOffscreen();

    void forceRedraw() { this->bNeedsRedraw = true; }

    CBaseUIBoxShadow *setColoredContent(bool coloredContent);
    CBaseUIBoxShadow *setColor(Color color);
    CBaseUIBoxShadow *setShadowColor(Color color);

    [[nodiscard]] inline float getRadius() const { return this->fRadius; }

    void onResized() override;

   private:
    void render();

    bool bNeedsRedraw;
    bool bColoredContent;

    float fRadius;

    Color shadowColor;
    Color color;

    GaussianBlur *blur;
};

class GaussianBlur {
   public:
    GaussianBlur(int x, int y, int width, int height, int kernelSize, float radius);
    ~GaussianBlur();

    void draw(int x, int y);
    void setColor(Color color);

    void enable();
    void disable();

    void setSize(Vector2 size);

    [[nodiscard]] inline const Vector2 getPos() const { return this->vPos; }
    [[nodiscard]] inline const Vector2 getSize() const { return this->vSize; }

   private:
    Vector2 vPos;
    Vector2 vSize;
    int iKernelSize;
    float fRadius;

    RenderTarget *rt;
    RenderTarget *rt2;
    GaussianBlurKernel *kernel;
    Shader *blurShader;
};
