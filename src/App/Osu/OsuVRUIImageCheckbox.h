//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple VR checkbox with an image/icon on top
//
// $NoKeywords: $osuvricb
//===============================================================================//

#ifndef OSUVRUIIMAGECHECKBOX_H
#define OSUVRUIIMAGECHECKBOX_H

#include "OsuVRUIButton.h"

class OsuVRUIImageCheckbox : public OsuVRUIButton {
   public:
    OsuVRUIImageCheckbox(OsuVR *vr, float x, float y, float width, float height, std::string imageResourceNameChecked,
                         std::string imageResourceNameUnchecked);

    virtual void drawVR(Graphics *g, Matrix4 &mvp);
    virtual void update(Vector2 cursorPos);

    void setChecked(bool checked) { m_bChecked = checked; }

    inline bool isChecked() const { return m_bChecked; }

   private:
    virtual void onClicked();

    virtual void onCursorInside();
    virtual void onCursorOutside();

    void updateImageResource();

    bool m_bChecked;

    std::string m_sImageResourceNameChecked;
    std::string m_sImageResourceNameUnchecked;
    Image *m_imageChecked;
    Image *m_imageUnchecked;

    float m_fAnimation;
};

#endif
