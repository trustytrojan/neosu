#pragma once
#include "OsuScreen.h"

class CBaseUIContainer;
class Osu;
class UIVolumeSlider;

class VolumeOverlay : public OsuScreen {
   public:
    VolumeOverlay(Osu *osu);

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

    void volumeUp(int multiplier = 1) { onVolumeChange(multiplier); }
    void volumeDown(int multiplier = 1) { onVolumeChange(-multiplier); }
    void onVolumeChange(int multiplier);
    void onMasterVolumeChange(UString oldValue, UString newValue);
    void onEffectVolumeChange();
    void onMusicVolumeChange(UString oldValue, UString newValue);

    float m_fLastVolume;
    float m_fVolumeChangeTime;
    float m_fVolumeChangeFade;
    bool m_bVolumeInactiveToActiveScheduled = false;
    float m_fVolumeInactiveToActiveAnim = 0.f;

    ConVar *osu_volume_master = nullptr;
    ConVar *osu_volume_effects = nullptr;
    ConVar *osu_volume_music = nullptr;
    ConVar *osu_hud_volume_size_multiplier = nullptr;
    ConVar *osu_volume_master_inactive = nullptr;
    ConVar *osu_volume_change_interval = nullptr;

    CBaseUIContainer *m_volumeSliderOverlayContainer = nullptr;
    UIVolumeSlider *m_volumeMaster = nullptr;
    UIVolumeSlider *m_volumeEffects = nullptr;
    UIVolumeSlider *m_volumeMusic = nullptr;
};
