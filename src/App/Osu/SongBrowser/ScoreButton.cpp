#include "ScoreButton.h"

#include "SongBrowser.h"
// ---

#include <chrono>
#include <utility>

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "ConVar.h"
#include "Console.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "GameRules.h"
#include "Icons.h"
#include "Keyboard.h"
#include "LeaderboardPPCalcThread.h"
#include "LegacyReplay.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"
#include "UIAvatar.h"
#include "UIContextMenu.h"

UString ScoreButton::recentScoreIconString;

ScoreButton::ScoreButton(UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize)
    : CBaseUIButton(xPos, yPos, xSize, ySize, "", "") {
    this->contextMenu = contextMenu;

    if(recentScoreIconString.length() < 1) recentScoreIconString.insert(0, Icons::ARROW_CIRCLE_UP);

    this->fIndexNumberAnim = 0.0f;
    this->bIsPulseAnim = false;

    this->bRightClick = false;
    this->bRightClickCheck = false;

    this->iScoreIndexNumber = 1;
    this->iScoreUnixTimestamp = 0;

    this->scoreGrade = FinishedScore::Grade::D;
}

ScoreButton::~ScoreButton() {
    anim->deleteExistingAnimation(&this->fIndexNumberAnim);
    SAFE_DELETE(this->avatar);
}

