#include "VinylScratcher.h"

#include <utility>

#include "AnimationHandler.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUISlider.h"
#include "ConVar.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "VSControlBar.h"
#include "VSMusicBrowser.h"
#include "VSTitleBar.h"



Sound *VinylScratcher::stream2 = NULL;

VinylScratcher::VinylScratcher() : CBaseUIWindow(220, 90, 1000, 700, "Vinyl Scratcher") {
    const float dpiScale = env->getDPIScale();
    const int baseDPI = 96;
    const int newDPI = dpiScale * baseDPI;

    McFont *font = resourceManager->getFont("FONT_DEFAULT");
    McFont *windowTitleFont =
        resourceManager->loadFont("ubuntu.ttf", "FONT_VS_WINDOW_TITLE", 10.0f, true, newDPI);
    McFont *controlBarFont = font;
    McFont *musicBrowserFont = font;

    this->titleBar = new VSTitleBar(0, 0, this->vSize.x + 2, font);
    this->controlBar = new VSControlBar(0, this->vSize.y - this->getTitleBarHeight() - this->titleBar->getSize().y,
                                        this->vSize.x, this->titleBar->getSize().y, controlBarFont);
    this->musicBrowser = new VSMusicBrowser(
        0, this->titleBar->getRelPos().y + this->titleBar->getSize().y, this->vSize.x,
        this->vSize.y - this->controlBar->getSize().y - this->titleBar->getSize().y - this->getTitleBarHeight() - 1,
        musicBrowserFont);

    this->getContainer()->addBaseUIElement(this->musicBrowser);
    this->getContainer()->addBaseUIElement(this->controlBar);
    this->getContainer()->addBaseUIElement(this->titleBar);

    this->controlBar->getPlayButton()->setClickCallback(
        fastdelegate::MakeDelegate(this, &VinylScratcher::onPlayClicked));
    this->controlBar->getNextButton()->setClickCallback(
        fastdelegate::MakeDelegate(this, &VinylScratcher::onNextClicked));
    this->controlBar->getPrevButton()->setClickCallback(
        fastdelegate::MakeDelegate(this, &VinylScratcher::onPrevClicked));
    this->musicBrowser->setFileClickedCallback(fastdelegate::MakeDelegate(this, &VinylScratcher::onFileClicked));
    this->titleBar->setSeekCallback(fastdelegate::MakeDelegate(this, &VinylScratcher::onSeek));
    this->controlBar->getVolumeSlider()->setChangeCallback(
        fastdelegate::MakeDelegate(this, &VinylScratcher::onVolumeChanged));

    // vars
    this->stream = resourceManager->loadSoundAbs("", "SND_VS_STREAM", true);
    VinylScratcher::stream2 = resourceManager->loadSoundAbs("", "SND_VS_STREAM2", true);
    this->fReverseMessageTimer = 0.0f;

    // window colors
    this->setBackgroundColor(0xffffffff);
    this->setTitleColor(0xff000000);
    this->setFrameColor(0xffcccccc);
    this->setTitleColor(0xff555555);
    this->getCloseButton()->setFrameColor(0xff888888);
    this->getMinimizeButton()->setFrameColor(0xff888888);

    // window settings
    this->setResizeLimit(240 * dpiScale, 160 * dpiScale);
    this->setTitleFont(windowTitleFont);
    this->getCloseButton()->setDrawBackground(false);
    this->getMinimizeButton()->setDrawBackground(false);

    // open();
}

void VinylScratcher::mouse_update(bool *propagate_clicks) {
    CBaseUIWindow::mouse_update(propagate_clicks);
    {
        // NOTE: do this even if the window is not visible
        if(this->stream->isFinished()) this->onFinished();
    }
    if(!this->bVisible) return;

    // restore title text after timer
    if(this->fReverseMessageTimer != 0.0f && engine->getTime() > this->fReverseMessageTimer) {
        this->fReverseMessageTimer = 0.0f;
        auto title = UString(this->stream->getFilePath().length() > 1
                                 ? env->getFileNameFromFilePath(this->stream->getFilePath()).c_str()
                                 : "Ready");
        this->titleBar->setTitle(title, true);
    }

    // update seekbar
    if(this->stream->isPlaying() && !this->titleBar->isSeeking()) cv_vs_percent.setValue(this->stream->getPosition());

    // update info text
    {
        unsigned long lengthMS = this->stream->getLengthMS();
        unsigned long positionMS = this->stream->getPositionMS();
        if(this->titleBar->isSeeking()) positionMS = (unsigned long)((float)lengthMS * cv_vs_percent.getFloat());

        this->controlBar->getInfoButton()->setText(UString::format("  %i:%02i / %i:%02i", (positionMS / 1000) / 60,
                                                                   (positionMS / 1000) % 60, (lengthMS / 1000) / 60,
                                                                   (lengthMS / 1000) % 60));
    }
}

