#pragma once
#include "NotificationOverlay.h"
#include "ScreenBackable.h"

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
    OptionsMenu();
    ~OptionsMenu() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onKeyDown(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

    void onResolutionChange(Vector2 newResolution) override;

    void onKey(KeyboardEvent &e) override;

    CBaseUIContainer *setVisible(bool visible) override;

    void save();

    void openAndScrollToSkinSection();

    void setFullscreen(bool fullscreen) { this->bFullscreen = fullscreen; }

    void setUsername(UString username);

    [[nodiscard]] inline bool isFullscreen() const { return this->bFullscreen; }
    bool isMouseInside() override;
    bool isBusy() override;

   public:
    enum class RenderCondition {
        NONE,
        ASIO_ENABLED,
        WASAPI_ENABLED,
        SCORE_SUBMISSION_POLICY,
    };

    struct OPTIONS_ELEMENT {
        OPTIONS_ELEMENT() {
            this->resetButton = NULL;
            this->cvar = NULL;
            this->type = -1;

            this->label1Width = 0.0f;
            this->relSizeDPI = 96.0f;

            this->allowOverscale = false;
            this->allowUnderscale = false;
            this->render_condition = RenderCondition::NONE;
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

    void updateLayout() override;
    void onBack() override;

    void setVisibleInt(bool visible, bool fromOnBack = false);
    void scheduleSearchUpdate();

    void askForLoginDetails();
    void updateOsuFolder();
    void updateFposuDPI();
    void updateFposuCMper360();
    void updateSkinNameLabel();
    void updateNotelockSelectLabel();

    // options
    void onFullscreenChange(CBaseUICheckbox *checkbox);
    void onBorderlessWindowedChange(CBaseUICheckbox *checkbox);
    void onDPIScalingChange(CBaseUICheckbox *checkbox);
    void onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox);
    void openCurrentSkinFolder();
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
    void onCM360CalculatorLinkClicked();
    void onNotelockSelect();
    void onNotelockSelect2(UString notelockType, int id = -1);
    void onNotelockSelectResetClicked();
    void onNotelockSelectResetUpdate();

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
    void onNightcoreToggle(CBaseUICheckbox *checkbox);

    void onUseSkinsSoundSamplesChange(UString oldValue, UString newValue);
    void onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox);
    void onHighQualitySlidersConVarChange(UString oldValue, UString newValue);

    // categories
    void onCategoryClicked(CBaseUIButton *button);

    // reset
    void onResetUpdate(CBaseUIButton *button);
    void onResetClicked(CBaseUIButton *button);
    void onResetEverythingClicked(CBaseUIButton *button);
    void onImportSettingsFromStable(CBaseUIButton *button);

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
    CBaseUIScrollView *categories;
    CBaseUIScrollView *options;
    UIContextMenu *contextMenu;
    UISearchOverlay *search;
    CBaseUILabel *spacer;
    OptionsMenuCategoryButton *fposuCategoryButton;

    std::vector<OptionsMenuCategoryButton *> categoryButtons;
    std::vector<OPTIONS_ELEMENT> elements;

    // custom
    bool bFullscreen;
    float fAnimation;

    CBaseUICheckbox *fullscreenCheckbox;
    CBaseUISlider *backgroundDimSlider;
    CBaseUISlider *backgroundBrightnessSlider;
    CBaseUISlider *hudSizeSlider;
    CBaseUISlider *hudComboScaleSlider;
    CBaseUISlider *hudScoreScaleSlider;
    CBaseUISlider *hudAccuracyScaleSlider;
    CBaseUISlider *hudHiterrorbarScaleSlider;
    CBaseUISlider *hudHiterrorbarURScaleSlider;
    CBaseUISlider *hudProgressbarScaleSlider;
    CBaseUISlider *hudScoreBarScaleSlider;
    CBaseUISlider *hudScoreBoardScaleSlider;
    CBaseUISlider *hudInputoverlayScaleSlider;
    CBaseUISlider *playfieldBorderSizeSlider;
    CBaseUISlider *statisticsOverlayScaleSlider;
    CBaseUISlider *statisticsOverlayXOffsetSlider;
    CBaseUISlider *statisticsOverlayYOffsetSlider;
    CBaseUISlider *cursorSizeSlider;
    CBaseUILabel *skinLabel;
    CBaseUIElement *skinSelectLocalButton;
    CBaseUIButton *resolutionSelectButton;
    CBaseUILabel *resolutionLabel;
    CBaseUITextbox *osuFolderTextbox;
    CBaseUIButton *outputDeviceSelectButton;
    CBaseUILabel *outputDeviceLabel;
    OptionsMenuResetButton *outputDeviceResetButton;
    CBaseUISlider *wasapiBufferSizeSlider;
    CBaseUISlider *wasapiPeriodSizeSlider;
    OptionsMenuResetButton *asioBufferSizeResetButton;
    OptionsMenuResetButton *wasapiBufferSizeResetButton;
    OptionsMenuResetButton *wasapiPeriodSizeResetButton;
    CBaseUISlider *asioBufferSizeSlider = NULL;
    CBaseUISlider *sliderQualitySlider;
    CBaseUISlider *letterboxingOffsetXSlider;
    CBaseUISlider *letterboxingOffsetYSlider;
    CBaseUIButton *letterboxingOffsetResetButton;
    OptionsMenuSliderPreviewElement *sliderPreviewElement;
    CBaseUITextbox *dpiTextbox;
    CBaseUITextbox *cm360Textbox;
    CBaseUIElement *skinSection;
    CBaseUISlider *uiScaleSlider;
    OptionsMenuResetButton *uiScaleResetButton;
    CBaseUIElement *notelockSelectButton;
    CBaseUILabel *notelockSelectLabel;
    OptionsMenuResetButton *notelockSelectResetButton;
    CBaseUIElement *hpDrainSelectButton;
    CBaseUILabel *hpDrainSelectLabel;
    OptionsMenuResetButton *hpDrainSelectResetButton;

    CBaseUIElement *sectionOnline;
    CBaseUITextbox *serverTextbox;
    CBaseUICheckbox *submitScoresCheckbox;
    CBaseUITextbox *nameTextbox;
    CBaseUITextbox *passwordTextbox;
    UIButton *logInButton;

    ConVar *waitingKey = NULL;

    float fOsuFolderTextboxInvalidAnim;
    float fVibrationStrengthExampleTimer;
    bool bLetterboxingOffsetUpdateScheduled;
    bool bUIScaleChangeScheduled;
    bool bUIScaleScrollToSliderScheduled;
    bool bDPIScalingScrollToSliderScheduled;
    bool bASIOBufferChangeScheduled;
    bool bWASAPIBufferChangeScheduled;
    bool bWASAPIPeriodChangeScheduled;

    int iNumResetAllKeyBindingsPressed;
    int iNumResetEverythingPressed;

    // search
    UString sSearchString;
    float fSearchOnCharKeybindHackTime;

    // notelock
    std::vector<UString> notelockTypes;

    bool updating_layout = false;
};
