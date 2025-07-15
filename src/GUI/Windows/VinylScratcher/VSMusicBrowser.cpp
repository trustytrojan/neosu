#include "VSMusicBrowser.h"

#include <utility>

#include "AnimationHandler.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "ConVar.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"
#include "VinylScratcher.h"

struct VSMusicBrowserNaturalSortStringComparator {
    // heavily modified version of https://github.com/scopeInfinity/NaturalSort

    static bool isDigit(const std::string::value_type &x) { return isdigit(x); }
    static bool isNotDigit(const std::string::value_type &x) { return !isDigit(x); }

    static bool compareCharactersLessThanIgnoreCase(const std::string::value_type &lhs,
                                                    const std::string::value_type &rhs) {
        return (tolower(lhs) < tolower(rhs));
    }

    static int compareIteratorsLessThanIgnoreCase(const std::string::const_iterator &lhs,
                                                  const std::string::const_iterator &rhs) {
        if(compareCharactersLessThanIgnoreCase(*lhs, *rhs)) return -1;
        if(compareCharactersLessThanIgnoreCase(*rhs, *lhs)) return 1;

        return 0;
    }

    static int compareNumbers(std::string::const_iterator lhsBegin, std::string::const_iterator lhsEnd,
                              bool isFractionalPart1, std::string::const_iterator rhsBegin,
                              std::string::const_iterator rhsEnd, bool isFractionalPart2) {
        if(isFractionalPart1 && !isFractionalPart2) return true;
        if(!isFractionalPart1 && isFractionalPart2) return false;

        if(isFractionalPart1) {
            while(lhsBegin < lhsEnd && rhsBegin < rhsEnd) {
                const int result = compareIteratorsLessThanIgnoreCase(lhsBegin, rhsBegin);
                if(result != 0) return result;

                lhsBegin++;
                rhsBegin++;
            }

            // skip intermediate zeroes
            while(lhsBegin < lhsEnd && *lhsBegin == '0') {
                lhsBegin++;
            }
            while(rhsBegin < rhsEnd && *rhsBegin == '0') {
                rhsBegin++;
            }

            if(lhsBegin == lhsEnd && rhsBegin != rhsEnd)
                return -1;
            else if(lhsBegin != lhsEnd && rhsBegin == rhsEnd)
                return 1;
            else
                return 0;
        } else {
            // skip initial zeroes
            while(lhsBegin < lhsEnd && *lhsBegin == '0') {
                lhsBegin++;
            }
            while(rhsBegin < rhsEnd && *rhsBegin == '0') {
                rhsBegin++;
            }

            // compare length of both strings
            if(lhsEnd - lhsBegin < rhsEnd - rhsBegin) return -1;
            if(lhsEnd - lhsBegin > rhsEnd - rhsBegin) return 1;

            // (equal in length, continue as normal)
            while(lhsBegin < lhsEnd) {
                const int result = compareIteratorsLessThanIgnoreCase(lhsBegin, rhsBegin);
                if(result != 0) return result;

                lhsBegin++;
                rhsBegin++;
            }

            return 0;
        }
    }

    static bool compareStringsNatural(const std::string &first, const std::string &second) {
        const std::string::const_iterator &lhsBegin = first.begin();
        const std::string::const_iterator &lhsEnd = first.end();
        const std::string::const_iterator &rhsBegin = second.begin();
        const std::string::const_iterator &rhsEnd = second.end();

        std::string::const_iterator current1 = lhsBegin;
        std::string::const_iterator current2 = rhsBegin;

        bool foundSpace1 = false;
        bool foundSpace2 = false;

        while(current1 != lhsEnd && current2 != rhsEnd) {
            // ignore more than one continuous space character
            {
                while(foundSpace1 && current1 != lhsEnd && *current1 == ' ') {
                    current1++;
                }
                foundSpace1 = (*current1 == ' ');

                while(foundSpace2 && current2 != rhsEnd && *current2 == ' ') {
                    current2++;
                }
                foundSpace2 = (*current2 == ' ');
            }

            if(!isDigit(*current1) || !isDigit(*current2)) {
                // normal comparison for non-digit characters
                if(compareCharactersLessThanIgnoreCase(*current1, *current2)) return true;
                if(compareCharactersLessThanIgnoreCase(*current2, *current1)) return false;

                current1++;
                current2++;
            } else {
                // comparison for digit characters (including well formed fractions)
                std::string::const_iterator lastNonDigit1 = std::find_if(current1, lhsEnd, isNotDigit);
                std::string::const_iterator lastNonDigit2 = std::find_if(current2, rhsEnd, isNotDigit);

                const int result =
                    compareNumbers(current1, lastNonDigit1, (current1 > lhsBegin && *(current1 - 1) == '.'), current2,
                                   lastNonDigit2, (current2 > rhsBegin && *(current2 - 1) == '.'));
                if(result < 0) return true;
                if(result > 0) return false;

                current1 = lastNonDigit1;
                current2 = lastNonDigit2;
            }
        }

        if(current1 == lhsEnd && current2 == rhsEnd)
            return false;
        else
            return (current1 == lhsEnd);
    }