void ScoreButton::draw() {
    if(!this->bVisible) return;

    // background
    // XXX: Make it flash with song BPM
    g->setColor(0xff000000);
    g->setAlpha(0.59f * (0.5f + 0.5f * this->fIndexNumberAnim));
    Image *backgroundImage = osu->getSkin()->getMenuButtonBackground();
    g->pushTransform();
    {
        auto screen = osu->getScreenSize();
        bool is_widescreen = ((i32)(std::max(0, (i32)((screen.x - (screen.y * 4.f / 3.f)) / 2.f))) > 0);
        f32 global_scale = screen.x / (is_widescreen ? 1366.f : 1024.f);

        f32 scale = global_scale * (osu->getSkin()->isMenuButtonBackground2x() ? 0.5f : 1.f);
        scale *= 0.555f;  // idk

        g->scale(scale, scale);
        g->translate(this->vPos.x, this->vPos.y + this->vSize.y / 2);
        g->drawImage(backgroundImage, AnchorPoint::LEFT);
    }
    g->popTransform();

    const int yPos = (int)this->vPos.y;  // avoid max shimmering

    // index number
    if(this->avatar) {
        const float margin = this->vSize.y * 0.1;
        f32 avatar_width = this->vSize.y - (2.f * margin);
        this->avatar->setPos(this->vPos.x + margin, this->vPos.y + margin);
        this->avatar->setSize(avatar_width, avatar_width);

        // Update avatar visibility status
        // NOTE: Not checking horizontal visibility
        auto m_scoreBrowser = osu->songBrowser2->scoreBrowser;
        bool is_below_top = this->avatar->getPos().y + this->avatar->getSize().y >= m_scoreBrowser->getPos().y;
        bool is_above_bottom = this->avatar->getPos().y <= m_scoreBrowser->getPos().y + m_scoreBrowser->getSize().y;
        this->avatar->on_screen = is_below_top && is_above_bottom;
        this->avatar->draw_avatar(1.f);
    }
    const float indexNumberScale = 0.35f;
    const float indexNumberWidthPercent = 0.15f;
    McFont *indexNumberFont = osu->getSongBrowserFontBold();
    g->pushTransform();
    {
        UString indexNumberString = UString::format("%i", this->iScoreIndexNumber);
        const float scale = (this->vSize.y / indexNumberFont->getHeight()) * indexNumberScale;

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + this->vSize.x * indexNumberWidthPercent * 0.5f -
                           indexNumberFont->getStringWidth(indexNumberString) * scale / 2.0f),
                     (int)(yPos + this->vSize.y / 2.0f + indexNumberFont->getHeight() * scale / 2.0f));
        g->translate(0.5f, 0.5f);
        g->setColor(0xff000000);
        g->setAlpha(1.0f - (1.0f - this->fIndexNumberAnim));
        g->drawString(indexNumberFont, indexNumberString);
        g->translate(-0.5f, -0.5f);
        g->setColor(0xffffffff);
        g->setAlpha(1.0f - (1.0f - this->fIndexNumberAnim) * (1.0f - this->fIndexNumberAnim));
        g->drawString(indexNumberFont, indexNumberString);
    }
    g->popTransform();

    // grade
    const float gradeHeightPercent = 0.8f;
    SkinImage *grade = getGradeImage(this->scoreGrade);
    int gradeWidth = 0;
    g->pushTransform();
    {
        const float scale = Osu::getImageScaleToFitResolution(
            grade->getSizeBaseRaw(),
            Vector2(this->vSize.x * (1.0f - indexNumberWidthPercent), this->vSize.y * gradeHeightPercent));
        gradeWidth = grade->getSizeBaseRaw().x * scale;

        g->setColor(0xffffffff);
        grade->drawRaw(Vector2((int)(this->vPos.x + this->vSize.x * indexNumberWidthPercent + gradeWidth / 2.0f),
                               (int)(this->vPos.y + this->vSize.y / 2.0f)),
                       scale);
    }
    g->popTransform();

    const float gradePaddingRight = this->vSize.y * 0.165f;

    // username | (artist + songName + diffName)
    const float usernameScale = 0.7f;
    McFont *usernameFont = osu->getSongBrowserFont();
    g->pushClipRect(McRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y));
    g->pushTransform();
    {
        const float height = this->vSize.y * 0.5f;
        const float paddingTopPercent = (1.0f - usernameScale) * 0.4f;
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / usernameFont->getHeight()) * usernameScale;

        UString &string = this->sScoreUsername;

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + this->vSize.x * indexNumberWidthPercent + gradeWidth + gradePaddingRight),
                     (int)(yPos + height / 2.0f + usernameFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(usernameFont, string);
        g->translate(-0.75f, -0.75f);
        g->setColor(this->is_friend ? 0xffD424B0 : 0xffffffff);
        g->drawString(usernameFont, string);
    }
    g->popTransform();
    g->popClipRect();

    // score | pp | weighted 95% (pp)
    const float scoreScale = 0.5f;
    McFont *scoreFont = (this->vSize.y < 50 ? resourceManager->getFont("FONT_DEFAULT")
                                            : usernameFont);  // HACKHACK: switch font for very low resolutions
    g->pushTransform();
    {
        const float height = this->vSize.y * 0.5f;
        const float paddingBottomPercent = (1.0f - scoreScale) * 0.25f;
        const float paddingBottom = height * paddingBottomPercent;
        const float scale = (height / scoreFont->getHeight()) * scoreScale;

        UString &string = this->sScoreScorePP;

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + this->vSize.x * indexNumberWidthPercent + gradeWidth + gradePaddingRight),
                     (int)(yPos + height * 1.5f + scoreFont->getHeight() * scale / 2.0f - paddingBottom));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(scoreFont, (cv::scores_sort_by_pp.getBool() ? string : this->sScoreScore));
        g->translate(-0.75f, -0.75f);
        g->setColor(0xffffffff);
        g->drawString(scoreFont, (cv::scores_sort_by_pp.getBool() ? string : this->sScoreScore));
    }
    g->popTransform();

    const float rightSideThirdHeight = this->vSize.y * 0.333f;
    const float rightSidePaddingRight = this->vSize.x * 0.025f;

    // mods
    const float modScale = 0.7f;
    McFont *modFont = osu->getSubTitleFont();
    g->pushTransform();
    {
        const float height = rightSideThirdHeight;
        const float paddingTopPercent = (1.0f - modScale) * 0.45f;
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / modFont->getHeight()) * modScale;

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + this->vSize.x - modFont->getStringWidth(this->sScoreMods) * scale -
                           rightSidePaddingRight),
                     (int)(yPos + height * 0.5f + modFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(modFont, this->sScoreMods);
        g->translate(-0.75f, -0.75f);
        g->setColor(0xffffffff);
        g->drawString(modFont, this->sScoreMods);
    }
    g->popTransform();

    // accuracy
    const float accScale = 0.65f;
    McFont *accFont = osu->getSubTitleFont();
    g->pushTransform();
    {
        const UString &scoreAccuracy = this->sScoreAccuracy;

        const float height = rightSideThirdHeight;
        const float paddingTopPercent = (1.0f - modScale) * 0.45f;
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / accFont->getHeight()) * accScale;

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + this->vSize.x - accFont->getStringWidth(scoreAccuracy) * scale -
                           rightSidePaddingRight),
                     (int)(yPos + height * 1.5f + accFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(accFont, scoreAccuracy);
        g->translate(-0.75f, -0.75f);
        g->setColor(0xffffffff);
        g->drawString(accFont, scoreAccuracy);
    }
    g->popTransform();

    // custom info (Spd.)
    if(this->sCustom.length() > 0) {
        const float customScale = 0.50f;
        McFont *customFont = osu->getSubTitleFont();
        g->pushTransform();
        {
            const float height = rightSideThirdHeight;
            const float paddingTopPercent = (1.0f - modScale) * 0.45f;
            const float paddingTop = height * paddingTopPercent;
            const float scale = (height / customFont->getHeight()) * customScale;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + this->vSize.x - customFont->getStringWidth(this->sCustom) * scale -
                               rightSidePaddingRight),
                         (int)(yPos + height * 2.325f + customFont->getHeight() * scale / 2.0f + paddingTop));
            g->translate(0.75f, 0.75f);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(customFont, this->sCustom);
            g->translate(-0.75f, -0.75f);
            g->setColor(0xffffffff);
            g->drawString(customFont, this->sCustom);
        }
        g->popTransform();
    }

    // recent icon + elapsed time since score
    const float upIconScale = 0.35f;
    const float timeElapsedScale = accScale;
    McFont *iconFont = osu->getFontIcons();
    McFont *timeFont = accFont;
    if(this->iScoreUnixTimestamp > 0) {
        const float iconScale = (this->vSize.y / iconFont->getHeight()) * upIconScale;
        const float iconHeight = iconFont->getHeight() * iconScale;
        const float iconPaddingLeft = 2;

        g->pushTransform();
        {
            g->scale(iconScale, iconScale);
            g->translate((int)(this->vPos.x + this->vSize.x + iconPaddingLeft),
                         (int)(yPos + this->vSize.y / 2 + iconHeight / 2));
            g->translate(1, 1);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(iconFont, recentScoreIconString);
            g->translate(-1, -1);
            g->setColor(0xffffffff);
            g->drawString(iconFont, recentScoreIconString);
        }
        g->popTransform();

        // elapsed time since score
        if(this->sScoreTime.length() > 0) {
            const float timeHeight = rightSideThirdHeight;
            const float timeScale = (timeHeight / timeFont->getHeight()) * timeElapsedScale;
            const float timePaddingLeft = 8;

            g->pushTransform();
            {
                g->scale(timeScale, timeScale);
                g->translate((int)(this->vPos.x + this->vSize.x + iconPaddingLeft +
                                   iconFont->getStringWidth(recentScoreIconString) * iconScale + timePaddingLeft),
                             (int)(yPos + this->vSize.y / 2 + timeFont->getHeight() * timeScale / 2));
                g->translate(0.75f, 0.75f);
                g->setColor(0xff000000);
                g->setAlpha(0.85f);
                g->drawString(timeFont, this->sScoreTime);
                g->translate(-0.75f, -0.75f);
                g->setColor(0xffffffff);
                g->drawString(timeFont, this->sScoreTime);
            }
            g->popTransform();
        }
    }

    // TODO: difference to below score in list, +12345
}

