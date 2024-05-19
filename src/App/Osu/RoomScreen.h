#pragma once
#include <unistd.h>

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

    CBaseUILabel *map_label = nullptr;
    CBaseUILabel *mods_label = nullptr;
    CBaseUIScrollView *m_settings = nullptr;
    CBaseUIScrollView *m_slotlist = nullptr;
    CBaseUIScrollView *m_map = nullptr;
    CBaseUILabel *m_host = nullptr;
    CBaseUILabel *m_room_name = nullptr;
    CBaseUILabel *m_room_name_iptl = nullptr;
    CBaseUITextbox *m_room_name_ipt = nullptr;
    UIButton *m_change_password_btn = nullptr;
    CBaseUILabel *m_win_condition = nullptr;
    UIButton *m_change_win_condition_btn = nullptr;
    CBaseUILabel *m_map_title = nullptr;
    CBaseUILabel *m_map_attributes = nullptr;
    CBaseUILabel *m_map_attributes2 = nullptr;
    CBaseUILabel *m_map_stars = nullptr;
    UIButton *m_select_map_btn = nullptr;
    UIButton *m_select_mods_btn = nullptr;
    UICheckbox *m_freemod = nullptr;
    UIModList *m_mods = nullptr;
    CBaseUILabel *m_no_mods_selected = nullptr;
    UIButton *m_ready_btn = nullptr;
    UIContextMenu *m_contextMenu = nullptr;

    MainMenuPauseButton *m_pauseButton = nullptr;
    McFont *font = nullptr;
    McFont *lfont = nullptr;
    time_t last_packet_tms = {0};
};