    bool operator()(std::string const &a, std::string const &b) const { return compareStringsNatural(a, b); }
};

class VSMusicBrowserButton : public CBaseUIButton {
   public:
    VSMusicBrowserButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
        this->bSelected = false;
        this->bIsDirectory = false;
        this->bPlaying = false;

        this->fSelectionAnim = 0.0f;
    }

    ~VSMusicBrowserButton() override { anim->deleteExistingAnimation(&this->fSelectionAnim); }

    void draw() override {
        if(!this->bVisible) return;

        const bool isAnimatingSelectionAnim = anim->isAnimating(&this->fSelectionAnim);

        // draw regular line
        if(!this->bSelected && !isAnimatingSelectionAnim) {
            g->setColor(this->frameColor);
            g->drawLine(this->vPos.x + this->vSize.x, this->vPos.y, this->vPos.x + this->vSize.x,
                        this->vPos.y + this->vSize.y);
            g->drawLine(this->vPos.x + this->vSize.x - 1, this->vPos.y, this->vPos.x + this->vSize.x - 1,
                        this->vPos.y + this->vSize.y);
        }

        // draw animated arrow and/or animated splitting line
        // NOTE: the arrow relies on the height being evenly divisible by 2 in order for the lines to not appear
        // crooked/aliased
        if(this->bSelected || isAnimatingSelectionAnim) {
            g->setColor(this->frameColor);
            g->setAlpha(this->fSelectionAnim * this->fSelectionAnim);

            g->pushClipRect(
                McRect(this->vPos.x + this->vSize.x / 2.0f, this->vPos.y, this->vSize.x / 2.0f + 2, this->vSize.y + 1));
            {
                float xAdd = (1.0f - this->fSelectionAnim) * (this->vSize.y / 2.0f);

                g->drawLine(this->vPos.x + this->vSize.x + xAdd, this->vPos.y,
                            this->vPos.x + this->vSize.x - this->vSize.y / 2.0f + xAdd,
                            this->vPos.y + this->vSize.y / 2.0f);
                g->drawLine(this->vPos.x + this->vSize.x - this->vSize.y / 2.0f + xAdd,
                            this->vPos.y + this->vSize.y / 2.0f, this->vPos.x + this->vSize.x + xAdd,
                            this->vPos.y + this->vSize.y);

                g->drawLine(this->vPos.x + this->vSize.x - 1 + xAdd, this->vPos.y,
                            this->vPos.x + this->vSize.x - this->vSize.y / 2.0f - 1 + xAdd,
                            this->vPos.y + this->vSize.y / 2.0f);
                g->drawLine(this->vPos.x + this->vSize.x - this->vSize.y / 2.0f - 1 + xAdd,
                            this->vPos.y + this->vSize.y / 2.0f, this->vPos.x + this->vSize.x - 1 + xAdd,
                            this->vPos.y + this->vSize.y);

                if(this->fSelectionAnim < 1.0f) {
                    g->setAlpha(1.0f);

                    g->drawLine(this->vPos.x + this->vSize.x, this->vPos.y, this->vPos.x + this->vSize.x,
                                this->vPos.y + (this->vSize.y / 2.0f) * (1.0f - this->fSelectionAnim));
                    g->drawLine(this->vPos.x + this->vSize.x - 1, this->vPos.y, this->vPos.x + this->vSize.x - 1,
                                this->vPos.y + (this->vSize.y / 2.0f) * (1.0f - this->fSelectionAnim));

                    g->drawLine(this->vPos.x + this->vSize.x,
                                this->vPos.y + this->vSize.y / 2.0f + (this->vSize.y / 2.0f) * (this->fSelectionAnim),
                                this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y);
                    g->drawLine(this->vPos.x + this->vSize.x - 1,
                                this->vPos.y + this->vSize.y / 2.0f + (this->vSize.y / 2.0f) * (this->fSelectionAnim),
                                this->vPos.x + this->vSize.x - 1, this->vPos.y + this->vSize.y);
                }
            }
            g->popClipRect();
        }

        // highlight currently playing
        if(this->bPlaying) {
            float alpha = std::abs(std::sin(engine->getTime() * 3));

            // don't animate unnecessarily while not in focus/foreground
            if(!engine->hasFocus()) alpha = 0.75f;

            g->setColor(argb((int)(alpha * 50.0f), 0, 196, 223));
            g->fillRect(this->vPos.x, this->vPos.y - 1, this->vSize.x, this->vSize.y + 2);
        }

        // g->setColor(0xffff0000);
        // g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);

        this->drawText();
    }

    void setSelected(bool selected) {
        if(selected && !this->bSelected)
            anim->moveQuadInOut(&this->fSelectionAnim, 1.0f, cv::vs_browser_animspeed.getFloat(), 0.0f, true);
        else if(!selected)
            anim->moveQuadInOut(&this->fSelectionAnim, 0.0f, cv::vs_browser_animspeed.getFloat(), 0.0f, true);

        this->bSelected = selected;
    }

    void setDirectory(bool directory) { this->bIsDirectory = directory; }
    void setPlaying(bool playing) { this->bPlaying = playing; }

    [[nodiscard]] inline bool isDirectory() const { return this->bIsDirectory; }

   private:
    bool bSelected;
    bool bIsDirectory;
    bool bPlaying;

    float fSelectionAnim;
};

