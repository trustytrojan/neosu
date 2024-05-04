#include "ScoreboardSlot.h"

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoUsers.h"
#include "Engine.h"
#include "Font.h"
#include "Osu.h"
#include "Skin.h"
#include "SkinImage.h"

ScoreboardSlot::ScoreboardSlot(SCORE_ENTRY score, int index) {
    m_avatar = new UIAvatar(score.player_id, 0, 0, 0, 0);
    m_score = score;
    m_index = index;

    auto user = get_user_info(score.player_id);
    is_friend = user->is_friend();
}

ScoreboardSlot::~ScoreboardSlot() {
    anim->deleteExistingAnimation(&m_fAlpha);
    anim->deleteExistingAnimation(&m_fFlash);
    anim->deleteExistingAnimation(&m_y);

    delete m_avatar;
}

void ScoreboardSlot::draw(Graphics *g) {
    if(m_fAlpha == 0.f) return;
    if(!convar->getConVarByName("osu_draw_scoreboard")->getBool() && !bancho.is_playing_a_multi_map()) return;
    if(!convar->getConVarByName("osu_draw_scoreboard_mp")->getBool() && bancho.is_playing_a_multi_map()) return;

    g->pushTransform();

    g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);

    const float height = roundf(bancho.osu->getScreenHeight() * 0.07f);
    const float width = roundf(height * 2.6f);  // does not include avatar_width
    const float avatar_height = height;
    const float avatar_width = avatar_height;
    const float padding = roundf(height * 0.05f);

    float start_y = bancho.osu->getScreenHeight() / 2.0f - (height * 2.5f);
    start_y += m_y * height;
    start_y = roundf(start_y);

    if(m_fFlash > 0.f && !convar->getConVarByName("avoid_flashes")->getBool()) {
        g->setColor(0xffffffff);
        g->setAlpha(m_fFlash);
        g->fillRect(0, start_y, avatar_width + width, height);
    }

    // Draw background
    if(m_score.dead) {
        g->setColor(0xff660000);
    } else if(m_score.highlight) {
        g->setColor(0xff777777);
    } else if(m_index == 0) {
        g->setColor(0xff1b6a8c);
    } else {
        g->setColor(is_friend ? 0xff9C205C : 0xff114459);
    }
    g->setAlpha(0.3f * m_fAlpha);

    if(convar->getConVarByName("osu_hud_scoreboard_use_menubuttonbackground")->getBool()) {
        float bg_scale = 0.625f;
        auto bg_img = bancho.osu->getSkin()->getMenuButtonBackground2();
        float oScale = bg_img->getResolutionScale() * 0.99f;
        g->fillRect(0, start_y, avatar_width, height);
        bg_img->draw(g,
                     Vector2(avatar_width + (bg_img->getSizeBase().x / 2) * bg_scale - (470 * oScale) * bg_scale,
                             start_y + height / 2),
                     bg_scale);
    } else {
        g->fillRect(0, start_y, avatar_width + width, height);
    }

    // Draw avatar
    m_avatar->on_screen = true;
    m_avatar->setPos(0, start_y);
    m_avatar->setSize(avatar_width, avatar_height);
    m_avatar->setVisible(true);
    m_avatar->draw_avatar(g, 0.8f * m_fAlpha);

    // Draw index
    g->pushTransform();
    {
        McFont *indexFont = bancho.osu->getSongBrowserFontBold();
        UString indexString = UString::format("%i", m_index + 1);
        const float scale = (avatar_height / indexFont->getHeight()) * 0.5f;

        g->scale(scale, scale);

        // * 0.9f because the returned font height isn't accurate :c
        g->translate(avatar_width / 2.0f - (indexFont->getStringWidth(indexString) * scale / 2.0f),
                     start_y + (avatar_height / 2.0f) + indexFont->getHeight() * scale / 2.0f * 0.9f);

        g->translate(0.5f, 0.5f);
        g->setColor(0xff000000);
        g->setAlpha(0.3f * m_fAlpha);
        g->drawString(indexFont, indexString);

        g->translate(-0.5f, -0.5f);
        g->setColor(0xffffffff);
        g->setAlpha(0.7f * m_fAlpha);
        g->drawString(indexFont, indexString);
    }
    g->popTransform();

    // Draw name
    const bool drawTextShadow = (convar->getConVarByName("osu_background_dim")->getFloat() < 0.7f);
    const Color textShadowColor = 0x66000000;
    const float nameScale = 0.315f;
    g->pushTransform();
    {
        McFont *nameFont = bancho.osu->getSongBrowserFont();
        g->pushClipRect(McRect(avatar_width, start_y, width, height));

        const float scale = (height / nameFont->getHeight()) * nameScale;
        g->scale(scale, scale);
        g->translate(avatar_width + padding, start_y + padding + nameFont->getHeight() * scale);
        if(drawTextShadow) {
            g->translate(1, 1);
            g->setColor(textShadowColor);
            g->setAlpha(m_fAlpha);
            g->drawString(nameFont, m_score.name);
            g->translate(-1, -1);
        }

        if(m_score.dead) {
            g->setColor(0xffee0000);
        } else if(m_score.highlight) {
            g->setColor(0xffffffff);
        } else if(m_index == 0) {
            g->setColor(0xffeeeeee);
        } else {
            g->setColor(0xffaaaaaa);
        }

        g->setAlpha(m_fAlpha);
        g->drawString(nameFont, m_score.name);
        g->popClipRect();
    }
    g->popTransform();

    // Draw combo
    const Color comboAccuracyColor = 0xff5d9ca1;
    const Color comboAccuracyColorHighlight = 0xff99fafe;
    const Color comboAccuracyColorTop = 0xff84dbe0;
    const float comboScale = 0.26f;
    McFont *scoreFont = bancho.osu->getSongBrowserFont();
    McFont *comboFont = scoreFont;
    McFont *accFont = comboFont;
    g->pushTransform();
    {
        const float scale = (height / comboFont->getHeight()) * comboScale;

        UString comboString = UString::format("%ix", m_score.combo);
        const float stringWidth = comboFont->getStringWidth(comboString);

        g->scale(scale, scale);
        g->translate(avatar_width + width - stringWidth * scale - padding * 1.35f, start_y + height - 2 * padding);
        if(drawTextShadow) {
            g->translate(1, 1);
            g->setColor(textShadowColor);
            g->setAlpha(m_fAlpha);
            g->drawString(comboFont, comboString);
            g->translate(-1, -1);
        }

        if(m_score.highlight) {
            g->setColor(comboAccuracyColorHighlight);
        } else if(m_index == 0) {
            g->setColor(comboAccuracyColorTop);
        } else {
            g->setColor(comboAccuracyColor);
        }

        g->setAlpha(m_fAlpha);
        g->drawString(scoreFont, comboString);
    }
    g->popTransform();

    // draw accuracy
    if(bancho.is_playing_a_multi_map() && bancho.room.win_condition == ACCURACY) {
        const float accScale = comboScale;
        g->pushTransform();
        {
            const float scale = (height / accFont->getHeight()) * accScale;
            UString accString = UString::format("%.2f%%", m_score.accuracy * 100.0f);
            g->scale(scale, scale);
            g->translate(avatar_width + padding * 1.35f, start_y + height - 2 * padding);
            {
                g->translate(1, 1);
                g->setColor(textShadowColor);
                g->setAlpha(m_fAlpha);
                g->drawString(accFont, accString);
                g->translate(-1, -1);
            }

            if(m_score.highlight) {
                g->setColor(comboAccuracyColorHighlight);
            } else if(m_index == 0) {
                g->setColor(comboAccuracyColorTop);
            } else {
                g->setColor(comboAccuracyColor);
            }

            g->setAlpha(m_fAlpha);
            g->drawString(accFont, accString);
        }

        g->popTransform();
    } else {
        // draw score
        const float scoreScale = comboScale;
        g->pushTransform();
        {
            const float scale = (height / scoreFont->getHeight()) * scoreScale;

            UString scoreString = UString::format("%llu", m_score.score);

            g->scale(scale, scale);
            g->translate(avatar_width + padding * 1.35f, start_y + height - 2 * padding);
            if(drawTextShadow) {
                g->translate(1, 1);
                g->setColor(textShadowColor);
                g->setAlpha(m_fAlpha);
                g->drawString(scoreFont, scoreString);
                g->translate(-1, -1);
            }

            if(m_score.dead) {
                g->setColor(0xffee0000);
            } else if(m_score.highlight) {
                g->setColor(0xffffffff);
            } else if(m_index == 0) {
                g->setColor(0xffeeeeee);
            } else {
                g->setColor(0xffaaaaaa);
            }

            g->setAlpha(m_fAlpha);
            g->drawString(scoreFont, scoreString);
        }
        g->popTransform();
    }

    g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);

    g->popTransform();
}

