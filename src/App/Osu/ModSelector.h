#pragma once
#include "OsuScreen.h"

class SkinImage;
class SongBrowser;

class CBaseUIElement;
class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;
class CBaseUISlider;
class CBaseUICheckbox;

class UIModSelectorModButton;
class ModSelectorOverrideSliderDescButton;
class UIButton;
class UICheckbox;

class ConVar;

struct ModSelection {
    u32 flags;
    bool fposu;
    std::vector<bool> override_locks;
    std::vector<float> override_values;
    std::vector<bool> experimental;
};

class ModSelector : public OsuScreen {
   public:
    ModSelector();
    virtual ~ModSelector();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyDown(KeyboardEvent &key);
    virtual void onKeyUp(KeyboardEvent &key);

    virtual void onResolutionChange(Vector2 newResolution);

    virtual CBaseUIContainer *setVisible(bool visible);

    void enableAuto();
    void toggleAuto();
    void resetModsUserInitiated();
    void resetMods();
    u32 getModFlags();
    void enableModsFromFlags(u32 flags);

    ModSelection getModSelection();
    void restoreMods(ModSelection selection);

    void setWaitForF1KeyUp(bool waitForF1KeyUp) { m_bWaitForF1KeyUp = waitForF1KeyUp; }

    bool isInCompactMode();
    bool isCSOverrideSliderActive();
    bool isMouseInScrollView();
    bool isMouseInside();

    void updateButtons(bool initial = false);
    void updateModConVar();

    CBaseUILabel *m_nonVanillaWarning;
    UIModSelectorModButton *m_modButtonHalftime;
    UIModSelectorModButton *m_modButtonDoubletime;
    UIModSelectorModButton *m_modButtonAuto;

   private:
    struct OVERRIDE_SLIDER {
        OVERRIDE_SLIDER() {
            lock = NULL;
            desc = NULL;
            slider = NULL;
            label = NULL;
            cvar = NULL;
            lockCvar = NULL;
        }

        CBaseUICheckbox *lock;
        ModSelectorOverrideSliderDescButton *desc;
        CBaseUISlider *slider;
        CBaseUILabel *label;
        ConVar *cvar;
        ConVar *lockCvar;
    };

    struct EXPERIMENTAL_MOD {
        CBaseUIElement *element;
        ConVar *cvar;
    };

    void updateScoreMultiplierLabelText();
    void updateExperimentalButtons(bool initial);
    void updateLayout();
    void updateExperimentalLayout();

    UIModSelectorModButton *setModButtonOnGrid(int x, int y, int state, bool initialState, UString modName,
                                               UString tooltipText, std::function<SkinImage *()> getImageFunc);
    UIModSelectorModButton *getModButtonOnGrid(int x, int y);

    OVERRIDE_SLIDER addOverrideSlider(UString text, UString labelText, ConVar *cvar, float min, float max,
                                      UString tooltipText = "", ConVar *lockCvar = NULL);
    void onOverrideSliderChange(CBaseUISlider *slider);
    void onOverrideSliderLockChange(CBaseUICheckbox *checkbox);
    void onOverrideARSliderDescClicked(CBaseUIButton *button);
    void onOverrideODSliderDescClicked(CBaseUIButton *button);
    void updateOverrideSliderLabels();
    UString getOverrideSliderLabelText(OVERRIDE_SLIDER s, bool active);

    CBaseUILabel *addExperimentalLabel(UString text);
    UICheckbox *addExperimentalCheckbox(UString text, UString tooltipText, ConVar *cvar = NULL);
    void onCheckboxChange(CBaseUICheckbox *checkbox);

    UIButton *addActionButton(UString text);

    void close();

    float m_fAnimation;
    float m_fExperimentalAnimation;
    bool m_bScheduledHide;
    bool m_bExperimentalVisible;
    CBaseUIContainer *m_overrideSliderContainer;
    CBaseUIScrollView *m_experimentalContainer;

    bool m_bWaitForF1KeyUp;

    bool m_bWaitForCSChangeFinished;
    bool m_bWaitForSpeedChangeFinished;
    bool m_bWaitForHPChangeFinished;

    // override sliders
    std::vector<OVERRIDE_SLIDER> m_overrideSliders;
    CBaseUISlider *m_CSSlider;
    CBaseUISlider *m_ARSlider;
    CBaseUISlider *m_ODSlider;
    CBaseUISlider *m_HPSlider;
    CBaseUISlider *m_speedSlider;
    CBaseUICheckbox *m_ARLock;
    CBaseUICheckbox *m_ODLock;
    bool m_bShowOverrideSliderALTHint;

    // mod grid buttons
    int m_iGridWidth;
    int m_iGridHeight;
    std::vector<UIModSelectorModButton *> m_modButtons;
    UIModSelectorModButton *m_modButtonEasy;
    UIModSelectorModButton *m_modButtonNofail;
    UIModSelectorModButton *m_modButtonHardrock;
    UIModSelectorModButton *m_modButtonSuddendeath;
    UIModSelectorModButton *m_modButtonHidden;
    UIModSelectorModButton *m_modButtonFlashlight;
    UIModSelectorModButton *m_modButtonRelax;
    UIModSelectorModButton *m_modButtonAutopilot;
    UIModSelectorModButton *m_modButtonSpunout;
    UIModSelectorModButton *m_modButtonScoreV2;
    UIModSelectorModButton *m_modButtonTD;

    // experimental mods
    std::vector<EXPERIMENTAL_MOD> m_experimentalMods;

    // score multiplier info label
    CBaseUILabel *m_scoreMultiplierLabel;

    // action buttons
    std::vector<UIButton *> m_actionButtons;
    UIButton *m_resetModsButton;
    UIButton *m_closeButton;

    // convar refs
    ConVar *m_osu_mod_touchdevice_ref;
};
