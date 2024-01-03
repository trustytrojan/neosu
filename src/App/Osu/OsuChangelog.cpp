//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		changelog screen
//
// $NoKeywords: $osulog
//===============================================================================//

#include "OsuChangelog.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"

OsuChangelog::OsuChangelog(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(-1, -1, 0, 0, "");
	m_scrollView = new CBaseUIScrollView(-1, -1, 0, 0, "");
	m_scrollView->setVerticalScrolling(true);
	m_scrollView->setHorizontalScrolling(true);
	m_scrollView->setDrawBackground(false);
	m_scrollView->setDrawFrame(false);
	m_scrollView->setScrollResistance(0);
	m_container->addBaseUIElement(m_scrollView);

	std::vector<CHANGELOG> changelogs;

	CHANGELOG alpha317;
	// TODO @kiwec: update changelog
	alpha317.title = UString::format("34.00 (Build Date: %s, %s)", __DATE__, __TIME__); // (09.01.2022 - ?)
	alpha317.changes.push_back("- Reenabled IME support to fix blocking keyboard language switching hotkeys (add \"-noime\" launch arg to get the old behavior back in case of problems)");
	alpha317.changes.push_back("- Improved console autocomplete");
	alpha317.changes.push_back("- Fixed pie progressbar fill being invisible under certain conditions");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added ConVars: osu_mod_random_seed, osu_hud_statistics_*_offset_x/y, osu_slider_max_ticks");
	alpha317.changes.push_back("- Updated mod selection screen to show rng seed when hovering over enabled \"Random\" experimental mod checkbox");
	alpha317.changes.push_back("- Fixed another set of star calc crashes on stupid aspire beatmaps (lowered slider tick limit, no timingpoints)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added option \"[Beta] RawInputBuffer\" (Options > Input > Mouse)");
	alpha317.changes.push_back("- Added ConVars: osu_background_color_r/g/b");
	alpha317.changes.push_back("- Linux: Fixed major executable corruption on newer distros (Ubuntu 23+) caused by gold linker (all files written were corrupt, e.g. scores.db/osu.cfg, also segfaults etc.)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Fixed visual vs scoring slider end check in new lazer star calc (@Khangaroo)");
	alpha317.changes.push_back("- Fixed star calc stacking sometimes being calculated inaccurately");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Updated star + pp algorithms to match current lazer implementation aka 20220902 (16) (thanks to @Khangaroo!)");
	alpha317.changes.push_back("- Added option \"Disable osu!lazer star/pp algorithm nerfs for relax/autopilot\" (Options > General > Player)");
	alpha317.changes.push_back("- Fixed extremely rare AMD OpenGL driver crash when slider preview in options menu comes into view (via workaround)");
	alpha317.changes.push_back("- Added ConVars: osu_options_slider_preview_use_legacy_renderer, osu_songbrowser_scorebrowser_enabled");
	alpha317.changes.push_back("- Disabled new star/pp algorithm relax/autopilot nerfs by default in order to match previous behavior");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added new experimental mod \"Half Timing Window\"");
	alpha317.changes.push_back("- Added \"Quick Seek\" key bindings (jump +-5 seconds, default LEFT/RIGHT arrow keys)");
	alpha317.changes.push_back("- Added hitobject type percentage support to songbrowser search (e.g. \"sliders>80%\")");
	alpha317.changes.push_back("- Added ConVars (1): osu_seek_delta, osu_end_skip, osu_mod_halfwindow_allow_300s");
	alpha317.changes.push_back("- Added ConVars (2): osu_songbrowser_search_hardcoded_filter, osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier");
	alpha317.changes.push_back("- Added ConVars (3): osu_skin_force_hitsound_sample_set, osu_hitobject_fade_in_time");
	alpha317.changes.push_back("- Added ConVars (4): osu_ar_overridenegative, osu_cs_overridenegative");
	alpha317.changes.push_back("- Added ConVars (5): osu_songbrowser_button_active/inactive_color_a/r/g/b, ...collection_active/inactive_color_a/r/g/b, ...difficulty_inactive_color_a/r/g/b");
	alpha317.changes.push_back("- Added ConVars (6): osu_hitresult_delta_colorize, osu_hitresult_delta_colorize_interpolate/multiplier/early_r/g/b/late_r/g/b");
	alpha317.changes.push_back("- Linux: Upgraded build system from Ubuntu 16 to Ubuntu 18 (anything running older glibc is no longer supported)");
	alpha317.changes.push_back("- Updated scrubbing to keep player invincible while scrubbing timeline is being clicked (even if mouse position does not change)");
	alpha317.changes.push_back("- Updated CS override to hard cap at CS +12.1429 (more than that never made sense anyway, the circle radius just goes negative)");
	alpha317.changes.push_back("- Updated audio output device change logic to restore music state (only in menu, changing output devices while playing will still kick you out)");
	alpha317.changes.push_back("- Improved songbrowser scrolling smoothness when switching beatmaps/sets (should reduce eye strain with less jumping around all the time)");
	alpha317.changes.push_back("- Improved songbrowser scrolling behavior when right-click absolute scrolling to always show full songbuttons (disabled scroll velocity offset)");
	alpha317.changes.push_back("- Improved songbrowser thumbnail/background image loading behavior");
	alpha317.changes.push_back("- Increased osu_ui_top_ranks_max from 100 to 200 by default");
	alpha317.changes.push_back("- Fixed very old legacy beatmaps (< v8) sometimes generating mismatched slider ticks (compared to stable) because of different tickDistance algorithm");
	alpha317.changes.push_back("- Fixed extremely rare infinite font/layout/resolution reloading bug killing performance caused by custom display scaling percentages (e.g. 124%, yes 124% scaling in Windows)");
	alpha317.changes.push_back("- Fixed extremely rare freeze bug caused by potential infinite stars in osu!.db");
	alpha317.changes.push_back("- Fixed multiple audio output devices with the exact same name not being selectable/handled correctly");
	alpha317.changes.push_back("- Fixed minimize_on_focus_lost_if_borderless_windowed_fullscreen not working");
	alpha317.changes.push_back("- Fixed fposu_mouse_cm_360 + fposu_mouse_dpi not updating in options menu if changed live via console/cfg");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Windows: Added support for mixed-DPI-scaling-multi-monitor setups (automatic detection based on which monitor the game is on)");
	alpha317.changes.push_back("- Windows: Added support for key binding all remaining mouse buttons (all mouse buttons can now be bound to key binding actions)");
	alpha317.changes.push_back("- Fixed osu_mod_random in cfg affecting main menu button logo text sliders");
	alpha317.changes.push_back("- Fixed very wide back button skin images overlapping other songbrowser buttons and making them impossible/invisible to click");
	alpha317.changes.push_back("- Fixed pen dragging playstyles potentially causing unintentional UI clicks when in-game screens/panels are switched and the pen is released");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Linux: Switched to SDL backend (mostly for Steam Deck multitouch support, also because both X11 and Wayland suck hard)");
	alpha317.changes.push_back("- FPoSu: Added cursor trail support (can be disabled in Options > Skin > \"Draw Cursor Trail\", or fposu_draw_cursor_trail)");
	alpha317.changes.push_back("- Added new experimental mod \"Approach Different\"");
	alpha317.changes.push_back("- Added new experimental mod \"Strict Tracking\"");
	alpha317.changes.push_back("- Added new main menu button logo text");
	alpha317.changes.push_back("- Added \"most common BPM\" in parentheses to top left songbrowser info label (e.g. \"BPM: 120-240 (190)\")");
	alpha317.changes.push_back("- Added beatmapID and beatmapSetID columns to osu_scores_export csv");
	alpha317.changes.push_back("- Added \"Reset all settings\" button to bottom of options menu");
	alpha317.changes.push_back("- Added PAGEUP/PAGEDOWN key support to songbrowser");
	alpha317.changes.push_back("- Added ConVars (1): osu_followpoints_connect_spinners, fposu_transparent_playfield");
	alpha317.changes.push_back("- Added ConVars (2): fposu_playfield_position_x/y/z, fposu_playfield_rotation_x/y/z");
	alpha317.changes.push_back("- Added ConVars (3): osu_mod_approach_different_initial_size, osu_mod_approach_different_style");
	alpha317.changes.push_back("- Added ConVars (4): osu_cursor_trail_scale, osu_hud_hiterrorbar_entry_additive, fposu_draw_cursor_trail");
	alpha317.changes.push_back("- Added ConVars (5): osu_mod_strict_tracking_remove_slider_ticks");
	alpha317.changes.push_back("- Updated songbrowser search to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated \"Draw Stats: BPM\" to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated \"Sort by BPM\" to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated UI DPI scaling to automatically enable/disable itself based on in-game resolution (instead of OS DPI)");
	alpha317.changes.push_back("- Updated hiterrorbar to use additive blending for entries/lines");
	alpha317.changes.push_back("- Updated preview music handling to fallback to 40% of song length (instead of beginning) if invalid/missing PreviewTime in beatmap");
	alpha317.changes.push_back("- Improved performance slightly (shader uniform caching)");
	alpha317.changes.push_back("- Fixed pp algorithm to allow AR/OD above 10 for non-1.0x speed multipliers and/or EZ/HT/HR/DT (please do Top Ranks > \"Recalculate pp\")");
	alpha317.changes.push_back("- Fixed \"Use mods\" inconsistent behavior (custom speed multiplier \"ignored once\", \"sticky\" experimental mods)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added collection management support (Add/Delete/Set/Unset collections, right-click context menu on beatmap buttons)");
	alpha317.changes.push_back("- FPoSu: Added new experimental mod \"Strafing\"");
	alpha317.changes.push_back("- Added gamemode selection button to songbrowser (no, skins abusing this as a decoration overlay are not supported)");
	alpha317.changes.push_back("- Added support for \"ScorePrefix\" + \"ComboPrefix\" + \"ScoreOverlap\" + \"ComboOverlap\" in skin.ini");
	alpha317.changes.push_back("- Added option \"Show osu! scores.db user names in user switcher\" (Options > General > Player)");
	alpha317.changes.push_back("- Added option \"Draw Strain Graph in Songbrowser\" (Options > General > Songbrowser)");
	alpha317.changes.push_back("- Added option \"Draw Strain Graph in Scrubbing Timeline\" (Options > General > Songbrowser)");
	alpha317.changes.push_back("- Added startup loading screen and animation");
	alpha317.changes.push_back("- Added beatmap ID to songbrowser tooltip");
	alpha317.changes.push_back("- Added hint text for experimental mods in mod selection screen");
	alpha317.changes.push_back("- Added ConVars (1): osu_mod_fposu_sound_panning, osu_mod_fps_sound_panning, osu_stacking_leniency_override");
	alpha317.changes.push_back("- Added ConVars (2): fposu_mod_strafing_strength_x/y/z, fposu_mod_strafing_frequency_x/y/z");
	alpha317.changes.push_back("- Added ConVars (3): snd_updateperiod, snd_dev_period, snd_dev_buffer, snd_wav_file_min_size");
	alpha317.changes.push_back("- Added ConVars (4): osu_ignore_beatmap_combo_numbers, osu_number_max");
	alpha317.changes.push_back("- Added ConVars (5): osu_scores_export, osu_auto_and_relax_block_user_input");
	alpha317.changes.push_back("- Updated songbrowser search to be async (avoids freezing the entire game when searching through 100k+ beatmaps)\n");
	alpha317.changes.push_back("- Updated \"PF\" text on scores to differentiate \"PFC\" (for perfect max possible combo) and \"FC\" (for no combo break, dropped sliderends allowed)");
	alpha317.changes.push_back("- Updated hitresult draw order to be correct (new results are now on top of old ones, was inverted previously and nobody noticed until now)");
	alpha317.changes.push_back("- Updated user switcher to be scrollable if the list gets too large");
	alpha317.changes.push_back("- Updated score list scrollbar size as to not overlap with text");
	alpha317.changes.push_back("- Updated score buttons to show AR/CS/OD/HP overrides directly in songbrowser (avoids having to open the score and waiting for the tooltip)");
	alpha317.changes.push_back("- Updated osu!.db loading to ignore corrupt entries with empty values (instead of producing empty songbuttons with \"//\" text in songbrowser)");
	alpha317.changes.push_back("- Linux: Updated osu! database loader to automatically rewrite backslashes into forward slashes for beatmap filepaths (as a workaround)");
	alpha317.changes.push_back("- Improved startup performance (skin loading)");
	alpha317.changes.push_back("- Fixed slider start circle hitresult getting overwritten by slider end circle hitresult in target practice mod");
	alpha317.changes.push_back("- Fixed animated hitresults being broken in target practice mod");
	alpha317.changes.push_back("- Fixed NotificationOverlay sometimes eating key inputs in options menu even while not in keybinding mode");
	alpha317.changes.push_back("- Fixed \"osu!stable\" notelock type eating the second input of frame perfect double inputs on overlapping/2b slider startcircles");
	alpha317.changes.push_back("- Fixed malformed/corrupt spinnerspin.wav skin files crashing the BASS audio library");
	changelogs.push_back(alpha317);

	CHANGELOG alpha316;
	alpha316.title = "32 - 32.04 (18.01.2021 - 25.07.2021)";
	alpha316.changes.push_back("- Added ability to copy (all) scores between profiles (Songbrowser > User > Top Ranks > Menu > Copy All Scores from ...)");
	alpha316.changes.push_back("- Added 2B support to \"osu!lazer 2020\" and \"McOsu\" notelock types");
	alpha316.changes.push_back("- Added SHIFT hotkey support to LEFT/RIGHT songbrowser group navigation");
	alpha316.changes.push_back("- Added new grouping options to songbrowser: \"By Artist\", \"By Creator\", \"By Difficulty\", \"By Length\", \"By Title\"");
	alpha316.changes.push_back("- Added dynamic star recalculation for all mods in songbrowser (stars will now recalculate to reflect active mods, including overrides and experimentals)");
	alpha316.changes.push_back("- Added ability to recalculate all McOsu pp scores (Songbrowser > User > Top Ranks > Menu > Recalculate)");
	alpha316.changes.push_back("- Added ability to convert/import osu! scores into McOsu pp scores (Songbrowser > User > Top Ranks > Menu > Import)");
	alpha316.changes.push_back("- Added ability to delete all scores of active user (Songbrowser > User > Top Ranks > Menu > Delete)");
	alpha316.changes.push_back("- Added menu button to \"Top Ranks\" screen (Recalculate pp, Import osu! Scores, Delete All Scores, etc.)");
	alpha316.changes.push_back("- Added \"Use Mods\" to context menu for score buttons (sets all mods, including overrides and experimentals, to whatever the score has)");
	alpha316.changes.push_back("- Added extra set of keybinds for key1/key2 (Options > Input > Keyboard)");
	alpha316.changes.push_back("- Added bonus pp calculation to user stats (previously total user pp were without bonus. Bonus is purely based on number of scores.)");
	alpha316.changes.push_back("- Added \"Max Possible pp\" to top left songbrowser info label (shows max possible pp with all active mods applied, including overrides and experimentals)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Max Possible Combo\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: pp (SS)\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Stars* (Total)\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Stars*** (Until Now)\" (aka live stars) (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added support for OGG files in skin sound samples");
	alpha316.changes.push_back("- Added Tolerance2B handling to osu!stable notelock algorithm (unlock if within 3 ms overlap)");
	alpha316.changes.push_back("- Added score multiplier info label to mod selection screen");
	alpha316.changes.push_back("- FPoSu: Added zooming/scoping (Options > Input > Keyboard > FPoSu > Zoom) (Options > FPoSu > \"FOV (Zoom)\")");
	alpha316.changes.push_back("- Added ConVars (1): osu_spinner_use_ar_fadein, osu_notelock_stable_tolerance2b");
	alpha316.changes.push_back("- Added ConVars (2): fposu_zoom_fov, fposu_zoom_sensitivity_ratio, fposu_zoom_anim_duration");
	alpha316.changes.push_back("- Added ConVars (3): osu_scores_rename, osu_scores_bonus_pp, osu_collections_legacy_enabled");
	alpha316.changes.push_back("- Added ConVars (4): osu_songbrowser_dynamic_star_recalc, osu_draw_songbrowser_strain_graph, osu_draw_scrubbing_timeline_strain_graph");
	alpha316.changes.push_back("- Added ConVars (5): osu_hud_hiterrorbar_hide_during_spinner, osu_hud_inputoverlay_offset_x/y");
	alpha316.changes.push_back("- Added ConVars (6): osu_slider_snake_duration_multiplier, osu_slider_reverse_arrow_fadein_duration");
	alpha316.changes.push_back("- Added ConVars (7): fposu_draw_scorebarbg_on_top");
	alpha316.changes.push_back("- Added ConVars (8): cursor_trail_expand");
	alpha316.changes.push_back("- Added ConVars (9): osu_playfield_circular, osu_quick_retry_time");
	alpha316.changes.push_back("- Initial rewrite of songbrowser and entire internal database class architecture (not fully finished yet)");
	alpha316.changes.push_back("- Songbrowser can now handle individual diff buttons and/or split from their parent beatmap/set button");
	alpha316.changes.push_back("- Collections now correctly only show contained diffs (previously always showed entire set)");
	alpha316.changes.push_back("- Similarly, searching will now match individual diffs, instead of always the entire set");
	alpha316.changes.push_back("- However, sorting still only sorts by beatmap set heuristics, this will be fixed over time with one of the next updates");
	alpha316.changes.push_back("- All pp scores can now be recalculated at will, so click on \"Recalculate pp\" as soon as possible (Songbrowser > User > Top Ranks > Menu)");
	alpha316.changes.push_back("- Updated star algorithm (15) (Added emu1337's diff spike nerf), see https://github.com/ppy/osu/pull/13483/");
	alpha316.changes.push_back("- Updated pp algorithm (14) (Added emu1337's AR/FL adjustments), see https://github.com/ppy/osu-performance/pull/137/");
	alpha316.changes.push_back("- Updated pp algorithm (13) (Added StanR's AR11 buff), see https://github.com/ppy/osu-performance/pull/135/");
	alpha316.changes.push_back("- Updated hiterrorbar colors to more closely match osu!");
	alpha316.changes.push_back("- Updated profile switcher to show all usernames which have any score in any db (so both osu!/McOsu scores.db)");
	alpha316.changes.push_back("- Updated universal offset handling by compensating for speed multiplier to match osu!");
	alpha316.changes.push_back("- Updated beatmap length calc to include initial skippable section/break to match osu!");
	alpha316.changes.push_back("- Updated slow speed (less than 1.0x) offset compensation to match osu!");
	alpha316.changes.push_back("- Updated fposu_fov and fposu_zoom_fov to allow underscale + overscale (via console or cfg)");
	alpha316.changes.push_back("- Updated all right-click context menus to be bigger and easier to hit (score buttons, song buttons)");
	alpha316.changes.push_back("- Updated SearchUIOverlay to simply move left on text overflow");
	alpha316.changes.push_back("- Updated \"DPI\" and \"cm per 360\" textboxes to support decimal values with comma (e.g. 4,2 vs 4.2)");
	alpha316.changes.push_back("- Updated mouse_raw_input_absolute_to_window to be ignored if raw input is disabled");
	alpha316.changes.push_back("- Updated pp algorithm (12) (Reverted Xexxar's accidental AR8 buff), see https://github.com/ppy/osu-performance/pull/133");
	alpha316.changes.push_back("- Updated pp algorithm (11) (Xexxar's miss curve changes), see https://github.com/ppy/osu-performance/pull/129/");
	alpha316.changes.push_back("- Updated pp algorithm (10) (Xexxar's low acc speed nerf), see https://github.com/ppy/osu-performance/pull/128/");
	alpha316.changes.push_back("- Updated pp algorithm (9) (StanR's NF multiplier based on amount of misses), see https://github.com/ppy/osu-performance/pull/127/");
	alpha316.changes.push_back("- Updated pp algorithm (8) (StanR's SO multiplier based on amount of spinners in map), see https://github.com/ppy/osu-performance/pull/110/");
	alpha316.changes.push_back("- Updated pp algorithm (7) (Xexxar's AR11 nerf and AR8 buff), see https://github.com/ppy/osu-performance/pull/125/");
	alpha316.changes.push_back("- Linux: Updated BASS + BASSFX libraries");
	alpha316.changes.push_back("- macOS: Updated BASS + BASSFX libraries");
	alpha316.changes.push_back("- Improved background star calc speed");
	alpha316.changes.push_back("- Improved star calc memory usage (slider curves, mostly fixes aspire out-of-memory crashes)");
	alpha316.changes.push_back("- Fixed disabled snd_speed_compensate_pitch causing broken speed multiplier calculations (e.g. always 1.0x for statistics overlay)");
	alpha316.changes.push_back("- Fixed incorrect visually shown OPM/CPM/SPM values in songbrowser tooltip (multiplied by speed multiplier twice)");
	alpha316.changes.push_back("- Fixed crash on star calc for sliders with 85899394 repeats (clamp to 9000 to match osu!)");
	alpha316.changes.push_back("- Fixed K1/K2 keys triggering volume change when bound to both and held until key repeat during gameplay");
	alpha316.changes.push_back("- Fixed beatmap combo colors not getting loaded");
	alpha316.changes.push_back("- Fixed background star calc not calculating osu database entries with negative stars");
	alpha316.changes.push_back("- Fixed keys getting stuck if held until after beatmap end");
	alpha316.changes.push_back("- Fixed aspire infinite slider ticks crashes");
	alpha316.changes.push_back("- Fixed object counts not being updated by background star calc for people without osu!.db");
	alpha316.changes.push_back("- Fixed slider ball color handling (default to white instead of osu!default-skin special case)");
	alpha316.changes.push_back("- Fixed cursor ripples in FPoSu mode being drawn on wrong layer");
	alpha316.changes.push_back("- Fixed collections not showing all contained beatmaps/diffs");
	alpha316.changes.push_back("- Fixed invalid default selection when opening beatmap sets with active search");
	alpha316.changes.push_back("- Fixed partial corrupt PNG skin images not loading due to newer/stricter lodepng library version (disabled CRC check)");
	alpha316.changes.push_back("- Fixed ScoreV2 score multipliers for HR and DT and NF (1.06x -> 1.10x, 1.12x -> 1.20x, 0.5x -> 1.0x)");
	alpha316.changes.push_back("- Fixed UI toggle being hardcoded to SHIFT+TAB and not respecting \"Toggle Scoreboard\" keybind (in combination with SHIFT)");
	alpha316.changes.push_back("- Fixed star cache not updating instantly when changing Speed Override with keyboard keys while playing (previously only recalculated upon closing mod selection screen)");
	alpha316.changes.push_back("- Fixed drain not being recalculated instantly when changing HP Override while playing (previously only recalculated upon closing mod selection screen)");
	alpha316.changes.push_back("- Fixed clicking mod selection screen buttons also triggering \"click on the orange cursor to continue play\" (unwanted click-through)");
	alpha316.changes.push_back("- Fixed animated followpoint.png scaling not respecting inconsistent @2x frames");
	alpha316.changes.push_back("- Fixed drawHitCircleNumber for variable number width skins (@yclee126)");
	alpha316.changes.push_back("- Fixed spinners not using hardcoded 400 ms fadein (previously used same AR-dependent fadein as circles, because that makes sense compared to this insanity)");
	alpha316.changes.push_back("- Fixed mod selection screen visually rounding non-1.0x difficulty multipliers to one decimal digit (e.g. HR CS showed 4.55 in songbrowser but 4.5 in override)");
	alpha316.changes.push_back("- Fixed songbrowser visually always showing raw beatmap HP value (without applying mods or overrides)");
	alpha316.changes.push_back("- Fixed skipping while loading potentially breaking hitobject state");
	alpha316.changes.push_back("- Fixed very rare beatmaps ending prematurely with music (hitobjects at exact end of mp3) causing lost scores due to missing judgements");
	changelogs.push_back(alpha316);

	CHANGELOG alpha315;
	alpha315.title = "31.1 - 31.12 (14.02.2020 - 15.07.2020)";
	alpha315.changes.push_back("- Added 3 new notelock algorithms: McOsu, osu!lazer 2018, osu!lazer 2020 (Karoo13's algorithm)");
	alpha315.changes.push_back("- Added option \"Select Notelock\" (Options > Gameplay > Mechanics)");
	alpha315.changes.push_back("- Added option \"Kill Player upon Failing\" (Options > Gameplay > Mechanics)");
	alpha315.changes.push_back("- Added option \"Draw Stats: HP\" (Options > Gameplay > HUD)");
	alpha315.changes.push_back("- Added support for ranking-perfect (skin element for full combo on ranking screen)");
	alpha315.changes.push_back("- Added \"FC\" text after 123x to indicate a perfect full combo on highscore and top ranks list");
	alpha315.changes.push_back("- Added new search keywords: opm, cpm, spm, objects, circles, sliders (objects/circles/sliders per minute, total count)");
	alpha315.changes.push_back("- Added support for fail-background (skin element)");
	alpha315.changes.push_back("- Added option \"Inactive\" (Options > Audio > Volume)");
	alpha315.changes.push_back("- Added hitresult fadein + scale wobble animations (previously became visible instantly as is)");
	alpha315.changes.push_back("- VR: Added option \"Draw Laser in Game\" (Options > Virtual Reality > Miscellaneous)");
	alpha315.changes.push_back("- VR: Added option \"Draw Laser in Menu\" (Options > Virtual Reality > Miscellaneous)");
	alpha315.changes.push_back("- VR: Added option \"Head Model Brightness\" (Options > Virtual Reality > Play Area / Playfield)");
	alpha315.changes.push_back("- Windows: Added option \"Audio compatibility mode\" (Options > Audio > Devices)");
	alpha315.changes.push_back("- Added ConVars (1): osu_slider_end_miss_breaks_combo");
	alpha315.changes.push_back("- Added ConVars (2): osu_hitresult_fadein_duration, osu_hitresult_fadeout_start_time, osu_hitresult_fadeout_duration");
	alpha315.changes.push_back("- Added ConVars (3): osu_hitresult_miss_fadein_scale, osu_hitresult_animated, osu_volume_master_inactive");
	alpha315.changes.push_back("- Added ConVars (4): osu_drain_kill_notification_duration, snd_play_interp_duration, snd_play_interp_ratio");
	alpha315.changes.push_back("- Linux: Added _NET_WM_BYPASS_COMPOSITOR extended window manager hint (avoids vsync)");
	alpha315.changes.push_back("- Improved hitresult animation timing and movement accuracy to exactly match osu!stable (fadein, fadeout, scaleanim)");
	alpha315.changes.push_back("- Increased maximum file size limit from 200 MB to 512 MB (giant osu!.db support)");
	alpha315.changes.push_back("- Improved osu!.db database loading speed");
	alpha315.changes.push_back("- Improved beatmap grouping for beatmaps with invalid SetID (find ID in folder name before falling back to artist/title)");
	alpha315.changes.push_back("- Improved scroll wheel scrolling behavior");
	alpha315.changes.push_back("- General engine improvements");
	alpha315.changes.push_back("- Updated mod selection screen to display two digits after comma for non-1.0x speed multipliers");
	alpha315.changes.push_back("- Updated CPM/SPM/OPM metric calculations to multiply with speed multiplier (search, tooltip)");
	alpha315.changes.push_back("- Updated osu_drain_lazer_break_before and osu_drain_lazer_break_after to match recent updates (Lazer 2020.602.0)");
	alpha315.changes.push_back("- Updated hp drain type \"osu!lazer 2020\" for slider tails to match recent updates (Lazer 2020.603.0)");
	alpha315.changes.push_back("- Updated scrubbing to cancel the failing animation");
	alpha315.changes.push_back("- Unhappily bring followpoint behavior back in line with osu!");
	alpha315.changes.push_back("- NOTE (1): I hate osu!'s followpoint behavior, because it allows cheesing high AR reading. You WILL develop bad habits.");
	alpha315.changes.push_back("- NOTE (2): osu! draws followpoints with a fixed hardcoded AR of ~7.68 (800 ms).");
	alpha315.changes.push_back("- NOTE (3): Up until now, McOsu has clamped this to the real AR. To go back, use \"osu_followpoints_clamp 1\".");
	alpha315.changes.push_back("- NOTE (4): This is more of an experiment, and the change may be reverted, we will see.");
	alpha315.changes.push_back("- VR: Updated option \"Draw Desktop Game (2D)\" to ignore screen clicks during gameplay if disabled");
	alpha315.changes.push_back("- VR: Updated option \"Draw Desktop Game (2D)\" to not draw spectator/desktop cursors if disabled");
	alpha315.changes.push_back("- Only show ranking tooltip on left half of screen");
	alpha315.changes.push_back("- Windows: Workaround for Windows 10 bug in \"OS TabletPC Support\" in combination with raw mouse input:");
	alpha315.changes.push_back("- Windows: Everyone who uses Windows 10 and plays with a mouse should DISABLE \"OS TabletPC Support\"!");
	alpha315.changes.push_back("- Fixed \"Inactive\" volume not applying when minimizing in windowed mode");
	alpha315.changes.push_back("- Fixed disabled \"Show pp instead of score in scorebrowser\" applying to Top Ranks screen");
	alpha315.changes.push_back("- Fixed aspire regression breaking timingpoints on old osu file format v5 beatmaps");
	alpha315.changes.push_back("- Fixed hitresult animations not respecting speed multiplier (previously always faded at 1x time)");
	alpha315.changes.push_back("- Fixed aspire freeze on mangled 3-point sliders (e.g. Ping)");
	alpha315.changes.push_back("- Fixed aspire timingpoint handling (e.g. XNOR) (2)");
	alpha315.changes.push_back("- Fixed \"Quick Load\" keybind not working while in failing animation");
	alpha315.changes.push_back("- Fixed very old beatmaps not using the old stacking algorithm (version < 6)");
	alpha315.changes.push_back("- VR: Fixed slider bodies not being drawn on top of virtual screen (reversed depth buffer)");
	alpha315.changes.push_back("- VR: Fixed oversized scorebar-bg skin elements z-fighting with other HUD elements (like score)");
	alpha315.changes.push_back("- FPoSu: Fixed hiterrorbar being drawn on the playfield instead of the HUD overlay");
	alpha315.changes.push_back("- Fixed section-pass/fail appearing in too small breaks");
	alpha315.changes.push_back("- Fixed F1 as K1/K2 interfering with mod select during gameplay");
	alpha315.changes.push_back("- Fixed approximate beatmap length (used for search) not getting populated during star calculation if without osu!.db");
	alpha315.changes.push_back("- Fixed quick retry sometimes causing weird small speedup/slowdown at start");
	alpha315.changes.push_back("- Fixed section-pass/fail appearing in empty sections which are not break events");
	alpha315.changes.push_back("- Fixed scorebar-ki always being drawn for legacy skins (unwanted default skin fallback)");
	alpha315.changes.push_back("- Fixed skip intro button appearing 1 second later than usual");
	alpha315.changes.push_back("- Fixed warning arrows appearing 1 second later than usual");
	changelogs.push_back(alpha315);

	CHANGELOG alpha314;
	alpha314.title = "31 (11.02.2020)";
	alpha314.changes.push_back("- Added HP drain support");
	alpha314.changes.push_back("- Added 4 different HP drain algorithms: None, VR, osu!stable, osu!lazer");
	alpha314.changes.push_back("- Added option \"Select HP Drain\" (Options > Gameplay > Mechanics)");
	alpha314.changes.push_back("- Added geki/katu combo finisher (scoring, skin elements, health)");
	alpha314.changes.push_back("- Added Health/HP/Score Bar to HUD");
	alpha314.changes.push_back("- Added option \"Draw ScoreBar\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added option \"ScoreBar Scale\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added section-pass/section-fail (sounds, skin elements)");
	alpha314.changes.push_back("- Added option \"Statistics X Offset\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added option \"Statistics Y Offset\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added keybind \"Toggle Mod Selection Screen\" (Options > Input > Keyboard > Keys - Song Select)");
	alpha314.changes.push_back("- Added keybind \"Random Beatmap\" (Options > Input > Keyboard > Keys - Song Select)");
	alpha314.changes.push_back("- Added ConVars (1): osu_hud_hiterrorbar_alpha, osu_hud_hiterrorbar_bar_alpha, osu_hud_hiterrorbar_centerline_alpha");
	alpha314.changes.push_back("- Added ConVars (2): osu_hud_hiterrorbar_entry_alpha, osu_hud_hiterrorbar_entry_300/100/50/miss_r/g/b");
	alpha314.changes.push_back("- Added ConVars (3): osu_hud_hiterrorbar_centerline_r/g/b, osu_hud_hiterrorbar_max_entries");
	alpha314.changes.push_back("- Added ConVars (4): osu_hud_hiterrorbar_entry_hit/miss_fade_time, osu_hud_hiterrorbar_offset_percent");
	alpha314.changes.push_back("- Added ConVars (5): osu_draw_hiterrorbar_bottom/top/left/right, osu_hud_hiterrorbar_offset_bottom/top/left/right_percent");
	alpha314.changes.push_back("- Added ConVars (6): osu_drain_*, osu_drain_vr_*, osu_drain_stable_*, osu_drain_lazer_*");
	alpha314.changes.push_back("- Added ConVars (7): osu_pause_dim_alpha/duration, osu_hud_scorebar_hide_during_breaks, osu_hud_scorebar_hide_anim_duration");
	alpha314.changes.push_back("- Updated BASS audio library to 2020 2.4.15.2 (all offset problems have been fixed, yay!)");
	alpha314.changes.push_back("- FPoSu: Rotated/Flipped/Mirrored background cube UV coordinates to wrap horizontally as expected");
	alpha314.changes.push_back("- Relaxed notelock (1) to unlock 2B circles at the exact end of sliders (previously unlocked after slider end)");
	alpha314.changes.push_back("- Relaxed notelock (2) to allow mashing both buttons within the same frame (previously did not update lock)");
	alpha314.changes.push_back("- Moved hiterrorbar behind hitobjects");
	alpha314.changes.push_back("- Updated SHIFT + TAB and SHIFT scoreboard toggle behavior");
	alpha314.changes.push_back("- Improved spinner accuracy");
	alpha314.changes.push_back("- Fixed kinetic tablet scrolling at very high framerates (> ~600 fps)");
	alpha314.changes.push_back("- Fixed ranking screen layout partially for weird skins (long grade overflow)");
	alpha314.changes.push_back("- Fixed enabling \"Ignore Beatmap Sample Volume\" not immediately updating sample volume");
	alpha314.changes.push_back("- Fixed stale context menu in top ranks screen potentially allowing random score deletion if clicked");
	changelogs.push_back(alpha314);

	CHANGELOG alpha313;
	alpha313.title = "30.13 (12.01.2020)";
	alpha313.changes.push_back("- Added searching by beatmap ID + beatmap set ID");
	alpha313.changes.push_back("- Added CTRL + V support to songbrowser search (paste clipboard)");
	alpha313.changes.push_back("- Added speed display to score buttons");
	alpha313.changes.push_back("- Added support for sliderslide sound");
	alpha313.changes.push_back("- Added Touch Device mod (allows simulating pp nerf)");
	alpha313.changes.push_back("- Added option \"Always enable touch device pp nerf mod\" (Options > General > Player)");
	alpha313.changes.push_back("- Added option \"Apply speed/pitch mods while browsing\" (Options > Audio > Songbrowser)");
	alpha313.changes.push_back("- Added option \"Draw Stats: 300 hitwindow\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Draw Stats: Accuracy Error\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Intro\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Breaks\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added ConVars (1): osu_followpoints_separation_multiplier, osu_songbrowser_search_delay");
	alpha313.changes.push_back("- Added ConVars (2): osu_slider_body_fade_out_time_multiplier, osu_beatmap_preview_music_loop");
	alpha313.changes.push_back("- Added ConVars (3): osu_skin_export, osu_hud_statistics_hitdelta_chunksize");
	alpha313.changes.push_back("- Windows: Added WASAPI option \"Period Size\" (Options > Audio > WASAPI) (wasapi-test beta)");
	alpha313.changes.push_back("- Allow overscaling osu_slider_body_alpha_multiplier/color_saturation, osu_cursor_scale, fposu_distance");
	alpha313.changes.push_back("- Improved engine background async loading (please report crashes)");
	alpha313.changes.push_back("- Loop music");
	alpha313.changes.push_back("- Fixed skin hit0/hit50/hit100/hit300 animation handling (keep last frame and fade)");
	alpha313.changes.push_back("- Fixed scrubbing during lead-in time breaking things");
	alpha313.changes.push_back("- Fixed right click scrolling in songbrowser stalling if cursor goes outside container");
	alpha313.changes.push_back("- Windows: Fixed Windows key not unlocking on focus loss if \"Pause on Focus Loss\" is disabled");
	changelogs.push_back(alpha313);

	CHANGELOG alpha312;
	alpha312.title = "30.12 (19.12.2019)";
	alpha312.changes.push_back("- Added button \"Random Skin\" (Options > Skin)");
	alpha312.changes.push_back("- Added option \"SHIFT + TAB toggles everything\" (Options > Gameplay > HUD)");
	alpha312.changes.push_back("- Added ConVars (1): osu_mod_random_circle/slider/spinner_offset_x/y_percent, osu_mod_hd_circle_fadein/fadeout_start/end_percent");
	alpha312.changes.push_back("- Added ConVars (2): osu_play_hitsound_on_click_while_playing, osu_alt_f4_quits_even_while_playing");
	alpha312.changes.push_back("- Added ConVars (3): osu_skin_random, osu_skin_random_elements, osu_slider_body_unit_circle_subdivisions");
	alpha312.changes.push_back("- Windows: Ignore Windows key while playing (osu_win_disable_windows_key_while_playing)");
	alpha312.changes.push_back("- Made skip button only skip if click started inside");
	alpha312.changes.push_back("- Made mod \"Jigsaw\" allow clicks during breaks and before first hitobject");
	alpha312.changes.push_back("- Made experimental mod \"Full Alternate\" allow any key for first hitobjects, and after break, and during/after spinners");
	alpha312.changes.push_back("- Improved Steam Workshop subscribed items refresh speed");
	alpha312.changes.push_back("- Fixed grade image on songbuttons ignoring score sorting setting");
	alpha312.changes.push_back("- Fixed notelock unlocking sliders too early (previously unlocked after sliderstartcircle, now unlocks after slider end)");
	alpha312.changes.push_back("- Fixed rare hitsound timingpoint offsets (accurate on slider start/end now)");
	alpha312.changes.push_back("- Fixed NaN timingpoint handling for aspire (maybe)");
	changelogs.push_back(alpha312);

	CHANGELOG alpha311;
	alpha311.title = "30.11 (07.11.2019)";
	alpha311.changes.push_back("- Improved osu collection.db loading speed");
	alpha311.changes.push_back("- Fixed new osu database format breaking loading");
	alpha311.changes.push_back("- Added upper osu database version loading limit");
	alpha311.changes.push_back("- Added \"Sort By Misses\" to score sorting options");
	alpha311.changes.push_back("- Added ConVars: osu_rich_presence_dynamic_windowtitle, osu_database_ignore_version");
	alpha311.changes.push_back("- FPoSu: Fixed disabling \"Show FPS Counter\" not working (was always shown)");
	alpha311.changes.push_back("- Fixed rare custom manual ConVars getting removed from osu.cfg");
	changelogs.push_back(alpha311);

	CHANGELOG alpha301;
	alpha301.title = "30.1 (13.09.2019)";
	alpha301.changes.push_back("- Added proper support for HiDPI displays (scaling)");
	alpha301.changes.push_back("- Added option \"UI Scale\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added option \"DPI Scaling\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added context menu for deleting scores in \"Top Ranks\" screen");
	alpha301.changes.push_back("- Added sorting options for local scores (sort by pp, accuracy, combo, date)");
	alpha301.changes.push_back("- FPoSu: Added option \"Vertical FOV\" (Options > FPoSu > General)");
	alpha301.changes.push_back("- Draw breaks in scrubbing timeline");
	alpha301.changes.push_back("- Made scrubbing smoother by only seeking if the cursor position actually changed");
	alpha301.changes.push_back("- Windows: Added option \"High Priority\" (Options > Graphics > Renderer)");
	alpha301.changes.push_back("- Windows: Allow windowed resolutions to overshoot window borders (offscreen)");
	alpha301.changes.push_back("- Added ConVars: osu_followpoints_connect_combos, osu_scrubbing_smooth");
	alpha301.changes.push_back("- VR: Removed LIV support (for now)");
	alpha301.changes.push_back("- Allow loading incorrect skin.ini \"[General]\" section props before section start");
	alpha301.changes.push_back("- FPoSu: Fixed rare pause menu button jitter/unclickable");
	alpha301.changes.push_back("- Windows: Fixed toggling fullscreen sometimes causing weird windowed resolutions");
	alpha301.changes.push_back("- Windows: Fixed letterboxed \"Map Absolute Raw Input to Window\" offsets not matching osu!");
	changelogs.push_back(alpha301);

	CHANGELOG alpha30;
	alpha30.title =  "30.0 (13.06.2019)";
	alpha30.changes.push_back("- Added Steam Workshop support (for skins)");
	alpha30.changes.push_back("- Added option \"Cursor ripples\" (Options > Input > Mouse)");
	alpha30.changes.push_back("- Added skinning support for menu-background and cursor-ripple");
	alpha30.changes.push_back("- Added support for using custom BeatmapDirectory even without an osu installation/database");
	alpha30.changes.push_back("- Added ConVars: osu_cursor_ripple_duration/alpha/additive/anim_start_scale/end/fadeout_delay/tint_r/g/b");
	alpha30.changes.push_back("- General engine stability improvements");
	alpha30.changes.push_back("- Fixed AR/OD lock buttons being ignored by Timewarp experimental mod");
	alpha30.changes.push_back("- Fixed custom ConVars being ignored in cfg: osu_mods, osu_speed/ar/od/cs_override");
	changelogs.push_back(alpha30);

	CHANGELOG padding;
	padding.title = "... (older changelogs are available on GitHub or Steam)";
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	changelogs.push_back(padding);

	for (int i=0; i<changelogs.size(); i++)
	{
		CHANGELOG_UI changelog;

		// title label
		changelog.title = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].title);
		if (i == 0)
			changelog.title->setTextColor(0xff00ff00);
		else
			changelog.title->setTextColor(0xff888888);

		changelog.title->setSizeToContent();
		changelog.title->setDrawBackground(false);
		changelog.title->setDrawFrame(false);

		m_scrollView->getContainer()->addBaseUIElement(changelog.title);

		// changes
		for (int c=0; c<changelogs[i].changes.size(); c++)
		{
			class CustomCBaseUILabel : public CBaseUILabel
			{
			public:
				CustomCBaseUILabel(UString text) : CBaseUILabel(0, 0, 0, 0, "", text) {;}

				virtual void draw(Graphics *g)
				{
					if (m_bVisible && isMouseInside())
					{
						g->setColor(0x3fffffff);

						const int margin = 0;
						const int marginX = margin + 10;
						g->fillRect(m_vPos.x - marginX, m_vPos.y - margin, m_vSize.x + marginX*2, m_vSize.y + margin*2);
					}

					CBaseUILabel::draw(g);
					if (!m_bVisible) return;
				}
			};

			CBaseUILabel *change = new CustomCBaseUILabel(changelogs[i].changes[c]);

			if (i > 0)
				change->setTextColor(0xff888888);

			change->setSizeToContent();
			change->setDrawBackground(false);
			change->setDrawFrame(false);

			changelog.changes.push_back(change);

			m_scrollView->getContainer()->addBaseUIElement(change);
		}

		m_changelogs.push_back(changelog);
	}
}

