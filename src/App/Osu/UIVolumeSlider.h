#pragma once
#include "CBaseUISlider.h"

class McFont;

class UIVolumeSlider : public CBaseUISlider {
   public:
    enum class TYPE { MASTER, MUSIC, EFFECTS };

   public:
    UIVolumeSlider(float xPos, float yPos, float xSize, float ySize, UString name);

    void setType(TYPE type) { m_type = type; }
    void setSelected(bool selected);

    bool checkWentMouseInside();

    float getMinimumExtraTextWidth();

    inline bool isSelected() const { return m_bSelected; }

   private:
    virtual void drawBlock(Graphics *g);

    virtual void onMouseInside();

    TYPE m_type;
    bool m_bSelected;

    bool m_bWentMouseInside;
    float m_fSelectionAnim;

    McFont *m_font;
};
