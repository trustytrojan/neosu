#pragma once
// Copyright (c) 2015, PG, All rights reserved.
#include "cbase.h"

class Image;
class Sound;
class Resource;
class ConVar;

class SkinImage;

class Skin {
   public:
    static void unpack(const char *filepath);

    Skin(const UString &name, std::string filepath, bool isDefaultSkin = false);
    virtual ~Skin();

    void update();

    bool isReady();

    void load();
    void loadBeatmapOverride(const std::string &filepath);
    void reloadSounds();

    void setAnimationSpeed(float animationSpeed) { this->animationSpeedMultiplier = animationSpeed; }
    float getAnimationSpeed() { return this->animationSpeedMultiplier; }

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
    inline std::string getName() { return this->sName; }
    inline std::string getFilePath() { return this->sFilePath; }

    // raw
    inline Image *getMissingTexture() { return Skin::m_missingTexture; }

    inline Image *getHitCircle() { return this->hitCircle; }
    inline SkinImage *getHitCircleOverlay2() { return this->hitCircleOverlay2; }
    inline Image *getApproachCircle() { return this->approachCircle; }
    inline Image *getReverseArrow() { return this->reverseArrow; }
    inline SkinImage *getFollowPoint2() { return this->followPoint2; }

    inline Image *getDefault0() { return this->default0; }
    inline Image *getDefault1() { return this->default1; }
    inline Image *getDefault2() { return this->default2; }
    inline Image *getDefault3() { return this->default3; }
    inline Image *getDefault4() { return this->default4; }
    inline Image *getDefault5() { return this->default5; }
    inline Image *getDefault6() { return this->default6; }
    inline Image *getDefault7() { return this->default7; }
    inline Image *getDefault8() { return this->default8; }
    inline Image *getDefault9() { return this->default9; }

    inline Image *getScore0() { return this->score0; }
    inline Image *getScore1() { return this->score1; }
    inline Image *getScore2() { return this->score2; }
    inline Image *getScore3() { return this->score3; }
    inline Image *getScore4() { return this->score4; }
    inline Image *getScore5() { return this->score5; }
    inline Image *getScore6() { return this->score6; }
    inline Image *getScore7() { return this->score7; }
    inline Image *getScore8() { return this->score8; }
    inline Image *getScore9() { return this->score9; }
    inline Image *getScoreX() { return this->scoreX; }
    inline Image *getScorePercent() { return this->scorePercent; }
    inline Image *getScoreDot() { return this->scoreDot; }

    inline Image *getCombo0() { return this->combo0; }
    inline Image *getCombo1() { return this->combo1; }
    inline Image *getCombo2() { return this->combo2; }
    inline Image *getCombo3() { return this->combo3; }
    inline Image *getCombo4() { return this->combo4; }
    inline Image *getCombo5() { return this->combo5; }
    inline Image *getCombo6() { return this->combo6; }
    inline Image *getCombo7() { return this->combo7; }
    inline Image *getCombo8() { return this->combo8; }
    inline Image *getCombo9() { return this->combo9; }
    inline Image *getComboX() { return this->comboX; }

    inline SkinImage *getPlaySkip() { return this->playSkip; }
    inline Image *getPlayWarningArrow() { return this->playWarningArrow; }
    inline SkinImage *getPlayWarningArrow2() { return this->playWarningArrow2; }
    inline Image *getCircularmetre() { return this->circularmetre; }
    inline SkinImage *getScorebarBg() { return this->scorebarBg; }
    inline SkinImage *getScorebarColour() { return this->scorebarColour; }
    inline SkinImage *getScorebarMarker() { return this->scorebarMarker; }
    inline SkinImage *getScorebarKi() { return this->scorebarKi; }
    inline SkinImage *getScorebarKiDanger() { return this->scorebarKiDanger; }
    inline SkinImage *getScorebarKiDanger2() { return this->scorebarKiDanger2; }
    inline SkinImage *getSectionPassImage() { return this->sectionPassImage; }
    inline SkinImage *getSectionFailImage() { return this->sectionFailImage; }
    inline SkinImage *getInputoverlayBackground() { return this->inputoverlayBackground; }
    inline SkinImage *getInputoverlayKey() { return this->inputoverlayKey; }

