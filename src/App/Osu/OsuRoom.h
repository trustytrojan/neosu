#pragma once
#include <unistd.h>

#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

class CBaseUICheckbox;
class CBaseUILabel;
class CBaseUITextbox;
class OsuDatabaseBeatmap;
class OsuMainMenuPauseButton;
class OsuUIButton;
class OsuUICheckbox;
class OsuUIContextMenu;


class OsuUIModList : public CBaseUIContainer {
public:
    OsuUIModList(uint32_t *flags) : CBaseUIContainer(0, 0, 0, 0, "mod_list") {
        m_flags = flags;
    }

    uint32_t *m_flags;

    virtual void draw(Graphics *g) override;
    virtual bool isVisible() override;
};

class OsuRoom : public OsuScreen {
public:
    OsuRoom(Osu *osu);

    virtual void draw(Graphics *g) override;
    virtual void mouse_update(bool *propagate_clicks) override;
    virtual void onKeyDown(KeyboardEvent &e) override;
    virtual void onKeyUp(KeyboardEvent &e) override;
    virtual void onChar(KeyboardEvent &e) override;
    virtual void onResolutionChange(Vector2 newResolution) override;
    virtual CBaseUIContainer* setVisible(bool visible) override; // does nothing

    void updateLayout(Vector2 newResolution);
    void updateSettingsLayout(Vector2 newResolution);
    void ragequit();

    static void process_beatmapset_info_response(Packet packet);
    void on_map_change(bool download = true);
    void on_room_joined(Room room);
    void on_room_updated(Room room);
    void on_match_started(Room room);
    void on_match_score_updated(Packet* packet);
    void on_all_players_loaded();
    void on_player_failed(int32_t slot_id);
    void on_match_finished();
    void on_all_players_skipped();
    void on_player_skip(int32_t user_id);
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

    std::unordered_map<uint32_t, uint32_t> mapset_by_mapid;

    CBaseUILabel *map_label = nullptr;
    CBaseUILabel *mods_label = nullptr;
    CBaseUIScrollView *m_settings = nullptr;
    CBaseUIScrollView *m_slotlist = nullptr;
    CBaseUIScrollView *m_map = nullptr;
    CBaseUILabel *m_host = nullptr;
    CBaseUILabel *m_room_name = nullptr;
    CBaseUILabel *m_room_name_iptl = nullptr;
    CBaseUITextbox *m_room_name_ipt = nullptr;
    OsuUIButton *m_change_password_btn = nullptr;
    CBaseUILabel *m_win_condition = nullptr;
    OsuUIButton *m_change_win_condition_btn = nullptr;
    CBaseUILabel *m_map_title = nullptr;
    CBaseUILabel *m_map_attributes = nullptr;
    CBaseUILabel *m_map_attributes2 = nullptr;
    CBaseUILabel *m_map_stars = nullptr;
    OsuUIButton *m_select_map_btn = nullptr;
    OsuUIButton *m_select_mods_btn = nullptr;
    OsuUICheckbox *m_freemod = nullptr;
    OsuUIModList *m_mods = nullptr;
    CBaseUILabel *m_no_mods_selected = nullptr;
    OsuUIButton *m_ready_btn = nullptr;
    OsuUIContextMenu *m_contextMenu = nullptr;

    OsuMainMenuPauseButton *m_pauseButton = nullptr;
    McFont* font = nullptr;
    McFont* lfont = nullptr;
    time_t last_packet_tms = {0};
};
