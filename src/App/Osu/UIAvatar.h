#pragma once
#include <pthread.h>

#include "CBaseUIButton.h"

class UIAvatar : public CBaseUIButton {
   public:
    UIAvatar(uint32_t player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar();

    virtual void draw(Graphics *g, float alpha = 1.f);

    void onAvatarClicked(CBaseUIButton *btn);

    uint32_t m_player_id;
    std::string avatar_path;
    Image *avatar = nullptr;
    bool on_screen = false;
};
