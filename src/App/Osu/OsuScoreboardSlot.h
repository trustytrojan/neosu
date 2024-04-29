#pragma once

#include "OsuHUD.h"
#include "OsuUIAvatar.h"

struct OsuScoreboardSlot {
    OsuScoreboardSlot(SCORE_ENTRY score, int index);
    ~OsuScoreboardSlot();

    void draw(Graphics *g);
    void updateIndex(int new_index, bool animate);

    OsuUIAvatar *m_avatar = nullptr;
    SCORE_ENTRY m_score;
    int m_index;
    float m_y = 0.f;
    float m_fAlpha = 0.f;
    float m_fFlash = 0.f;
    bool is_friend = false;
    bool was_visible = false;
};
