#pragma once
#include "OsuScreen.h"

class CBaseUIContainer;
class UIVolumeSlider;

class VolumeOverlay : public OsuScreen {
   public:
    VolumeOverlay();

    void animate();
    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onResolutionChange(Vector2 newResolution);
    virtual void onKeyDown(KeyboardEvent &key);
    void updateLayout();
    bool isBusy();
    bool isVisible();
    bool canChangeVolume();
    void gainFocus();
    void loseFocus();

    void volumeUp(int multiplier = 1) { this->onVolumeChange(multiplier); }
    void volumeDown(int multiplier = 1) { this->onVolumeChange(-multiplier); }
    void onVolumeChange(int multiplier);
    void onMasterVolumeChange(UString oldValue, UString newValue);
    void onEffectVolumeChange();
    void onMusicVolumeChange(UString oldValue, UString newValue);

    float fLastVolume;
    float fVolumeChangeTime;
    float fVolumeChangeFade;
    bool bVolumeInactiveToActiveScheduled = false;
    float fVolumeInactiveToActiveAnim = 0.f;

    CBaseUIContainer *volumeSliderOverlayContainer = NULL;
    UIVolumeSlider *volumeMaster = NULL;
    UIVolumeSlider *volumeEffects = NULL;
    UIVolumeSlider *volumeMusic = NULL;
};
