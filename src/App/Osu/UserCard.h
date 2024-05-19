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
    UIAvatar *m_avatar = nullptr;
    i32 m_user_id = 0;

    ConVar *m_osu_scores_enabled_ref;

    float m_fPP;
    float m_fAcc;
    int m_iLevel;
    float m_fPercentToNextLevel;

    float m_fPPDelta;
    float m_fPPDeltaAnim;
};
