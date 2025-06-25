#pragma once
#include <future>

#include "BeatmapInterface.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "LegacyReplay.h"
#include "Timing.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"
#include "uwu.h"

class Sound;
class ConVar;
class Skin;
class HitObject;
class DatabaseBeatmap;
class SimulatedBeatmap;

struct Click {
    long click_time;
    Vector2 pos;
};

class Beatmap : public BeatmapInterface {
   public:
    Beatmap();
    ~Beatmap() override;

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

    Vector2 pixels2OsuCoords(Vector2 pixelCoords) const override;  // only used for positional audio atm
    Vector2 osuCoords2Pixels(
        Vector2 coords) const override;  // hitobjects should use this one (includes lots of special behaviour)
    Vector2 osuCoords2RawPixels(
        Vector2 coords) const override;  // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
    Vector2 osuCoords2LegacyPixels(
        Vector2 coords) const override;  // only applies vanilla osu mods and static mods to the coordinates (used for generating
                                // the static slider mesh) centered at (0, 0, 0)

    // cursor
    Vector2 getMousePos() const;
    Vector2 getCursorPos() const override;
    Vector2 getFirstPersonCursorDelta() const;
    inline Vector2 getContinueCursorPoint() const { return this->vContinueCursorPoint; }

    // playfield
    inline Vector2 getPlayfieldSize() const { return this->vPlayfieldSize; }
    inline Vector2 getPlayfieldCenter() const { return this->vPlayfieldCenter; }
    inline f32 getPlayfieldRotation() const { return this->fPlayfieldRotation; }

    // hitobjects
    inline f32 getHitcircleXMultiplier() const {
        return this->fXMultiplier;
    }  // multiply osu!pixels with this to get screen pixels
    inline f32 getNumberScale() const { return this->fNumberScale; }
    inline f32 getHitcircleOverlapScale() const { return this->fHitcircleOverlapScale; }
    inline bool isInMafhamRenderChunk() const { return this->bInMafhamRenderChunk; }

    // score
    inline int getNumHitObjects() const { return this->hitobjects.size(); }
    inline f32 getAimStars() const { return this->fAimStars; }
    inline f32 getAimSliderFactor() const { return this->fAimSliderFactor; }
    inline f32 getSpeedStars() const { return this->fSpeedStars; }
    inline f32 getSpeedNotes() const { return this->fSpeedNotes; }

    // hud
    inline bool isSpinnerActive() const { return this->bIsSpinnerActive; }

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
    void fail(bool force_death = false);
    void cancelFailing();
    void resetScore();

    // music/sound
    void loadMusic(bool stream = true);
    void unloadMusic();
    f32 getIdealVolume();
    void setSpeed(f32 speed);
    void seekPercent(f64 percent);
    void seekPercentPlayable(f64 percent);

    inline Sound *getMusic() const { return this->music; }
    u32 getTime() const;
    u32 getStartTimePlayable() const;
    u32 getLength() const override;
    u32 getLengthPlayable() const override;
    f32 getPercentFinished() const;
    f32 getPercentFinishedPlayable() const;

    // live statistics
    int getMostCommonBPM() const;
    f32 getSpeedMultiplier() const override;
    inline int getNPS() const { return this->iNPS; }
    inline int getND() const { return this->iND; }

    std::vector<f64> aimStrains;
    std::vector<f64> speedStrains;

    // set to false when using non-vanilla mods (disables score submission)
    bool vanilla = true;

    // replay recording
    void write_frame();
    std::vector<LegacyReplay::Frame> live_replay;
    f64 last_event_time = 0.0;
    i32 last_event_ms = 0;
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying (prerecorded)
    // current_keys, last_keys also reused
    std::vector<LegacyReplay::Frame> spectated_replay;
    Vector2 interpolatedMousePos;
    bool is_watching = false;
    long current_frame_idx = 0;
    SimulatedBeatmap *sim = NULL;

    // getting spectated (live)
    void broadcast_spectator_frames();
    std::vector<LiveReplayFrame> frame_batch;
    f64 last_spectator_broadcast = 0;
    u16 spectator_sequence = 0;

    // spectating (live)
    std::vector<ScoreFrame> score_frames;
    bool is_buffering = false;
    i32 last_frame_ms = 0;
    bool spectate_pause = false;  // the player we're spectating has paused

    // used by HitObject children and ModSelector
    Skin *getSkin() const;  // maybe use this for beatmap skins, maybe

    inline long getCurMusicPos() const { return this->iCurMusicPos; }
    inline long getCurMusicPosWithOffsets() const { return this->iCurMusicPosWithOffsets; }

    u32 getScoreV1DifficultyMultiplier() const override;
    f32 getRawAR() const override;
    f32 getAR() const override;
    f32 getCS() const override;
    f32 getHP() const override;
    f32 getRawOD() const override;
    f32 getOD() const override;
    f32 getApproachTime() const override;
    f32 getRawApproachTime() const override;

    // health
    inline f64 getHealth() const { return this->fHealth; }
    inline bool hasFailed() const { return this->bFailed; }

    // database (legacy)
    inline DatabaseBeatmap *getSelectedDifficulty2() const { return this->selectedDifficulty2; }

    // generic state
    inline bool isPlaying() const override { return this->bIsPlaying; }
    inline bool isPaused() const override { return this->bIsPaused; }
    inline bool isRestartScheduled() const { return this->bIsRestartScheduled; }
    inline bool isContinueScheduled() const override { return this->bContinueScheduled; }
    inline bool isInSkippableSection() const { return this->bIsInSkippableSection; }
    inline bool isInBreak() const { return this->bInBreak; }
    inline bool shouldFlashWarningArrows() const { return this->bShouldFlashWarningArrows; }
    inline f32 shouldFlashSectionPass() const { return this->fShouldFlashSectionPass; }
    inline f32 shouldFlashSectionFail() const { return this->fShouldFlashSectionFail; }
    bool isWaiting() const override { return this->bIsWaiting; }
    bool isKey1Down() const override;
    bool isKey2Down() const override;
    bool isClickHeld() const override;
    Replay::Mods getMods() const override;
    i32 getModsLegacy() const override;

