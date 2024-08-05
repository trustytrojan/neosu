#pragma once
#include "Beatmap.h"
#include "ConVar.h"
#include "Osu.h"

class GameRules {
   public:
    //********************//
    //  Positional Audio  //
    //********************//

    static float osuCoords2Pan(float x) { return (x / (float)GameRules::OSU_COORD_WIDTH - 0.5f) * 0.8f; }

    //************************//
    //	Hitobject Animations  //
    //************************//

    static ConVar osu_hitobject_hittable_dim;
    static ConVar osu_hitobject_hittable_dim_start_percent;
    static ConVar osu_hitobject_hittable_dim_duration;

    static ConVar osu_hitobject_fade_in_time;

    static ConVar osu_hitobject_fade_out_time;
    static ConVar osu_hitobject_fade_out_time_speed_multiplier_min;

    static ConVar osu_circle_fade_out_scale;

    static ConVar osu_slider_followcircle_fadein_fade_time;
    static ConVar osu_slider_followcircle_fadeout_fade_time;
    static ConVar osu_slider_followcircle_fadein_scale;
    static ConVar osu_slider_followcircle_fadein_scale_time;
    static ConVar osu_slider_followcircle_fadeout_scale;
    static ConVar osu_slider_followcircle_fadeout_scale_time;
    static ConVar osu_slider_followcircle_tick_pulse_time;
    static ConVar osu_slider_followcircle_tick_pulse_scale;

    static ConVar osu_spinner_fade_out_time_multiplier;

    // this scales the fadeout duration with the current speed multiplier
    static float getFadeOutTime(Beatmap *beatmap) {
        const float fade_out_time = osu_hitobject_fade_out_time.getFloat();
        const float multiplier_min = osu_hitobject_fade_out_time_speed_multiplier_min.getFloat();
        return fade_out_time * (1.0f / max(osu->getAnimationSpeedMultiplier(), multiplier_min));
    }

    static inline long getFadeInTime() { return (long)osu_hitobject_fade_in_time.getInt(); }

    //*********************//
    //	Experimental Mods  //
    //*********************//

    static ConVar osu_mod_fps;
    static ConVar osu_mod_no50s;
    static ConVar osu_mod_no100s;
    static ConVar osu_mod_ming3012;
    static ConVar osu_mod_millhioref;
    static ConVar osu_mod_millhioref_multiplier;
    static ConVar osu_mod_mafham;
    static ConVar osu_mod_mafham_render_livesize;
    static ConVar osu_mod_halfwindow;
    static ConVar osu_mod_halfwindow_allow_300s;

    //********************//
    //	Hitobject Timing  //
    //********************//

    static ConVar osu_approachtime_min;
    static ConVar osu_approachtime_mid;
    static ConVar osu_approachtime_max;

    // ignore all mods and overrides
    static inline float getRawMinApproachTime() { return osu_approachtime_min.getFloat(); }
    static inline float getRawMidApproachTime() { return osu_approachtime_mid.getFloat(); }
    static inline float getRawMaxApproachTime() { return osu_approachtime_max.getFloat(); }

