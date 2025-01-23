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
    virtual ~VSControlBar();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    inline CBaseUISlider *getVolumeSlider() const { return this->volume; }
    inline CBaseUIButton *getPlayButton() const { return this->play; }
    inline CBaseUIButton *getPrevButton() const { return this->prev; }
    inline CBaseUIButton *getNextButton() const { return this->next; }
    inline CBaseUIButton *getInfoButton() const { return this->info; }

   protected:
    virtual void onResized();
    virtual void onMoved();
    virtual void onFocusStolen();
    virtual void onEnabled();
    virtual void onDisabled();

   private:
    void onRepeatCheckboxChanged(CBaseUICheckbox *box);
    void onShuffleCheckboxChanged(CBaseUICheckbox *box);
    void onVolumeChanged(UString oldValue, UString newValue);

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
