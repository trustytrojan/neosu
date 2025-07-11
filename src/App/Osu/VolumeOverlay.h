#pragma once
#include "OsuScreen.h"

class CBaseUIContainer;
class UIVolumeSlider;

class VolumeOverlay : public OsuScreen {
   public:
    VolumeOverlay();

    void animate();
    void draw() override;
    void mouse_update(bool *propagate_clicks) override;
    void onResolutionChange(Vector2 newResolution) override;
    void onKeyDown(KeyboardEvent &key) override;
    void updateLayout();
    bool isBusy() override;
    bool isVisible() override;
    bool canChangeVolume();
    void gainFocus();
    void loseFocus();

    void volumeUp(int multiplier = 1) { this->onVolumeChange(multiplier); }
    void volumeDown(int multiplier = 1) { this->onVolumeChange(-multiplier); }
    void onVolumeChange(int multiplier);
    void onMasterVolumeChange(const UString &oldValue, const UString &newValue);
    void onEffectVolumeChange();
    void onMusicVolumeChange(const UString &oldValue, const UString &newValue);

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