    inline SkinImage *getHit0() { return this->hit0; }
    inline SkinImage *getHit50() { return this->hit50; }
    inline SkinImage *getHit50g() { return this->hit50g; }
    inline SkinImage *getHit50k() { return this->hit50k; }
    inline SkinImage *getHit100() { return this->hit100; }
    inline SkinImage *getHit100g() { return this->hit100g; }
    inline SkinImage *getHit100k() { return this->hit100k; }
    inline SkinImage *getHit300() { return this->hit300; }
    inline SkinImage *getHit300g() { return this->hit300g; }
    inline SkinImage *getHit300k() { return this->hit300k; }

    inline Image *getParticle50() { return this->particle50; }
    inline Image *getParticle100() { return this->particle100; }
    inline Image *getParticle300() { return this->particle300; }

    inline Image *getSliderGradient() { return this->sliderGradient; }
    inline SkinImage *getSliderb() { return this->sliderb; }
    inline SkinImage *getSliderFollowCircle2() { return this->sliderFollowCircle2; }
    inline Image *getSliderScorePoint() { return this->sliderScorePoint; }
    inline Image *getSliderStartCircle() { return this->sliderStartCircle; }
    inline SkinImage *getSliderStartCircle2() { return this->sliderStartCircle2; }
    inline Image *getSliderStartCircleOverlay() { return this->sliderStartCircleOverlay; }
    inline SkinImage *getSliderStartCircleOverlay2() { return this->sliderStartCircleOverlay2; }
    inline Image *getSliderEndCircle() { return this->sliderEndCircle; }
    inline SkinImage *getSliderEndCircle2() { return this->sliderEndCircle2; }
    inline Image *getSliderEndCircleOverlay() { return this->sliderEndCircleOverlay; }
    inline SkinImage *getSliderEndCircleOverlay2() { return this->sliderEndCircleOverlay2; }

    inline Image *getSpinnerBackground() { return this->spinnerBackground; }
    inline Image *getSpinnerCircle() { return this->spinnerCircle; }
    inline Image *getSpinnerApproachCircle() { return this->spinnerApproachCircle; }
    inline Image *getSpinnerBottom() { return this->spinnerBottom; }
    inline Image *getSpinnerMiddle() { return this->spinnerMiddle; }
    inline Image *getSpinnerMiddle2() { return this->spinnerMiddle2; }
    inline Image *getSpinnerTop() { return this->spinnerTop; }
    inline Image *getSpinnerSpin() { return this->spinnerSpin; }
    inline Image *getSpinnerClear() { return this->spinnerClear; }

    inline Image *getDefaultCursor() { return this->defaultCursor; }
    inline Image *getCursor() { return this->cursor; }
    inline Image *getCursorMiddle() { return this->cursorMiddle; }
    inline Image *getCursorTrail() { return this->cursorTrail; }
    inline Image *getCursorRipple() { return this->cursorRipple; }
    inline Image *getCursorSmoke() { return this->cursorSmoke; }

