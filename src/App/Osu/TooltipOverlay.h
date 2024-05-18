#pragma once
#include "OsuScreen.h"

class TooltipOverlay : public OsuScreen {
   public:
    TooltipOverlay();
    virtual ~TooltipOverlay();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void begin();
    void addLine(UString text);
    void end();

   private:
    float m_fAnim;
    std::vector<UString> m_lines;

    bool m_bDelayFadeout;
};
