//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUOPTIONSMENU_H
#define OSUOPTIONSMENU_H

#include "NotificationOverlay.h"
#include "ScreenBackable.h"

class Osu;

class UIButton;
class UISlider;
class UIContextMenu;
class UISearchOverlay;

class OptionsMenuSliderPreviewElement;
class OptionsMenuCategoryButton;
class OptionsMenuKeyBindButton;
class OptionsMenuResetButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUICheckbox;
class CBaseUIButton;
class CBaseUISlider;
class CBaseUIScrollView;
class CBaseUILabel;
class CBaseUITextbox;

class ConVar;

class OptionsMenu : public ScreenBackable, public NotificationOverlayKeyListener {
   public:
    OptionsMenu(Osu *osu);
    ~OptionsMenu();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    virtual void onResolutionChange(Vector2 newResolution);

    virtual void onKey(KeyboardEvent &e);

    virtual CBaseUIContainer *setVisible(bool visible);

    void save();

    void openAndScrollToSkinSection();

    void setFullscreen(bool fullscreen) { m_bFullscreen = fullscreen; }

    void setUsername(UString username);

    inline bool isFullscreen() const { return m_bFullscreen; }
    bool isMouseInside();
    bool isBusy();

   public:
    static const char *OSU_CONFIG_FILE_NAME;

    enum class RenderCondition {
        NONE,
        ASIO_ENABLED,
        WASAPI_ENABLED,
        SCORE_SUBMISSION_POLICY,
    };

    struct OPTIONS_ELEMENT {
        OPTIONS_ELEMENT() {
            resetButton = NULL;
            cvar = NULL;
            type = -1;

            label1Width = 0.0f;
            relSizeDPI = 96.0f;

            allowOverscale = false;
            allowUnderscale = false;
            render_condition = RenderCondition::NONE;
        }

        OptionsMenuResetButton *resetButton;
        std::vector<CBaseUIElement *> elements;
        ConVar *cvar;
        int type;

        float label1Width;
        float relSizeDPI;

        RenderCondition render_condition;

        bool allowOverscale;
        bool allowUnderscale;

        UString searchTags;
    };

    virtual void updateLayout();
    virtual void onBack();

    void setVisibleInt(bool visible, bool fromOnBack = false);
    void scheduleSearchUpdate();

    void askForLoginDetails();
    void updateOsuFolder();
    void updateFposuDPI();
    void updateFposuCMper360();
    void updateSkinNameLabel();
    void updateNotelockSelectLabel();
    void updateHPDrainSelectLabel();

