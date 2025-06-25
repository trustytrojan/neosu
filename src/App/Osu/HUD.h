#pragma once
#include "OsuScreen.h"

class UIAvatar;
class Beatmap;
struct ScoreboardSlot;

class McFont;
class ConVar;
class Image;
class Shader;
class VertexArrayObject;

class CBaseUIContainer;

struct SCORE_ENTRY {
    UString name;
    i32 entry_id = 0;
    u32 player_id = 0;

    int combo;
    unsigned long long score;
    float accuracy;
    bool dead;
    bool highlight;
};

class HUD : public OsuScreen {
   public:
    HUD();
    ~HUD() override;

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;

    void drawDummy(Graphics *g);

    void drawCursor(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f, bool secondTrail = false,
                    bool updateAndDrawTrail = true);
    void drawCursorTrail(
        Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f,
        bool secondTrail = false);  // NOTE: only use if drawCursor() with updateAndDrawTrail = false (FPoSu)
    void drawCursorRipples(Graphics *g);
    void drawFps(Graphics *g) { this->drawFps(g, this->tempFont, this->fCurFps); }
    void drawHitErrorBar(Graphics *g, Beatmap *beatmap);
    void drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter);
    void drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter,
                             float borderSize);
    void drawLoadingSmall(Graphics *g, UString text);
    void drawBeatmapImportSpinner(Graphics *g);
    void drawScoreNumber(Graphics *g, unsigned long long number, float scale = 1.0f, bool drawLeadingZeroes = false);
    void drawComboNumber(Graphics *g, unsigned long long number, float scale = 1.0f, bool drawLeadingZeroes = false);
    void drawComboSimple(Graphics *g, int combo, float scale = 1.0f);          // used by RankingScreen
    void drawAccuracySimple(Graphics *g, float accuracy, float scale = 1.0f);  // used by RankingScreen
    void drawWarningArrow(Graphics *g, Vector2 pos, bool flipVertically, bool originLeft = true);

    std::vector<SCORE_ENTRY> getCurrentScores();
    void resetScoreboard();
    void updateScoreboard(bool animate);
    void drawFancyScoreboard(Graphics *g);

    void drawScorebarBg(Graphics *g, float alpha, float breakAnim);
    void drawSectionPass(Graphics *g, float alpha);
    void drawSectionFail(Graphics *g, float alpha);

    void animateCombo();
    void addHitError(long delta, bool miss = false, bool misaim = false);
    void addTarget(float delta, float angle);
    void animateInputoverlay(int key, bool down);

    void addCursorRipple(Vector2 pos);
    void animateCursorExpand();
    void animateCursorShrink();
    void animateKiBulge();
    void animateKiExplode();

    void resetHitErrorBar();

    McRect getSkipClickRect();

    void drawSkip(Graphics *g);

    // ILLEGAL:
    inline float getScoreBarBreakAnim() const { return this->fScoreBarBreakAnim; }

    ScoreboardSlot *player_slot = NULL;
    std::vector<ScoreboardSlot *> slots;
    MD5Hash beatmap_md5;

    f32 live_pp = 0.0;
    f32 live_stars = 0.0;

   private:
    struct CURSORTRAIL {
        Vector2 pos;
        float time;
        float alpha;
        float scale;
    };

    struct CURSORRIPPLE {
        Vector2 pos;
        float time;
    };

    struct HITERROR {
        float time;
        long delta;
        bool miss;
        bool misaim;
    };

    struct TARGET {
        float time;
        float delta;
        float angle;
    };

    struct BREAK {
        float startPercent;
        float endPercent;
    };

    void addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty = false);

    void drawCursorInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos,
                       float alphaMultiplier = 1.0f, bool emptyTrailFrame = false, bool updateAndDrawTrail = true);
    void drawCursorRaw(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
    void drawCursorTrailInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp,
                            Vector2 pos, float alphaMultiplier = 1.0f, bool emptyTrailFrame = false);
    void drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos);
    void drawFps(Graphics *g, McFont *font, float fps);
    void drawAccuracy(Graphics *g, float accuracy);
    void drawCombo(Graphics *g, int combo);
    void drawScore(Graphics *g, unsigned long long score);
    void drawHPBar(Graphics *g, double health, float alpha, float breakAnim);

    void drawWarningArrows(Graphics *g, float hitcircleDiameter = 0.0f);
    void drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter = 0.0f);
    void drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss,
                         int ur);
    void drawHitErrorBarInt(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50,
                            float hitWindowMiss);
    void drawHitErrorBarInt2(Graphics *g, Vector2 center, int ur);
    void drawProgressBar(Graphics *g, float percent, bool waiting);
    void drawStatistics(Graphics *g, int misses, int sliderbreaks, int maxPossibleCombo, float liveStars,
                        float totalStars, int bpm, float ar, float cs, float od, float hp, int nps, int nd, int ur,
                        float pp, float ppfc, float hitWindow300, int hitdeltaMin, int hitdeltaMax);
    void drawTargetHeatmap(Graphics *g, float hitcircleDiameter);
    void drawScrubbingTimeline(Graphics *g, unsigned long beatmapTime, unsigned long beatmapLength,
                               unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable,
                               float beatmapPercentFinishedPlayable, const std::vector<BREAK> &breaks);
    void drawInputOverlay(Graphics *g, int numK1, int numK2, int numM1, int numM2);

    void drawStatisticText(Graphics *g, const UString text);

    float getCursorScaleFactor();
    float getCursorTrailScaleFactor();

    float getScoreScale();

    McFont *tempFont;

    // shit code
    float fAccuracyXOffset;
    float fAccuracyYOffset;
    float fScoreHeight;

    float fComboAnim1;
    float fComboAnim2;

    // fps counter
    float fCurFps;
    float fCurFpsSmooth;
    float fFpsUpdate;

    // hit error bar
    std::vector<HITERROR> hiterrors;

    // inputoverlay / key overlay
    float fInputoverlayK1AnimScale;
    float fInputoverlayK2AnimScale;
    float fInputoverlayM1AnimScale;
    float fInputoverlayM2AnimScale;

    float fInputoverlayK1AnimColor;
    float fInputoverlayK2AnimColor;
    float fInputoverlayM1AnimColor;
    float fInputoverlayM2AnimColor;

    // cursor & trail & ripples
    float fCursorExpandAnim;
    std::vector<CURSORTRAIL> cursorTrail;
    std::vector<CURSORTRAIL> cursorTrail2;
    std::vector<CURSORTRAIL> cursorTrailSpectator1;
    std::vector<CURSORTRAIL> cursorTrailSpectator2;
    Shader *cursorTrailShader;
    VertexArrayObject *cursorTrailVAO;
    std::vector<CURSORRIPPLE> cursorRipples;

    // target heatmap
    std::vector<TARGET> targets;

    std::vector<UIAvatar *> avatars;

    // health
    double fHealth;
    float fScoreBarBreakAnim;
    float fKiScaleAnim;
};
