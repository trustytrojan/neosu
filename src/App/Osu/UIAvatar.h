#pragma once
// Copyright (c) 2024, kiwec, All rights reserved.

#include "CBaseUIButton.h"

class UIAvatar : public CBaseUIButton {
   public:
    UIAvatar(i32 player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar() override;

    void draw() override { this->draw_avatar(1.f); }
    void draw_avatar(float alpha);

    void onAvatarClicked(CBaseUIButton *btn);

    i32 player_id;
    std::string avatar_path;
    Image *avatar = NULL;
    bool on_screen = false;
};
