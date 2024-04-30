#pragma once
#include "CBaseUILabel.h"
#include "OsuScreen.h"

class Osu;
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
    UIUserContextMenuScreen(Osu *osu);

    virtual void onResolutionChange(Vector2 newResolution);
    virtual void stealFocus();

    void open(uint32_t user_id);
    void close();
    void on_action(UString text, int user_action);

    uint32_t m_user_id;
    UIContextMenu *menu = nullptr;
};

class UIUserLabel : public CBaseUILabel {
   public:
    UIUserLabel(Osu *osu, uint32_t user_id, UString username);

    virtual void onMouseUpInside();

    uint32_t m_user_id;
};
