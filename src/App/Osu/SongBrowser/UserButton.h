#pragma once
#include "CBaseUIButton.h"

class ConVar;

class Osu;
class UIAvatar;

class UserButton : public CBaseUIButton {
   public:
    UserButton(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void updateUserStats();

    void addTooltipLine(UString text) { m_vTooltipLines.push_back(text); }

    UIAvatar *m_avatar = nullptr;

   private:
    virtual void onMouseInside();
    virtual void onMouseOutside();

    ConVar *m_osu_scores_enabled_ref;

    Osu *m_osu;

    float m_fPP;
    float m_fAcc;
    int m_iLevel;
    float m_fPercentToNextLevel;

    float m_fPPDelta;
    float m_fPPDeltaAnim;

    float m_fHoverAnim;
    std::vector<UString> m_vTooltipLines;
};
