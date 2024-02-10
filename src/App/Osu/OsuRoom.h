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


struct OsuUIModList : public CBaseUIContainer {
    OsuUIModList() : CBaseUIContainer(0, 0, 0, 0, "mod_list") {}
    virtual void draw(Graphics *g);
    virtual bool isVisible() override;
};

struct OsuRoom : public OsuScreen {
    OsuRoom(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);
    virtual CBaseUIContainer* setVisible(bool visible); // does nothing

    void updateLayout(Vector2 newResolution);
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
    void onFreemodCheckboxChanged(CBaseUICheckbox *checkbox);

    std::unordered_map<uint32_t, uint32_t> mapset_by_mapid;

    CBaseUIScrollView *m_settings = nullptr;
    CBaseUIScrollView *m_slotlist = nullptr;
    CBaseUIScrollView *m_map = nullptr;
    CBaseUILabel *m_host = nullptr;
    CBaseUILabel *m_room_name = nullptr;
    CBaseUILabel *m_room_name_iptl = nullptr;
    CBaseUITextbox *m_room_name_ipt = nullptr;
    // CBaseUILabel *m_room_password_iptl = nullptr;
    // CBaseUITextbox *m_room_password_ipt = nullptr;
    CBaseUILabel *m_map_title = nullptr;
    CBaseUILabel *m_map_attributes = nullptr;
    CBaseUILabel *m_map_stars = nullptr;
    OsuUIButton *m_select_map_btn = nullptr;
    OsuUIButton *m_select_mods_btn = nullptr;
    OsuUICheckbox *m_freemod = nullptr;
    OsuUIModList *m_mods = nullptr;
    CBaseUILabel *m_no_mods_selected = nullptr;
    OsuUIButton *m_ready_btn = nullptr;

    OsuMainMenuPauseButton *m_pauseButton = nullptr;
    McFont* font = nullptr;
    McFont* lfont = nullptr;
    time_t last_packet_tms = {0};
};
