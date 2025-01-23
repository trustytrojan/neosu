#pragma once
#include "CBaseUIButton.h"

class ConVar;

class UIAvatar;

class UserCard : public CBaseUIButton {
   public:
    UserCard(i32 user_id);
    ~UserCard();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void updateUserStats();
    void setID(i32 new_id);

   private:
    UIAvatar *avatar = NULL;
    i32 user_id = 0;

    float fPP;
    float fAcc;
    int iLevel;
    float fPercentToNextLevel;

    float fPPDelta;
    float fPPDeltaAnim;
};