    // options
    void onFullscreenChange(CBaseUICheckbox *checkbox);
    void onBorderlessWindowedChange(CBaseUICheckbox *checkbox);
    void onDPIScalingChange(CBaseUICheckbox *checkbox);
    void onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox);
    void openSkinsFolder();
    void onSkinSelect();
    void onSkinSelect2(UString skinName, int id = -1);
    void onSkinReload();
    void onSkinRandom();
    void onResolutionSelect();
    void onResolutionSelect2(UString resolution, int id = -1);
    void onOutputDeviceSelect();
    void onOutputDeviceSelect2(UString outputDeviceName, int id = -1);
    void onOutputDeviceResetClicked();
    void onOutputDeviceResetUpdate();
    void onOutputDeviceRestart();
    void onLogInClicked();
    void onDownloadOsuClicked();
    void onManuallyManageBeatmapsClicked();
    void onCM360CalculatorLinkClicked();
    void onNotelockSelect();
    void onNotelockSelect2(UString notelockType, int id = -1);
    void onNotelockSelectResetClicked();
    void onNotelockSelectResetUpdate();
    void onHPDrainSelect();
    void onHPDrainSelect2(UString hpDrainType, int id = -1);
    void onHPDrainSelectResetClicked();
    void onHPDrainSelectResetUpdate();

    void onCheckboxChange(CBaseUICheckbox *checkbox);
    void onSliderChange(CBaseUISlider *slider);
    void onSliderChangeOneDecimalPlace(CBaseUISlider *slider);
    void onSliderChangeTwoDecimalPlaces(CBaseUISlider *slider);
    void onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider);
    void onSliderChangeInt(CBaseUISlider *slider);
    void onSliderChangeIntMS(CBaseUISlider *slider);
    void onSliderChangeFloatMS(CBaseUISlider *slider);
    void onSliderChangePercent(CBaseUISlider *slider);

    void onKeyBindingButtonPressed(CBaseUIButton *button);
    void onKeyUnbindButtonPressed(CBaseUIButton *button);
    void onKeyBindingsResetAllPressed(CBaseUIButton *button);
    void onSliderChangeSliderQuality(CBaseUISlider *slider);
    void onSliderChangeLetterboxingOffset(CBaseUISlider *slider);
    void onSliderChangeUIScale(CBaseUISlider *slider);

    void OpenASIOSettings();
    void onASIOBufferChange(CBaseUISlider *slider);
    void onWASAPIBufferChange(CBaseUISlider *slider);
    void onWASAPIPeriodChange(CBaseUISlider *slider);
    void onLoudnessNormalizationToggle(CBaseUICheckbox *checkbox);

    void onUseSkinsSoundSamplesChange(UString oldValue, UString newValue);
    void onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox);
    void onHighQualitySlidersConVarChange(UString oldValue, UString newValue);
    void onNightcorePreferenceChange(CBaseUICheckbox *checkbox);

    // categories
    void onCategoryClicked(CBaseUIButton *button);

    // reset
    void onResetUpdate(CBaseUIButton *button);
    void onResetClicked(CBaseUIButton *button);
    void onResetEverythingClicked(CBaseUIButton *button);

    // options
    void addSpacer();
    CBaseUILabel *addSection(UString text);
    CBaseUILabel *addSubSection(UString text, UString searchTags = "");
    CBaseUILabel *addLabel(UString text);
    UIButton *addButton(UString text);
    OPTIONS_ELEMENT addButton(UString text, UString labelText, bool withResetButton = false);
    OPTIONS_ELEMENT addButtonButton(UString text1, UString text2);
    OPTIONS_ELEMENT addButtonButtonLabel(UString text1, UString text2, UString labelText, bool withResetButton = false);
    OptionsMenuKeyBindButton *addKeyBindButton(UString text, ConVar *cvar);
    CBaseUICheckbox *addCheckbox(UString text, ConVar *cvar);
    CBaseUICheckbox *addCheckbox(UString text, UString tooltipText = "", ConVar *cvar = NULL);
    UISlider *addSlider(UString text, float min = 0.0f, float max = 1.0f, ConVar *cvar = NULL, float label1Width = 0.0f,
                        bool allowOverscale = false, bool allowUnderscale = false);
    CBaseUITextbox *addTextbox(UString text, ConVar *cvar = NULL);
    CBaseUITextbox *addTextbox(UString text, UString labelText, ConVar *cvar = NULL);
    CBaseUIElement *addSkinPreview();
    CBaseUIElement *addSliderPreview();

    // categories
    OptionsMenuCategoryButton *addCategory(CBaseUIElement *section, wchar_t icon);

    // vars
    Osu *m_osu;
    CBaseUIScrollView *m_categories;
    CBaseUIScrollView *m_options;
    UIContextMenu *m_contextMenu;
    UISearchOverlay *m_search;
    CBaseUILabel *m_spacer;
    OptionsMenuCategoryButton *m_fposuCategoryButton;

    std::vector<OptionsMenuCategoryButton *> m_categoryButtons;
    std::vector<OPTIONS_ELEMENT> m_elements;

    // custom
    bool m_bFullscreen;
    float m_fAnimation;

    CBaseUICheckbox *m_fullscreenCheckbox;
    CBaseUISlider *m_backgroundDimSlider;
    CBaseUISlider *m_backgroundBrightnessSlider;
    CBaseUISlider *m_hudSizeSlider;
    CBaseUISlider *m_hudComboScaleSlider;
    CBaseUISlider *m_hudScoreScaleSlider;
    CBaseUISlider *m_hudAccuracyScaleSlider;
    CBaseUISlider *m_hudHiterrorbarScaleSlider;
    CBaseUISlider *m_hudHiterrorbarURScaleSlider;
    CBaseUISlider *m_hudProgressbarScaleSlider;
    CBaseUISlider *m_hudScoreBarScaleSlider;
    CBaseUISlider *m_hudScoreBoardScaleSlider;
    CBaseUISlider *m_hudInputoverlayScaleSlider;
    CBaseUISlider *m_playfieldBorderSizeSlider;
    CBaseUISlider *m_statisticsOverlayScaleSlider;
    CBaseUISlider *m_statisticsOverlayXOffsetSlider;
    CBaseUISlider *m_statisticsOverlayYOffsetSlider;
    CBaseUISlider *m_cursorSizeSlider;
    CBaseUILabel *m_skinLabel;
    CBaseUIElement *m_skinSelectLocalButton;
    CBaseUIButton *m_resolutionSelectButton;
    CBaseUILabel *m_resolutionLabel;
    CBaseUITextbox *m_osuFolderTextbox;
    CBaseUIButton *m_outputDeviceSelectButton;
    CBaseUILabel *m_outputDeviceLabel;
    OptionsMenuResetButton *m_outputDeviceResetButton;
    CBaseUISlider *m_wasapiBufferSizeSlider;
    CBaseUISlider *m_wasapiPeriodSizeSlider;
    OptionsMenuResetButton *m_asioBufferSizeResetButton;
    OptionsMenuResetButton *m_wasapiBufferSizeResetButton;
    OptionsMenuResetButton *m_wasapiPeriodSizeResetButton;
    CBaseUISlider *m_asioBufferSizeSlider = nullptr;
    CBaseUISlider *m_sliderQualitySlider;
    CBaseUISlider *m_letterboxingOffsetXSlider;
    CBaseUISlider *m_letterboxingOffsetYSlider;
    CBaseUIButton *m_letterboxingOffsetResetButton;
    OptionsMenuSliderPreviewElement *m_sliderPreviewElement;
    CBaseUITextbox *m_dpiTextbox;
    CBaseUITextbox *m_cm360Textbox;
    CBaseUIElement *m_skinSection;
    CBaseUISlider *m_uiScaleSlider;
    OptionsMenuResetButton *m_uiScaleResetButton;
    CBaseUIElement *m_notelockSelectButton;
    CBaseUILabel *m_notelockSelectLabel;
    OptionsMenuResetButton *m_notelockSelectResetButton;
    CBaseUIElement *m_hpDrainSelectButton;
    CBaseUILabel *m_hpDrainSelectLabel;
    OptionsMenuResetButton *m_hpDrainSelectResetButton;

    CBaseUIElement *sectionOnline;
    CBaseUITextbox *m_serverTextbox;
    CBaseUICheckbox *m_submitScoresCheckbox;
    CBaseUITextbox *m_nameTextbox;
    CBaseUITextbox *m_passwordTextbox;
    UIButton *logInButton;

    ConVar *m_waitingKey;
    ConVar *m_osu_slider_curve_points_separation_ref;
    ConVar *m_osu_letterboxing_offset_x_ref;
    ConVar *m_osu_letterboxing_offset_y_ref;
    ConVar *m_osu_mod_fposu_ref;
    ConVar *m_osu_skin_ref;
    ConVar *m_osu_skin_random_ref;
    ConVar *m_osu_ui_scale_ref;
    ConVar *m_win_snd_wasapi_buffer_size_ref;
    ConVar *m_win_snd_wasapi_period_size_ref;
    ConVar *m_osu_notelock_type_ref;
    ConVar *m_osu_drain_type_ref;
    ConVar *m_osu_background_color_r_ref;
    ConVar *m_osu_background_color_g_ref;
    ConVar *m_osu_background_color_b_ref;

    float m_fOsuFolderTextboxInvalidAnim;
    float m_fVibrationStrengthExampleTimer;
    bool m_bLetterboxingOffsetUpdateScheduled;
    bool m_bUIScaleChangeScheduled;
    bool m_bUIScaleScrollToSliderScheduled;
    bool m_bDPIScalingScrollToSliderScheduled;
    bool m_bASIOBufferChangeScheduled;
    bool m_bWASAPIBufferChangeScheduled;
    bool m_bWASAPIPeriodChangeScheduled;

    int m_iNumResetAllKeyBindingsPressed;
    int m_iNumResetEverythingPressed;

    // search
    UString m_sSearchString;
    float m_fSearchOnCharKeybindHackTime;

    // notelock
    std::vector<UString> m_notelockTypes;

    // drain
    std::vector<UString> m_drainTypes;

    bool m_updating_layout = false;
};

#endif
