#include "AnimationHandler.h"
#include "Engine.h"
#include "LeaderboardPPCalcThread.h"
#include "LoudnessCalcThread.h"
#include "MapCalcThread.h"
#include "Mouse.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "ScoreConverterThread.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser.h"
#include "UserCard.h"

static enum { MODE = 0, MODS = 1, RANDOM = 2, OPTIONS = 3 };
static i32 hovered_btn = -1;
static f32 global_scale = 1.f;
static f32 btns_x[4] = {0.f, 0.f, 0.f, 0.f};
static f32 btn_hovers[4] = {0.f, 0.f, 0.f, 0.f};
static f32 btn_widths[4] = {0.f, 0.f, 0.f, 0.f};
static f32 btn_heights[4] = {0.f, 0.f, 0.f, 0.f};

void press_bottombar_button(i32 btn_index) {
    if(hovered_btn != btn_index) {
        btn_hovers[btn_index] = 1.f;
        anim->moveLinear(&btn_hovers[btn_index], 0.0f, 0.1f, 0.05f, true);
    }

    switch(btn_index) {
        case MODE:
            osu->getSongBrowser()->onSelectionMode();
            break;
        case MODS:
            osu->getSongBrowser()->onSelectionMods();
            break;
        case RANDOM:
            osu->getSongBrowser()->onSelectionRandom();
            break;
        case OPTIONS:
            osu->getSongBrowser()->onSelectionOptions();
            break;
        default:
            abort();
    }
}

f32 get_bottombar_height() {
    f32 max = 0.f;
    for(i32 i = 0; i < 4; i++) {
        if(btn_heights[i] > max) {
            max = btn_heights[i];
        }
    }
    return max * global_scale;
}

void update_bottombar(bool* propagate_clicks) {
    static bool mouse_was_down = false;

    auto mouse = engine->getMouse()->getPos();
    bool clicked = !mouse_was_down && engine->getMouse()->isLeftDown();
    mouse_was_down = engine->getMouse()->isLeftDown();

    auto screen = engine->getScreenSize();
    bool is_widescreen =
        ((int)(std::max(0, (int)((osu->getScreenWidth() - (osu->getScreenHeight() * 4.0f / 3.0f)) / 2.0f))) > 0);

    // selection-mode has special logic: if the image exceeds screen width, we'll rescale
    // the whole bottom bar to fit the screen.
    auto mode_img = osu->getSkin()->selectionModeOver;
    btns_x[MODE] = Osu::getUIScale((is_widescreen ? 140.0f : 120.0f));
    {
        auto img = osu->getSkin()->selectionMode;
        f32 max_width = engine->getScreenWidth() - btns_x[MODE];
        f32 img_width = img->getImageSizeForCurrentFrame().x * (img->is_2x ? 0.5f : 1.f);
        f32 scale = max_width / img_width;
        global_scale = std::min(1.f, scale);
    }
    btn_widths[MODE] = global_scale * mode_img->getImageSizeForCurrentFrame().x * (mode_img->is_2x ? 0.5f : 1.f);
    btn_heights[MODE] = global_scale * mode_img->getImageSizeForCurrentFrame().y * (mode_img->is_2x ? 0.5f : 1.f);

    auto mods_img = osu->getSkin()->selectionModsOver;
    btns_x[MODS] = btns_x[MODE] + (i32)(92.5f * global_scale);
    btn_widths[MODS] = global_scale * mods_img->getImageSizeForCurrentFrame().x * (mods_img->is_2x ? 0.5f : 1.f);
    btn_heights[MODS] = global_scale * mods_img->getImageSizeForCurrentFrame().y * (mods_img->is_2x ? 0.5f : 1.f);
    auto random_img = osu->getSkin()->selectionRandomOver;
    btns_x[RANDOM] = btns_x[MODS] + (i32)(77.5f * global_scale);
    btn_widths[RANDOM] = global_scale * random_img->getImageSizeForCurrentFrame().x * (random_img->is_2x ? 0.5f : 1.f);
    btn_heights[RANDOM] = global_scale * random_img->getImageSizeForCurrentFrame().y * (random_img->is_2x ? 0.5f : 1.f);
    auto options_img = osu->getSkin()->selectionOptionsOver;
    btns_x[OPTIONS] = btns_x[RANDOM] + (i32)(77.5f * global_scale);
    btn_widths[OPTIONS] =
        global_scale * options_img->getImageSizeForCurrentFrame().x * (options_img->is_2x ? 0.5f : 1.f);
    btn_heights[OPTIONS] =
        global_scale * options_img->getImageSizeForCurrentFrame().y * (options_img->is_2x ? 0.5f : 1.f);

    // NOTE: Skin template shows 640:150px size, and 320px from options button origin, I cheated a bit
    osu->userButton->setSize(global_scale * 640 * 0.5, global_scale * 176 * 0.5);
    osu->userButton->setPos(btns_x[OPTIONS] + (i32)(global_scale * 290.f * 0.5f),
                            engine->getScreenHeight() - osu->userButton->getSize().y);
    osu->userButton->mouse_update(propagate_clicks);

    // Yes, the order looks whack. That's the correct order.
    i32 new_hover = -1;
    if(mouse.x > btns_x[OPTIONS] && mouse.x < btns_x[OPTIONS] + btn_widths[OPTIONS] &&
       mouse.y > screen.y - btn_heights[OPTIONS]) {
        new_hover = OPTIONS;
    } else if(mouse.x > btns_x[MODE] && mouse.x < btns_x[MODE] + btn_widths[MODE] &&
              mouse.y > screen.y - btn_heights[MODE]) {
        new_hover = MODE;
    } else if(mouse.x > btns_x[MODS] && mouse.x < btns_x[MODS] + btn_widths[MODS] &&
              mouse.y > screen.y - btn_heights[MODS]) {
        new_hover = MODS;
    } else if(mouse.x > btns_x[RANDOM] && mouse.x < btns_x[RANDOM] + btn_widths[RANDOM] &&
              mouse.y > screen.y - btn_heights[RANDOM]) {
        new_hover = RANDOM;
    } else {
        clicked = false;
    }

    if(hovered_btn != new_hover) {
        if(hovered_btn != -1) {
            anim->moveLinear(&btn_hovers[hovered_btn], 0.0f, btn_hovers[hovered_btn] * 0.1f, true);
        }
        if(new_hover != -1) {
            anim->moveLinear(&btn_hovers[new_hover], 1.f, 0.1f, true);
        }

        hovered_btn = new_hover;
    }

    if(clicked && *propagate_clicks) {
        *propagate_clicks = false;
        press_bottombar_button(hovered_btn);
    }

    // TODO @kiwec: do hovers make sound?
}

