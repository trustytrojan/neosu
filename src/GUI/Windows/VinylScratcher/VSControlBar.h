//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		play/pause/forward/shuffle/repeat/eq/settings/volume/time bar
//
// $NoKeywords: $
//===============================================================================//

#ifndef VSCONTROLBAR_H
#define VSCONTROLBAR_H

#include "CBaseUIElement.h"

class McFont;

class CBaseUIContainer;
class CBaseUIButton;
class CBaseUICheckbox;
class CBaseUISlider;

class VSControlBar : public CBaseUIElement {
   public:
    VSControlBar(int x, int y, int xSize, int ySize, McFont *font);
    ~VSControlBar() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    [[nodiscard]] inline CBaseUISlider *getVolumeSlider() const { return this->volume; }
    [[nodiscard]] inline CBaseUIButton *getPlayButton() const { return this->play; }
    [[nodiscard]] inline CBaseUIButton *getPrevButton() const { return this->prev; }
    [[nodiscard]] inline CBaseUIButton *getNextButton() const { return this->next; }
    [[nodiscard]] inline CBaseUIButton *getInfoButton() const { return this->info; }

   protected:
    void onResized() override;
    void onMoved() override;
    void onFocusStolen() override;
    void onEnabled() override;
    void onDisabled() override;

   private:
    void onRepeatCheckboxChanged(CBaseUICheckbox *box);
    void onShuffleCheckboxChanged(CBaseUICheckbox *box);
    void onVolumeChanged(const UString &oldValue, const UString &newValue);

    CBaseUIContainer *container;

    CBaseUISlider *volume;
    CBaseUIButton *play;
    CBaseUIButton *prev;
    CBaseUIButton *next;

    CBaseUIButton *info;

    CBaseUIButton *settings;
    CBaseUICheckbox *shuffle;
    CBaseUICheckbox *eq;
    CBaseUICheckbox *repeat;
};

#endif
