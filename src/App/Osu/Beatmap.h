#pragma once
#include "DatabaseBeatmap.h"
#include "Replay.h"
#include "Timer.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"

class Sound;
class ConVar;

class Osu;
class Skin;
class HitObject;

class DatabaseBeatmap;

class BackgroundStarCacheLoader;
class BackgroundStarCalcHandler;

struct Click {
    long tms;
    Vector2 pos;
};

class Beatmap {
   public:
    friend class BackgroundStarCacheLoader;
    friend class BackgroundStarCalcHandler;

    Beatmap(Osu *osu);
    ~Beatmap();

    void draw(Graphics *g);
    void drawDebug(Graphics *g);
    void drawBackground(Graphics *g);

    void update();
    void update2();  // Used to be Beatmap::update()

    void onKeyDown(KeyboardEvent &e);

    // Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects
    long getPVS();

    // this should make all the necessary internal updates to hitobjects when legacy osu mods or static mods change
    // live (but also on start)
    void onModUpdate(bool rebuildSliderVertexBuffers = true, bool recomputeDrainRate = true);

    // Returns true if we're loading or waiting on other players
    bool isLoading();

    // Returns true if the local player is loading
    bool isActuallyLoading();

    Vector2 pixels2OsuCoords(Vector2 pixelCoords) const;  // only used for positional audio atm
    Vector2 osuCoords2Pixels(
        Vector2 coords) const;  // hitobjects should use this one (includes lots of special behaviour)
    Vector2 osuCoords2RawPixels(
        Vector2 coords) const;  // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
    Vector3 osuCoordsTo3D(Vector2 coords, const HitObject *hitObject) const;
    Vector3 osuCoordsToRaw3D(Vector2 coords) const;  // (without any mods whatsoever)
    Vector2 osuCoords2LegacyPixels(
        Vector2 coords) const;  // only applies vanilla osu mods and static mods to the coordinates (used for generating
                                // the static slider mesh) centered at (0, 0, 0)

    // cursor
    Vector2 getMousePos() const;
    Vector2 getCursorPos() const;
    Vector2 getFirstPersonCursorDelta() const;
    inline Vector2 getContinueCursorPoint() const { return m_vContinueCursorPoint; }

    // playfield
    inline Vector2 getPlayfieldSize() const { return m_vPlayfieldSize; }
    inline Vector2 getPlayfieldCenter() const { return m_vPlayfieldCenter; }
    inline float getPlayfieldRotation() const { return m_fPlayfieldRotation; }

    // hitobjects
    float getHitcircleDiameter() const;  // in actual scaled pixels to the current resolution
    inline float getRawHitcircleDiameter() const { return m_fRawHitcircleDiameter; }  // in osu!pixels
    inline float getHitcircleXMultiplier() const {
        return m_fXMultiplier;
    }  // multiply osu!pixels with this to get screen pixels
    inline float getNumberScale() const { return m_fNumberScale; }
    inline float getHitcircleOverlapScale() const { return m_fHitcircleOverlapScale; }
    inline float getSliderFollowCircleDiameter() const { return m_fSliderFollowCircleDiameter; }
    inline float getRawSliderFollowCircleDiameter() const { return m_fRawSliderFollowCircleDiameter; }
    inline bool isInMafhamRenderChunk() const { return m_bInMafhamRenderChunk; }

    // score
    inline int getNumHitObjects() const { return m_hitobjects.size(); }
    inline float getAimStars() const { return m_fAimStars; }
    inline float getAimSliderFactor() const { return m_fAimSliderFactor; }
    inline float getSpeedStars() const { return m_fSpeedStars; }
    inline float getSpeedNotes() const { return m_fSpeedNotes; }

    // hud
    inline bool isSpinnerActive() const { return m_bIsSpinnerActive; }

    // callbacks called by the Osu class (osu!standard)
    void skipEmptySection();
    void keyPressed1(bool mouse);
    void keyPressed2(bool mouse);
    void keyReleased1(bool mouse);
    void keyReleased2(bool mouse);

