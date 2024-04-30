#pragma once

// Important clarification: "Lobby" here refers to the place where you look
// for rooms. This is the ppy naming; to remember it, think of the #lobby
// channel the client joins when entering the lobby. See Room.h if you
// were looking for the 16-player holder thing.

#include "BanchoProtocol.h"
#include "CBaseUIScrollView.h"
#include "OsuScreen.h"

class CBaseUIButton;
class Lobby;
class UIButton;
class Room;

// NOTE: We make a CBaseUIScrollView but won't enable scrolling.
//       It's just to draw the frame! ^_^
struct RoomUIElement : CBaseUIScrollView {
    RoomUIElement(Lobby* multi, Room* room, float x, float y, float width, float height);

    UIButton* join_btn;
    CBaseUIScrollView* ui;
    Lobby* m_multi;
    i32 room_id;
    bool has_password;

    void updateLayout(Vector2 pos, Vector2 size);
    void onRoomJoinButtonClick(CBaseUIButton* btn);
};

class Lobby : public OsuScreen {
   public:
    Lobby(Osu* osu);

    virtual void onKeyDown(KeyboardEvent& e);
    virtual void onKeyUp(KeyboardEvent& e);
    virtual void onChar(KeyboardEvent& e);
    virtual void onResolutionChange(Vector2 newResolution);

    // /!\ Side-effect: sends bancho packets when changing state
    virtual CBaseUIContainer* setVisible(bool visible);

    void addRoom(Room* room);
    void joinRoom(u32 id, UString password);
    void updateRoom(Room room);
    void removeRoom(u32 room_id);
    void updateLayout(Vector2 newResolution);

    void on_create_room_clicked();

    void on_room_join_with_password(UString password);
    void on_room_join_failed();

    std::vector<Room*> rooms;
    UIButton* m_create_room_btn;
    CBaseUIScrollView* m_list;
    i32 room_to_join;
    McFont* font;
};