void ScoreButton::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;

    // Update pp
    if(this->score.get_pp() == -1.0) {
        pp_calc_request request;
        request.mods_legacy = this->score.mods.to_legacy();
        request.speed = this->score.mods.speed;
        request.AR = this->score.diff2->getAR();
        request.CS = this->score.diff2->getCS();
        request.OD = this->score.diff2->getOD();
        if(this->score.mods.ar_override != -1.f) request.AR = this->score.mods.ar_override;
        if(this->score.mods.cs_override != -1.f) request.CS = this->score.mods.cs_override;
        if(this->score.mods.od_override != -1.f) request.OD = this->score.mods.od_override;
        request.rx = this->score.mods.flags & Replay::ModFlags::Relax;
        request.td = this->score.mods.flags & Replay::ModFlags::TouchDevice;
        request.comboMax = this->score.comboMax;
        request.numMisses = this->score.numMisses;
        request.num300s = this->score.num300s;
        request.num100s = this->score.num100s;
        request.num50s = this->score.num50s;

        auto info = lct_get_pp(request);
        if(info.pp != -1.0) {
            // NOTE: Allows dropped sliderends. Should fix with @PPV3
            const bool fullCombo =
                (this->score.maxPossibleCombo > 0 && this->score.numMisses == 0 && this->score.numSliderBreaks == 0);

            this->score.ppv2_score = info.pp;
            this->score.ppv2_version = DifficultyCalculator::PP_ALGORITHM_VERSION;
            this->score.ppv2_total_stars = info.total_stars;
            this->score.ppv2_aim_stars = info.aim_stars;
            this->score.ppv2_speed_stars = info.speed_stars;

            std::scoped_lock lock(db->scores_mtx);
            for(auto &other : db->scores[this->score.beatmap_hash]) {
                if(other.unixTimestamp == this->score.unixTimestamp) {
                    other = this->score;
                    osu->getSongBrowser()->score_resort_scheduled = true;
                    break;
                }
            }

            this->sScoreScorePP = UString::format(
                (this->score.perfect ? "PP: %ipp (%ix PFC)" : (fullCombo ? "PP: %ipp (%ix FC)" : "PP: %ipp (%ix)")),
                (int)std::round(this->score.get_pp()), this->score.comboMax);
        }
    }

    if(this->avatar) {
        this->avatar->mouse_update(propagate_clicks);
        if(!*propagate_clicks) return;
    }

    CBaseUIButton::mouse_update(propagate_clicks);

    // HACKHACK: this should really be part of the UI base
    // right click detection
    if(mouse->isRightDown()) {
        if(!this->bRightClickCheck) {
            this->bRightClickCheck = true;
            this->bRightClick = this->isMouseInside();
        }
    } else {
        if(this->bRightClick) {
            if(this->isMouseInside()) this->onRightMouseUpInside();
        }

        this->bRightClickCheck = false;
        this->bRightClick = false;
    }

    // tooltip (extra stats)
    if(this->isMouseInside()) {
        if(!this->isContextMenuVisible()) {
            if(this->fIndexNumberAnim > 0.0f) {
                osu->getTooltipOverlay()->begin();
                {
                    for(int i = 0; i < this->tooltipLines.size(); i++) {
                        if(this->tooltipLines[i].length() > 0) osu->getTooltipOverlay()->addLine(this->tooltipLines[i]);
                    }
                }
                osu->getTooltipOverlay()->end();
            }
        } else {
            anim->deleteExistingAnimation(&this->fIndexNumberAnim);
            this->fIndexNumberAnim = 0.0f;
        }
    }

    // update elapsed time string
    this->updateElapsedTimeString();

    // stuck anim reset
    if(!this->isMouseInside() && !anim->isAnimating(&this->fIndexNumberAnim)) this->fIndexNumberAnim = 0.0f;
}

