//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#include "KeyBindings.h"

#include "Keyboard.h"

ConVar KeyBindings::LEFT_CLICK("osu_key_left_click", (int)KEY_Z, FCVAR_NONE);
ConVar KeyBindings::RIGHT_CLICK("osu_key_right_click", (int)KEY_X, FCVAR_NONE);
ConVar KeyBindings::LEFT_CLICK_2("osu_key_left_click_2", 0, FCVAR_NONE);
ConVar KeyBindings::RIGHT_CLICK_2("osu_key_right_click_2", 0, FCVAR_NONE);

ConVar KeyBindings::FPOSU_ZOOM("osu_key_fposu_zoom", 0, FCVAR_NONE);

ConVar KeyBindings::INCREASE_VOLUME("osu_key_increase_volume", (int)KEY_UP, FCVAR_NONE);
ConVar KeyBindings::DECREASE_VOLUME("osu_key_decrease_volume", (int)KEY_DOWN, FCVAR_NONE);

ConVar KeyBindings::INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (int)KEY_ADD, FCVAR_NONE);
ConVar KeyBindings::DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (int)KEY_SUBTRACT, FCVAR_NONE);

ConVar KeyBindings::GAME_PAUSE("osu_key_game_pause", (int)KEY_ESCAPE, FCVAR_NONE);
ConVar KeyBindings::SKIP_CUTSCENE("osu_key_skip_cutscene", (int)KEY_SPACE, FCVAR_NONE);
ConVar KeyBindings::TOGGLE_SCOREBOARD("osu_key_toggle_scoreboard", (int)KEY_TAB, FCVAR_NONE);
ConVar KeyBindings::SEEK_TIME("osu_key_seek_time", (int)KEY_SHIFT, FCVAR_NONE);
ConVar KeyBindings::SEEK_TIME_BACKWARD("osu_key_seek_time_backward", (int)KEY_LEFT, FCVAR_NONE);
ConVar KeyBindings::SEEK_TIME_FORWARD("osu_key_seek_time_forward", (int)KEY_RIGHT, FCVAR_NONE);
ConVar KeyBindings::QUICK_RETRY("osu_key_quick_retry", (int)KEY_BACKSPACE, FCVAR_NONE);
ConVar KeyBindings::QUICK_SAVE("osu_key_quick_save", (int)KEY_F6, FCVAR_NONE);
ConVar KeyBindings::QUICK_LOAD("osu_key_quick_load", (int)KEY_F7, FCVAR_NONE);
ConVar KeyBindings::INSTANT_REPLAY("osu_key_instant_replay", (int)KEY_F2, FCVAR_NONE);
ConVar KeyBindings::TOGGLE_CHAT("osu_key_toggle_chat", (int)KEY_F8, FCVAR_NONE);
ConVar KeyBindings::SAVE_SCREENSHOT("osu_key_save_screenshot", (int)KEY_F12, FCVAR_NONE);
ConVar KeyBindings::DISABLE_MOUSE_BUTTONS("osu_key_disable_mouse_buttons", (int)KEY_F10, FCVAR_NONE);
ConVar KeyBindings::BOSS_KEY("osu_key_boss", (int)KEY_INSERT, FCVAR_NONE);

ConVar KeyBindings::TOGGLE_MODSELECT("osu_key_toggle_modselect", (int)KEY_F1, FCVAR_NONE);
ConVar KeyBindings::RANDOM_BEATMAP("osu_key_random_beatmap", (int)KEY_F2, FCVAR_NONE);

ConVar KeyBindings::MOD_EASY("osu_key_mod_easy", (int)KEY_Q, FCVAR_NONE);
ConVar KeyBindings::MOD_NOFAIL("osu_key_mod_nofail", (int)KEY_W, FCVAR_NONE);
ConVar KeyBindings::MOD_HALFTIME("osu_key_mod_halftime", (int)KEY_E, FCVAR_NONE);
ConVar KeyBindings::MOD_HARDROCK("osu_key_mod_hardrock", (int)KEY_A, FCVAR_NONE);
ConVar KeyBindings::MOD_SUDDENDEATH("osu_key_mod_suddendeath", (int)KEY_S, FCVAR_NONE);
ConVar KeyBindings::MOD_DOUBLETIME("osu_key_mod_doubletime", (int)KEY_D, FCVAR_NONE);
ConVar KeyBindings::MOD_HIDDEN("osu_key_mod_hidden", (int)KEY_F, FCVAR_NONE);
ConVar KeyBindings::MOD_FLASHLIGHT("osu_key_mod_flashlight", (int)KEY_G, FCVAR_NONE);
ConVar KeyBindings::MOD_RELAX("osu_key_mod_relax", (int)KEY_Z, FCVAR_NONE);
ConVar KeyBindings::MOD_AUTOPILOT("osu_key_mod_autopilot", (int)KEY_X, FCVAR_NONE);
ConVar KeyBindings::MOD_SPUNOUT("osu_key_mod_spunout", (int)KEY_C, FCVAR_NONE);
ConVar KeyBindings::MOD_AUTO("osu_key_mod_auto", (int)KEY_V, FCVAR_NONE);
ConVar KeyBindings::MOD_SCOREV2("osu_key_mod_scorev2", (int)KEY_B, FCVAR_NONE);

std::vector<ConVar*> KeyBindings::ALL = {&KeyBindings::LEFT_CLICK,
                                         &KeyBindings::RIGHT_CLICK,
                                         &KeyBindings::LEFT_CLICK_2,
                                         &KeyBindings::RIGHT_CLICK_2,

                                         &KeyBindings::FPOSU_ZOOM,

                                         &KeyBindings::INCREASE_VOLUME,
                                         &KeyBindings::DECREASE_VOLUME,

                                         &KeyBindings::INCREASE_LOCAL_OFFSET,
                                         &KeyBindings::DECREASE_LOCAL_OFFSET,

                                         &KeyBindings::GAME_PAUSE,
                                         &KeyBindings::SKIP_CUTSCENE,
                                         &KeyBindings::TOGGLE_SCOREBOARD,
                                         &KeyBindings::SEEK_TIME,
                                         &KeyBindings::SEEK_TIME_BACKWARD,
                                         &KeyBindings::SEEK_TIME_FORWARD,
                                         &KeyBindings::QUICK_RETRY,
                                         &KeyBindings::QUICK_SAVE,
                                         &KeyBindings::QUICK_LOAD,
                                         &KeyBindings::INSTANT_REPLAY,
                                         &KeyBindings::TOGGLE_CHAT,
                                         &KeyBindings::SAVE_SCREENSHOT,
                                         &KeyBindings::DISABLE_MOUSE_BUTTONS,
                                         &KeyBindings::BOSS_KEY,

                                         &KeyBindings::TOGGLE_MODSELECT,
                                         &KeyBindings::RANDOM_BEATMAP,

                                         &KeyBindings::MOD_EASY,
                                         &KeyBindings::MOD_NOFAIL,
                                         &KeyBindings::MOD_HALFTIME,
                                         &KeyBindings::MOD_HARDROCK,
                                         &KeyBindings::MOD_SUDDENDEATH,
                                         &KeyBindings::MOD_DOUBLETIME,
                                         &KeyBindings::MOD_HIDDEN,
                                         &KeyBindings::MOD_FLASHLIGHT,
                                         &KeyBindings::MOD_RELAX,
                                         &KeyBindings::MOD_AUTOPILOT,
                                         &KeyBindings::MOD_SPUNOUT,
                                         &KeyBindings::MOD_AUTO,
                                         &KeyBindings::MOD_SCOREV2};
