#pragma once
#include "CBaseUILabel.h"
#include "OsuScreen.h"

class Osu;
class OsuUIContextMenu;

enum UserActions {
    UA_TRANSFER_HOST,
    KICK,
    VIEW_PROFILE,
    TOGGLE_SPECTATE,
    START_CHAT,
    INVITE_TO_GAME,
    UA_ADD_FRIEND,
    UA_REMOVE_FRIEND,
};

class OsuUIUserContextMenuScreen : public OsuScreen {
   public:
    OsuUIUserContextMenuScreen(Osu *osu);

    virtual void onResolutionChange(Vector2 newResolution);
    virtual void stealFocus();

    void open(uint32_t user_id);
    void close();
    void on_action(UString text, int user_action);

    uint32_t m_user_id;
    OsuUIContextMenu *menu = nullptr;
};

class OsuUIUserLabel : public CBaseUILabel {
   public:
    OsuUIUserLabel(Osu *osu, uint32_t user_id, UString username);

    virtual void onMouseUpInside();

    uint32_t m_user_id;
};
