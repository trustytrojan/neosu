#pragma once
#include "CBaseUIImageButton.h"

class SkinImage;
class ModSelector;

class UIModSelectorModButton : public CBaseUIButton {
   public:
    UIModSelectorModButton(ModSelector *osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name);

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;
    void onClicked() override;

    void resetState();

    void setState(int state);
    void setState(unsigned int state, bool initialState, ConVar *cvar, UString modName, UString tooltipText,
                  std::function<SkinImage *()> getImageFunc);
    void setBaseScale(float xScale, float yScale);
    void setAvailable(bool available) { this->bAvailable = available; }

    UString getActiveModName();
    [[nodiscard]] inline int getState() const { return this->iState; }
    [[nodiscard]] inline bool isOn() const { return this->bOn; }
    void setOn(bool on, bool silent = false);

   private:
    void onFocusStolen() override;

    ModSelector *osuModSelector;

    bool bOn;
    bool bAvailable;
    int iState;
    float fEnabledScaleMultiplier;
    float fEnabledRotationDeg;
    Vector2 vBaseScale;

    struct STATE {
        ConVar *cvar;
        UString modName;
        std::vector<UString> tooltipTextLines;
        std::function<SkinImage *()> getImageFunc;
    };
    std::vector<STATE> states;

    Vector2 vScale;
    float fRot;
    std::function<SkinImage *()> getActiveImageFunc;

    bool bFocusStolenDelay;
};
