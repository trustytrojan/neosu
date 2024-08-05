#pragma once
#include <future>

#include "BeatmapInterface.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "Replay.h"
#include "Timer.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"
#include "uwu.h"

class Sound;
class ConVar;
class Skin;
class HitObject;
class DatabaseBeatmap;

struct Click {
    long tms;
    Vector2 pos;
};

class Beatmap : public BeatmapInterface {
   public:
    Beatmap();
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

    // HACK: Updates buffering state and pauses/unpauses the music!
    bool isBuffering();

    // Returns true if we're loading or waiting on other players
    bool isLoading();

    // Returns true if the local player is loading
    bool isActuallyLoading();

    Vector2 pixels2OsuCoords(Vector2 pixelCoords) const;  // only used for positional audio atm
    Vector2 osuCoords2Pixels(
        Vector2 coords) const;  // hitobjects should use this one (includes lots of special behaviour)
    Vector2 osuCoords2RawPixels(
        Vector2 coords) const;  // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
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
    inline f32 getPlayfieldRotation() const { return m_fPlayfieldRotation; }

    // hitobjects
    inline f32 getHitcircleXMultiplier() const {
        return m_fXMultiplier;
    }  // multiply osu!pixels with this to get screen pixels
    inline f32 getNumberScale() const { return m_fNumberScale; }
    inline f32 getHitcircleOverlapScale() const { return m_fHitcircleOverlapScale; }
    inline bool isInMafhamRenderChunk() const { return m_bInMafhamRenderChunk; }

    // score
    inline int getNumHitObjects() const { return m_hitobjects.size(); }
    inline f32 getAimStars() const { return m_fAimStars; }
    inline f32 getAimSliderFactor() const { return m_fAimSliderFactor; }
    inline f32 getSpeedStars() const { return m_fSpeedStars; }
    inline f32 getSpeedNotes() const { return m_fSpeedNotes; }

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

    bool play();
    bool watch(FinishedScore score, f64 start_percent);
    bool spectate();

    bool start();
    void restart(bool quick = false);
    void pause(bool quitIfWaiting = true);
    void pausePreviewMusic(bool toggle = true);
    bool isPreviewMusicPlaying();
    void stop(bool quit = true);
    void fail();
    void cancelFailing();
    void resetScore();

    // music/sound
    void loadMusic(bool stream = true);
    void unloadMusic();
    f32 getIdealVolume();
    void setSpeed(f32 speed);
    void seekPercent(f64 percent);
    void seekPercentPlayable(f64 percent);

    inline Sound *getMusic() const { return m_music; }
    u32 getTime() const;
    u32 getStartTimePlayable() const;
    u32 getLength() const;
    u32 getLengthPlayable() const;
    f32 getPercentFinished() const;
    f32 getPercentFinishedPlayable() const;

    // live statistics
    int getMostCommonBPM() const;
    f32 getSpeedMultiplier() const;
    inline int getNPS() const { return m_iNPS; }
    inline int getND() const { return m_iND; }
    inline int getHitObjectIndexForCurrentTime() const { return m_iCurrentHitObjectIndex; }
    inline int getNumCirclesForCurrentTime() const { return m_iCurrentNumCircles; }
    inline int getNumSlidersForCurrentTime() const { return m_iCurrentNumSliders; }
    inline int getNumSpinnersForCurrentTime() const { return m_iCurrentNumSpinners; }

    std::vector<f64> m_aimStrains;
    std::vector<f64> m_speedStrains;

    // set to false when using non-vanilla mods (disables score submission)
    bool vanilla = true;

    // replay recording
    void write_frame();
    std::vector<Replay::Frame> live_replay;
    f64 last_event_time = 0.0;
    long last_event_ms = 0;
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying (prerecorded)
    // current_keys, last_keys also reused
    std::vector<Replay::Frame> spectated_replay;
    Vector2 m_interpolatedMousePos;
    bool is_watching = false;
    long current_frame_idx = 0;

    // getting spectated (live)
    void broadcast_spectator_frames();
    std::vector<LiveReplayFrame> frame_batch;
    f64 last_spectator_broadcast = 0;
    u16 spectator_sequence = 0;

    // spectating (live)
    std::vector<ScoreFrame> score_frames;
    bool is_spectating = false;
    bool is_buffering = false;
    i32 last_frame_ms = 0;

    // used by HitObject children and ModSelector
    Skin *getSkin() const;  // maybe use this for beatmap skins, maybe

