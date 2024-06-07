#pragma once
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "BanchoProtocol.h"
#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

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
    UIModList(u32 *flags) : CBaseUIContainer(0, 0, 0, 0, "mod_list") { m_flags = flags; }

    u32 *m_flags;

    virtual void draw(Graphics *g) override;
    virtual bool isVisible() override;
};

class RoomScreen : public OsuScreen {
   public:
    RoomScreen();
    ~RoomScreen();

    virtual void draw(Graphics *g) override;
    virtual void mouse_update(bool *propagate_clicks) override;
    virtual void onKeyDown(KeyboardEvent &e) override;
    virtual void onKeyUp(KeyboardEvent &e) override;
    virtual void onChar(KeyboardEvent &e) override;
    virtual void onResolutionChange(Vector2 newResolution) override;
    virtual CBaseUIContainer *setVisible(bool visible) override;  // does nothing

    void updateLayout(Vector2 newResolution);
    void updateSettingsLayout(Vector2 newResolution);
    void ragequit();

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
    CBaseUIScrollView *m_settings = NULL;
    CBaseUIScrollView *m_slotlist = NULL;
    CBaseUIScrollView *m_map = NULL;
    CBaseUILabel *m_host = NULL;
    CBaseUILabel *m_room_name = NULL;
    CBaseUILabel *m_room_name_iptl = NULL;
    CBaseUITextbox *m_room_name_ipt = NULL;
    UIButton *m_change_password_btn = NULL;
    CBaseUILabel *m_win_condition = NULL;
    UIButton *m_change_win_condition_btn = NULL;
    CBaseUILabel *m_map_title = NULL;
    CBaseUILabel *m_map_attributes = NULL;
    CBaseUILabel *m_map_attributes2 = NULL;
    CBaseUILabel *m_map_stars = NULL;
    UIButton *m_select_map_btn = NULL;
    UIButton *m_select_mods_btn = NULL;
    UICheckbox *m_freemod = NULL;
    UIModList *m_mods = NULL;
    CBaseUILabel *m_no_mods_selected = NULL;
    UIButton *m_ready_btn = NULL;
    UIContextMenu *m_contextMenu = NULL;

    MainMenuPauseButton *m_pauseButton = NULL;
    McFont *font = NULL;
    McFont *lfont = NULL;
    time_t last_packet_tms = {0};
};
