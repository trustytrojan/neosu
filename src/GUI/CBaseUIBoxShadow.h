#pragma once
#include "CBaseUIElement.h"

class Shader;
class GaussianBlur;
class RenderTarget;
class GaussianBlurKernel;

class CBaseUIBoxShadow : public CBaseUIElement {
   public:
    CBaseUIBoxShadow(Color color = COLOR(0, 0, 0, 0), float radius = 0, float xPos = 0, float yPos = 0, float xSize = 0,
                     float ySize = 0, UString name = "");
    virtual ~CBaseUIBoxShadow();

    virtual void draw(Graphics *g);
    void renderOffscreen(Graphics *g);

    void forceRedraw() { this->bNeedsRedraw = true; }

    CBaseUIBoxShadow *setColoredContent(bool coloredContent);
    CBaseUIBoxShadow *setColor(Color color);
    CBaseUIBoxShadow *setShadowColor(Color color);

    inline float getRadius() const { return this->fRadius; }

    virtual void onResized();

   private:
    void render(Graphics *g);

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

    void draw(Graphics *g, int x, int y);
    void setColor(Color color);

    void enable();
    void disable(Graphics *g);

    void setSize(Vector2 size);

    inline const Vector2 getPos() const { return this->vPos; }
    inline const Vector2 getSize() const { return this->vSize; }

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
