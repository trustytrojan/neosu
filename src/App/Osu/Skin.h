#pragma once
#include "cbase.h"

class Image;
class Sound;
class Resource;
class ConVar;

class SkinImage;

class Skin {
   public:
    static void unpack(const char *filepath);

    static ConVar *m_osu_skin_async;
    static ConVar *m_osu_skin_hd;

    Skin(UString name, std::string filepath, bool isDefaultSkin = false);
    virtual ~Skin();

    void update();

    bool isReady();

    void load();
    void loadBeatmapOverride(std::string filepath);
    void reloadSounds();

    void setAnimationSpeed(float animationSpeed) { m_animationSpeedMultiplier = animationSpeed; }
    float getAnimationSpeed() { return m_animationSpeedMultiplier; }

    // samples
    void setSampleSet(int sampleSet);
    void setSampleVolume(float volume, bool force = false);
    void resetSampleVolume();

    void playHitCircleSound(int sampleType, float pan = 0.0f, long delta = 0);
    void playSliderTickSound(float pan = 0.0f);
    void playSliderSlideSound(float pan = 0.0f);
    void playSpinnerSpinSound();
    void playSpinnerBonusSound();

    void stopSliderSlideSound(int sampleSet = -2);
    void stopSpinnerSpinSound();

    // custom
    void randomizeFilePath();

    // drawable helpers
    inline std::string getName() { return m_sName; }
    inline std::string getFilePath() { return m_sFilePath; }

    // raw
    inline Image *getMissingTexture() { return m_missingTexture; }

    inline Image *getHitCircle() { return m_hitCircle; }
    inline SkinImage *getHitCircleOverlay2() { return m_hitCircleOverlay2; }
    inline Image *getApproachCircle() { return m_approachCircle; }
    inline Image *getReverseArrow() { return m_reverseArrow; }
    inline SkinImage *getFollowPoint2() { return m_followPoint2; }

    inline Image *getDefault0() { return m_default0; }
    inline Image *getDefault1() { return m_default1; }
    inline Image *getDefault2() { return m_default2; }
    inline Image *getDefault3() { return m_default3; }
    inline Image *getDefault4() { return m_default4; }
    inline Image *getDefault5() { return m_default5; }
    inline Image *getDefault6() { return m_default6; }
    inline Image *getDefault7() { return m_default7; }
    inline Image *getDefault8() { return m_default8; }
    inline Image *getDefault9() { return m_default9; }

    inline Image *getScore0() { return m_score0; }
    inline Image *getScore1() { return m_score1; }
    inline Image *getScore2() { return m_score2; }
    inline Image *getScore3() { return m_score3; }
    inline Image *getScore4() { return m_score4; }
    inline Image *getScore5() { return m_score5; }
    inline Image *getScore6() { return m_score6; }
    inline Image *getScore7() { return m_score7; }
    inline Image *getScore8() { return m_score8; }
    inline Image *getScore9() { return m_score9; }
    inline Image *getScoreX() { return m_scoreX; }
    inline Image *getScorePercent() { return m_scorePercent; }
    inline Image *getScoreDot() { return m_scoreDot; }

    inline Image *getCombo0() { return m_combo0; }
    inline Image *getCombo1() { return m_combo1; }
    inline Image *getCombo2() { return m_combo2; }
    inline Image *getCombo3() { return m_combo3; }
    inline Image *getCombo4() { return m_combo4; }
    inline Image *getCombo5() { return m_combo5; }
    inline Image *getCombo6() { return m_combo6; }
    inline Image *getCombo7() { return m_combo7; }
    inline Image *getCombo8() { return m_combo8; }
    inline Image *getCombo9() { return m_combo9; }
    inline Image *getComboX() { return m_comboX; }