    std::string getTitle() const;
    std::string getArtist() const;

    inline const std::vector<DatabaseBeatmap::BREAK> &getBreaks() const { return this->breaks; }
    u32 getBreakDurationTotal() const override;
    DatabaseBeatmap::BREAK getBreakForTimeRange(i64 startMS, i64 positionMS, i64 endMS) const;

    // HitObject and other helper functions
    LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false) override;
    void addSliderBreak() override;
    void addScorePoints(int points, bool isSpinner = false) override;
    void addHealth(f64 percent, bool isFromHitResult);
    void updateTimingPoints(long curPos);

    // ILLEGAL:
    inline const std::vector<HitObject *> &getHitObjectsPointer() const { return this->hitobjects; }
    inline f32 getBreakBackgroundFadeAnim() const { return this->fBreakBackgroundFade; }

    Sound *music;
    bool bForceStreamPlayback;

    // live pp/stars
    uwu::lazy_promise<std::function<pp_info()>, pp_info> ppv2_calc{pp_info{}};
    i32 last_calculated_hitobject = -1;
    int iCurrentHitObjectIndex;
    int iCurrentNumCircles;
    int iCurrentNumSliders;
    int iCurrentNumSpinners;

    // beatmap state
    bool bIsPlaying;
    bool bIsPaused;
    bool bIsWaiting;
    bool bIsRestartScheduled;
    bool bIsRestartScheduledQuick;

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

    bool bIsInSkippableSection;
    bool bShouldFlashWarningArrows;
    f32 fShouldFlashSectionPass;
    f32 fShouldFlashSectionFail;
    bool bContinueScheduled;
    u32 iContinueMusicPos;
    f32 fWaitTime;

    // database
    DatabaseBeatmap *selectedDifficulty2;

    // sound
    f32 fMusicFrequencyBackup;
    long iCurMusicPos;
    long iCurMusicPosWithOffsets;
    bool bWasSeekFrame;
    f64 fInterpolatedMusicPos;
    f64 fLastAudioTimeAccurateSet;
    f64 fLastRealTimeForInterpolationDelta;
    int iResourceLoadUpdateDelayHack;
    f32 fAfterMusicIsFinishedVirtualAudioTimeStart;
    bool bIsFirstMissSound;

    // health
    bool bFailed;
    f32 fFailAnim;
    f64 fHealth;
    f32 fHealth2;

    // drain
    f64 fDrainRate;

    // breaks
    std::vector<DatabaseBeatmap::BREAK> breaks;
    f32 fBreakBackgroundFade;
    bool bInBreak;
    HitObject *currentHitObject;
    long iNextHitObjectTime;
    long iPreviousHitObjectTime;
    long iPreviousSectionPassFailTime;

    // player input
    bool bClick1Held;
    bool bClick2Held;
    bool bClickedContinue;
    int iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex;
    std::vector<Click> clicks;

    // hitobjects
    std::vector<HitObject *> hitobjects;
    std::vector<HitObject *> hitobjectsSortedByEndTime;
    std::vector<HitObject *> misaimObjects;

    // statistics
    int iNPS;
    int iND;

    // custom
    int iPreviousFollowPointObjectIndex;  // TODO: this shouldn't be in this class

   private:
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
    bool bIsSpinnerActive;
    Vector2 vContinueCursorPoint;

    // playfield
    f32 fPlayfieldRotation;
    f32 fScaleFactor;
    Vector2 vPlayfieldCenter;
    Vector2 vPlayfieldOffset;
    Vector2 vPlayfieldSize;

    // hitobject scaling
    f32 fXMultiplier;
    f32 fNumberScale;
    f32 fHitcircleOverlapScale;

    // auto
    Vector2 vAutoCursorPos;
    int iAutoCursorDanceIndex;

    // live pp/stars
    void resetLiveStarsTasks();

    // pp calculation buffer (only needs to be recalculated in onModUpdate(), instead of on every hit)
    f32 fAimStars;
    f32 fAimSliderFactor;
    f32 fSpeedStars;
    f32 fSpeedNotes;

    // dynamic slider vertex buffer and other recalculation checks (for live mod switching)
    f32 fPrevHitCircleDiameter;
    bool bWasHorizontalMirrorEnabled;
    bool bWasVerticalMirrorEnabled;
    bool bWasEZEnabled;
    bool bWasMafhamEnabled;
    f32 fPrevPlayfieldRotationFromConVar;

    // custom
    bool bIsPreLoading;
    int iPreLoadingIndex;
    bool bWasHREnabled;  // dynamic stack recalculation

    RenderTarget *mafhamActiveRenderTarget;
    RenderTarget *mafhamFinishedRenderTarget;
    bool bMafhamRenderScheduled;
    int iMafhamHitObjectRenderIndex;  // scene buffering for rendering entire beatmaps at once with an acceptable
                                      // framerate
    int iMafhamPrevHitObjectIndex;
    int iMafhamActiveRenderHitObjectIndex;
    int iMafhamFinishedRenderHitObjectIndex;
    bool bInMafhamRenderChunk;  // used by Slider to not animate the reverse arrow, and by Circle to not animate
                                // note blocking shaking, while being rendered into the scene buffer
};
