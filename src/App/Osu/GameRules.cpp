//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty & playfield behaviour
//
// $NoKeywords: $osugr
//===============================================================================//

#include "GameRules.h"

ConVar GameRules::osu_playfield_border_top_percent("osu_playfield_border_top_percent", 0.117f, FCVAR_DEFAULT);
ConVar GameRules::osu_playfield_border_bottom_percent("osu_playfield_border_bottom_percent", 0.0834f, FCVAR_DEFAULT);

ConVar GameRules::osu_hitobject_hittable_dim(
    "osu_hitobject_hittable_dim", true, FCVAR_DEFAULT,
    "whether to dim objects not yet within the miss-range (when they can't even be missed yet)");
ConVar GameRules::osu_hitobject_hittable_dim_start_percent(
    "osu_hitobject_hittable_dim_start_percent", 0.7647f, FCVAR_DEFAULT,
    "dimmed objects start at this brightness value before becoming fullbright (only RGB, this does not affect "
    "alpha/transparency)");
ConVar GameRules::osu_hitobject_hittable_dim_duration("osu_hitobject_hittable_dim_duration", 100, FCVAR_DEFAULT,
                                                      "in milliseconds (!)");

ConVar GameRules::osu_hitobject_fade_in_time("osu_hitobject_fade_in_time", 400, FCVAR_LOCKED, "in milliseconds (!)");
ConVar GameRules::osu_hitobject_fade_out_time("osu_hitobject_fade_out_time", 0.293f, FCVAR_LOCKED, "in seconds (!)");
ConVar GameRules::osu_hitobject_fade_out_time_speed_multiplier_min(
    "osu_hitobject_fade_out_time_speed_multiplier_min", 0.5f, FCVAR_LOCKED,
    "The minimum multiplication factor allowed for the speed multiplier influencing the fadeout duration");

ConVar GameRules::osu_circle_fade_out_scale("osu_circle_fade_out_scale", 0.4f, FCVAR_LOCKED);

ConVar GameRules::osu_slider_followcircle_fadein_fade_time("osu_slider_followcircle_fadein_fade_time", 0.06f,
                                                           FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_fadeout_fade_time("osu_slider_followcircle_fadeout_fade_time", 0.25f,
                                                            FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_fadein_scale("osu_slider_followcircle_fadein_scale", 0.5f, FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_fadein_scale_time("osu_slider_followcircle_fadein_scale_time", 0.18f,
                                                            FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_fadeout_scale("osu_slider_followcircle_fadeout_scale", 0.8f, FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_fadeout_scale_time("osu_slider_followcircle_fadeout_scale_time", 0.25f,
                                                             FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_tick_pulse_time("osu_slider_followcircle_tick_pulse_time", 0.2f,
                                                          FCVAR_DEFAULT);
ConVar GameRules::osu_slider_followcircle_tick_pulse_scale("osu_slider_followcircle_tick_pulse_scale", 0.1f,
                                                           FCVAR_DEFAULT);

ConVar GameRules::osu_spinner_fade_out_time_multiplier("osu_spinner_fade_out_time_multiplier", 0.7f, FCVAR_LOCKED);

ConVar GameRules::osu_slider_followcircle_size_multiplier("osu_slider_followcircle_size_multiplier", 2.4f,
                                                          FCVAR_LOCKED);

ConVar GameRules::osu_mod_fps("osu_mod_fps", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_no50s("osu_mod_no50s", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_no100s("osu_mod_no100s", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_ming3012("osu_mod_ming3012", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_millhioref("osu_mod_millhioref", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_millhioref_multiplier("osu_mod_millhioref_multiplier", 2.0f, FCVAR_LOCKED);
ConVar GameRules::osu_mod_mafham("osu_mod_mafham", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_mafham_render_livesize(
    "osu_mod_mafham_render_livesize", 25, FCVAR_DEFAULT,
    "render this many hitobjects without any scene buffering, higher = more lag but more up-to-date scene");
ConVar GameRules::osu_stacking_ar_override("osu_stacking_ar_override", -1, FCVAR_LOCKED,
                                           "allows overriding the approach time used for the stacking calculations. "
                                           "behaves as if disabled if the value is less than 0.");
ConVar GameRules::osu_mod_halfwindow("osu_mod_halfwindow", false, FCVAR_UNLOCKED);
ConVar GameRules::osu_mod_halfwindow_allow_300s("osu_mod_halfwindow_allow_300s", true, FCVAR_DEFAULT,
                                                "should positive hit deltas be allowed within 300 range");

// all values here are in milliseconds
ConVar GameRules::osu_approachtime_min("osu_approachtime_min", 1800, FCVAR_LOCKED);
ConVar GameRules::osu_approachtime_mid("osu_approachtime_mid", 1200, FCVAR_LOCKED);
ConVar GameRules::osu_approachtime_max("osu_approachtime_max", 450, FCVAR_LOCKED);

ConVar GameRules::osu_hitwindow_300_min("osu_hitwindow_300_min", 80, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_300_mid("osu_hitwindow_300_mid", 50, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_300_max("osu_hitwindow_300_max", 20, FCVAR_LOCKED);

ConVar GameRules::osu_hitwindow_100_min("osu_hitwindow_100_min", 140, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_100_mid("osu_hitwindow_100_mid", 100, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_100_max("osu_hitwindow_100_max", 60, FCVAR_LOCKED);

ConVar GameRules::osu_hitwindow_50_min("osu_hitwindow_50_min", 200, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_50_mid("osu_hitwindow_50_mid", 150, FCVAR_LOCKED);
ConVar GameRules::osu_hitwindow_50_max("osu_hitwindow_50_max", 100, FCVAR_LOCKED);

ConVar GameRules::osu_hitwindow_miss("osu_hitwindow_miss", 400, FCVAR_LOCKED);