void ScoreButton::highlight() {
    this->bIsPulseAnim = true;

    const int numPulses = 10;
    const float timescale = 1.75f;
    for(int i = 0; i < 2 * numPulses; i++) {
        if(i % 2 == 0)
            anim->moveQuadOut(&this->fIndexNumberAnim, 1.0f, 0.125f * timescale,
                              ((i / 2) * (0.125f + 0.15f)) * timescale - 0.001f, (i == 0));
        else
            anim->moveLinear(&this->fIndexNumberAnim, 0.0f, 0.15f * timescale,
                             (0.125f + (i / 2) * (0.125f + 0.15f)) * timescale - 0.001f);
    }
}

void ScoreButton::resetHighlight() {
    this->bIsPulseAnim = false;
    anim->deleteExistingAnimation(&this->fIndexNumberAnim);
    this->fIndexNumberAnim = 0.0f;
}

void ScoreButton::updateElapsedTimeString() {
    if(this->iScoreUnixTimestamp > 0) {
        const u64 curUnixTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        const u64 delta = curUnixTime - this->iScoreUnixTimestamp;

        const u64 deltaInSeconds = delta;
        const u64 deltaInMinutes = delta / 60;
        const u64 deltaInHours = deltaInMinutes / 60;
        const u64 deltaInDays = deltaInHours / 24;
        const u64 deltaInYears = deltaInDays / 365;

        if(deltaInHours < 96) {
            if(deltaInDays > 364)
                this->sScoreTime = UString::format("%iy", (int)(deltaInYears));
            else if(deltaInHours > 47)
                this->sScoreTime = UString::format("%id", (int)(deltaInDays));
            else if(deltaInHours >= 1)
                this->sScoreTime = UString::format("%ih", (int)(deltaInHours));
            else if(deltaInMinutes > 0)
                this->sScoreTime = UString::format("%im", (int)(deltaInMinutes));
            else
                this->sScoreTime = UString::format("%is", (int)(deltaInSeconds));
        } else {
            this->iScoreUnixTimestamp = 0;

            if(this->sScoreTime.length() > 0) this->sScoreTime.clear();
        }
    }
}