    inline SkinImage *getPlaySkip() { return m_playSkip; }
    inline Image *getPlayWarningArrow() { return m_playWarningArrow; }
    inline SkinImage *getPlayWarningArrow2() { return m_playWarningArrow2; }
    inline Image *getCircularmetre() { return m_circularmetre; }
    inline SkinImage *getScorebarBg() { return m_scorebarBg; }
    inline SkinImage *getScorebarColour() { return m_scorebarColour; }
    inline SkinImage *getScorebarMarker() { return m_scorebarMarker; }
    inline SkinImage *getScorebarKi() { return m_scorebarKi; }
    inline SkinImage *getScorebarKiDanger() { return m_scorebarKiDanger; }
    inline SkinImage *getScorebarKiDanger2() { return m_scorebarKiDanger2; }
    inline SkinImage *getSectionPassImage() { return m_sectionPassImage; }
    inline SkinImage *getSectionFailImage() { return m_sectionFailImage; }
    inline SkinImage *getInputoverlayBackground() { return m_inputoverlayBackground; }
    inline SkinImage *getInputoverlayKey() { return m_inputoverlayKey; }

    inline SkinImage *getHit0() { return m_hit0; }
    inline SkinImage *getHit50() { return m_hit50; }
    inline SkinImage *getHit50g() { return m_hit50g; }
    inline SkinImage *getHit50k() { return m_hit50k; }
    inline SkinImage *getHit100() { return m_hit100; }
    inline SkinImage *getHit100g() { return m_hit100g; }
    inline SkinImage *getHit100k() { return m_hit100k; }
    inline SkinImage *getHit300() { return m_hit300; }
    inline SkinImage *getHit300g() { return m_hit300g; }
    inline SkinImage *getHit300k() { return m_hit300k; }

    inline Image *getParticle50() { return m_particle50; }
    inline Image *getParticle100() { return m_particle100; }
    inline Image *getParticle300() { return m_particle300; }

    inline Image *getSliderGradient() { return m_sliderGradient; }
    inline SkinImage *getSliderb() { return m_sliderb; }
    inline SkinImage *getSliderFollowCircle2() { return m_sliderFollowCircle2; }
    inline Image *getSliderScorePoint() { return m_sliderScorePoint; }
    inline Image *getSliderStartCircle() { return m_sliderStartCircle; }
    inline SkinImage *getSliderStartCircle2() { return m_sliderStartCircle2; }
    inline Image *getSliderStartCircleOverlay() { return m_sliderStartCircleOverlay; }
    inline SkinImage *getSliderStartCircleOverlay2() { return m_sliderStartCircleOverlay2; }
    inline Image *getSliderEndCircle() { return m_sliderEndCircle; }
    inline SkinImage *getSliderEndCircle2() { return m_sliderEndCircle2; }
    inline Image *getSliderEndCircleOverlay() { return m_sliderEndCircleOverlay; }
    inline SkinImage *getSliderEndCircleOverlay2() { return m_sliderEndCircleOverlay2; }

    inline Image *getSpinnerBackground() { return m_spinnerBackground; }
    inline Image *getSpinnerCircle() { return m_spinnerCircle; }
    inline Image *getSpinnerApproachCircle() { return m_spinnerApproachCircle; }
    inline Image *getSpinnerBottom() { return m_spinnerBottom; }
    inline Image *getSpinnerMiddle() { return m_spinnerMiddle; }
    inline Image *getSpinnerMiddle2() { return m_spinnerMiddle2; }
    inline Image *getSpinnerTop() { return m_spinnerTop; }
    inline Image *getSpinnerSpin() { return m_spinnerSpin; }
    inline Image *getSpinnerClear() { return m_spinnerClear; }

    inline Image *getDefaultCursor() { return m_defaultCursor; }
    inline Image *getCursor() { return m_cursor; }
    inline Image *getCursorMiddle() { return m_cursorMiddle; }
    inline Image *getCursorTrail() { return m_cursorTrail; }
    inline Image *getCursorRipple() { return m_cursorRipple; }