    inline SkinImage *getSelectionModEasy() { return this->selectionModEasy; }
    inline SkinImage *getSelectionModNoFail() { return this->selectionModNoFail; }
    inline SkinImage *getSelectionModHalfTime() { return this->selectionModHalfTime; }
    inline SkinImage *getSelectionModDayCore() { return this->selectionModDayCore; }
    inline SkinImage *getSelectionModHardRock() { return this->selectionModHardRock; }
    inline SkinImage *getSelectionModSuddenDeath() { return this->selectionModSuddenDeath; }
    inline SkinImage *getSelectionModPerfect() { return this->selectionModPerfect; }
    inline SkinImage *getSelectionModDoubleTime() { return this->selectionModDoubleTime; }
    inline SkinImage *getSelectionModNightCore() { return this->selectionModNightCore; }
    inline SkinImage *getSelectionModHidden() { return this->selectionModHidden; }
    inline SkinImage *getSelectionModFlashlight() { return this->selectionModFlashlight; }
    inline SkinImage *getSelectionModRelax() { return this->selectionModRelax; }
    inline SkinImage *getSelectionModAutopilot() { return this->selectionModAutopilot; }
    inline SkinImage *getSelectionModSpunOut() { return this->selectionModSpunOut; }
    inline SkinImage *getSelectionModAutoplay() { return this->selectionModAutoplay; }
    inline SkinImage *getSelectionModNightmare() { return this->selectionModNightmare; }
    inline SkinImage *getSelectionModTarget() { return this->selectionModTarget; }
    inline SkinImage *getSelectionModScorev2() { return this->selectionModScorev2; }
    inline SkinImage *getSelectionModTD() { return this->selectionModTD; }

    inline Image *getPauseContinue() { return this->pauseContinue; }
    inline Image *getPauseRetry() { return this->pauseRetry; }
    inline Image *getPauseBack() { return this->pauseBack; }
    inline Image *getPauseOverlay() { return this->pauseOverlay; }
    inline Image *getFailBackground() { return this->failBackground; }
    inline Image *getUnpause() { return this->unpause; }

    inline Image *getButtonLeft() { return this->buttonLeft; }
    inline Image *getButtonMiddle() { return this->buttonMiddle; }
    inline Image *getButtonRight() { return this->buttonRight; }
    inline Image *getDefaultButtonLeft() { return this->defaultButtonLeft; }
    inline Image *getDefaultButtonMiddle() { return this->defaultButtonMiddle; }
    inline Image *getDefaultButtonRight() { return this->defaultButtonRight; }
    inline SkinImage *getMenuBack2() { return this->menuBackImg; }

    inline Image *getMenuButtonBackground() { return this->menuButtonBackground; }
    inline SkinImage *getMenuButtonBackground2() { return this->menuButtonBackground2; }
    inline Image *getStar() { return this->star; }
    inline Image *getRankingPanel() { return this->rankingPanel; }
    inline Image *getRankingGraph() { return this->rankingGraph; }
    inline Image *getRankingTitle() { return this->rankingTitle; }
    inline Image *getRankingMaxCombo() { return this->rankingMaxCombo; }
    inline Image *getRankingAccuracy() { return this->rankingAccuracy; }
    inline Image *getRankingA() { return this->rankingA; }
    inline Image *getRankingB() { return this->rankingB; }
    inline Image *getRankingC() { return this->rankingC; }
    inline Image *getRankingD() { return this->rankingD; }
    inline Image *getRankingS() { return this->rankingS; }
    inline Image *getRankingSH() { return this->rankingSH; }
    inline Image *getRankingX() { return this->rankingX; }
    inline Image *getRankingXH() { return this->rankingXH; }
    inline SkinImage *getRankingAsmall() { return this->rankingAsmall; }
    inline SkinImage *getRankingBsmall() { return this->rankingBsmall; }
    inline SkinImage *getRankingCsmall() { return this->rankingCsmall; }
    inline SkinImage *getRankingDsmall() { return this->rankingDsmall; }
    inline SkinImage *getRankingSsmall() { return this->rankingSsmall; }
    inline SkinImage *getRankingSHsmall() { return this->rankingSHsmall; }
    inline SkinImage *getRankingXsmall() { return this->rankingXsmall; }
    inline SkinImage *getRankingXHsmall() { return this->rankingXHsmall; }
    inline SkinImage *getRankingPerfect() { return this->rankingPerfect; }

