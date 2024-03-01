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

class VSMusicBrowser : public CBaseUIElement
{
public:
	typedef fastdelegate::FastDelegate2<std::string, bool> FileClickedCallback;

public:
	VSMusicBrowser(int x, int y, int xSize, int ySize, McFont *font);
	virtual ~VSMusicBrowser();

	virtual void draw(Graphics *g);
	virtual void mouse_update(bool *propagate_clicks);

	void fireNextSong(bool previous);

	void onInvalidFile();

	void setFileClickedCallback(FileClickedCallback callback) {m_fileClickedCallback = callback;}

protected:
	virtual void onMoved();
	virtual void onResized();
	virtual void onDisabled();
	virtual void onEnabled();
	virtual void onFocusStolen();

private:
	struct COLUMN
	{
		CBaseUIScrollView *view;
		std::vector<VSMusicBrowserButton*> buttons;

		COLUMN()
		{
			view = NULL;
		}
	};

private:
	void updateFolder(std::string baseFolder, size_t fromDepth);
	void updateDrives();
	void updatePlayingSelection(bool fromInvalidSelection = false);

	void onButtonClicked(CBaseUIButton *button);

	FileClickedCallback m_fileClickedCallback;

	McFont *m_font;

	Color m_defaultTextColor;
	Color m_playingTextBrightColor;
	Color m_playingTextDarkColor;

	CBaseUIScrollView *m_mainContainer;
	std::vector<COLUMN> m_columns;

	std::string m_activeSong;
	std::string m_previousActiveSong;
	std::vector<std::string> m_playlist;
};

#endif