void ScoreButton::onClicked() {
    soundEngine->play(osu->getSkin()->getMenuHit());
    CBaseUIButton::onClicked();
}

void ScoreButton::onMouseInside() {
    this->bIsPulseAnim = false;

    if(!this->isContextMenuVisible())
        anim->moveQuadOut(&this->fIndexNumberAnim, 1.0f, 0.125f * (1.0f - this->fIndexNumberAnim), true);
}

void ScoreButton::onMouseOutside() {
    this->bIsPulseAnim = false;

    anim->moveLinear(&this->fIndexNumberAnim, 0.0f, 0.15f * this->fIndexNumberAnim, true);
}

void ScoreButton::onFocusStolen() {
    CBaseUIButton::onFocusStolen();

    this->bRightClick = false;

    if(!this->bIsPulseAnim) {
        anim->deleteExistingAnimation(&this->fIndexNumberAnim);
        this->fIndexNumberAnim = 0.0f;
    }
}

void ScoreButton::onRightMouseUpInside() {
    const Vector2 pos = mouse->getPos();

    if(this->contextMenu != NULL) {
        this->contextMenu->setPos(pos);
        this->contextMenu->setRelPos(pos);
        this->contextMenu->begin(0, true);
        {
            this->contextMenu->addButton("Use Mods", 1);  // for scores without mods this will just nomod
            this->contextMenu->addButton("View replay", 2);

            CBaseUIButton *spacer = this->contextMenu->addButton("---");
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);
            CBaseUIButton *deleteButton = this->contextMenu->addButton("Delete Score", 3);
            if(this->score.is_peppy_imported()) {
                // XXX: gray it out and have hover reason why user can't delete instead
                //      ...or allow delete and just store it as hidden in db
                deleteButton->setEnabled(false);
                deleteButton->setTextColor(0xff888888);
                deleteButton->setTextDarkColor(0xff000000);
            }
        }
        this->contextMenu->end(false, false);
        this->contextMenu->setClickCallback(SA::MakeDelegate<&ScoreButton::onContextMenu>(this));
        UIContextMenu::clampToRightScreenEdge(this->contextMenu);
        UIContextMenu::clampToBottomScreenEdge(this->contextMenu);
    }
}

void ScoreButton::onContextMenu(const UString &text, int id) {
    if(id == 1) {
        this->onUseModsClicked();
        return;
    }

    if(id == 2) {
        LegacyReplay::load_and_watch(this->score);
        return;
    }

    if(id == 3) {
        if(keyboard->isShiftDown())
            this->onDeleteScoreConfirmed(text, 1);
        else
            this->onDeleteScoreClicked();

        return;
    }
}

void ScoreButton::onUseModsClicked() {
    osu->useMods(&this->score);
    soundEngine->play(osu->getSkin()->getCheckOn());
}