    inline Image *getBeatmapImportSpinner() { return this->beatmapImportSpinner; }
    inline Image *getLoadingSpinner() { return this->loadingSpinner; }
    inline Image *getCircleEmpty() { return this->circleEmpty; }
    inline Image *getCircleFull() { return this->circleFull; }
    inline Image *getSeekTriangle() { return this->seekTriangle; }
    inline Image *getUserIcon() { return this->userIcon; }
    inline Image *getBackgroundCube() { return this->backgroundCube; }
    inline Image *getMenuBackground() { return this->menuBackground; }
    inline Image *getSkybox() { return this->skybox; }

    inline Sound *getSpinnerBonus() { return this->spinnerBonus; }
    inline Sound *getSpinnerSpinSound() { return this->spinnerSpinSound; }

    inline Sound *getCombobreak() { return this->combobreak; }
    inline Sound *getFailsound() { return this->failsound; }
    inline Sound *getApplause() { return this->applause; }
    inline Sound *getMenuHit() { return this->menuHit; }
    inline Sound *getMenuHover() { return this->menuHover; }
    inline Sound *getCheckOn() { return this->checkOn; }
    inline Sound *getCheckOff() { return this->checkOff; }
    inline Sound *getShutter() { return this->shutter; }
    inline Sound *getSectionPassSound() { return this->sectionPassSound; }
    inline Sound *getSectionFailSound() { return this->sectionFailSound; }

    inline bool isCursor2x() { return this->bCursor2x; }
    inline bool isCursorTrail2x() { return this->bCursorTrail2x; }
    inline bool isCursorRipple2x() { return this->bCursorRipple2x; }
    inline bool isCursorSmoke2x() { return this->bCursorSmoke2x; }
    inline bool isApproachCircle2x() { return this->bApproachCircle2x; }
    inline bool isReverseArrow2x() { return this->bReverseArrow2x; }
    inline bool isHitCircle2x() { return this->bHitCircle2x; }
    inline bool isDefault02x() { return this->bIsDefault02x; }
    inline bool isDefault12x() { return this->bIsDefault12x; }
    inline bool isScore02x() { return this->bIsScore02x; }
    inline bool isCombo02x() { return this->bIsCombo02x; }
    inline bool isSpinnerApproachCircle2x() { return this->bSpinnerApproachCircle2x; }
    inline bool isSpinnerBottom2x() { return this->bSpinnerBottom2x; }
    inline bool isSpinnerCircle2x() { return this->bSpinnerCircle2x; }
    inline bool isSpinnerTop2x() { return this->bSpinnerTop2x; }
    inline bool isSpinnerMiddle2x() { return this->bSpinnerMiddle2x; }
    inline bool isSpinnerMiddle22x() { return this->bSpinnerMiddle22x; }
    inline bool isSliderScorePoint2x() { return this->bSliderScorePoint2x; }
    inline bool isSliderStartCircle2x() { return this->bSliderStartCircle2x; }
    inline bool isSliderEndCircle2x() { return this->bSliderEndCircle2x; }

    inline bool isCircularmetre2x() { return this->bCircularmetre2x; }

    inline bool isPauseContinue2x() { return this->bPauseContinue2x; }

    inline bool isMenuButtonBackground2x() { return this->bMenuButtonBackground2x; }
    inline bool isStar2x() { return this->bStar2x; }
    inline bool isRankingPanel2x() { return this->bRankingPanel2x; }
    inline bool isRankingMaxCombo2x() { return this->bRankingMaxCombo2x; }
    inline bool isRankingAccuracy2x() { return this->bRankingAccuracy2x; }
    inline bool isRankingA2x() { return this->bRankingA2x; }
    inline bool isRankingB2x() { return this->bRankingB2x; }
    inline bool isRankingC2x() { return this->bRankingC2x; }
    inline bool isRankingD2x() { return this->bRankingD2x; }
    inline bool isRankingS2x() { return this->bRankingS2x; }
    inline bool isRankingSH2x() { return this->bRankingSH2x; }
    inline bool isRankingX2x() { return this->bRankingX2x; }
    inline bool isRankingXH2x() { return this->bRankingXH2x; }