    inline SkinImage *getSelectionModEasy() { return m_selectionModEasy; }
    inline SkinImage *getSelectionModNoFail() { return m_selectionModNoFail; }
    inline SkinImage *getSelectionModHalfTime() { return m_selectionModHalfTime; }
    inline SkinImage *getSelectionModDayCore() { return m_selectionModDayCore; }
    inline SkinImage *getSelectionModHardRock() { return m_selectionModHardRock; }
    inline SkinImage *getSelectionModSuddenDeath() { return m_selectionModSuddenDeath; }
    inline SkinImage *getSelectionModPerfect() { return m_selectionModPerfect; }
    inline SkinImage *getSelectionModDoubleTime() { return m_selectionModDoubleTime; }
    inline SkinImage *getSelectionModNightCore() { return m_selectionModNightCore; }
    inline SkinImage *getSelectionModHidden() { return m_selectionModHidden; }
    inline SkinImage *getSelectionModFlashlight() { return m_selectionModFlashlight; }
    inline SkinImage *getSelectionModRelax() { return m_selectionModRelax; }
    inline SkinImage *getSelectionModAutopilot() { return m_selectionModAutopilot; }
    inline SkinImage *getSelectionModSpunOut() { return m_selectionModSpunOut; }
    inline SkinImage *getSelectionModAutoplay() { return m_selectionModAutoplay; }
    inline SkinImage *getSelectionModNightmare() { return m_selectionModNightmare; }
    inline SkinImage *getSelectionModTarget() { return m_selectionModTarget; }
    inline SkinImage *getSelectionModScorev2() { return m_selectionModScorev2; }
    inline SkinImage *getSelectionModTD() { return m_selectionModTD; }

    inline Image *getPauseContinue() { return m_pauseContinue; }
    inline Image *getPauseRetry() { return m_pauseRetry; }
    inline Image *getPauseBack() { return m_pauseBack; }
    inline Image *getPauseOverlay() { return m_pauseOverlay; }
    inline Image *getFailBackground() { return m_failBackground; }
    inline Image *getUnpause() { return m_unpause; }

    inline Image *getButtonLeft() { return m_buttonLeft; }
    inline Image *getButtonMiddle() { return m_buttonMiddle; }
    inline Image *getButtonRight() { return m_buttonRight; }
    inline Image *getDefaultButtonLeft() { return m_defaultButtonLeft; }
    inline Image *getDefaultButtonMiddle() { return m_defaultButtonMiddle; }
    inline Image *getDefaultButtonRight() { return m_defaultButtonRight; }
    inline SkinImage *getMenuBack2() { return m_menuBackImg; }
    inline SkinImage *getSelectionMode() { return m_selectionMode; }
    inline SkinImage *getSelectionModeOver() { return m_selectionModeOver; }
    inline SkinImage *getSelectionMods() { return m_selectionMods; }
    inline SkinImage *getSelectionModsOver() { return m_selectionModsOver; }
    inline SkinImage *getSelectionRandom() { return m_selectionRandom; }
    inline SkinImage *getSelectionRandomOver() { return m_selectionRandomOver; }
    inline SkinImage *getSelectionOptions() { return m_selectionOptions; }
    inline SkinImage *getSelectionOptionsOver() { return m_selectionOptionsOver; }

    inline Image *getSongSelectTop() { return m_songSelectTop; }
    inline Image *getSongSelectBottom() { return m_songSelectBottom; }
    inline Image *getMenuButtonBackground() { return m_menuButtonBackground; }
    inline SkinImage *getMenuButtonBackground2() { return m_menuButtonBackground2; }
    inline Image *getStar() { return m_star; }
    inline Image *getRankingPanel() { return m_rankingPanel; }
    inline Image *getRankingGraph() { return m_rankingGraph; }
    inline Image *getRankingTitle() { return m_rankingTitle; }
    inline Image *getRankingMaxCombo() { return m_rankingMaxCombo; }
    inline Image *getRankingAccuracy() { return m_rankingAccuracy; }
    inline Image *getRankingA() { return m_rankingA; }
    inline Image *getRankingB() { return m_rankingB; }
    inline Image *getRankingC() { return m_rankingC; }
    inline Image *getRankingD() { return m_rankingD; }
    inline Image *getRankingS() { return m_rankingS; }
    inline Image *getRankingSH() { return m_rankingSH; }
    inline Image *getRankingX() { return m_rankingX; }
    inline Image *getRankingXH() { return m_rankingXH; }
    inline SkinImage *getRankingAsmall() { return m_rankingAsmall; }
    inline SkinImage *getRankingBsmall() { return m_rankingBsmall; }
    inline SkinImage *getRankingCsmall() { return m_rankingCsmall; }
    inline SkinImage *getRankingDsmall() { return m_rankingDsmall; }
    inline SkinImage *getRankingSsmall() { return m_rankingSsmall; }
    inline SkinImage *getRankingSHsmall() { return m_rankingSHsmall; }
    inline SkinImage *getRankingXsmall() { return m_rankingXsmall; }
    inline SkinImage *getRankingXHsmall() { return m_rankingXHsmall; }
    inline SkinImage *getRankingPerfect() { return m_rankingPerfect; }