void ScoreButton::onDeleteScoreClicked() {
    if(this->contextMenu != NULL) {
        this->contextMenu->begin(0, true);
        {
            this->contextMenu->addButton("Really delete score?")->setEnabled(false);
            CBaseUIButton *spacer = this->contextMenu->addButton("---");
            spacer->setTextLeft(false);
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);
            this->contextMenu->addButton("Yes", 1)->setTextLeft(false);
            this->contextMenu->addButton("No")->setTextLeft(false);
        }
        this->contextMenu->end(false, false);
        this->contextMenu->setClickCallback(SA::MakeDelegate<&ScoreButton::onDeleteScoreConfirmed>(this));
        UIContextMenu::clampToRightScreenEdge(this->contextMenu);
        UIContextMenu::clampToBottomScreenEdge(this->contextMenu);
    }
}

void ScoreButton::onDeleteScoreConfirmed(const UString & /*text*/, int id) {
    if(id != 1) return;

    debugLog("Deleting score\n");

    // absolutely disgusting
    osu->getSongBrowser()->onScoreContextMenu(this, 2);
}

void ScoreButton::setScore(const FinishedScore &score, DatabaseBeatmap *diff2, int index, const UString & /*titleString*/,
                           float  /*weight*/) {
    this->score = score;
    this->score.beatmap_hash = diff2->getMD5Hash();
    this->score.diff2 = diff2;
    this->iScoreIndexNumber = index;

    f32 AR = score.mods.ar_override;
    f32 OD = score.mods.od_override;
    f32 HP = score.mods.hp_override;
    f32 CS = score.mods.cs_override;

    const float accuracy =
        LiveScore::calculateAccuracy(score.num300s, score.num100s, score.num50s, score.numMisses) * 100.0f;

    // NOTE: Allows dropped sliderends. Should fix with @PPV3
    const bool fullCombo = (score.maxPossibleCombo > 0 && score.numMisses == 0 && score.numSliderBreaks == 0);

    if(this->avatar) {
        delete this->avatar;
        this->avatar = NULL;
    }
    if(score.player_id != 0) {
        this->avatar = new UIAvatar(score.player_id, this->vPos.x, this->vPos.y, this->vSize.y, this->vSize.y);

        auto user = BANCHO::User::try_get_user_info(score.player_id);
        this->is_friend = user && user->is_friend();
    }

    // display
    this->scoreGrade = score.calculate_grade();
    this->sScoreUsername = UString(score.playerName.c_str());
    this->sScoreScore = UString::format(
        (score.perfect ? "Score: %llu (%ix PFC)" : (fullCombo ? "Score: %llu (%ix FC)" : "Score: %llu (%ix)")),
        score.score, score.comboMax);

    if(score.get_pp() == -1.0) {
        this->sScoreScorePP = UString::format(
            (score.perfect ? "PP: ??? (%ix PFC)" : (fullCombo ? "PP: ??? (%ix FC)" : "PP: ??? (%ix)")), score.comboMax);
    } else {
        this->sScoreScorePP = UString::format(
            (score.perfect ? "PP: %ipp (%ix PFC)" : (fullCombo ? "PP: %ipp (%ix FC)" : "PP: %ipp (%ix)")),
            (int)std::round(score.get_pp()), score.comboMax);
    }

    this->sScoreAccuracy = UString::format("%.2f%%", accuracy);
    this->sScoreAccuracyFC =
        UString::format((score.perfect ? "PFC %.2f%%" : (fullCombo ? "FC %.2f%%" : "%.2f%%")), accuracy);
    this->sScoreMods = getModsStringForDisplay(score.mods);
    this->sCustom = (score.mods.speed != 1.0f ? UString::format("Spd: %gx", score.mods.speed) : UString(""));
    if(diff2 != NULL) {
        const LegacyReplay::BEATMAP_VALUES beatmapValuesForModsLegacy = LegacyReplay::getBeatmapValuesForModsLegacy(
            score.mods.to_legacy(), diff2->getAR(), diff2->getCS(), diff2->getOD(), diff2->getHP());
        if(AR == -1.f) AR = beatmapValuesForModsLegacy.AR;
        if(OD == -1.f) OD = beatmapValuesForModsLegacy.OD;
        if(HP == -1.f) HP = beatmapValuesForModsLegacy.HP;
        if(CS == -1.f) CS = beatmapValuesForModsLegacy.CS;

        // only show these values if they are not default (or default with applied mods)
        // only show these values if they are not default with applied mods

        if(beatmapValuesForModsLegacy.CS != CS) {
            if(this->sCustom.length() > 0) this->sCustom.append(", ");

            this->sCustom.append(UString::format("CS:%.4g", CS));
        }

        if(beatmapValuesForModsLegacy.AR != AR) {
            if(this->sCustom.length() > 0) this->sCustom.append(", ");

            this->sCustom.append(UString::format("AR:%.4g", AR));
        }

        if(beatmapValuesForModsLegacy.OD != OD) {
            if(this->sCustom.length() > 0) this->sCustom.append(", ");

            this->sCustom.append(UString::format("OD:%.4g", OD));
        }

        if(beatmapValuesForModsLegacy.HP != HP) {
            if(this->sCustom.length() > 0) this->sCustom.append(", ");

            this->sCustom.append(UString::format("HP:%.4g", HP));
        }
    }

    char dateString[64];
    memset(dateString, '\0', 64);
    std::tm *tm = std::localtime((std::time_t *)(&score.unixTimestamp));
    std::strftime(dateString, 63, "%d-%b-%y %H:%M:%S", tm);
    this->sScoreDateTime = UString(dateString);
    this->iScoreUnixTimestamp = score.unixTimestamp;

    UString achievedOn = "Achieved on ";
    achievedOn.append(this->sScoreDateTime);

    // tooltip
    this->tooltipLines.clear();
    this->tooltipLines.push_back(achievedOn);

    this->tooltipLines.push_back(UString::format("300:%i 100:%i 50:%i Miss:%i SBreak:%i", score.num300s, score.num100s,
                                                 score.num50s, score.numMisses, score.numSliderBreaks));

    this->tooltipLines.push_back(UString::format("Accuracy: %.2f%%", accuracy));

    UString tooltipMods = "Mods: ";
    if(this->sScoreMods.length() > 0)
        tooltipMods.append(this->sScoreMods);
    else
        tooltipMods.append("None");

    this->tooltipLines.push_back(tooltipMods);
    if(score.mods.flags & Replay::ModFlags::ApproachDifferent) this->tooltipLines.emplace_back("+ approach different");
    if(score.mods.flags & Replay::ModFlags::ARTimewarp) this->tooltipLines.emplace_back("+ AR timewarp");
    if(score.mods.flags & Replay::ModFlags::ARWobble) this->tooltipLines.emplace_back("+ AR wobble");
    if(score.mods.flags & Replay::ModFlags::FadingCursor) this->tooltipLines.emplace_back("+ fading cursor");
    if(score.mods.flags & Replay::ModFlags::FullAlternate) this->tooltipLines.emplace_back("+ full alternate");
    if(score.mods.flags & Replay::ModFlags::FPoSu_Strafing) this->tooltipLines.emplace_back("+ FPoSu strafing");
    if(score.mods.flags & Replay::ModFlags::FPS) this->tooltipLines.emplace_back("+ FPS");
    if(score.mods.flags & Replay::ModFlags::HalfWindow) this->tooltipLines.emplace_back("+ half window");
    if(score.mods.flags & Replay::ModFlags::Jigsaw1) this->tooltipLines.emplace_back("+ jigsaw1");
    if(score.mods.flags & Replay::ModFlags::Jigsaw2) this->tooltipLines.emplace_back("+ jigsaw2");
    if(score.mods.flags & Replay::ModFlags::Mafham) this->tooltipLines.emplace_back("+ mafham");
    if(score.mods.flags & Replay::ModFlags::Millhioref) this->tooltipLines.emplace_back("+ millhioref");
    if(score.mods.flags & Replay::ModFlags::Minimize) this->tooltipLines.emplace_back("+ minimize");
    if(score.mods.flags & Replay::ModFlags::Ming3012) this->tooltipLines.emplace_back("+ ming3012");
    if(score.mods.flags & Replay::ModFlags::MirrorHorizontal) this->tooltipLines.emplace_back("+ mirror (horizontal)");
    if(score.mods.flags & Replay::ModFlags::MirrorVertical) this->tooltipLines.emplace_back("+ mirror (vertical)");
    if(score.mods.flags & Replay::ModFlags::No50s) this->tooltipLines.emplace_back("+ no 50s");
    if(score.mods.flags & Replay::ModFlags::No100s) this->tooltipLines.emplace_back("+ no 100s");
    if(score.mods.flags & Replay::ModFlags::ReverseSliders) this->tooltipLines.emplace_back("+ reverse sliders");
    if(score.mods.flags & Replay::ModFlags::Timewarp) this->tooltipLines.emplace_back("+ timewarp");
    if(score.mods.flags & Replay::ModFlags::Shirone) this->tooltipLines.emplace_back("+ shirone");
    if(score.mods.flags & Replay::ModFlags::StrictTracking) this->tooltipLines.emplace_back("+ strict tracking");
    if(score.mods.flags & Replay::ModFlags::Wobble1) this->tooltipLines.emplace_back("+ wobble1");
    if(score.mods.flags & Replay::ModFlags::Wobble2) this->tooltipLines.emplace_back("+ wobble2");

    // custom
    this->updateElapsedTimeString();
}

