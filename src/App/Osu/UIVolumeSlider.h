#pragma once
#include "CBaseUISlider.h"

class McFont;

class UIVolumeSlider : public CBaseUISlider {
   public:
    enum class TYPE { MASTER, MUSIC, EFFECTS };

   public:
    UIVolumeSlider(float xPos, float yPos, float xSize, float ySize, UString name);

    void setType(TYPE type) { this->type = type; }
    void setSelected(bool selected);

    bool checkWentMouseInside();

    float getMinimumExtraTextWidth();

    inline bool isSelected() const { return this->bSelected; }

   private:
    virtual void drawBlock(Graphics *g);

    virtual void onMouseInside();

    TYPE type;
    bool bSelected;

    bool bWentMouseInside;
    float fSelectionAnim;

    McFont *font;
};