    // skin.ini
    inline float getVersion() { return this->fVersion; }
    inline float getAnimationFramerate() { return this->fAnimationFramerate; }
    Color getComboColorForCounter(int i, int offset);
    void setBeatmapComboColors(std::vector<Color> colors);
    inline Color getSpinnerApproachCircleColor() { return this->spinnerApproachCircleColor; }
    inline Color getSliderBorderColor() { return this->sliderBorderColor; }
    inline Color getSliderTrackOverride() { return this->sliderTrackOverride; }
    inline Color getSliderBallColor() { return this->sliderBallColor; }

    inline Color getSongSelectActiveText() { return this->songSelectActiveText; }
    inline Color getSongSelectInactiveText() { return this->songSelectInactiveText; }

    inline Color getInputOverlayText() { return this->inputOverlayText; }

    inline bool getCursorCenter() { return this->bCursorCenter; }
    inline bool getCursorRotate() { return this->bCursorRotate; }
    inline bool getCursorExpand() { return this->bCursorExpand; }

    inline bool getSliderBallFlip() { return this->bSliderBallFlip; }
    inline bool getAllowSliderBallTint() { return this->bAllowSliderBallTint; }
    inline int getSliderStyle() { return this->iSliderStyle; }
    inline bool getHitCircleOverlayAboveNumber() { return this->bHitCircleOverlayAboveNumber; }
    inline bool isSliderTrackOverridden() { return this->bSliderTrackOverride; }

    inline std::string getComboPrefix() { return this->sComboPrefix; }
    inline int getComboOverlap() { return this->iComboOverlap; }

    inline std::string getScorePrefix() { return this->sScorePrefix; }
    inline int getScoreOverlap() { return this->iScoreOverlap; }

    inline std::string getHitCirclePrefix() { return this->sHitCirclePrefix; }
    inline int getHitCircleOverlap() { return this->iHitCircleOverlap; }

    // custom
    [[nodiscard]] inline bool useSmoothCursorTrail() const { return this->cursorMiddle != m_missingTexture; }
    [[nodiscard]] inline bool isDefaultSkin() const { return this->bIsDefaultSkin; }
    [[nodiscard]] inline int getSampleSet() const { return this->iSampleSet; }

    static Image *m_missingTexture;

    struct SOUND_SAMPLE {
        Sound *sound;
        float hardcodedVolumeMultiplier;  // some samples in osu have hardcoded multipliers which can not be modified
                                          // (i.e. you can NEVER reach 100% volume with them)
    };

    bool parseSkinINI(std::string filepath);

    bool compareFilenameWithSkinElementName(const std::string &filename, const std::string &skinElementName);

    SkinImage *createSkinImage(const std::string &skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
                               bool ignoreDefaultSkin = false, const std::string &animationSeparator = "-");
    void checkLoadImage(Image **addressOfPointer, const std::string &skinElementName, const std::string &resourceName,
                        bool ignoreDefaultSkin = false, const std::string &fileExtension = "png",
                        bool forceLoadMipmaps = false);

    void checkLoadSound(Sound **addressOfPointer, const std::string &skinElementName, std::string resourceName,
                        bool isOverlayable = false, bool isSample = false, bool loop = false,
                        bool fallback_to_default = true, float hardcodedVolumeMultiplier = -1.0f);

    void onEffectVolumeChange(const UString &oldValue, const UString &newValue);
    void onIgnoreBeatmapSampleVolumeChange();

    bool bReady;
    bool bIsDefaultSkin;
    f32 animationSpeedMultiplier = 1.f;
    std::string sName;
    std::string sFilePath;
    std::string sSkinIniFilePath;
    std::vector<Resource *> resources;
    std::vector<Sound *> sounds;
    std::vector<SOUND_SAMPLE> soundSamples;
    std::vector<SkinImage *> images;