bool ScoreButton::isContextMenuVisible() { return (this->contextMenu != NULL && this->contextMenu->isVisible()); }

SkinImage *ScoreButton::getGradeImage(FinishedScore::Grade grade) {
    switch(grade) {
        case FinishedScore::Grade::XH:
            return osu->getSkin()->getRankingXHsmall();
        case FinishedScore::Grade::SH:
            return osu->getSkin()->getRankingSHsmall();
        case FinishedScore::Grade::X:
            return osu->getSkin()->getRankingXsmall();
        case FinishedScore::Grade::S:
            return osu->getSkin()->getRankingSsmall();
        case FinishedScore::Grade::A:
            return osu->getSkin()->getRankingAsmall();
        case FinishedScore::Grade::B:
            return osu->getSkin()->getRankingBsmall();
        case FinishedScore::Grade::C:
            return osu->getSkin()->getRankingCsmall();
        default:
            return osu->getSkin()->getRankingDsmall();
    }
}

UString ScoreButton::getModsStringForDisplay(Replay::Mods mods) {
    UString modsString;

    if(mods.flags & Replay::ModFlags::NoFail) modsString.append("NF,");
    if(mods.flags & Replay::ModFlags::Easy) modsString.append("EZ,");
    if(mods.flags & Replay::ModFlags::TouchDevice) modsString.append("TD,");
    if(mods.flags & Replay::ModFlags::Hidden) modsString.append("HD,");
    if(mods.flags & Replay::ModFlags::HardRock) modsString.append("HR,");
    if(mods.flags & Replay::ModFlags::SuddenDeath) modsString.append("SD,");
    if(mods.flags & Replay::ModFlags::Relax) modsString.append("Relax,");
    if(mods.flags & Replay::ModFlags::Flashlight) modsString.append("FL,");
    if(mods.flags & Replay::ModFlags::SpunOut) modsString.append("SO,");
    if(mods.flags & Replay::ModFlags::Autopilot) modsString.append("AP,");
    if(mods.flags & Replay::ModFlags::Perfect) modsString.append("PF,");
    if(mods.flags & Replay::ModFlags::ScoreV2) modsString.append("v2,");
    if(mods.flags & Replay::ModFlags::Target) modsString.append("Target,");
    if(mods.flags & Replay::ModFlags::Nightmare) modsString.append("Nightmare,");
    if(mods.flags & (Replay::ModFlags::MirrorHorizontal | Replay::ModFlags::MirrorVertical))
        modsString.append("Mirror,");
    if(mods.flags & Replay::ModFlags::FPoSu) modsString.append("FPoSu,");

    if(modsString.length() > 0) modsString = modsString.substr(0, modsString.length() - 1);

    return modsString;
}
