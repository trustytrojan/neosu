#pragma once
#include <unistd.h>

#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

class CBaseUILabel;
class OsuDatabaseBeatmap;
class OsuMainMenuPauseButton;
class OsuUIButton;


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

    std::unordered_map<uint32_t, uint32_t> mapset_by_mapid;

    CBaseUIContainer *m_container = nullptr;
    CBaseUIScrollView *m_settings = nullptr;
    CBaseUIScrollView *m_slotlist = nullptr;
    CBaseUIScrollView *m_map = nullptr;
    CBaseUILabel *m_room_name = nullptr;
    CBaseUILabel *m_map_title = nullptr;

    OsuUIModList *m_mods = nullptr;
    CBaseUILabel *m_no_mods_selected = nullptr;

    OsuUIButton *m_ready_btn = nullptr;
    OsuMainMenuPauseButton *m_pauseButton = nullptr;
    McFont* font = nullptr;
    McFont* lfont = nullptr;
    time_t last_packet_tms = {0};
};