    inline Image *getBeatmapImportSpinner() { return m_beatmapImportSpinner; }
    inline Image *getLoadingSpinner() { return m_loadingSpinner; }
    inline Image *getCircleEmpty() { return m_circleEmpty; }
    inline Image *getCircleFull() { return m_circleFull; }
    inline Image *getSeekTriangle() { return m_seekTriangle; }
    inline Image *getUserIcon() { return m_userIcon; }
    inline Image *getBackgroundCube() { return m_backgroundCube; }
    inline Image *getMenuBackground() { return m_menuBackground; }
    inline Image *getSkybox() { return m_skybox; }

    inline Sound *getSpinnerBonus() { return m_spinnerBonus; }
    inline Sound *getSpinnerSpinSound() { return m_spinnerSpinSound; }

    inline Sound *getCombobreak() { return m_combobreak; }
    inline Sound *getFailsound() { return m_failsound; }
    inline Sound *getApplause() { return m_applause; }
    inline Sound *getMenuHit() { return m_menuHit; }
    inline Sound *getMenuHover() { return m_menuHover; }
    inline Sound *getCheckOn() { return m_checkOn; }
    inline Sound *getCheckOff() { return m_checkOff; }
    inline Sound *getShutter() { return m_shutter; }
    inline Sound *getSectionPassSound() { return m_sectionPassSound; }
    inline Sound *getSectionFailSound() { return m_sectionFailSound; }

    inline bool isCursor2x() { return m_bCursor2x; }
    inline bool isCursorTrail2x() { return m_bCursorTrail2x; }
    inline bool isCursorRipple2x() { return m_bCursorRipple2x; }
    inline bool isApproachCircle2x() { return m_bApproachCircle2x; }
    inline bool isReverseArrow2x() { return m_bReverseArrow2x; }
    inline bool isHitCircle2x() { return m_bHitCircle2x; }
    inline bool isDefault02x() { return m_bIsDefault02x; }
    inline bool isDefault12x() { return m_bIsDefault12x; }
    inline bool isScore02x() { return m_bIsScore02x; }
    inline bool isCombo02x() { return m_bIsCombo02x; }
    inline bool isSpinnerApproachCircle2x() { return m_bSpinnerApproachCircle2x; }
    inline bool isSpinnerBottom2x() { return m_bSpinnerBottom2x; }
    inline bool isSpinnerCircle2x() { return m_bSpinnerCircle2x; }
    inline bool isSpinnerTop2x() { return m_bSpinnerTop2x; }
    inline bool isSpinnerMiddle2x() { return m_bSpinnerMiddle2x; }
    inline bool isSpinnerMiddle22x() { return m_bSpinnerMiddle22x; }
    inline bool isSliderScorePoint2x() { return m_bSliderScorePoint2x; }
    inline bool isSliderStartCircle2x() { return m_bSliderStartCircle2x; }
    inline bool isSliderEndCircle2x() { return m_bSliderEndCircle2x; }

    inline bool isCircularmetre2x() { return m_bCircularmetre2x; }

    inline bool isPauseContinue2x() { return m_bPauseContinue2x; }

    inline bool isMenuButtonBackground2x() { return m_bMenuButtonBackground2x; }
    inline bool isStar2x() { return m_bStar2x; }
    inline bool isRankingPanel2x() { return m_bRankingPanel2x; }
    inline bool isRankingMaxCombo2x() { return m_bRankingMaxCombo2x; }
    inline bool isRankingAccuracy2x() { return m_bRankingAccuracy2x; }
    inline bool isRankingA2x() { return m_bRankingA2x; }
    inline bool isRankingB2x() { return m_bRankingB2x; }
    inline bool isRankingC2x() { return m_bRankingC2x; }
    inline bool isRankingD2x() { return m_bRankingD2x; }
    inline bool isRankingS2x() { return m_bRankingS2x; }
    inline bool isRankingSH2x() { return m_bRankingSH2x; }
    inline bool isRankingX2x() { return m_bRankingX2x; }
    inline bool isRankingXH2x() { return m_bRankingXH2x; }