class VSMusicBrowserColumnScrollView : public CBaseUIScrollView {
   public:
    VSMusicBrowserColumnScrollView(float xPos, float yPos, float xSize, float ySize, const UString& name)
        : CBaseUIScrollView(xPos, yPos, xSize, ySize, name) {
        this->fAnim = 0.0f;

        // spawn animation
        anim->moveQuadInOut(&this->fAnim, 1.0f, cv::vs_browser_animspeed.getFloat(), 0.0f, true);
    }

    ~VSMusicBrowserColumnScrollView() override { anim->deleteExistingAnimation(&this->fAnim); }

    void draw() override {
        if(anim->isAnimating(&this->fAnim)) {
            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y));
            {
                g->offset3DScene(-this->vSize.x / 2, 0, 0);
                g->rotate3DScene(0, 100 - this->fAnim * 100, 0);
                CBaseUIScrollView::draw();
            }
            g->pop3DScene();
        } else
            CBaseUIScrollView::draw();
    }

   private:
    float fAnim;
};

VSMusicBrowser::VSMusicBrowser(int x, int y, int xSize, int ySize, McFont *font)
    : CBaseUIElement(x, y, xSize, ySize, "") {
    this->font = font;

    this->defaultTextColor = argb(215, 55, 55, 55);
    this->playingTextBrightColor = argb(255, 0, 196, 223);
    this->playingTextDarkColor = argb(150, 0, 80, 130);

    this->mainContainer = new CBaseUIScrollView(x, y, xSize, ySize, "");
    this->mainContainer->setDrawBackground(false);
    this->mainContainer->setDrawFrame(false);
    this->mainContainer->setVerticalScrolling(false);
    this->mainContainer->setScrollbarColor(0x99666666);

    this->updateDrives();
}

VSMusicBrowser::~VSMusicBrowser() { SAFE_DELETE(this->mainContainer); }

void VSMusicBrowser::draw() { this->mainContainer->draw(); }

void VSMusicBrowser::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    this->mainContainer->mouse_update(propagate_clicks);
}