void VinylScratcher::onKeyDown(KeyboardEvent &e) {
    CBaseUIWindow::onKeyDown(e);

    if(!this->bVisible) return;

    // hotkeys
    if(e == KEY_LEFT || e == KEY_UP || e == KEY_A) this->onPrevClicked();
    if(e == KEY_SPACE || e == KEY_ENTER || e == KEY_NUMPAD_ENTER) this->onPlayClicked();
    if(e == KEY_RIGHT || e == KEY_DOWN || e == KEY_D) this->onNextClicked();
}

void VinylScratcher::onFinished() {
    if(cv_vs_repeat.getBool())
        soundEngine->play(this->stream);
    else {
        // reset and stop (since we can't know yet if there even is a next song)
        this->stream->setPosition(0);
        soundEngine->pause(this->stream);
        this->controlBar->getPlayButton()->setText(">");

        // play the next song
        this->musicBrowser->fireNextSong(false);
    }

    // update seekbar
    cv_vs_percent.setValue(0.0f);
}

void VinylScratcher::onFileClicked(std::string filepath, bool reverse) {
    // check if the file is valid and can be played, if it's valid then play it
    if(tryPlayFile(filepath)) {
        soundEngine->stop(this->stream);

        this->stream->rebuild(filepath);
        this->stream->setVolume(this->controlBar->getVolumeSlider()->getFloat());

        if(soundEngine->play(this->stream)) {
            this->controlBar->getPlayButton()->setText("II");
            auto title = UString(env->getFileNameFromFilePath(filepath).c_str());
            this->titleBar->setTitle(title, reverse);

            if(this->fReverseMessageTimer > 0.0f) this->fReverseMessageTimer = 0.0f;
        }
    } else {
        if(this->fReverseMessageTimer == 0.0f) {
            this->titleBar->setTitle("Not a valid audio file!");
            this->fReverseMessageTimer = engine->getTime() + 1.5f;
        }
        this->musicBrowser->onInvalidFile();
    }
}

void VinylScratcher::onVolumeChanged(CBaseUISlider *slider) { this->stream->setVolume(slider->getFloat()); }

void VinylScratcher::onSeek() { this->stream->setPosition(cv_vs_percent.getFloat()); }

void VinylScratcher::onPlayClicked() {
    if(this->stream->isPlaying()) {
        soundEngine->pause(this->stream);
        this->controlBar->getPlayButton()->setText(">");
    } else {
        if(soundEngine->play(this->stream)) this->controlBar->getPlayButton()->setText("II");
    }
}

void VinylScratcher::onNextClicked() { this->musicBrowser->fireNextSong(false); }

void VinylScratcher::onPrevClicked() { this->musicBrowser->fireNextSong(true); }

void VinylScratcher::onResized() {
    CBaseUIWindow::onResized();

    this->titleBar->setSizeX(this->vSize.x + 2);
    this->controlBar->setSizeX(this->vSize.x);
    this->controlBar->setRelPosY(this->vSize.y - this->getTitleBarHeight() - this->controlBar->getSize().y);
    this->musicBrowser->setSize(this->vSize.x, this->vSize.y - this->controlBar->getSize().y -
                                                   this->titleBar->getSize().y - this->getTitleBarHeight());

    this->getContainer()->update_pos();
}

bool VinylScratcher::tryPlayFile(std::string filepath) {
    VinylScratcher::stream2->rebuild(std::move(filepath));

    if(soundEngine->play(VinylScratcher::stream2)) {
        soundEngine->stop(VinylScratcher::stream2);
        return true;
    }

    return false;
}
