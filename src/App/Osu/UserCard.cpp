#include "UserCard.h"

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoUsers.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"
#include "Icons.h"
#include "Mouse.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SongBrowser/SongBrowser.h"
#include "TooltipOverlay.h"
#include "UIAvatar.h"

using namespace std;

// NOTE: selected username is stored in m_sText

UserCard::UserCard(i32 user_id) : CBaseUIButton() {
    this->setID(user_id);

    this->fPP = 0.0f;
    this->fAcc = 0.0f;
    this->iLevel = 0;
    this->fPercentToNextLevel = 0.0f;

    this->fPPDelta = 0.0f;
    this->fPPDeltaAnim = 0.0f;
}

UserCard::~UserCard() { SAFE_DELETE(this->avatar); }

void UserCard::draw(Graphics *g) {
    if(!this->bVisible) return;

    int yCounter = 0;
    const float iconBorder = this->vSize.y * 0.03f;
    const float iconHeight = this->vSize.y - 2 * iconBorder;
    const float iconWidth = iconHeight;

    // draw user icon background
    g->setColor(COLORf(1.0f, 0.1f, 0.1f, 0.1f));
    g->fillRect(this->vPos.x + iconBorder + 1, this->vPos.y + iconBorder + 1, iconWidth, iconHeight);

    // draw user icon
    if(this->avatar) {
        this->avatar->setPos(this->vPos.x + iconBorder + 1, this->vPos.y + iconBorder + 1);
        this->avatar->setSize(iconWidth, iconHeight);
        this->avatar->draw_avatar(g, 1.f);
    } else {
        g->setColor(0xffffffff);
        g->pushClipRect(McRect(this->vPos.x + iconBorder + 1, this->vPos.y + iconBorder + 2, iconWidth, iconHeight));
        g->pushTransform();
        {
            const float scale =
                Osu::getImageScaleToFillResolution(osu->getSkin()->getUserIcon(), Vector2(iconWidth, iconHeight));
            g->scale(scale, scale);
            g->translate(this->vPos.x + iconBorder + iconWidth / 2 + 1, this->vPos.y + iconBorder + iconHeight / 2 + 1);
            g->drawImage(osu->getSkin()->getUserIcon());
        }
        g->popTransform();
        g->popClipRect();
    }

    // draw username
    McFont *usernameFont = osu->getSongBrowserFont();
    const float usernameScale = 0.5f;
    float usernamePaddingLeft = 0.0f;
    g->pushClipRect(
        McRect(this->vPos.x + iconBorder, this->vPos.y + iconBorder, this->vSize.x - 2 * iconBorder, iconHeight));
    g->pushTransform();
    {
        const float height = this->vSize.y * 0.5f;
        const float paddingTopPercent = (1.0f - usernameScale) * 0.1f;
        const float paddingTop = height * paddingTopPercent;
        const float paddingLeftPercent = (1.0f - usernameScale) * 0.31f;
        usernamePaddingLeft = height * paddingLeftPercent;
        const float scale = (height / usernameFont->getHeight()) * usernameScale;

        yCounter += (int)(this->vPos.y + usernameFont->getHeight() * scale + paddingTop);

        g->scale(scale, scale);
        g->translate((int)(this->vPos.x + iconWidth + usernamePaddingLeft), yCounter);
        g->setColor(0xffffffff);
        g->drawString(usernameFont, this->sText);
    }
    g->popTransform();
    g->popClipRect();

    if(cv_scores_enabled.getBool()) {
        // draw performance (pp), and accuracy
        McFont *performanceFont = osu->getSubTitleFont();
        const float performanceScale = 0.3f;
        g->pushTransform();
        {
            UString performanceString = UString::format("Performance: %ipp", (int)std::round(this->fPP));
            UString accuracyString = UString::format("Accuracy: %.2f%%", this->fAcc * 100.0f);

            const float height = this->vSize.y * 0.5f;
            const float paddingTopPercent = (1.0f - performanceScale) * 0.25f;
            const float paddingTop = height * paddingTopPercent;
            const float paddingMiddlePercent = (1.0f - performanceScale) * 0.15f;
            const float paddingMiddle = height * paddingMiddlePercent;
            const float scale = (height / performanceFont->getHeight()) * performanceScale;

            yCounter += performanceFont->getHeight() * scale + paddingTop;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + iconWidth + usernamePaddingLeft), yCounter);
            g->setColor(0xffffffff);
            if(cv_user_draw_pp.getBool()) g->drawString(performanceFont, performanceString);

            yCounter += performanceFont->getHeight() * scale + paddingMiddle;

            g->translate(0, performanceFont->getHeight() * scale + paddingMiddle);
            if(cv_user_draw_accuracy.getBool()) g->drawString(performanceFont, accuracyString);
        }
        g->popTransform();

        // draw level
        McFont *scoreFont = osu->getSubTitleFont();
        const float scoreScale = 0.3f;
        g->pushTransform();
        {
            UString scoreString = UString::format("LV%i", this->iLevel);

            const float height = this->vSize.y * 0.5f;
            const float paddingTopPercent = (1.0f - scoreScale) * 0.25f;
            const float paddingTop = height * paddingTopPercent;
            const float scale = (height / scoreFont->getHeight()) * scoreScale;

            yCounter += scoreFont->getHeight() * scale + paddingTop;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + iconWidth + usernamePaddingLeft), yCounter);
            g->setColor(0xffffffff);
            if(cv_user_draw_level.getBool()) g->drawString(scoreFont, scoreString);
        }
        g->popTransform();

        // draw level percentage bar (to next level)
        if(cv_user_draw_level_bar.getBool()) {
            const float barBorder = (int)(iconBorder);
            const float barHeight = (int)(this->vSize.y - 2 * barBorder) * 0.1f;
            const float barWidth = (int)((this->vSize.x - 2 * barBorder) * 0.55f);
            g->setColor(0xffaaaaaa);
            g->drawRect(this->vPos.x + this->vSize.x - barWidth - barBorder - 1,
                        this->vPos.y + this->vSize.y - barHeight - barBorder, barWidth, barHeight);
            g->fillRect(this->vPos.x + this->vSize.x - barWidth - barBorder - 1,
                        this->vPos.y + this->vSize.y - barHeight - barBorder,
                        barWidth * clamp<float>(this->fPercentToNextLevel, 0.0f, 1.0f), barHeight);
        }

        // draw pp increase/decrease delta
        McFont *deltaFont = performanceFont;
        const float deltaScale = 0.4f;
        if(this->fPPDeltaAnim > 0.0f) {
            UString performanceDeltaString = UString::format("%.1fpp", this->fPPDelta);
            if(this->fPPDelta > 0.0f) performanceDeltaString.insert(0, L'+');

            const float border = (int)(iconBorder);

            const float height = this->vSize.y * 0.5f;
            const float scale = (height / deltaFont->getHeight()) * deltaScale;

            const float performanceDeltaStringWidth = deltaFont->getStringWidth(performanceDeltaString) * scale;

            const Vector2 backgroundSize =
                Vector2(performanceDeltaStringWidth + border, deltaFont->getHeight() * scale + border * 3);
            const Vector2 pos =
                Vector2(this->vPos.x + this->vSize.x - performanceDeltaStringWidth - border, this->vPos.y + border);
            const Vector2 textPos = Vector2(pos.x, pos.y + deltaFont->getHeight() * scale);

            // background (to ensure readability even with stupid long usernames)
            g->setColor(COLORf(1.0f, 0.f, 0.f, 0.f));
            g->setAlpha(1.0f - (1.0f - this->fPPDeltaAnim) * (1.0f - this->fPPDeltaAnim));
            g->fillRect(pos.x, pos.y, backgroundSize.x, backgroundSize.y);

            // delta text
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate((int)textPos.x, (int)textPos.y);

                g->translate(1, 1);
                g->setColor(0xff000000);
                g->setAlpha(this->fPPDeltaAnim);
                g->drawString(deltaFont, performanceDeltaString);

                g->translate(-1, -1);
                g->setColor(this->fPPDelta > 0.0f ? 0xff00ff00 : 0xffff0000);
                g->setAlpha(this->fPPDeltaAnim);
                g->drawString(deltaFont, performanceDeltaString);
            }
            g->popTransform();
        }
    }
}

