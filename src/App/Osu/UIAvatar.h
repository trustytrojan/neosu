#pragma once
#include <pthread.h>

#include "CBaseUIButton.h"

class UIAvatar : public CBaseUIButton {
   public:
    UIAvatar(u32 player_id, float xPos, float yPos, float xSize, float ySize);
    ~UIAvatar();

    virtual void draw(Graphics *g) { draw_avatar(g, 1.f); }
    void draw_avatar(Graphics *g, float alpha);

    void onAvatarClicked(CBaseUIButton *btn);

    u32 m_player_id;
    std::string avatar_path;
    Image *avatar = NULL;
    bool on_screen = false;
};
