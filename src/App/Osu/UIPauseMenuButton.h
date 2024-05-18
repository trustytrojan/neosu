#pragma once
#include "CBaseUIButton.h"

class Image;

class UIPauseMenuButton : public CBaseUIButton {
   public:
    UIPauseMenuButton(std::function<Image *()> getImageFunc, float xPos, float yPos, float xSize, float ySize,
                      UString name);

    virtual void draw(Graphics *g);

    virtual void onMouseInside();
    virtual void onMouseOutside();

    void setBaseScale(float xScale, float yScale);
    void setAlpha(float alpha) { m_fAlpha = alpha; }

    Image *getImage() { return getImageFunc != NULL ? getImageFunc() : NULL; }

   private:
    Vector2 m_vScale;
    Vector2 m_vBaseScale;
    float m_fScaleMultiplier;

    float m_fAlpha;

    std::function<Image *()> getImageFunc;
};
