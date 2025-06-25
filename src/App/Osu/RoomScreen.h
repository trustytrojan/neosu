#pragma once
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "BanchoProtocol.h"
#include "CBaseUIScrollView.h"
#include "OsuScreen.h"
#include "score.h"

class CBaseUICheckbox;
class CBaseUILabel;
class CBaseUITextbox;
class DatabaseBeatmap;
class MainMenuPauseButton;
class UIButton;
class UICheckbox;
class UIContextMenu;

class UIModList : public CBaseUIContainer {
   public:
    UIModList(u32 *flags) : CBaseUIContainer(0, 0, 0, 0, "mod_list") { this->flags = flags; }

    u32 *flags;

    void draw() override;
    bool isVisible() override;
};

class RoomScreen : public OsuScreen {
   public:
    RoomScreen();
    ~RoomScreen() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;
    void onResolutionChange(Vector2 newResolution) override;
    CBaseUIContainer *setVisible(bool visible) override;  // does nothing

    void updateLayout(Vector2 newResolution);
    void updateSettingsLayout(Vector2 newResolution);
    void ragequit(bool play_sound = true);

    void on_map_change();
    void on_room_joined(Room room);
    void on_room_updated(Room room);
    void on_match_started(Room room);
    void on_match_score_updated(Packet *packet);
    void on_all_players_loaded();
    void on_player_failed(i32 slot_id);
    void on_match_finished();
    void on_all_players_skipped();
    void on_player_skip(i32 user_id);
    void on_match_aborted();
    void onClientScoreChange(bool force = false);
    void onReadyButtonClick();

    FinishedScore get_approximate_score();

    // Host only
    void onStartGameClicked();
    void onSelectModsClicked();
    void onSelectMapClicked();
    void onChangePasswordClicked();
    void onChangeWinConditionClicked();
    void onWinConditionSelected(UString win_condition_str, int win_condition);
    void set_new_password(UString new_password);
    void onFreemodCheckboxChanged(CBaseUICheckbox *checkbox);

    CBaseUILabel *map_label = NULL;
    CBaseUILabel *mods_label = NULL;
    CBaseUIScrollView *settings = NULL;
    CBaseUIScrollView *slotlist = NULL;
    CBaseUIScrollView *map = NULL;
    CBaseUILabel *host = NULL;
    CBaseUILabel *room_name = NULL;
    CBaseUILabel *room_name_iptl = NULL;
    CBaseUITextbox *room_name_ipt = NULL;
    UIButton *change_password_btn = NULL;
    CBaseUILabel *win_condition = NULL;
    UIButton *change_win_condition_btn = NULL;
    CBaseUILabel *map_title = NULL;
    CBaseUILabel *map_attributes = NULL;
    CBaseUILabel *map_attributes2 = NULL;
    CBaseUILabel *map_stars = NULL;
    UIButton *select_map_btn = NULL;
    UIButton *select_mods_btn = NULL;
    UICheckbox *freemod = NULL;
    UIModList *mods = NULL;
    CBaseUILabel *no_mods_selected = NULL;
    UIButton *ready_btn = NULL;
    UIContextMenu *contextMenu = NULL;

    MainMenuPauseButton *pauseButton = NULL;
    McFont *font = NULL;
    McFont *lfont = NULL;
    time_t last_packet_tms = {0};
};