    // songbrowser & player logic
    void select();  // loads the music of the currently selected diff and starts playing from the previewTime (e.g.
                    // clicking on a beatmap)
    void selectDifficulty2(DatabaseBeatmap *difficulty2);
    void deselect();  // stops + unloads the currently loaded music and deletes all hitobjects
    bool watch(FinishedScore score, double start_percent);
    bool play();
    void restart(bool quick = false);
    void pause(bool quitIfWaiting = true);
    void pausePreviewMusic(bool toggle = true);
    bool isPreviewMusicPlaying();
    void stop(bool quit = true);
    void fail();
    void cancelFailing();
    void resetScore();

    // loader
    void setMaxPossibleCombo(int maxPossibleCombo) { m_iMaxPossibleCombo = maxPossibleCombo; }
    void setScoreV2ComboPortionMaximum(unsigned long long scoreV2ComboPortionMaximum) {
        m_iScoreV2ComboPortionMaximum = scoreV2ComboPortionMaximum;
    }

    // music/sound
    void loadMusic(bool stream = true, bool prescan = false);
    void unloadMusic();
    float getIdealVolume();
    void setSpeed(float speed);
    void seekPercent(double percent);
    void seekPercentPlayable(double percent);

    inline Sound *getMusic() const { return m_music; }
    unsigned long getTime() const;
    unsigned long getStartTimePlayable() const;
    unsigned long getLength() const;
    unsigned long getLengthPlayable() const;
    float getPercentFinished() const;
    float getPercentFinishedPlayable() const;

