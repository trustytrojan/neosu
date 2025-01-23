#pragma once
#include "CBaseUIButton.h"

class CBaseUICheckbox : public CBaseUIButton {
   public:
    CBaseUICheckbox(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "",
                    UString text = "");
    virtual ~CBaseUICheckbox() { ; }

    virtual void draw(Graphics *g);

    inline float getBlockSize() { return this->vSize.y / 2; }
    inline float getBlockBorder() { return this->vSize.y / 4; }
    inline bool isChecked() const { return this->bChecked; }

    CBaseUICheckbox *setChecked(bool checked, bool fireChangeEvent = true);
    CBaseUICheckbox *setSizeToContent(int horizontalBorderSize = 1, int verticalBorderSize = 1);
    CBaseUICheckbox *setWidthToContent(int horizontalBorderSize = 1);

    typedef fastdelegate::FastDelegate1<CBaseUICheckbox *> CheckboxChangeCallback;
    CBaseUICheckbox *setChangeCallback(CheckboxChangeCallback clickCallback) {
        this->changeCallback = clickCallback;
        return this;
    }

   protected:
    virtual void onPressed();

    bool bChecked;
    CheckboxChangeCallback changeCallback;
};
