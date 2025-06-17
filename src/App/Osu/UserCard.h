#pragma once
#include "CBaseUIButton.h"

class ConVar;

class UIAvatar;

class UserCard : public CBaseUIButton {
   public:
    UserCard(u32 user_id);
    ~UserCard();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void updateUserStats();
    void setID(u32 new_id);

    u32 user_id = 0;

   private:
    UIAvatar *avatar = NULL;

    float fPP;
    float fAcc;
    int iLevel;
    float fPercentToNextLevel;

    float fPPDelta;
    float fPPDeltaAnim;
};