void UserCard::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;

    if(this->user_id > 0) {
        UserInfo *my = get_user_info(this->user_id);
        this->sText = my->name;

        static i64 total_score = 0;
        if(total_score != my->total_score) {
            total_score = my->total_score;
            this->updateUserStats();
        }
    }

    if(this->avatar) {
        this->avatar->mouse_update(propagate_clicks);
        if(!*propagate_clicks) return;
    }

    CBaseUIButton::mouse_update(propagate_clicks);
}

void UserCard::updateUserStats() {
    Database::PlayerStats stats = db->calculatePlayerStats(this->sText);

    if(this->user_id > 0) {
        UserInfo *my = get_user_info(this->user_id);

        int level = Database::getLevelForScore(my->total_score);
        float percentToNextLevel = 1.f;
        u32 score_for_current_level = Database::getRequiredScoreForLevel(level);
        u32 score_for_next_level = Database::getRequiredScoreForLevel(level + 1);
        if(score_for_next_level > score_for_current_level) {
            percentToNextLevel = (float)(my->total_score - score_for_current_level) /
                                 (float)(score_for_next_level - score_for_current_level);
        }

        stats = Database::PlayerStats{
            .name = my->name,
            .pp = (float)my->pp,
            .accuracy = my->accuracy,
            .numScoresWithPP = 0,
            .level = level,
            .percentToNextLevel = percentToNextLevel,
            .totalScore = (u32)my->total_score,
        };
    }

    const bool changedPP = (this->fPP != stats.pp);
    const bool changed = (changedPP || this->fAcc != stats.accuracy || this->iLevel != stats.level ||
                          this->fPercentToNextLevel != stats.percentToNextLevel);

    const bool isFirstLoad = (this->fPP == 0.0f);
    const float newPPDelta = (stats.pp - this->fPP);
    if(newPPDelta != 0.0f) this->fPPDelta = newPPDelta;

    this->fPP = stats.pp;
    this->fAcc = stats.accuracy;
    this->iLevel = stats.level;
    this->fPercentToNextLevel = stats.percentToNextLevel;

    if(changed) {
        if(changedPP && !isFirstLoad && this->fPPDelta != 0.0f && this->fPP != 0.0f) {
            this->fPPDeltaAnim = 1.0f;
            anim->moveLinear(&this->fPPDeltaAnim, 0.0f, 25.0f, true);
        }
    }
}

void UserCard::setID(i32 new_id) {
    SAFE_DELETE(this->avatar);

    this->user_id = new_id;
    if(this->user_id > 0) {
        this->avatar = new UIAvatar(this->user_id, 0.f, 0.f, 0.f, 0.f);
        this->avatar->on_screen = true;
        this->sText = "Mysterious user";
    } else {
        this->sText = cv_name.getString();
    }
}
