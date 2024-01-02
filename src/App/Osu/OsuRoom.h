#pragma once

#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

struct OsuRoom : public OsuScreen {
    OsuRoom(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void update();
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);
    virtual void setVisible(bool visible); // does nothing

    void updateLayout(Vector2 newResolution);

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

    CBaseUIContainer *m_container;
    CBaseUIScrollView *m_slotlist;
    McFont* font;
};
