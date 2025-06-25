#pragma once
#include "CBaseUIButton.h"

class ConVar;

class UIAvatar;
struct UserInfo;

class UserCard2 : public CBaseUIButton {
   public:
    UserCard2(u32 user_id);
    ~UserCard2() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onClick(CBaseUIButton *btn);

    UserInfo *info = NULL;
    UIAvatar *avatar = NULL;
};