void ScoreboardSlot::updateIndex(int new_index, bool animate) {
    bool is_player = bancho.osu->m_hud->player_slot == this;
    int player_idx = bancho.osu->m_hud->player_slot->m_index;
    if(is_player) {
        if(animate && new_index < m_index) {
            m_fFlash = 1.f;
            anim->moveQuartOut(&m_fFlash, 0.0f, 0.5f, 0.0f, true);
        }

        // Ensure the player is always visible
        player_idx = new_index;
    }

    int min_visible_idx = player_idx - 4;
    if(min_visible_idx < 0) min_visible_idx = 0;

    int max_visible_idx = player_idx;
    if(max_visible_idx < 5) max_visible_idx = 5;

    bool is_visible = new_index == 0 || (new_index >= min_visible_idx && new_index <= max_visible_idx);

    float scoreboard_y = 0;
    if(min_visible_idx == 0) {
        scoreboard_y = new_index;
    } else if(new_index > 0) {
        scoreboard_y = (new_index + 1) - min_visible_idx;
    }

    if(was_visible && !is_visible) {
        if(animate) {
            anim->moveQuartOut(&m_y, scoreboard_y, 0.5f, 0.0f, true);
            anim->moveQuartOut(&m_fAlpha, 0.0f, 0.5f, 0.0f, true);
        } else {
            m_y = scoreboard_y;
            m_fAlpha = 0.0f;
        }
        was_visible = false;
    } else if(!was_visible && is_visible) {
        anim->deleteExistingAnimation(&m_y);
        m_y = scoreboard_y;
        if(animate) {
            m_fAlpha = 0.f;
            anim->moveQuartOut(&m_fAlpha, 1.0f, 0.5f, 0.0f, true);
        } else {
            m_fAlpha = 1.0f;
        }
        was_visible = true;
    } else if(was_visible || is_visible) {
        if(animate) {
            anim->moveQuartOut(&m_y, scoreboard_y, 0.5f, 0.0f, true);
        } else {
            m_y = scoreboard_y;
        }
    }

    m_index = new_index;
}
