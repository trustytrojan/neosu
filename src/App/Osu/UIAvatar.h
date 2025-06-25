#pragma once

#include "CBaseUIButton.h"

class UIAvatar : public CBaseUIButton {
   public:
    UIAvatar(u32 player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar() override;

    void draw(Graphics *g) override { this->draw_avatar(g, 1.f); }
    void draw_avatar(Graphics *g, float alpha);

    void onAvatarClicked(CBaseUIButton *btn);

    u32 player_id;
    std::string avatar_path;
    Image *avatar = NULL;
    bool on_screen = false;
};