    // images
    Image *hitCircle;
    SkinImage *hitCircleOverlay2;
    Image *approachCircle;
    Image *reverseArrow;
    SkinImage *followPoint2;

    Image *default0;
    Image *default1;
    Image *default2;
    Image *default3;
    Image *default4;
    Image *default5;
    Image *default6;
    Image *default7;
    Image *default8;
    Image *default9;

    Image *score0;
    Image *score1;
    Image *score2;
    Image *score3;
    Image *score4;
    Image *score5;
    Image *score6;
    Image *score7;
    Image *score8;
    Image *score9;
    Image *scoreX;
    Image *scorePercent;
    Image *scoreDot;

    Image *combo0;
    Image *combo1;
    Image *combo2;
    Image *combo3;
    Image *combo4;
    Image *combo5;
    Image *combo6;
    Image *combo7;
    Image *combo8;
    Image *combo9;
    Image *comboX;

    SkinImage *playSkip;
    Image *playWarningArrow;
    SkinImage *playWarningArrow2;
    Image *circularmetre;
    SkinImage *scorebarBg;
    SkinImage *scorebarColour;
    SkinImage *scorebarMarker;
    SkinImage *scorebarKi;
    SkinImage *scorebarKiDanger;
    SkinImage *scorebarKiDanger2;
    SkinImage *sectionPassImage;
    SkinImage *sectionFailImage;
    SkinImage *inputoverlayBackground;
    SkinImage *inputoverlayKey;

    SkinImage *hit0;
    SkinImage *hit50;
    SkinImage *hit50g;
    SkinImage *hit50k;
    SkinImage *hit100;
    SkinImage *hit100g;
    SkinImage *hit100k;
    SkinImage *hit300;
    SkinImage *hit300g;
    SkinImage *hit300k;

    Image *particle50;
    Image *particle100;
    Image *particle300;

    Image *sliderGradient;
    SkinImage *sliderb;
    SkinImage *sliderFollowCircle2;
    Image *sliderScorePoint;
    Image *sliderStartCircle;
    SkinImage *sliderStartCircle2;
    Image *sliderStartCircleOverlay;
    SkinImage *sliderStartCircleOverlay2;
    Image *sliderEndCircle;
    SkinImage *sliderEndCircle2;
    Image *sliderEndCircleOverlay;
    SkinImage *sliderEndCircleOverlay2;

    Image *spinnerBackground;
    Image *spinnerCircle;
    Image *spinnerApproachCircle;
    Image *spinnerBottom;
    Image *spinnerMiddle;
    Image *spinnerMiddle2;
    Image *spinnerTop;
    Image *spinnerSpin;
    Image *spinnerClear;

    Image *defaultCursor;
    Image *cursor;
    Image *cursorMiddle;
    Image *cursorTrail;
    Image *cursorRipple;
    Image *cursorSmoke;

    SkinImage *selectionModEasy;
    SkinImage *selectionModNoFail;
    SkinImage *selectionModHalfTime;
    SkinImage *selectionModDayCore;
    SkinImage *selectionModHardRock;
    SkinImage *selectionModSuddenDeath;
    SkinImage *selectionModPerfect;
    SkinImage *selectionModDoubleTime;
    SkinImage *selectionModNightCore;
    SkinImage *selectionModHidden;
    SkinImage *selectionModFlashlight;
    SkinImage *selectionModRelax;
    SkinImage *selectionModAutopilot;
    SkinImage *selectionModSpunOut;
    SkinImage *selectionModAutoplay;
    SkinImage *selectionModNightmare;
    SkinImage *selectionModTarget;
    SkinImage *selectionModScorev2;
    SkinImage *selectionModTD;
    SkinImage *selectionModCinema;

    SkinImage *mode_osu;
    SkinImage *mode_osu_small;

    Image *pauseContinue;
    Image *pauseReplay;
    Image *pauseRetry;
    Image *pauseBack;
    Image *pauseOverlay;
    Image *failBackground;
    Image *unpause;

