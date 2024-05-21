#pragma once

#include "HUD.h"
#include "UIAvatar.h"

struct ScoreboardSlot {
    ScoreboardSlot(SCORE_ENTRY score, int index);
    ~ScoreboardSlot();

    void draw(Graphics *g);
    void updateIndex(int new_index, bool animate);

    UIAvatar *m_avatar = NULL;
    SCORE_ENTRY m_score;
    int m_index;
    float m_y = 0.f;
    float m_fAlpha = 0.f;
    float m_fFlash = 0.f;
    bool is_friend = false;
    bool was_visible = false;
};