    // live statistics
    int getMostCommonBPM() const;
    float getSpeedMultiplier() const;
    inline int getNPS() const { return m_iNPS; }
    inline int getND() const { return m_iND; }
    inline int getHitObjectIndexForCurrentTime() const { return m_iCurrentHitObjectIndex; }
    inline int getNumCirclesForCurrentTime() const { return m_iCurrentNumCircles; }
    inline int getNumSlidersForCurrentTime() const { return m_iCurrentNumSliders; }
    inline int getNumSpinnersForCurrentTime() const { return m_iCurrentNumSpinners; }
    inline int getMaxPossibleCombo() const { return m_iMaxPossibleCombo; }
    inline unsigned long long getScoreV2ComboPortionMaximum() const { return m_iScoreV2ComboPortionMaximum; }
    inline double getAimStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {
        return (
            m_aimStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1
                ? m_aimStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_aimStarsForNumHitObjects.size() - 1)]
                : 0);
    }
    inline double getAimSliderFactorForUpToHitObjectIndex(int upToHitObjectIndex) const {
        return (m_aimSliderFactorForNumHitObjects.size() > 0 && upToHitObjectIndex > -1
                    ? m_aimSliderFactorForNumHitObjects[clamp<int>(upToHitObjectIndex, 0,
                                                                   m_aimSliderFactorForNumHitObjects.size() - 1)]
                    : 0);
    }
    inline double getSpeedStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {
        return (m_speedStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1
                    ? m_speedStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0,
                                                              m_speedStarsForNumHitObjects.size() - 1)]
                    : 0);
    }
    inline double getSpeedNotesForUpToHitObjectIndex(int upToHitObjectIndex) const {
        return (m_speedNotesForNumHitObjects.size() > 0 && upToHitObjectIndex > -1
                    ? m_speedNotesForNumHitObjects[clamp<int>(upToHitObjectIndex, 0,
                                                              m_speedNotesForNumHitObjects.size() - 1)]
                    : 0);
    }
    inline const std::vector<double> &getAimStrains() const { return m_aimStrains; }
    inline const std::vector<double> &getSpeedStrains() const { return m_speedStrains; }

    // set to false when using non-vanilla mods (disables score submission)
    bool vanilla = true;

    // replay recording
    void write_frame();
    std::vector<Replay::Frame> live_replay;
    double last_event_time = 0.0;
    long last_event_ms = 0;
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying
    // current_keys, last_keys also reused
    std::vector<Replay::Frame> spectated_replay;
    Vector2 m_interpolatedMousePos;
    bool m_bIsWatchingReplay = false;
    long current_frame_idx = 0;

    // used by HitObject children and ModSelector
    inline Osu *getOsu() const { return m_osu; }
    Skin *getSkin() const;  // maybe use this for beatmap skins, maybe
    inline int getRandomSeed() const { return m_iRandomSeed; }

    inline long getCurMusicPos() const { return m_iCurMusicPos; }
    inline long getCurMusicPosWithOffsets() const { return m_iCurMusicPosWithOffsets; }

    float getRawAR() const;
    float getAR() const;
    float getCS() const;
    float getHP() const;
    float getRawOD() const;
    float getOD() const;

    // health
    inline double getHealth() const { return m_fHealth; }
    inline bool hasFailed() const { return m_bFailed; }
    inline double getHPMultiplierNormal() const { return m_fHpMultiplierNormal; }
    inline double getHPMultiplierComboEnd() const { return m_fHpMultiplierComboEnd; }

    // database (legacy)
    inline DatabaseBeatmap *getSelectedDifficulty2() const { return m_selectedDifficulty2; }

    // generic state
    inline bool isPlaying() const { return m_bIsPlaying; }
    inline bool isPaused() const { return m_bIsPaused; }
    inline bool isRestartScheduled() const { return m_bIsRestartScheduled; }
    inline bool isContinueScheduled() const { return m_bContinueScheduled; }
    inline bool isWaiting() const { return m_bIsWaiting; }
    inline bool isInSkippableSection() const { return m_bIsInSkippableSection; }
    inline bool isInBreak() const { return m_bInBreak; }
    inline bool shouldFlashWarningArrows() const { return m_bShouldFlashWarningArrows; }
    inline float shouldFlashSectionPass() const { return m_fShouldFlashSectionPass; }
    inline float shouldFlashSectionFail() const { return m_fShouldFlashSectionFail; }
    bool isKey1Down();
    bool isKey2Down();
    bool isClickHeld();
    bool isLastKeyDownKey1();

    std::string getTitle() const;
    std::string getArtist() const;

    inline const std::vector<DatabaseBeatmap::BREAK> &getBreaks() const { return m_breaks; }
    unsigned long getBreakDurationTotal() const;
    DatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

    // HitObject and other helper functions
    LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, long delta, bool isEndOfCombo = false,
                                bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
    void addSliderBreak();
    void addScorePoints(int points, bool isSpinner = false);
    void addHealth(double percent, bool isFromHitResult);
    void updateTimingPoints(long curPos);

    // ILLEGAL:
    inline const std::vector<HitObject *> &getHitObjectsPointer() const { return m_hitobjects; }
    inline float getBreakBackgroundFadeAnim() const { return m_fBreakBackgroundFade; }

   protected:
    // internal
    bool canDraw();
    bool canUpdate();

    void actualRestart();

    void handlePreviewPlay();
    void unloadObjects();

    void resetHitObjects(long curPos = 0);

    void playMissSound();

    unsigned long getMusicPositionMSInterpolated();

    Osu *m_osu;

    // beatmap state
    bool m_bIsPlaying;
    bool m_bIsPaused;
    bool m_bIsWaiting;
    bool m_bIsRestartScheduled;
    bool m_bIsRestartScheduledQuick;

    bool m_bIsInSkippableSection;
    bool m_bShouldFlashWarningArrows;
    float m_fShouldFlashSectionPass;
    float m_fShouldFlashSectionFail;
    bool m_bContinueScheduled;
    unsigned long m_iContinueMusicPos;
    float m_fWaitTime;

    // database
    DatabaseBeatmap *m_selectedDifficulty2;

    // sound
    Sound *m_music;
    float m_fMusicFrequencyBackup;
    long m_iCurMusicPos;
    long m_iCurMusicPosWithOffsets;
    bool m_bWasSeekFrame;
    double m_fInterpolatedMusicPos;
    double m_fLastAudioTimeAccurateSet;
    double m_fLastRealTimeForInterpolationDelta;
    int m_iResourceLoadUpdateDelayHack;
    bool m_bForceStreamPlayback;
    float m_fAfterMusicIsFinishedVirtualAudioTimeStart;
    bool m_bIsFirstMissSound;

    // health
    bool m_bFailed;
    float m_fFailAnim;
    double m_fHealth;
    float m_fHealth2;

    // drain
    double m_fDrainRate;
    double m_fHpMultiplierNormal;
    double m_fHpMultiplierComboEnd;

    // breaks
    std::vector<DatabaseBeatmap::BREAK> m_breaks;
    float m_fBreakBackgroundFade;
    bool m_bInBreak;
    HitObject *m_currentHitObject;
    long m_iNextHitObjectTime;
    long m_iPreviousHitObjectTime;
    long m_iPreviousSectionPassFailTime;

    // player input
    bool m_bClick1Held;
    bool m_bClick2Held;
    bool m_bClickedContinue;
    bool m_bPrevKeyWasKey1;
    int m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex;
    std::vector<Click> m_clicks;

    // hitobjects
    std::vector<HitObject *> m_hitobjects;
    std::vector<HitObject *> m_hitobjectsSortedByEndTime;
    std::vector<HitObject *> m_misaimObjects;
    int m_iRandomSeed;

    // statistics
    int m_iNPS;
    int m_iND;
    int m_iCurrentHitObjectIndex;
    int m_iCurrentNumCircles;
    int m_iCurrentNumSliders;
    int m_iCurrentNumSpinners;
    int m_iMaxPossibleCombo;
    unsigned long long m_iScoreV2ComboPortionMaximum;
    std::vector<double> m_aimStarsForNumHitObjects;
    std::vector<double> m_aimSliderFactorForNumHitObjects;
    std::vector<double> m_speedStarsForNumHitObjects;
    std::vector<double> m_speedNotesForNumHitObjects;
    std::vector<double> m_aimStrains;
    std::vector<double> m_speedStrains;

    // custom
    int m_iPreviousFollowPointObjectIndex;  // TODO: this shouldn't be in this class

   private:
    ConVar *m_osu_pvs = nullptr;
    ConVar *m_osu_draw_hitobjects_ref = nullptr;
    ConVar *m_osu_followpoints_prevfadetime_ref = nullptr;
    ConVar *m_osu_universal_offset_ref = nullptr;
    ConVar *m_osu_early_note_time_ref = nullptr;
    ConVar *m_osu_fail_time_ref = nullptr;
    ConVar *m_osu_drain_type_ref = nullptr;
    ConVar *m_osu_draw_hud_ref = nullptr;
    ConVar *m_osu_draw_scorebarbg_ref = nullptr;
    ConVar *m_osu_hud_scorebar_hide_during_breaks_ref = nullptr;
    ConVar *m_osu_drain_stable_hpbar_maximum_ref = nullptr;
    ConVar *m_osu_volume_music_ref = nullptr;
    ConVar *m_osu_mod_fposu_ref = nullptr;
    ConVar *m_fposu_draw_scorebarbg_on_top_ref = nullptr;
    ConVar *m_osu_draw_statistics_pp_ref = nullptr;
    ConVar *m_osu_draw_statistics_livestars_ref = nullptr;
    ConVar *m_osu_mod_fullalternate_ref = nullptr;
    ConVar *m_fposu_distance_ref = nullptr;
    ConVar *m_fposu_curved_ref = nullptr;
    ConVar *m_fposu_mod_strafing_ref = nullptr;
    ConVar *m_fposu_mod_strafing_frequency_x_ref = nullptr;
    ConVar *m_fposu_mod_strafing_frequency_y_ref = nullptr;
    ConVar *m_fposu_mod_strafing_frequency_z_ref = nullptr;
    ConVar *m_fposu_mod_strafing_strength_x_ref = nullptr;
    ConVar *m_fposu_mod_strafing_strength_y_ref = nullptr;
    ConVar *m_fposu_mod_strafing_strength_z_ref = nullptr;
    ConVar *m_fposu_mod_3d_depthwobble_ref = nullptr;
    ConVar *m_osu_slider_scorev2_ref = nullptr;

    static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in) {
        return Vector2(in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f));
    }

    static float quadLerp3f(float left, float center, float right, float percent) {
        if(percent >= 0.5f) {
            percent = (percent - 0.5f) / 0.5f;
            percent *= percent;
            return lerp<float>(center, right, percent);
        } else {
            percent = percent / 0.5f;
            percent = 1.0f - (1.0f - percent) * (1.0f - percent);
            return lerp<float>(left, center, percent);
        }
    }

    void saveAndSubmitScore(bool quit);
    void onPaused(bool first);

    void drawFollowPoints(Graphics *g);
    void drawHitObjects(Graphics *g);

    void updateAutoCursorPos();
    void updatePlayfieldMetrics();
    void updateHitobjectMetrics();
    void updateSliderVertexBuffers();

    void calculateStacks();
    void computeDrainRate();

    void updateStarCache();
    void stopStarCacheLoader();
    bool isLoadingStarCache();

    // beatmap
    bool m_bIsSpinnerActive;
    Vector2 m_vContinueCursorPoint;

    // playfield
    float m_fPlayfieldRotation;
    float m_fScaleFactor;
    Vector2 m_vPlayfieldCenter;
    Vector2 m_vPlayfieldOffset;
    Vector2 m_vPlayfieldSize;

    // hitobject scaling
    float m_fXMultiplier;
    float m_fRawHitcircleDiameter;
    float m_fHitcircleDiameter;
    float m_fNumberScale;
    float m_fHitcircleOverlapScale;
    float m_fSliderFollowCircleDiameter;
    float m_fRawSliderFollowCircleDiameter;

    // auto
    Vector2 m_vAutoCursorPos;
    int m_iAutoCursorDanceIndex;

    // pp calculation buffer (only needs to be recalculated in onModUpdate(), instead of on every hit)
    float m_fAimStars;
    float m_fAimSliderFactor;
    float m_fSpeedStars;
    float m_fSpeedNotes;
    BackgroundStarCacheLoader *m_starCacheLoader;
    float m_fStarCacheTime;

    // dynamic slider vertex buffer and other recalculation checks (for live mod switching)
    float m_fPrevHitCircleDiameter;
    bool m_bWasHorizontalMirrorEnabled;
    bool m_bWasVerticalMirrorEnabled;
    bool m_bWasEZEnabled;
    bool m_bWasMafhamEnabled;
    float m_fPrevPlayfieldRotationFromConVar;
    float m_fPrevPlayfieldStretchX;
    float m_fPrevPlayfieldStretchY;
    float m_fPrevHitCircleDiameterForStarCache;
    float m_fPrevSpeedForStarCache;

    // custom
    bool m_bIsPreLoading;
    int m_iPreLoadingIndex;
    bool m_bWasHREnabled;  // dynamic stack recalculation

    RenderTarget *m_mafhamActiveRenderTarget;
    RenderTarget *m_mafhamFinishedRenderTarget;
    bool m_bMafhamRenderScheduled;
    int m_iMafhamHitObjectRenderIndex;  // scene buffering for rendering entire beatmaps at once with an acceptable
                                        // framerate
    int m_iMafhamPrevHitObjectIndex;
    int m_iMafhamActiveRenderHitObjectIndex;
    int m_iMafhamFinishedRenderHitObjectIndex;
    bool m_bInMafhamRenderChunk;  // used by Slider to not animate the reverse arrow, and by Circle to not animate
                                  // note blocking shaking, while being rendered into the scene buffer

    int m_iMandalaIndex;
};