    // skin.ini
    inline float getVersion() { return m_fVersion; }
    inline float getAnimationFramerate() { return m_fAnimationFramerate; }
    Color getComboColorForCounter(int i, int offset);
    void setBeatmapComboColors(std::vector<Color> colors);
    inline Color getSpinnerApproachCircleColor() { return m_spinnerApproachCircleColor; }
    inline Color getSliderBorderColor() { return m_sliderBorderColor; }
    inline Color getSliderTrackOverride() { return m_sliderTrackOverride; }
    inline Color getSliderBallColor() { return m_sliderBallColor; }

    inline Color getSongSelectActiveText() { return m_songSelectActiveText; }
    inline Color getSongSelectInactiveText() { return m_songSelectInactiveText; }

    inline Color getInputOverlayText() { return m_inputOverlayText; }

    inline bool getCursorCenter() { return m_bCursorCenter; }
    inline bool getCursorRotate() { return m_bCursorRotate; }
    inline bool getCursorExpand() { return m_bCursorExpand; }

    inline bool getSliderBallFlip() { return m_bSliderBallFlip; }
    inline bool getAllowSliderBallTint() { return m_bAllowSliderBallTint; }
    inline int getSliderStyle() { return m_iSliderStyle; }
    inline bool getHitCircleOverlayAboveNumber() { return m_bHitCircleOverlayAboveNumber; }
    inline bool isSliderTrackOverridden() { return m_bSliderTrackOverride; }

    inline std::string getComboPrefix() { return m_sComboPrefix; }
    inline int getComboOverlap() { return m_iComboOverlap; }

    inline std::string getScorePrefix() { return m_sScorePrefix; }
    inline int getScoreOverlap() { return m_iScoreOverlap; }

    inline std::string getHitCirclePrefix() { return m_sHitCirclePrefix; }
    inline int getHitCircleOverlap() { return m_iHitCircleOverlap; }

    // custom
    inline bool useSmoothCursorTrail() const { return m_cursorMiddle != m_missingTexture; }
    inline bool isDefaultSkin() const { return m_bIsDefaultSkin; }
    inline int getSampleSet() const { return m_iSampleSet; }

    static ConVar *m_osu_skin_ref;
    static ConVar *m_osu_mod_fposu_ref;

    static Image *m_missingTexture;

    struct SOUND_SAMPLE {
        Sound *sound;
        float hardcodedVolumeMultiplier;  // some samples in osu have hardcoded multipliers which can not be modified
                                          // (i.e. you can NEVER reach 100% volume with them)
    };

    bool parseSkinINI(std::string filepath);

    bool compareFilenameWithSkinElementName(std::string filename, std::string skinElementName);

    SkinImage *createSkinImage(std::string skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
                               bool ignoreDefaultSkin = false, std::string animationSeparator = "-");
    void checkLoadImage(Image **addressOfPointer, std::string skinElementName, std::string resourceName,
                        bool ignoreDefaultSkin = false, std::string fileExtension = "png",
                        bool forceLoadMipmaps = false);

    void checkLoadSound(Sound **addressOfPointer, std::string skinElementName, std::string resourceName,
                        bool isOverlayable = false, bool isSample = false, bool loop = false,
                        bool fallback_to_default = true, float hardcodedVolumeMultiplier = -1.0f);

    void onEffectVolumeChange(UString oldValue, UString newValue);
    void onIgnoreBeatmapSampleVolumeChange(UString oldValue, UString newValue);

    bool m_bReady;
    bool m_bIsDefaultSkin;
    float m_animationSpeedMultiplier;
    std::string m_sName;
    std::string m_sFilePath;
    std::string m_sSkinIniFilePath;
    std::vector<Resource *> m_resources;
    std::vector<Sound *> m_sounds;
    std::vector<SOUND_SAMPLE> m_soundSamples;
    std::vector<SkinImage *> m_images;

    // images
    Image *m_hitCircle;
    SkinImage *m_hitCircleOverlay2;
    Image *m_approachCircle;
    Image *m_reverseArrow;
    SkinImage *m_followPoint2;

    Image *m_default0;
    Image *m_default1;
    Image *m_default2;
    Image *m_default3;
    Image *m_default4;
    Image *m_default5;
    Image *m_default6;
    Image *m_default7;
    Image *m_default8;
    Image *m_default9;

