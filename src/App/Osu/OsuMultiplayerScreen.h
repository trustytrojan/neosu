#pragma once

#include "BanchoProtocol.h"
#include "CBaseUIScrollView.h"
#include "OsuScreen.h"


class CBaseUIButton;
class OsuMultiplayerScreen;
class Room;

// NOTE: We make a CBaseUIScrollView but won't enable scrolling.
//       It's just to draw the frame! ^_^
struct RoomUIElement : CBaseUIScrollView {
    RoomUIElement(OsuMultiplayerScreen* multi, Room* room, float x, float y, float width, float height);

    CBaseUIScrollView* ui;
    OsuMultiplayerScreen* m_multi;
    uint32_t room_id;

    void updateLayout(Vector2 pos, Vector2 size);
    void onRoomJoinButtonClick(CBaseUIButton* btn);
};

struct OsuMultiplayerScreen : public OsuScreen
{
    OsuMultiplayerScreen(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void update();
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);

    // /!\ Side-effect: sends bancho packets when changing state
    virtual void setVisible(bool visible);

    void addRoom(Room* room);
    void updateRoom(Room* room);
    void removeRoom(uint32_t room_id);
    void updateLayout(Vector2 newResolution);

    std::vector<Room*> rooms;
    CBaseUIContainer *m_container;
    CBaseUIScrollView *m_list;
    McFont* font;
};
