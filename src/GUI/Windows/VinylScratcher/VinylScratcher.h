//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a music player with style
//
// $NoKeywords: $vs
//===============================================================================//

#ifndef VINYLSCRATCHER_H
#define VINYLSCRATCHER_H

#include "CBaseUIWindow.h"

class Sound;
class ConVar;

class CBaseUISlider;

class VSTitleBar;
class VSControlBar;
class VSMusicBrowser;

class VinylScratcher : public CBaseUIWindow
{
public:
	static bool tryPlayFile(std::string filepath);

public:
	VinylScratcher();
	virtual ~VinylScratcher() {;}

	virtual void mouse_update(bool *propagate_clicks);

	virtual void onKeyDown(KeyboardEvent &e);

protected:
	virtual void onResized();

private:
	void onFinished();
	void onFileClicked(std::string filepath, bool reverse);
	void onVolumeChanged(CBaseUISlider *slider);
	void onSeek();
	void onPlayClicked();
	void onNextClicked();
	void onPrevClicked();

	static Sound *m_stream2;

	VSTitleBar *m_titleBar;
	VSControlBar *m_controlBar;
	VSMusicBrowser *m_musicBrowser;

	Sound *m_stream;
	float m_fReverseMessageTimer;

	ConVar *m_vs_percent_ref;
};

#endif
