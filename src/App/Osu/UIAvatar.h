#pragma once

#include "CBaseUIButton.h"

class UIAvatar : public CBaseUIButton {
   public:
    UIAvatar(u32 player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar() override;

    void draw() override { this->draw_avatar(1.f); }
    void draw_avatar(float alpha);

    void onAvatarClicked(CBaseUIButton *btn);

    u32 player_id;
    std::string avatar_path;
    Image *avatar = NULL;
    bool on_screen = false;
};
