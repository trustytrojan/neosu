#pragma once
#include "OsuScreen.h"

class TooltipOverlay : public OsuScreen {
   public:
    TooltipOverlay();
    ~TooltipOverlay() override;

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;

    void begin();
    void addLine(UString text);
    void end();

   private:
    float fAnim;
    std::vector<UString> lines;

    bool bDelayFadeout;
};