    Image *m_score0;
    Image *m_score1;
    Image *m_score2;
    Image *m_score3;
    Image *m_score4;
    Image *m_score5;
    Image *m_score6;
    Image *m_score7;
    Image *m_score8;
    Image *m_score9;
    Image *m_scoreX;
    Image *m_scorePercent;
    Image *m_scoreDot;

    Image *m_combo0;
    Image *m_combo1;
    Image *m_combo2;
    Image *m_combo3;
    Image *m_combo4;
    Image *m_combo5;
    Image *m_combo6;
    Image *m_combo7;
    Image *m_combo8;
    Image *m_combo9;
    Image *m_comboX;

    SkinImage *m_playSkip;
    Image *m_playWarningArrow;
    SkinImage *m_playWarningArrow2;
    Image *m_circularmetre;
    SkinImage *m_scorebarBg;
    SkinImage *m_scorebarColour;
    SkinImage *m_scorebarMarker;
    SkinImage *m_scorebarKi;
    SkinImage *m_scorebarKiDanger;
    SkinImage *m_scorebarKiDanger2;
    SkinImage *m_sectionPassImage;
    SkinImage *m_sectionFailImage;
    SkinImage *m_inputoverlayBackground;
    SkinImage *m_inputoverlayKey;

    SkinImage *m_hit0;
    SkinImage *m_hit50;
    SkinImage *m_hit50g;
    SkinImage *m_hit50k;
    SkinImage *m_hit100;
    SkinImage *m_hit100g;
    SkinImage *m_hit100k;
    SkinImage *m_hit300;
    SkinImage *m_hit300g;
    SkinImage *m_hit300k;

    Image *m_particle50;
    Image *m_particle100;
    Image *m_particle300;

    Image *m_sliderGradient;
    SkinImage *m_sliderb;
    SkinImage *m_sliderFollowCircle2;
    Image *m_sliderScorePoint;
    Image *m_sliderStartCircle;
    SkinImage *m_sliderStartCircle2;
    Image *m_sliderStartCircleOverlay;
    SkinImage *m_sliderStartCircleOverlay2;
    Image *m_sliderEndCircle;
    SkinImage *m_sliderEndCircle2;
    Image *m_sliderEndCircleOverlay;
    SkinImage *m_sliderEndCircleOverlay2;

    Image *m_spinnerBackground;
    Image *m_spinnerCircle;
    Image *m_spinnerApproachCircle;
    Image *m_spinnerBottom;
    Image *m_spinnerMiddle;
    Image *m_spinnerMiddle2;
    Image *m_spinnerTop;
    Image *m_spinnerSpin;
    Image *m_spinnerClear;

    Image *m_defaultCursor;
    Image *m_cursor;
    Image *m_cursorMiddle;
    Image *m_cursorTrail;
    Image *m_cursorRipple;

    SkinImage *m_selectionModEasy;
    SkinImage *m_selectionModNoFail;
    SkinImage *m_selectionModHalfTime;
    SkinImage *m_selectionModDayCore;
    SkinImage *m_selectionModHardRock;
    SkinImage *m_selectionModSuddenDeath;
    SkinImage *m_selectionModPerfect;
    SkinImage *m_selectionModDoubleTime;
    SkinImage *m_selectionModNightCore;
    SkinImage *m_selectionModHidden;
    SkinImage *m_selectionModFlashlight;
    SkinImage *m_selectionModRelax;
    SkinImage *m_selectionModAutopilot;
    SkinImage *m_selectionModSpunOut;
    SkinImage *m_selectionModAutoplay;
    SkinImage *m_selectionModNightmare;
    SkinImage *m_selectionModTarget;
    SkinImage *m_selectionModScorev2;
    SkinImage *m_selectionModTD;
    SkinImage *m_selectionModCinema;

    Image *m_pauseContinue;
    Image *m_pauseReplay;
    Image *m_pauseRetry;
    Image *m_pauseBack;
    Image *m_pauseOverlay;
    Image *m_failBackground;
    Image *m_unpause;