void VSMusicBrowser::onButtonClicked(CBaseUIButton *button) {
    VSMusicBrowserButton *btn = dynamic_cast<VSMusicBrowserButton *>(button);
    if(btn == NULL) return;  // sanity

    // set flags / next song
    if(btn->isDirectory())
        btn->setSelected(true);
    else {
        this->previousActiveSong = this->activeSong;
        this->activeSong = btn->getName().toUtf8();

        btn->setPlaying(true);
        btn->setTextDarkColor(this->playingTextDarkColor);
        btn->setTextBrightColor(this->playingTextBrightColor);
    }

    // get column this press came from and deselect all others, also rebuild all columns > current
    bool fromInvalidSelection = true;
    for(size_t i = 0; i < this->columns.size(); i++) {
        for(size_t b = 0; b < this->columns[i].buttons.size(); b++) {
            if(this->columns[i].buttons[b]->getName() == btn->getName()) {
                if(btn->isDirectory()) {
                    // rebuild it
                    this->updateFolder(std::string(btn->getName().toUtf8()), i);

                    // reset selection flag of all buttons in THIS column except the one we just clicked
                    for(size_t r = 0; r < this->columns[i].buttons.size(); r++) {
                        if(this->columns[i].buttons[r] != btn) this->columns[i].buttons[r]->setSelected(false);
                    }
                } else if(this->fileClickedCallback != NULL) {
                    // rebuild playlist, but only if this click would result in a successful play
                    if(VinylScratcher::tryPlayFile(std::string(btn->getName().toUtf8()))) {
                        fromInvalidSelection = false;

                        this->playlist.clear();
                        for(size_t p = 0; p < this->columns[i].buttons.size(); p++) {
                            if(!this->columns[i].buttons[p]->isDirectory())
                                this->playlist.emplace_back(this->columns[i].buttons[p]->getName().toUtf8());
                        }
                    }

                    // fire play event
                    this->fileClickedCallback(std::string(btn->getName().toUtf8()), false);
                }

                break;
            }
        }
    }

    // update button highlight
    this->updatePlayingSelection(fromInvalidSelection);
}

void VSMusicBrowser::updatePlayingSelection(bool fromInvalidSelection) {
    for(size_t i = 0; i < this->columns.size(); i++) {
        for(size_t b = 0; b < this->columns[i].buttons.size(); b++) {
            VSMusicBrowserButton *button = this->columns[i].buttons[b];

            if(button->getName() != UString(this->activeSong.c_str())) {
                button->setTextColor(this->defaultTextColor);
                button->setPlaying(false);
            } else {
                button->setTextBrightColor(this->playingTextBrightColor);
                button->setTextDarkColor(this->playingTextDarkColor);
                button->setPlaying(true);

                if(!fromInvalidSelection) {
                    const float relativeButtonY = button->getPos().y - this->columns[i].view->getPos().y;
                    if(relativeButtonY <= 0 ||
                       relativeButtonY + button->getSize().y >= this->columns[i].view->getSize().y)
                        this->columns[i].view->scrollToElement(
                            button, 0, this->columns[i].view->getSize().y / 2 - button->getSize().y / 2);
                }
            }
        }
    }
}