OsuChangelog::~OsuChangelog()
{
	m_changelogs.clear();

	SAFE_DELETE(m_container);
}

void OsuChangelog::draw(Graphics *g)
{
	if (!m_bVisible) return;

	m_container->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuChangelog::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// HACKHACK:
	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
		engine->getMouse()->resetWheelDelta();

	m_container->update();

	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
	{
		stealFocus();
		m_container->stealFocus();
	}
}

void OsuChangelog::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
		updateLayout();
}

void OsuChangelog::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale(m_osu);

	m_container->setSize(m_osu->getScreenSize() + Vector2(2, 2));
	m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2, 2));

	float yCounter = 0;
	for (const CHANGELOG_UI &changelog : m_changelogs)
	{
		changelog.title->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
		changelog.title->setSizeToContent();

		yCounter += changelog.title->getSize().y;
		changelog.title->setRelPos(15 * dpiScale, yCounter);
		///yCounter += 10 * dpiScale;

		for (CBaseUILabel *change : changelog.changes)
		{
			change->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
			change->setSizeToContent();
			change->setSizeY(change->getSize().y*2.0f);
			yCounter += change->getSize().y/* + 13 * dpiScale*/;
			change->setRelPos(35 * dpiScale, yCounter);
		}

		// gap to previous version
		yCounter += 65 * dpiScale;
	}

	m_scrollView->setScrollSizeToContent(15 * dpiScale);
}

void OsuChangelog::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleChangelog();
}
