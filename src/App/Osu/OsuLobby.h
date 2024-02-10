#pragma once

// Important clarification: "Lobby" here refers to the place where you look
// for rooms. This is the ppy naming; to remember it, think of the #lobby
// channel the client joins when entering the lobby. See OsuRoom.h if you
// were looking for the 16-player holder thing.

#include "BanchoProtocol.h"
#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

class CBaseUIButton;
class CBaseUILabel;
class OsuLobby;
class OsuUIButton;
class Room;

// NOTE: We make a CBaseUIScrollView but won't enable scrolling.
//       It's just to draw the frame! ^_^
struct RoomUIElement : CBaseUIScrollView {
    RoomUIElement(OsuLobby* multi, Room* room, float x, float y, float width, float height);

    OsuUIButton* join_btn;
    CBaseUIScrollView* ui;
    OsuLobby* m_multi;
    int32_t room_id;

    void updateLayout(Vector2 pos, Vector2 size);
    void onRoomJoinButtonClick(CBaseUIButton* btn);
};

struct OsuLobby : public OsuScreen
{
    OsuLobby(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);

    // /!\ Side-effect: sends bancho packets when changing state
    virtual CBaseUIContainer* setVisible(bool visible);

    void addRoom(Room* room);
    void updateRoom(Room room);
    void removeRoom(uint32_t room_id);
    void updateLayout(Vector2 newResolution);

    void on_room_join_failed();

    std::vector<Room*> rooms;
    CBaseUIContainer *m_container;
    CBaseUIScrollView *m_list;
    CBaseUILabel *m_noRoomsOpenElement;
    McFont* font;
};
