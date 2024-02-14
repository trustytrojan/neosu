#pragma once
#include <pthread.h>
#include "CBaseUIButton.h"

struct OsuUIAvatar : public CBaseUIButton {
    OsuUIAvatar(uint32_t player_id, float xPos, float yPos, float xSize, float ySize);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void onAvatarClicked(CBaseUIButton *btn);

    uint32_t m_player_id;
    UString avatar_path;
    Image *avatar = nullptr;
    bool is_loading = true;
    bool on_screen = false;
};

// Accessed from BanchoNetworking
extern int avatar_downloading_thread_id;
extern pthread_mutex_t avatars_mtx;
void* avatar_downloading_thread(void *arg);
