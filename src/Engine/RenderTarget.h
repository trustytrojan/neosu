#pragma once
#include "Resource.h"

class ConVar;

class RenderTarget : public Resource {
   public:
    RenderTarget(int x, int y, int width, int height,
                 Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
    ~RenderTarget() override { ; }

    virtual void draw(Graphics *g, int x, int y);
    virtual void draw(Graphics *g, int x, int y, int width, int height);
    virtual void drawRect(Graphics *g, int x, int y, int width, int height);

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual void bind(unsigned int textureUnit = 0) = 0;
    virtual void unbind() = 0;

    void rebuild(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType);
    void rebuild(int x, int y, int width, int height);
    void rebuild(int width, int height);
    void rebuild(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType);

    // set
    void setPos(int x, int y) {
        this->vPos.x = x;
        this->vPos.y = y;
    }
    void setPos(Vector2 pos) { this->vPos = pos; }
    void setColor(Color color) { this->color = color; }
    void setClearColor(Color clearColor) { this->clearColor = clearColor; }
    void setClearColorOnDraw(bool clearColorOnDraw) { this->bClearColorOnDraw = clearColorOnDraw; }
    void setClearDepthOnDraw(bool clearDepthOnDraw) { this->bClearDepthOnDraw = clearDepthOnDraw; }

    // get
    float getWidth() const { return this->vSize.x; }
    float getHeight() const { return this->vSize.y; }
    inline Vector2 getSize() const { return this->vSize; }
    inline Vector2 getPos() const { return this->vPos; }
    inline Graphics::MULTISAMPLE_TYPE getMultiSampleType() const { return this->multiSampleType; }

    inline bool isMultiSampled() const { return this->multiSampleType != Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X; }

   protected:
    void init() override = 0;
    void initAsync() override = 0;
    void destroy() override = 0;

    bool bClearColorOnDraw;
    bool bClearDepthOnDraw;

    Vector2 vPos;
    Vector2 vSize;

    Graphics::MULTISAMPLE_TYPE multiSampleType;

    Color color;
    Color clearColor;
};