void VSMusicBrowser::updateFolder(const std::string& baseFolder, size_t fromDepth) {
    if(this->columns.size() < 1) return;

    // remove all columns > fromDepth
    if(fromDepth + 1 < this->columns.size()) {
        for(size_t i = fromDepth + 1; i < this->columns.size(); i++) {
            this->mainContainer->getContainer()->deleteBaseUIElement(this->columns[i].view);

            this->columns.erase(this->columns.begin() + i);
            i--;
        }
    }

    // create column
    COLUMN col;
    {
        const CBaseUIScrollView *previousView = this->columns[this->columns.size() - 1].view;
        const int xPos = previousView->getRelPos().x + previousView->getSize().x;

        const float dpiScale = env->getDPIScale();
        const int border = 20 * dpiScale;
        const int height = 28 * dpiScale;
        const Color frameColor = argb(255, 150, 150, 150);

        col.view = new VSMusicBrowserColumnScrollView(xPos, -1, 100, this->vSize.y, "");
        col.view->setScrollMouseWheelMultiplier((1 / 3.5f) * 0.5f);
        col.view->setDrawBackground(false);
        col.view->setFrameColor(0xffff0000);
        col.view->setDrawFrame(false);
        col.view->setHorizontalScrolling(false);
        col.view->setScrollbarColor(0x99666666);

        float maxWidthCounter = 1;

        // go through the file system
        std::vector<std::string> folders = env->getFoldersInFolder(baseFolder);
        std::vector<std::string> files = env->getFilesInFolder(baseFolder);

        // sort both lists naturally
        std::sort(folders.begin(), folders.end(), VSMusicBrowserNaturalSortStringComparator());
        std::sort(files.begin(), files.end(), VSMusicBrowserNaturalSortStringComparator());

        // first, add all folders
        int elementCounter = 0;
        for(const std::string &folder : folders) {
            if(folder.compare(".") == 0 || folder.compare("..") == 0) continue;

            std::string completeName = baseFolder;
            completeName.append(folder);
            completeName.append("/");

            VSMusicBrowserButton *folderButton =
                new VSMusicBrowserButton(border, border + elementCounter * height, 50 * dpiScale, height,
                                         UString(completeName.c_str()), UString(folder.c_str()));
            folderButton->setClickCallback(SA::MakeDelegate<&VSMusicBrowser::onButtonClicked>(this));
            folderButton->setTextColor(this->defaultTextColor);
            folderButton->setFrameColor(frameColor);
            folderButton->setSizeToContent(12 * dpiScale, 5 * dpiScale);
            folderButton->setSizeY(height);
            folderButton->setDirectory(true);

            if(folderButton->getSize().x > maxWidthCounter) maxWidthCounter = folderButton->getSize().x;

            col.view->getContainer()->addBaseUIElement(folderButton);
            col.buttons.push_back(folderButton);

            elementCounter++;
        }

        // then add all files
        for(const std::string &file : files) {
            if(file.compare(".") == 0 || file.compare("..") == 0) continue;

            std::string completeName = baseFolder;
            completeName.append(file);

            VSMusicBrowserButton *fileButton =
                new VSMusicBrowserButton(border, border + elementCounter * height, 50 * dpiScale, height,
                                         UString(completeName.c_str()), UString(file.c_str()));
            fileButton->setClickCallback(SA::MakeDelegate<&VSMusicBrowser::onButtonClicked>(this));
            fileButton->setDrawBackground(false);
            fileButton->setTextColor(this->defaultTextColor);
            fileButton->setFrameColor(frameColor);
            fileButton->setSizeToContent(12 * dpiScale, 5 * dpiScale);
            fileButton->setSizeY(height);
            fileButton->setDirectory(false);

            // preemptively updatePlayingSelection() manually here, code duplication ahoy
            if(completeName == this->activeSong) {
                fileButton->setTextBrightColor(this->playingTextBrightColor);
                fileButton->setTextDarkColor(this->playingTextDarkColor);
                fileButton->setPlaying(true);
            }

            if(fileButton->getSize().x > maxWidthCounter) maxWidthCounter = fileButton->getSize().x;

            col.view->getContainer()->addBaseUIElementBack(fileButton);
            col.buttons.push_back(fileButton);

            elementCounter++;
        }

        // normalize button width
        for(size_t i = 0; i < col.buttons.size(); i++) {
            col.buttons[i]->setSizeX(maxWidthCounter);
            col.buttons[i]->setTextLeft(true);
        }

        // update & add everything
        col.view->setSizeX(border + maxWidthCounter + 20 * dpiScale);
        col.view->setScrollSizeToContent(20 * dpiScale);
    }
    this->columns.push_back(col);
    this->mainContainer->getContainer()->addBaseUIElement(col.view);
    this->mainContainer->setScrollSizeToContent(0);
    this->mainContainer->scrollToRight();
}

