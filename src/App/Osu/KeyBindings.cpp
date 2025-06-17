#include "KeyBindings.h"

#include "Keyboard.h"

std::vector<ConVar*> KeyBindings::ALL = {&cv_LEFT_CLICK,
                                         &cv_RIGHT_CLICK,
                                         &cv_LEFT_CLICK_2,
                                         &cv_RIGHT_CLICK_2,

                                         &cv_FPOSU_ZOOM,

                                         &cv_INCREASE_VOLUME,
                                         &cv_DECREASE_VOLUME,

                                         &cv_INCREASE_LOCAL_OFFSET,
                                         &cv_DECREASE_LOCAL_OFFSET,

                                         &cv_GAME_PAUSE,
                                         &cv_SKIP_CUTSCENE,
                                         &cv_TOGGLE_SCOREBOARD,
                                         &cv_SEEK_TIME,
                                         &cv_SEEK_TIME_BACKWARD,
                                         &cv_SEEK_TIME_FORWARD,
                                         &cv_QUICK_RETRY,
                                         &cv_QUICK_SAVE,
                                         &cv_QUICK_LOAD,
                                         &cv_INSTANT_REPLAY,
                                         &cv_TOGGLE_CHAT,
                                         &cv_TOGGLE_EXTENDED_CHAT,
                                         &cv_SAVE_SCREENSHOT,
                                         &cv_DISABLE_MOUSE_BUTTONS,
                                         &cv_TOGGLE_MAP_BACKGROUND,
                                         &cv_BOSS_KEY,
                                         &cv_OPEN_SKIN_SELECT_MENU,

                                         &cv_TOGGLE_MODSELECT,
                                         &cv_RANDOM_BEATMAP,

                                         &cv_MOD_EASY,
                                         &cv_MOD_NOFAIL,
                                         &cv_MOD_HALFTIME,
                                         &cv_MOD_HARDROCK,
                                         &cv_MOD_SUDDENDEATH,
                                         &cv_MOD_DOUBLETIME,
                                         &cv_MOD_HIDDEN,
                                         &cv_MOD_FLASHLIGHT,
                                         &cv_MOD_RELAX,
                                         &cv_MOD_AUTOPILOT,
                                         &cv_MOD_SPUNOUT,
                                         &cv_MOD_AUTO,
                                         &cv_MOD_SCOREV2};
