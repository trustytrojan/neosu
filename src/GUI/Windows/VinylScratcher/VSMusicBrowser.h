//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a simple drive and file selector
//
// $NoKeywords: $
//===============================================================================//

#ifndef VSMUSICBROWSER_H
#define VSMUSICBROWSER_H

#include "CBaseUIElement.h"

class McFont;

class CBaseUIScrollView;
class CBaseUIButton;

class VSMusicBrowserButton;

class VSMusicBrowser : public CBaseUIElement {
   public:
    typedef fastdelegate::FastDelegate2<std::string, bool> FileClickedCallback;

   public:
    VSMusicBrowser(int x, int y, int xSize, int ySize, McFont *font);
    ~VSMusicBrowser() override;

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;

    void fireNextSong(bool previous);

    void onInvalidFile();

    void setFileClickedCallback(FileClickedCallback callback) { this->fileClickedCallback = callback; }

   protected:
    void onMoved() override;
    void onResized() override;
    void onDisabled() override;
    void onEnabled() override;
    void onFocusStolen() override;

   private:
    struct COLUMN {
        CBaseUIScrollView *view;
        std::vector<VSMusicBrowserButton *> buttons;

        COLUMN() { this->view = NULL; }
    };

   private:
    void updateFolder(std::string baseFolder, size_t fromDepth);
    void updateDrives();
    void updatePlayingSelection(bool fromInvalidSelection = false);

    void onButtonClicked(CBaseUIButton *button);

    FileClickedCallback fileClickedCallback;

    McFont *font;

    Color defaultTextColor;
    Color playingTextBrightColor;
    Color playingTextDarkColor;

    CBaseUIScrollView *mainContainer;
    std::vector<COLUMN> columns;

    std::string activeSong;
    std::string previousActiveSong;
    std::vector<std::string> playlist;
};

#endif