    Image *m_buttonLeft;
    Image *m_buttonMiddle;
    Image *m_buttonRight;
    Image *m_defaultButtonLeft;
    Image *m_defaultButtonMiddle;
    Image *m_defaultButtonRight;
    SkinImage *m_menuBackImg;
    SkinImage *m_selectionMode;
    SkinImage *m_selectionModeOver;
    SkinImage *m_selectionMods;
    SkinImage *m_selectionModsOver;
    SkinImage *m_selectionRandom;
    SkinImage *m_selectionRandomOver;
    SkinImage *m_selectionOptions;
    SkinImage *m_selectionOptionsOver;

    Image *m_songSelectTop;
    Image *m_songSelectBottom;
    Image *m_menuButtonBackground;
    SkinImage *m_menuButtonBackground2;
    Image *m_star;
    Image *m_rankingPanel;
    Image *m_rankingGraph;
    Image *m_rankingTitle;
    Image *m_rankingMaxCombo;
    Image *m_rankingAccuracy;
    Image *m_rankingA;
    Image *m_rankingB;
    Image *m_rankingC;
    Image *m_rankingD;
    Image *m_rankingS;
    Image *m_rankingSH;
    Image *m_rankingX;
    Image *m_rankingXH;
    SkinImage *m_rankingAsmall;
    SkinImage *m_rankingBsmall;
    SkinImage *m_rankingCsmall;
    SkinImage *m_rankingDsmall;
    SkinImage *m_rankingSsmall;
    SkinImage *m_rankingSHsmall;
    SkinImage *m_rankingXsmall;
    SkinImage *m_rankingXHsmall;
    SkinImage *m_rankingPerfect;

    Image *m_beatmapImportSpinner;
    Image *m_loadingSpinner;
    Image *m_circleEmpty;
    Image *m_circleFull;
    Image *m_seekTriangle;
    Image *m_userIcon;
    Image *m_backgroundCube;
    Image *m_menuBackground;
    Image *m_skybox;

    // sounds
    Sound *m_normalHitNormal;
    Sound *m_normalHitWhistle;
    Sound *m_normalHitFinish;
    Sound *m_normalHitClap;

    Sound *m_normalSliderTick;
    Sound *m_normalSliderSlide;
    Sound *m_normalSliderWhistle;

    Sound *m_softHitNormal;
    Sound *m_softHitWhistle;
    Sound *m_softHitFinish;
    Sound *m_softHitClap;

    Sound *m_softSliderTick;
    Sound *m_softSliderSlide;
    Sound *m_softSliderWhistle;

    Sound *m_drumHitNormal;
    Sound *m_drumHitWhistle;
    Sound *m_drumHitFinish;
    Sound *m_drumHitClap;

    Sound *m_drumSliderTick;
    Sound *m_drumSliderSlide;
    Sound *m_drumSliderWhistle;

    Sound *m_spinnerBonus;
    Sound *m_spinnerSpinSound;

    Sound *m_tooearly;
    Sound *m_toolate;

    // Plays when sending a message in chat
    Sound *m_messageSent = NULL;

    // Plays when deleting text in a message in chat
    Sound *m_deletingText = NULL;

    // Plays when changing the text cursor position
    Sound *m_movingTextCursor = NULL;

    // Plays when pressing a key for chat, search, edit, etc
    Sound *m_typing1 = NULL;
    Sound *m_typing2 = NULL;
    Sound *m_typing3 = NULL;
    Sound *m_typing4 = NULL;

    // Plays when returning to the previous screen
    Sound *m_menuBack = NULL;

    // Plays when closing a chat tab
    Sound *m_closeChatTab = NULL;

    // Plays when hovering above all selectable boxes except beatmaps or main screen buttons
    Sound *m_hoverButton = NULL;

    // Plays when clicking to confirm a button or dropdown option, opening or
    // closing chat, switching between chat tabs, or switching groups
    Sound *m_clickButton = NULL;

    // Main menu sounds
    Sound *m_clickMainMenuCube = NULL;
    Sound *m_hoverMainMenuCube = NULL;
    Sound *m_clickSingleplayer = NULL;
    Sound *m_hoverSingleplayer = NULL;
    Sound *m_clickMultiplayer = NULL;
    Sound *m_hoverMultiplayer = NULL;
    Sound *m_clickOptions = NULL;
    Sound *m_hoverOptions = NULL;
    Sound *m_clickExit = NULL;
    Sound *m_hoverExit = NULL;

