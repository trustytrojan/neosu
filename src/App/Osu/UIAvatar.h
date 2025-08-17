#pragma once
// Copyright (c) 2024, kiwec, All rights reserved.

#include "CBaseUIButton.h"

class UIAvatar final : public CBaseUIButton {
   public:
    UIAvatar(i32 player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar() override;

    UIAvatar &operator=(const UIAvatar &) = delete;
    UIAvatar &operator=(UIAvatar &&) = delete;
    UIAvatar(const UIAvatar &) = delete;
    UIAvatar(UIAvatar &&) = delete;

    void draw() override { this->draw_avatar(1.f); }
    void draw_avatar(float alpha);

    void onAvatarClicked(CBaseUIButton *btn);

    std::pair<i32, std::string> player_id_for_endpoint;
    bool on_screen{false};
};
