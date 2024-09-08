#pragma once
#include "CBaseUIImageButton.h"

class SkinImage;
class ModSelector;

class UIModSelectorModButton : public CBaseUIButton {
   public:
    UIModSelectorModButton(ModSelector *osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onClicked();

    void resetState();

    void setState(int state);
    void setState(unsigned int state, bool initialState, ConVar *cvar, UString modName, UString tooltipText,
                  std::function<SkinImage *()> getImageFunc);
    void setBaseScale(float xScale, float yScale);
    void setAvailable(bool available) { m_bAvailable = available; }

    UString getActiveModName();
    inline int getState() const { return m_iState; }
    inline bool isOn() const { return m_bOn; }
    void setOn(bool on, bool silent = false);

   private:
    virtual void onFocusStolen();

    ModSelector *m_osuModSelector;

    bool m_bOn;
    bool m_bAvailable;
    int m_iState;
    float m_fEnabledScaleMultiplier;
    float m_fEnabledRotationDeg;
    Vector2 m_vBaseScale;

    struct STATE {
        ConVar *cvar;
        UString modName;
        std::vector<UString> tooltipTextLines;
        std::function<SkinImage *()> getImageFunc;
    };
    std::vector<STATE> m_states;

    Vector2 m_vScale;
    float m_fRot;
    std::function<SkinImage *()> getActiveImageFunc;

    bool m_bFocusStolenDelay;
};