    Image *buttonLeft;
    Image *buttonMiddle;
    Image *buttonRight;
    Image *defaultButtonLeft;
    Image *defaultButtonMiddle;
    Image *defaultButtonRight;
    SkinImage *menuBackImg;
    SkinImage *selectionMode;
    SkinImage *selectionModeOver;
    SkinImage *selectionMods;
    SkinImage *selectionModsOver;
    SkinImage *selectionRandom;
    SkinImage *selectionRandomOver;
    SkinImage *selectionOptions;
    SkinImage *selectionOptionsOver;

    Image *songSelectTop;
    Image *songSelectBottom;
    Image *menuButtonBackground;
    SkinImage *menuButtonBackground2;
    Image *star;
    Image *rankingPanel;
    Image *rankingGraph;
    Image *rankingTitle;
    Image *rankingMaxCombo;
    Image *rankingAccuracy;
    Image *rankingA;
    Image *rankingB;
    Image *rankingC;
    Image *rankingD;
    Image *rankingS;
    Image *rankingSH;
    Image *rankingX;
    Image *rankingXH;
    SkinImage *rankingAsmall;
    SkinImage *rankingBsmall;
    SkinImage *rankingCsmall;
    SkinImage *rankingDsmall;
    SkinImage *rankingSsmall;
    SkinImage *rankingSHsmall;
    SkinImage *rankingXsmall;
    SkinImage *rankingXHsmall;
    SkinImage *rankingPerfect;

    Image *beatmapImportSpinner;
    Image *loadingSpinner;
    Image *circleEmpty;
    Image *circleFull;
    Image *seekTriangle;
    Image *userIcon;
    Image *backgroundCube;
    Image *menuBackground;
    Image *skybox;

    // sounds
    Sound *normalHitNormal;
    Sound *normalHitWhistle;
    Sound *normalHitFinish;
    Sound *normalHitClap;

    Sound *normalSliderTick;
    Sound *normalSliderSlide;
    Sound *normalSliderWhistle;

    Sound *softHitNormal;
    Sound *softHitWhistle;
    Sound *softHitFinish;
    Sound *softHitClap;

    Sound *softSliderTick;
    Sound *softSliderSlide;
    Sound *softSliderWhistle;

    Sound *drumHitNormal;
    Sound *drumHitWhistle;
    Sound *drumHitFinish;
    Sound *drumHitClap;

    Sound *drumSliderTick;
    Sound *drumSliderSlide;
    Sound *drumSliderWhistle;

    Sound *spinnerBonus;
    Sound *spinnerSpinSound;

    // Plays when sending a message in chat
    Sound *messageSent = nullptr;

    // Plays when deleting text in a message in chat
    Sound *deletingText = nullptr;

    // Plays when changing the text cursor position
    Sound *movingTextCursor = nullptr;

    // Plays when pressing a key for chat, search, edit, etc
    Sound *typing1 = nullptr;
    Sound *typing2 = nullptr;
    Sound *typing3 = nullptr;
    Sound *typing4 = nullptr;

    // Plays when returning to the previous screen
    Sound *menuBack = nullptr;

    // Plays when closing a chat tab
    Sound *closeChatTab = nullptr;

    // Plays when hovering above all selectable boxes except beatmaps or main screen buttons
    Sound *hoverButton = nullptr;

    // Plays when clicking to confirm a button or dropdown option, opening or
    // closing chat, switching between chat tabs, or switching groups
    Sound *clickButton = nullptr;

    // Main menu sounds
    Sound *clickMainMenuCube = nullptr;
    Sound *hoverMainMenuCube = nullptr;
    Sound *clickSingleplayer = nullptr;
    Sound *hoverSingleplayer = nullptr;
    Sound *clickMultiplayer = nullptr;
    Sound *hoverMultiplayer = nullptr;
    Sound *clickOptions = nullptr;
    Sound *hoverOptions = nullptr;
    Sound *clickExit = nullptr;
    Sound *hoverExit = nullptr;