    // respect mods and overrides
    static inline float getMinApproachTime() {
        return getRawMinApproachTime() *
               (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
    }
    static inline float getMidApproachTime() {
        return getRawMidApproachTime() *
               (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
    }
    static inline float getMaxApproachTime() {
        return getRawMaxApproachTime() *
               (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
    }

    static inline float getMinHitWindow300() { return 80.f; }
    static inline float getMidHitWindow300() { return 50.f; }
    static inline float getMaxHitWindow300() { return 20.f; }

    static inline float getMinHitWindow100() { return 140.f; }
    static inline float getMidHitWindow100() { return 100.f; }
    static inline float getMaxHitWindow100() { return 60.f; }

    static inline float getMinHitWindow50() { return 200.f; }
    static inline float getMidHitWindow50() { return 150.f; }
    static inline float getMaxHitWindow50() { return 100.f; }

    // AR 5 -> 1200 ms
    static float mapDifficultyRange(float scaledDiff, float min, float mid, float max) {
        if(scaledDiff > 5.0f) return mid + (max - mid) * (scaledDiff - 5.0f) / 5.0f;

        if(scaledDiff < 5.0f) return mid - (mid - min) * (5.0f - scaledDiff) / 5.0f;

        return mid;
    }

    static double mapDifficultyRangeDouble(double scaledDiff, double min, double mid, double max) {
        if(scaledDiff > 5.0) return mid + (max - mid) * (scaledDiff - 5.0) / 5.0;

        if(scaledDiff < 5.0) return mid - (mid - min) * (5.0 - scaledDiff) / 5.0;

        return mid;
    }

    // 1200 ms -> AR 5
    static float mapDifficultyRangeInv(float val, float min, float mid, float max) {
        if(val < mid)  // > 5.0f (inverted)
            return ((val * 5.0f - mid * 5.0f) / (max - mid)) + 5.0f;

        if(val > mid)  // < 5.0f (inverted)
            return 5.0f - ((mid * 5.0f - val * 5.0f) / (mid - min));

        return 5.0f;
    }

    // 1200 ms -> AR 5
    static float getRawApproachRateForSpeedMultiplier(float approachTime,
                                                      float speedMultiplier)  // ignore all mods and overrides
    {
        return mapDifficultyRangeInv(approachTime * (1.0f / speedMultiplier), getRawMinApproachTime(),
                                     getRawMidApproachTime(), getRawMaxApproachTime());
    }
    static float getRawConstantApproachRateForSpeedMultiplier(
        float approachTime,
        float speedMultiplier)  // ignore all mods and overrides, keep AR consistent through speed changes
    {
        return mapDifficultyRangeInv(approachTime * speedMultiplier, getRawMinApproachTime(), getRawMidApproachTime(),
                                     getRawMaxApproachTime());
    }

    // 50 ms -> OD 5
    static float getRawOverallDifficultyForSpeedMultiplier(float hitWindow300,
                                                           float speedMultiplier)  // ignore all mods and overrides
    {
        return mapDifficultyRangeInv(hitWindow300 * (1.0f / speedMultiplier), getMinHitWindow300(),
                                     getMidHitWindow300(), getMaxHitWindow300());
    }
    static float getRawConstantOverallDifficultyForSpeedMultiplier(
        float hitWindow300,
        float speedMultiplier)  // ignore all mods and overrides, keep OD consistent through speed changes
    {
        return mapDifficultyRangeInv(hitWindow300 * speedMultiplier, getMinHitWindow300(), getMidHitWindow300(),
                                     getMaxHitWindow300());
    }

    static float getRawApproachTime(float AR)  // ignore all mods and overrides
    {
        return mapDifficultyRange(AR, getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
    }
    static float getApproachTimeForStacking(float AR) {
        return mapDifficultyRange(AR, getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
    }

    static float getRawHitWindow300(float OD)  // ignore all mods and overrides
    {
        return mapDifficultyRange(OD, getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
    }

    static inline float getHitWindowMiss() { return 400.f; }

    static float getSpinnerSpinsPerSecond(BeatmapInterface *beatmap)  // raw spins required per second
    {
        return mapDifficultyRange(beatmap->getOD(), 3.0f, 5.0f, 7.5f);
    }
    static float getSpinnerRotationsForSpeedMultiplier(BeatmapInterface *beatmap, long spinnerDuration,
                                                       float speedMultiplier) {
        /// return (int)((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)); // actual
        return (int)((((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)) * 0.5f) *
                     (min(1.0f / speedMultiplier, 1.0f)));  // Mc
    }
    static float getSpinnerRotationsForSpeedMultiplier(
        BeatmapInterface *beatmap,
        long spinnerDuration)  // spinner length compensated rotations // respect all mods and overrides
    {
        return getSpinnerRotationsForSpeedMultiplier(beatmap, spinnerDuration, osu->getSpeedMultiplier());
    }

    //*********************//
    //	Hitobject Scaling  //
    //*********************//

    // "Builds of osu! up to 2013-05-04 had the gamefield being rounded down, which caused incorrect radius calculations
    // in widescreen cases. This ratio adjusts to allow for old replays to work post-fix, which in turn increases the
    // lenience for all plays, but by an amount so small it should only be effective in replays."
    static constexpr const float broken_gamefield_rounding_allowance = 1.00041f;

    static float getRawHitCircleDiameter(float CS, bool applyBrokenGamefieldRoundingAllowance = true) {
        return max(0.0f,
                   ((1.0f - 0.7f * (CS - 5.0f) / 5.0f) / 2.0f) * 128.0f *
                       (applyBrokenGamefieldRoundingAllowance
                            ? broken_gamefield_rounding_allowance
                            : 1.0f));  // gives the circle diameter in osu!pixels, goes negative above CS 12.1429
    }

    static float getHitCircleXMultiplier() {
        return getPlayfieldSize().x / OSU_COORD_WIDTH;  // scales osu!pixels to the actual playfield size
    }

    //*************//
    //	Playfield  //
    //*************//

    static ConVar osu_playfield_border_top_percent;
    static ConVar osu_playfield_border_bottom_percent;

    static const int OSU_COORD_WIDTH;
    static const int OSU_COORD_HEIGHT;

    static float getPlayfieldScaleFactor() {
        const int engineScreenWidth = osu->getScreenWidth();
        const int topBorderSize = osu_playfield_border_top_percent.getFloat() * osu->getScreenHeight();
        const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat() * osu->getScreenHeight();
        const int engineScreenHeight = osu->getScreenHeight() - bottomBorderSize - topBorderSize;

        return osu->getScreenWidth() / (float)OSU_COORD_WIDTH > engineScreenHeight / (float)OSU_COORD_HEIGHT
                   ? engineScreenHeight / (float)OSU_COORD_HEIGHT
                   : engineScreenWidth / (float)OSU_COORD_WIDTH;
    }

    static Vector2 getPlayfieldSize() {
        const float scaleFactor = getPlayfieldScaleFactor();

        return Vector2(OSU_COORD_WIDTH * scaleFactor, OSU_COORD_HEIGHT * scaleFactor);
    }

    static Vector2 getPlayfieldOffset() {
        const Vector2 playfieldSize = getPlayfieldSize();
        const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat() * osu->getScreenHeight();
        int playfieldYOffset = (osu->getScreenHeight() / 2.0f - (playfieldSize.y / 2.0f)) - bottomBorderSize;

        if(osu_mod_fps.getBool())
            playfieldYOffset =
                0;  // first person mode doesn't need any offsets, cursor/crosshair should be centered on screen

        return Vector2((osu->getScreenWidth() - playfieldSize.x) / 2.0f,
                       (osu->getScreenHeight() - playfieldSize.y) / 2.0f + playfieldYOffset);
    }

    static Vector2 getPlayfieldCenter() {
        const float scaleFactor = getPlayfieldScaleFactor();
        const Vector2 playfieldOffset = getPlayfieldOffset();

        return Vector2((OSU_COORD_WIDTH / 2) * scaleFactor + playfieldOffset.x,
                       (OSU_COORD_HEIGHT / 2) * scaleFactor + playfieldOffset.y);
    }
};