    inline long getCurMusicPos() const { return m_iCurMusicPos; }
    inline long getCurMusicPosWithOffsets() const { return m_iCurMusicPosWithOffsets; }

    u32 getScoreV1DifficultyMultiplier() const;
    f32 getRawAR() const;
    f32 getAR() const;
    f32 getCS() const;
    f32 getHP() const;
    f32 getRawOD() const;
    f32 getOD() const;
    virtual f32 getApproachTime() const;
    virtual f32 getRawApproachTime() const;

    // health
    inline f64 getHealth() const { return m_fHealth; }
    inline bool hasFailed() const { return m_bFailed; }

    // database (legacy)
    inline DatabaseBeatmap *getSelectedDifficulty2() const { return m_selectedDifficulty2; }

    // generic state
    inline bool isPlaying() const { return m_bIsPlaying; }
    inline bool isPaused() const { return m_bIsPaused; }
    inline bool isRestartScheduled() const { return m_bIsRestartScheduled; }
    inline bool isContinueScheduled() const { return m_bContinueScheduled; }
    inline bool isInSkippableSection() const { return m_bIsInSkippableSection; }
    inline bool isInBreak() const { return m_bInBreak; }
    inline bool shouldFlashWarningArrows() const { return m_bShouldFlashWarningArrows; }
    inline f32 shouldFlashSectionPass() const { return m_fShouldFlashSectionPass; }
    inline f32 shouldFlashSectionFail() const { return m_fShouldFlashSectionFail; }
    virtual bool isWaiting() const { return m_bIsWaiting; }
    virtual bool isKey1Down() const;
    virtual bool isKey2Down() const;
    virtual bool isClickHeld() const;
    virtual i32 getModsLegacy() const;

    std::string getTitle() const;
    std::string getArtist() const;

    inline const std::vector<DatabaseBeatmap::BREAK> &getBreaks() const { return m_breaks; }
    u32 getBreakDurationTotal() const;
    DatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

    // HitObject and other helper functions
    virtual LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
    virtual void addSliderBreak();
    virtual void addScorePoints(int points, bool isSpinner = false);
    void addHealth(f64 percent, bool isFromHitResult);
    void updateTimingPoints(long curPos);

    // ILLEGAL:
    inline const std::vector<HitObject *> &getHitObjectsPointer() const { return m_hitobjects; }
    inline f32 getBreakBackgroundFadeAnim() const { return m_fBreakBackgroundFade; }

    Sound *m_music;
    bool m_bForceStreamPlayback;

    // live pp/stars
    uwu::lazy_promise<std::function<pp_info()>, pp_info> m_ppv2_calc{pp_info{}};
    i32 last_calculated_hitobject = -1;

   protected:
    // internal
    bool canDraw();
    bool canUpdate();

    void actualRestart();

    void handlePreviewPlay();
    void unloadObjects();

    void resetHitObjects(long curPos = 0);

    void playMissSound();

    u32 getMusicPositionMSInterpolated();

    // beatmap state
    bool m_bIsPlaying;
    bool m_bIsPaused;
    bool m_bIsWaiting;
    bool m_bIsRestartScheduled;
    bool m_bIsRestartScheduledQuick;

    bool m_bIsInSkippableSection;
    bool m_bShouldFlashWarningArrows;
    f32 m_fShouldFlashSectionPass;
    f32 m_fShouldFlashSectionFail;
    bool m_bContinueScheduled;
    u32 m_iContinueMusicPos;
    f32 m_fWaitTime;

    // database
    DatabaseBeatmap *m_selectedDifficulty2;

    // sound
    f32 m_fMusicFrequencyBackup;
    long m_iCurMusicPos;
    long m_iCurMusicPosWithOffsets;
    bool m_bWasSeekFrame;
    f64 m_fInterpolatedMusicPos;
    f64 m_fLastAudioTimeAccurateSet;
    f64 m_fLastRealTimeForInterpolationDelta;
    int m_iResourceLoadUpdateDelayHack;
    f32 m_fAfterMusicIsFinishedVirtualAudioTimeStart;
    bool m_bIsFirstMissSound;

    // health
    bool m_bFailed;
    f32 m_fFailAnim;
    f64 m_fHealth;
    f32 m_fHealth2;

    // drain
    f64 m_fDrainRate;

    // breaks
    std::vector<DatabaseBeatmap::BREAK> m_breaks;
    f32 m_fBreakBackgroundFade;
    bool m_bInBreak;
    HitObject *m_currentHitObject;
    long m_iNextHitObjectTime;
    long m_iPreviousHitObjectTime;
    long m_iPreviousSectionPassFailTime;

