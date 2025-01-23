#pragma once
#include "CBaseUILabel.h"
#include "OsuScreen.h"

class UIContextMenu;

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

class UIUserContextMenuScreen : public OsuScreen {
   public:
    UIUserContextMenuScreen();

    virtual void onResolutionChange(Vector2 newResolution);
    virtual void stealFocus();

    void open(u32 user_id);
    void close();
    void on_action(UString text, int user_action);

    u32 user_id;
    UIContextMenu *menu = NULL;
};

class UIUserLabel : public CBaseUILabel {
   public:
    UIUserLabel(u32 user_id, UString username);

    virtual void onMouseUpInside();

    u32 user_id;
};