    // Pause menu sounds
    Sound *pauseLoop = nullptr;
    Sound *pauseHover = nullptr;
    Sound *clickPauseBack = nullptr;
    Sound *hoverPauseBack = nullptr;
    Sound *clickPauseContinue = nullptr;
    Sound *hoverPauseContinue = nullptr;
    Sound *clickPauseRetry = nullptr;
    Sound *hoverPauseRetry = nullptr;

    // Back button sounds
    Sound *backButtonClick = nullptr;
    Sound *backButtonHover = nullptr;

    // Plays when switching into song selection, selecting a beatmap, opening dropdown boxes, opening chat tabs
    Sound *expand = nullptr;

    // Plays when selecting a difficulty of a beatmap
    Sound *selectDifficulty = nullptr;

    // Plays when changing the options via a slider
    Sound *sliderbar = nullptr;

    // Multiplayer sounds
    Sound *matchConfirm = nullptr;  // all players are ready
    Sound *roomJoined = nullptr;    // a player joined
    Sound *roomQuit = nullptr;      // a player left
    Sound *roomNotReady = nullptr;  // a player is no longer ready
    Sound *roomReady = nullptr;     // a player is now ready
    Sound *matchStart = nullptr;    // match started

    Sound *combobreak;
    Sound *failsound;
    Sound *applause;
    Sound *menuHit;
    Sound *menuHover;
    Sound *checkOn;
    Sound *checkOff;
    Sound *shutter;
    Sound *sectionPassSound;
    Sound *sectionFailSound;

    // colors
    std::vector<Color> comboColors;
    std::vector<Color> beatmapComboColors;
    Color spinnerApproachCircleColor;
    Color sliderBorderColor;
    Color sliderTrackOverride;
    Color sliderBallColor;

    Color songSelectInactiveText;
    Color songSelectActiveText;

    Color inputOverlayText;

    // scaling
    bool bCursor2x;
    bool bCursorTrail2x;
    bool bCursorRipple2x;
    bool bCursorSmoke2x;
    bool bApproachCircle2x;
    bool bReverseArrow2x;
    bool bHitCircle2x;
    bool bIsDefault02x;
    bool bIsDefault12x;
    bool bIsScore02x;
    bool bIsCombo02x;
    bool bSpinnerApproachCircle2x;
    bool bSpinnerBottom2x;
    bool bSpinnerCircle2x;
    bool bSpinnerTop2x;
    bool bSpinnerMiddle2x;
    bool bSpinnerMiddle22x;
    bool bSliderScorePoint2x;
    bool bSliderStartCircle2x;
    bool bSliderEndCircle2x;

    bool bCircularmetre2x;

    bool bPauseContinue2x;

    bool bMenuButtonBackground2x;
    bool bStar2x;
    bool bRankingPanel2x;
    bool bRankingMaxCombo2x;
    bool bRankingAccuracy2x;
    bool bRankingA2x;
    bool bRankingB2x;
    bool bRankingC2x;
    bool bRankingD2x;
    bool bRankingS2x;
    bool bRankingSH2x;
    bool bRankingX2x;
    bool bRankingXH2x;

    // skin.ini
    float fVersion;
    float fAnimationFramerate;
    bool bCursorCenter;
    bool bCursorRotate;
    bool bCursorExpand;

    bool bSliderBallFlip;
    bool bAllowSliderBallTint;
    int iSliderStyle;
    bool bHitCircleOverlayAboveNumber;
    bool bSliderTrackOverride;

    std::string sComboPrefix;
    int iComboOverlap;

    std::string sScorePrefix;
    int iScoreOverlap;

    std::string sHitCirclePrefix;
    int iHitCircleOverlap;

    // custom
    int iSampleSet;
    int iSampleVolume;

    std::vector<std::string> filepathsForRandomSkin;
    bool bIsRandom;
    bool bIsRandomElements;

    std::vector<std::string> filepathsForExport;
};