    // player input
    bool m_bClick1Held;
    bool m_bClick2Held;
    bool m_bClickedContinue;
    int m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex;
    std::vector<Click> m_clicks;

    // hitobjects
    std::vector<HitObject *> m_hitobjects;
    std::vector<HitObject *> m_hitobjectsSortedByEndTime;
    std::vector<HitObject *> m_misaimObjects;

    // statistics
    int m_iNPS;
    int m_iND;
    int m_iCurrentHitObjectIndex;
    int m_iCurrentNumCircles;
    int m_iCurrentNumSliders;
    int m_iCurrentNumSpinners;

    // custom
    int m_iPreviousFollowPointObjectIndex;  // TODO: this shouldn't be in this class

   private:
    ConVar *m_osu_pvs = NULL;
    ConVar *m_osu_draw_hitobjects_ref = NULL;
    ConVar *m_osu_followpoints_prevfadetime_ref = NULL;
    ConVar *m_osu_universal_offset_ref = NULL;
    ConVar *m_osu_early_note_time_ref = NULL;
    ConVar *m_osu_fail_time_ref = NULL;
    ConVar *m_osu_draw_hud_ref = NULL;
    ConVar *m_osu_draw_scorebarbg_ref = NULL;
    ConVar *m_osu_hud_scorebar_hide_during_breaks_ref = NULL;
    ConVar *m_osu_volume_music_ref = NULL;
    ConVar *m_osu_mod_fposu_ref = NULL;
    ConVar *m_fposu_draw_scorebarbg_on_top_ref = NULL;
    ConVar *m_osu_mod_fullalternate_ref = NULL;
    ConVar *m_fposu_distance_ref = NULL;
    ConVar *m_fposu_curved_ref = NULL;
    ConVar *m_fposu_mod_strafing_ref = NULL;
    ConVar *m_fposu_mod_strafing_frequency_x_ref = NULL;
    ConVar *m_fposu_mod_strafing_frequency_y_ref = NULL;
    ConVar *m_fposu_mod_strafing_frequency_z_ref = NULL;
    ConVar *m_fposu_mod_strafing_strength_x_ref = NULL;
    ConVar *m_fposu_mod_strafing_strength_y_ref = NULL;
    ConVar *m_fposu_mod_strafing_strength_z_ref = NULL;

    static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in) {
        return Vector2(in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f));
    }

    static f32 quadLerp3f(f32 left, f32 center, f32 right, f32 percent) {
        if(percent >= 0.5f) {
            percent = (percent - 0.5f) / 0.5f;
            percent *= percent;
            return lerp<f32>(center, right, percent);
        } else {
            percent = percent / 0.5f;
            percent = 1.0f - (1.0f - percent) * (1.0f - percent);
            return lerp<f32>(left, center, percent);
        }
    }

    FinishedScore saveAndSubmitScore(bool quit);

    void drawFollowPoints(Graphics *g);
    void drawHitObjects(Graphics *g);

    void updateAutoCursorPos();
    void updatePlayfieldMetrics();
    void updateHitobjectMetrics();
    void updateSliderVertexBuffers();

    void calculateStacks();
    void computeDrainRate();

    // beatmap
    bool m_bIsSpinnerActive;
    Vector2 m_vContinueCursorPoint;

    // playfield
    f32 m_fPlayfieldRotation;
    f32 m_fScaleFactor;
    Vector2 m_vPlayfieldCenter;
    Vector2 m_vPlayfieldOffset;
    Vector2 m_vPlayfieldSize;

    // hitobject scaling
    f32 m_fXMultiplier;
    f32 m_fNumberScale;
    f32 m_fHitcircleOverlapScale;

    // auto
    Vector2 m_vAutoCursorPos;
    int m_iAutoCursorDanceIndex;

    // live pp/stars
    void resetLiveStarsTasks();

    // pp calculation buffer (only needs to be recalculated in onModUpdate(), instead of on every hit)
    f32 m_fAimStars;
    f32 m_fAimSliderFactor;
    f32 m_fSpeedStars;
    f32 m_fSpeedNotes;

    // dynamic slider vertex buffer and other recalculation checks (for live mod switching)
    f32 m_fPrevHitCircleDiameter;
    bool m_bWasHorizontalMirrorEnabled;
    bool m_bWasVerticalMirrorEnabled;
    bool m_bWasEZEnabled;
    bool m_bWasMafhamEnabled;
    f32 m_fPrevPlayfieldRotationFromConVar;

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
};