void draw_bottombar(Graphics* g) {
    g->pushTransform();
    {
        Image* img = osu->getSkin()->songSelectBottom;
        g->setColor(0xffffffff);
        g->scale((f32)engine->getScreenWidth() / (f32)img->getWidth(), get_bottombar_height() / (f32)img->getHeight());
        g->translate(0, engine->getScreenHeight() - get_bottombar_height());
        g->drawImage(img, AnchorPoint::TOP_LEFT);
    }
    g->popTransform();

    // Draw the user card under selection elements, which can cover it for fancy effects
    // (we don't match stable perfectly, but close enough)
    osu->userButton->draw(g);

    SkinImage* base_imgs[4] = {osu->getSkin()->selectionMode, osu->getSkin()->selectionMods,
                               osu->getSkin()->selectionRandom, osu->getSkin()->selectionOptions};
    for(i32 i = 0; i < 4; i++) {
        if(base_imgs[i] == NULL) continue;

        f32 scale = global_scale * (base_imgs[i]->is_2x ? 0.5f : 1.f);
        g->setColor(0xffffffff);
        base_imgs[i]->drawRaw(g, Vector2(btns_x[i], engine->getScreenHeight()), scale, AnchorPoint::BOTTOM_LEFT);
    }

    // Ok, and now for the hover images... which are drawn in a weird order, same as update_bottombar()
    auto random_img = osu->getSkin()->selectionRandomOver;
    f32 random_scale = global_scale * (random_img->is_2x ? 0.5f : 1.f);
    g->setColor(0xffffffff);
    g->setAlpha(btn_hovers[RANDOM]);
    random_img->drawRaw(g, Vector2(btns_x[RANDOM], engine->getScreenHeight()), random_scale, AnchorPoint::BOTTOM_LEFT);

    auto mods_img = osu->getSkin()->selectionModsOver;
    f32 mods_scale = global_scale * (mods_img->is_2x ? 0.5f : 1.f);
    g->setColor(0xffffffff);
    g->setAlpha(btn_hovers[MODS]);
    mods_img->drawRaw(g, Vector2(btns_x[MODS], engine->getScreenHeight()), mods_scale, AnchorPoint::BOTTOM_LEFT);

    auto mode_img = osu->getSkin()->selectionModeOver;
    f32 mode_scale = global_scale * (mode_img->is_2x ? 0.5f : 1.f);
    g->setColor(0xffffffff);
    g->setAlpha(btn_hovers[MODE]);
    mode_img->drawRaw(g, Vector2(btns_x[MODE], engine->getScreenHeight()), mode_scale, AnchorPoint::BOTTOM_LEFT);

    auto options_img = osu->getSkin()->selectionOptionsOver;
    f32 options_scale = global_scale * (options_img->is_2x ? 0.5f : 1.f);
    g->setColor(0xffffffff);
    g->setAlpha(btn_hovers[OPTIONS]);
    options_img->drawRaw(g, Vector2(btns_x[OPTIONS], engine->getScreenHeight()), options_scale,
                         AnchorPoint::BOTTOM_LEFT);

    // mode-osu-small (often used as overlay)
    auto mos_img = osu->getSkin()->mode_osu_small;
    if(mos_img != NULL) {
        f32 mos_scale = global_scale * (mos_img->is_2x ? 0.5f : 1.f);
        g->setColor(0xffffffff);
        mos_img->drawRaw(g,
                         Vector2(btns_x[MODE] + (btns_x[MODS] - btns_x[MODE]) * 0.5f,
                                 engine->getScreenHeight() - (global_scale * 56.f)),
                         mos_scale, AnchorPoint::CENTER);
    }

    // background task busy notification
    McFont* font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    i32 calcx = osu->userButton->getPos().x + osu->userButton->getSize().x + 20;
    i32 calcy = osu->userButton->getPos().y + 30;
    if(mct_total.load() > 0) {
        UString msg = UString::format("Calculating stars (%i/%i) ...", mct_computed.load(), mct_total.load());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    if(cv_normalize_loudness.getBool() && loct_total() > 0 && loct_computed() < loct_total()) {
        UString msg = UString::format("Computing loudness (%i/%i) ...", loct_computed(), loct_total());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    if(sct_total.load() > 0 && sct_computed.load() < sct_total.load()) {
        UString msg = UString::format("Converting scores (%i/%i) ...", sct_computed.load(), sct_total.load());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
}

// TODO @kiwec: draw mode-osu
// TODO @kiwec: remake usercard so it matches stable more closely
// TODO @kiwec: test multiple skins on multiple resolutions
