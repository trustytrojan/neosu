#pragma once
#include "CBaseUIWindow.h"

class Sound;
class ConVar;

class CBaseUISlider;

class VSTitleBar;
class VSControlBar;
class VSMusicBrowser;

class VinylScratcher : public CBaseUIWindow {
   public:
    static bool tryPlayFile(std::string filepath);

   public:
    VinylScratcher();
    virtual ~VinylScratcher() { ; }

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

    static Sound *stream2;

    VSTitleBar *titleBar;
    VSControlBar *controlBar;
    VSMusicBrowser *musicBrowser;

    Sound *stream;
    float fReverseMessageTimer;
};