void VSMusicBrowser::updateDrives() {
    // drive selection is always at column 0
    if(this->columns.size() < 1) {
        COLUMN col;
        {
            col.view = new CBaseUIScrollView(-1, -1, 100, this->vSize.y, "");
            col.view->setDrawBackground(false);
            col.view->setFrameColor(0xffff0000);
            col.view->setDrawFrame(false);
            col.view->setHorizontalScrolling(false);
            col.view->setScrollbarColor(0x99666666);

            this->mainContainer->getContainer()->addBaseUIElement(col.view);
        }
        this->columns.push_back(col);
    }
    COLUMN &driveColumn = this->columns[0];

    driveColumn.view->clear();

    const float dpiScale = env->getDPIScale();
    const int border = 20 * dpiScale;
    const int height = 28 * dpiScale;
    const Color frameColor = argb(255, 150, 150, 150);

    float maxWidthCounter = 1;

    const std::vector<UString> drives = env->getLogicalDrives();
    for(size_t i = 0; i < drives.size(); i++) {
        const UString &drive = drives[i];

        if(drive.length() < 1) continue;  // sanity

        VSMusicBrowserButton *driveButton =
            new VSMusicBrowserButton(border, border + i * height, 50, height, drive, drive.substr(0, 1));
        driveButton->setTextColor(this->defaultTextColor);
        driveButton->setClickCallback(SA::MakeDelegate<&VSMusicBrowser::onButtonClicked>(this));
        driveButton->setDirectory(true);
        driveButton->setDrawBackground(false);
        driveButton->setFrameColor(frameColor);
        driveButton->setFont(this->font);
        driveButton->setSizeToContent(this->font->getHeight() / 1.5f, 5 * dpiScale);
        driveButton->setTextLeft(true);
        driveButton->setSizeY(height);

        driveColumn.view->getContainer()->addBaseUIElement(driveButton);
        driveColumn.buttons.push_back(driveButton);

        if(driveButton->getSize().x > maxWidthCounter) maxWidthCounter = driveButton->getSize().x;
    }

    // normalize button widths
    for(size_t i = 0; i < driveColumn.buttons.size(); i++) {
        driveColumn.buttons[i]->setSizeX(maxWidthCounter);
    }

    // update & add everything
    driveColumn.view->setSizeX(border + maxWidthCounter + 15 * dpiScale);
    driveColumn.view->setScrollSizeToContent(15 * dpiScale);
    this->mainContainer->setScrollSizeToContent(0);
}

void VSMusicBrowser::fireNextSong(bool previous) {
    for(int i = 0; i < (int)this->playlist.size(); i++) {
        // find current position
        if(this->playlist[i] == this->activeSong) {
            // look for the next playable file, if we can find one
            int breaker = 0;
            while(true) {
                int nextSongIndex = ((i + 1) < this->playlist.size() ? (i + 1) : 0);
                if(previous) nextSongIndex = ((i - 1) >= 0 ? (i - 1) : this->playlist.size() - 1);

                // if we have found the next playable file
                if(VinylScratcher::tryPlayFile(this->playlist[nextSongIndex])) {
                    // set it, update the possibly visible button of the current song (and reset all others), and play
                    // it
                    this->previousActiveSong = this->activeSong;
                    this->activeSong = this->playlist[nextSongIndex];

                    this->updatePlayingSelection();

                    // fire play event
                    this->fileClickedCallback(this->activeSong, previous);

                    return;
                } else {
                    if(previous)
                        i--;
                    else
                        i++;

                    if(previous) {
                        if(i < 0) i = this->playlist.size() - 1;
                    } else {
                        if(i >= this->playlist.size()) i = 0;
                    }
                }
                breaker++;

                if(breaker > 2 * this->playlist.size()) return;
            }

            return;
        }
    }
}

void VSMusicBrowser::onInvalidFile() {
    this->activeSong = this->previousActiveSong;
    this->updatePlayingSelection(true);
}

void VSMusicBrowser::onMoved() { this->mainContainer->setPos(this->vPos); }

void VSMusicBrowser::onResized() {
    for(size_t i = 0; i < this->columns.size(); i++) {
        this->columns[i].view->setSizeY(this->vSize.y);
        this->columns[i].view->scrollToY(this->columns[i].view->getRelPosY());
    }

    this->mainContainer->setSize(this->vSize);
    this->mainContainer->setScrollSizeToContent(0);
    this->mainContainer->scrollToX(this->mainContainer->getRelPosX());
}

void VSMusicBrowser::onFocusStolen() {
    // forward
    this->mainContainer->stealFocus();
}

void VSMusicBrowser::onEnabled() {
    // forward
    this->mainContainer->setEnabled(true);
}

void VSMusicBrowser::onDisabled() {
    // forward
    this->mainContainer->setEnabled(false);
}