    // Pause menu sounds
    Sound *m_pauseLoop = NULL;
    Sound *m_pauseHover = NULL;
    Sound *m_clickPauseBack = NULL;
    Sound *m_hoverPauseBack = NULL;
    Sound *m_clickPauseContinue = NULL;
    Sound *m_hoverPauseContinue = NULL;
    Sound *m_clickPauseRetry = NULL;
    Sound *m_hoverPauseRetry = NULL;

    // Back button sounds
    Sound *m_backButtonClick = NULL;
    Sound *m_backButtonHover = NULL;

    // Plays when switching into song selection, selecting a beatmap, opening dropdown boxes, opening chat tabs
    Sound *m_expand = NULL;

    // Plays when selecting a difficulty of a beatmap
    Sound *m_selectDifficulty = NULL;

    // Plays when changing the options via a slider
    Sound *m_sliderbar = NULL;

    // Multiplayer sounds
    Sound *m_matchConfirm = NULL;  // all players are ready
    Sound *m_roomJoined = NULL;    // a player joined
    Sound *m_roomQuit = NULL;      // a player left
    Sound *m_roomNotReady = NULL;  // a player is no longer ready
    Sound *m_roomReady = NULL;     // a player is now ready
    Sound *m_matchStart = NULL;    // match started

    Sound *m_combobreak;
    Sound *m_failsound;
    Sound *m_applause;
    Sound *m_menuHit;
    Sound *m_menuHover;
    Sound *m_checkOn;
    Sound *m_checkOff;
    Sound *m_shutter;
    Sound *m_sectionPassSound;
    Sound *m_sectionFailSound;

    // colors
    std::vector<Color> m_comboColors;
    std::vector<Color> m_beatmapComboColors;
    Color m_spinnerApproachCircleColor;
    Color m_sliderBorderColor;
    Color m_sliderTrackOverride;
    Color m_sliderBallColor;

    Color m_songSelectInactiveText;
    Color m_songSelectActiveText;

    Color m_inputOverlayText;

    // scaling
    bool m_bCursor2x;
    bool m_bCursorTrail2x;
    bool m_bCursorRipple2x;
    bool m_bApproachCircle2x;
    bool m_bReverseArrow2x;
    bool m_bHitCircle2x;
    bool m_bIsDefault02x;
    bool m_bIsDefault12x;
    bool m_bIsScore02x;
    bool m_bIsCombo02x;
    bool m_bSpinnerApproachCircle2x;
    bool m_bSpinnerBottom2x;
    bool m_bSpinnerCircle2x;
    bool m_bSpinnerTop2x;
    bool m_bSpinnerMiddle2x;
    bool m_bSpinnerMiddle22x;
    bool m_bSliderScorePoint2x;
    bool m_bSliderStartCircle2x;
    bool m_bSliderEndCircle2x;

    bool m_bCircularmetre2x;

    bool m_bPauseContinue2x;

    bool m_bMenuButtonBackground2x;
    bool m_bStar2x;
    bool m_bRankingPanel2x;
    bool m_bRankingMaxCombo2x;
    bool m_bRankingAccuracy2x;
    bool m_bRankingA2x;
    bool m_bRankingB2x;
    bool m_bRankingC2x;
    bool m_bRankingD2x;
    bool m_bRankingS2x;
    bool m_bRankingSH2x;
    bool m_bRankingX2x;
    bool m_bRankingXH2x;

    // skin.ini
    float m_fVersion;
    float m_fAnimationFramerate;
    bool m_bCursorCenter;
    bool m_bCursorRotate;
    bool m_bCursorExpand;

    bool m_bSliderBallFlip;
    bool m_bAllowSliderBallTint;
    int m_iSliderStyle;
    bool m_bHitCircleOverlayAboveNumber;
    bool m_bSliderTrackOverride;

    std::string m_sComboPrefix;
    int m_iComboOverlap;

    std::string m_sScorePrefix;
    int m_iScoreOverlap;

    std::string m_sHitCirclePrefix;
    int m_iHitCircleOverlap;

    // custom
    int m_iSampleSet;
    int m_iSampleVolume;

    std::vector<std::string> filepathsForRandomSkin;
    bool m_bIsRandom;
    bool m_bIsRandomElements;

    std::vector<std::string> m_filepathsForExport;
};
