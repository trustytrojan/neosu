#include "Slider.h"

#include <utility>

#include "AnimationHandler.h"
#include "Bancho.h"
#include "Beatmap.h"
#include "Camera.h"
#include "Circle.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "ModFPoSu.h"
#include "Osu.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SliderCurves.h"
#include "SliderRenderer.h"
#include "SoundEngine.h"
#include "VertexArrayObject.h"



Slider::Slider(char stype, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds,
               std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType,
               int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, BeatmapInterface *beatmap)
    : HitObject(time, sampleType, comboNumber, isEndOfCombo, colorCounter, colorOffset, beatmap) {
    this->type = HitObjectType::SLIDER;

    this->cType = stype;
    this->iRepeat = repeat;
    this->fPixelLength = pixelLength;
    this->points = std::move(points);
    this->hitSounds = std::move(hitSounds);
    this->fSliderTime = sliderTime;
    this->fSliderTimeWithoutRepeats = sliderTimeWithoutRepeats;

    // build raw ticks
    for(int i = 0; i < ticks.size(); i++) {
        SLIDERTICK st;
        st.finished = false;
        st.percent = ticks[i];
        this->ticks.push_back(st);
    }

    // build curve
    this->curve = SliderCurve::createCurve(this->cType, this->points, this->fPixelLength);

    // build repeats
    for(int i = 0; i < (this->iRepeat - 1); i++) {
        SLIDERCLICK sc;

        sc.finished = false;
        sc.successful = false;
        sc.type = 0;
        sc.sliderend = ((i % 2) == 0);  // for hit animation on repeat hit
        sc.time = this->click_time + (long)(this->fSliderTimeWithoutRepeats * (i + 1));

        this->clicks.push_back(sc);
    }

    // build ticks
    for(int i = 0; i < this->iRepeat; i++) {
        for(int t = 0; t < this->ticks.size(); t++) {
            // NOTE: repeat ticks are not necessarily symmetric.
            //
            // e.g. this slider: [1]=======*==[2]
            //
            // the '*' is where the tick is, let's say percent = 0.75
            // on repeat 0, the tick is at: click_time + 0.75*m_fSliderTimeWithoutRepeats
            // but on repeat 1, the tick is at: click_time + 1*m_fSliderTimeWithoutRepeats + (1.0 -
            // 0.75)*m_fSliderTimeWithoutRepeats this gives better readability at the cost of invalid rhythms: ticks are
            // guaranteed to always be at the same position, even in repeats so, depending on which repeat we are in
            // (even or odd), we either do (percent) or (1.0 - percent)

            const float tickPercentRelativeToRepeatFromStartAbs =
                (((i + 1) % 2) != 0 ? this->ticks[t].percent : 1.0f - this->ticks[t].percent);

            SLIDERCLICK sc;

            sc.time = this->click_time + (long)(this->fSliderTimeWithoutRepeats * i) +
                      (long)(tickPercentRelativeToRepeatFromStartAbs * this->fSliderTimeWithoutRepeats);
            sc.finished = false;
            sc.successful = false;
            sc.type = 1;
            sc.tickIndex = t;

            this->clicks.push_back(sc);
        }
    }

    this->duration = (long)this->fSliderTime;
    this->duration = this->duration >= 0 ? this->duration : 1;  // force clamp to positive range

    this->fSlidePercent = 0.0f;
    this->fActualSlidePercent = 0.0f;
    this->fSliderSnakePercent = 0.0f;
    this->fReverseArrowAlpha = 0.0f;
    this->fBodyAlpha = 0.0f;

    this->startResult = LiveScore::HIT::HIT_NULL;
    this->endResult = LiveScore::HIT::HIT_NULL;
    this->bStartFinished = false;
    this->fStartHitAnimation = 0.0f;
    this->bEndFinished = false;
    this->fEndHitAnimation = 0.0f;
    this->fEndSliderBodyFadeAnimation = 0.0f;
    this->iStrictTrackingModLastClickHeldTime = 0;
    this->iFatFingerKey = 0;
    this->iPrevSliderSlideSoundSampleSet = -1;
    this->bCursorLeft = true;
    this->bCursorInside = false;
    this->bHeldTillEnd = false;
    this->bHeldTillEndForLenienceHack = false;
    this->bHeldTillEndForLenienceHackCheck = false;
    this->fFollowCircleTickAnimationScale = 0.0f;
    this->fFollowCircleAnimationScale = 0.0f;
    this->fFollowCircleAnimationAlpha = 0.0f;

    this->iReverseArrowPos = 0;
    this->iCurRepeat = 0;
    this->iCurRepeatCounterForHitSounds = 0;
    this->bInReverse = false;
    this->bHideNumberAfterFirstRepeatHit = false;

    this->vao = NULL;
}

Slider::~Slider() {
    this->onReset(0);

    SAFE_DELETE(this->curve);
    SAFE_DELETE(this->vao);
}

void Slider::draw() {
    if(this->points.size() <= 0) return;

    const float foscale = cv_circle_fade_out_scale.getFloat();
    Skin *skin = this->bm->getSkin();

    const bool isCompletelyFinished = this->bStartFinished && this->bEndFinished && this->bFinished;

    if((this->bVisible || (this->bStartFinished && !this->bFinished)) &&
       !isCompletelyFinished)  // extra possibility to avoid flicker between HitObject::m_bVisible delay and the
                               // fadeout animation below this if block
    {
        float alpha = (cv_mod_hd_slider_fast_fade.getBool() ? this->fAlpha : this->fBodyAlpha);
        float sliderSnake = cv_snaking_sliders.getBool() ? this->fSliderSnakePercent : 1.0f;

        // shrinking sliders
        float sliderSnakeStart = 0.0f;
        if(cv_slider_shrink.getBool() && this->iReverseArrowPos == 0) {
            sliderSnakeStart = (this->bInReverse ? 0.0f : this->fSlidePercent);
            if(this->bInReverse) sliderSnake = this->fSlidePercent;
        }

        // draw slider body
        if(alpha > 0.0f && cv_slider_draw_body.getBool()) this->drawBody(alpha, sliderSnakeStart, sliderSnake);

        // draw slider ticks
        Color tickColor = 0xffffffff;
        tickColor = Colors::scale(tickColor, this->fHittableDimRGBColorMultiplierPercent);
        const float tickImageScale =
            (this->bm->fHitcircleDiameter / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f))) * 0.125f;
        for(int t = 0; t < this->ticks.size(); t++) {
            if(this->ticks[t].finished || this->ticks[t].percent > sliderSnake) continue;

            Vector2 pos = this->bm->osuCoords2Pixels(this->curve->pointAt(this->ticks[t].percent));

            g->setColor(tickColor);
            g->setAlpha(alpha);
            g->pushTransform();
            {
                g->scale(tickImageScale, tickImageScale);
                g->translate(pos.x, pos.y);
                g->drawImage(skin->getSliderScorePoint());
            }
            g->popTransform();
        }

        // draw start & end circle & reverse arrows
        if(this->points.size() > 1) {
            // HACKHACK: very dirty code
            bool sliderRepeatStartCircleFinished = (this->iRepeat < 2);
            bool sliderRepeatEndCircleFinished = false;
            bool endCircleIsAtActualSliderEnd = true;
            for(int i = 0; i < this->clicks.size(); i++) {
                // repeats
                if(this->clicks[i].type == 0) {
                    endCircleIsAtActualSliderEnd = this->clicks[i].sliderend;

                    if(endCircleIsAtActualSliderEnd)
                        sliderRepeatEndCircleFinished = this->clicks[i].finished;
                    else
                        sliderRepeatStartCircleFinished = this->clicks[i].finished;
                }
            }

            const bool ifStrictTrackingModShouldDrawEndCircle =
                (!cv_mod_strict_tracking.getBool() || this->endResult != LiveScore::HIT::HIT_MISS);

            // end circle
            if((!this->bEndFinished && this->iRepeat % 2 != 0 && ifStrictTrackingModShouldDrawEndCircle) ||
               (!sliderRepeatEndCircleFinished &&
                (ifStrictTrackingModShouldDrawEndCircle || (this->iRepeat > 1 && endCircleIsAtActualSliderEnd) ||
                 (this->iRepeat > 1 && std::abs(this->iRepeat - this->iCurRepeat) > 2))))
                this->drawEndCircle(alpha, sliderSnake);

            // start circle
            if(!this->bStartFinished ||
               (!sliderRepeatStartCircleFinished &&
                (ifStrictTrackingModShouldDrawEndCircle || (this->iRepeat > 1 && !endCircleIsAtActualSliderEnd) ||
                 (this->iRepeat > 1 && std::abs(this->iRepeat - this->iCurRepeat) > 2))) ||
               (!this->bEndFinished && this->iRepeat % 2 == 0 && ifStrictTrackingModShouldDrawEndCircle))
                this->drawStartCircle(alpha);

            // reverse arrows
            if(this->fReverseArrowAlpha > 0.0f) {
                // if the combo color is nearly white, blacken the reverse arrow
                Color comboColor = skin->getComboColorForCounter(this->iColorCounter, this->iColorOffset);
                Color reverseArrowColor = 0xffffffff;
                if((comboColor.Rf() + comboColor.Gf() + comboColor.Bf()) / 3.0f >
                   cv_slider_reverse_arrow_black_threshold.getFloat())
                    reverseArrowColor = 0xff000000;

                reverseArrowColor = Colors::scale(reverseArrowColor, this->fHittableDimRGBColorMultiplierPercent);

                float div = 0.30f;
                float pulse = (div - fmod(std::abs(this->bm->getCurMusicPos()) / 1000.0f, div)) / div;
                pulse *= pulse;  // quad in

                if(!cv_slider_reverse_arrow_animated.getBool() || this->bm->isInMafhamRenderChunk()) pulse = 0.0f;

                // end
                if(this->iReverseArrowPos == 2 || this->iReverseArrowPos == 3) {
                    Vector2 pos = this->bm->osuCoords2Pixels(this->curve->pointAt(1.0f));
                    float rotation = this->curve->getEndAngle() - cv_playfield_rotation.getFloat() -
                                     this->bm->getPlayfieldRotation();
                    if((this->bm->getModsLegacy() & LegacyFlags::HardRock)) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_horizontal.getBool()) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_vertical.getBool()) rotation = 180.0f - rotation;

                    const float osuCoordScaleMultiplier =
                        this->bm->fHitcircleDiameter / this->bm->fRawHitcircleDiameter;
                    float reverseArrowImageScale =
                        (this->bm->fRawHitcircleDiameter / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) *
                        osuCoordScaleMultiplier;

                    reverseArrowImageScale *= 1.0f + pulse * 0.30f;

                    g->setColor(reverseArrowColor);
                    g->setAlpha(this->fReverseArrowAlpha);
                    g->pushTransform();
                    {
                        g->rotate(rotation);
                        g->scale(reverseArrowImageScale, reverseArrowImageScale);
                        g->translate(pos.x, pos.y);
                        g->drawImage(skin->getReverseArrow());
                    }
                    g->popTransform();
                }

                // start
                if(this->iReverseArrowPos == 1 || this->iReverseArrowPos == 3) {
                    Vector2 pos = this->bm->osuCoords2Pixels(this->curve->pointAt(0.0f));
                    float rotation = this->curve->getStartAngle() - cv_playfield_rotation.getFloat() -
                                     this->bm->getPlayfieldRotation();
                    if((this->bm->getModsLegacy() & LegacyFlags::HardRock)) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_horizontal.getBool()) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_vertical.getBool()) rotation = 180.0f - rotation;

                    const float osuCoordScaleMultiplier =
                        this->bm->fHitcircleDiameter / this->bm->fRawHitcircleDiameter;
                    float reverseArrowImageScale =
                        (this->bm->fRawHitcircleDiameter / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) *
                        osuCoordScaleMultiplier;

                    reverseArrowImageScale *= 1.0f + pulse * 0.30f;

                    g->setColor(reverseArrowColor);
                    g->setAlpha(this->fReverseArrowAlpha);
                    g->pushTransform();
                    {
                        g->rotate(rotation);
                        g->scale(reverseArrowImageScale, reverseArrowImageScale);
                        g->translate(pos.x, pos.y);
                        g->drawImage(skin->getReverseArrow());
                    }
                    g->popTransform();
                }
            }
        }
    }

    // slider body fade animation, draw start/end circle hit animation
    bool instafade_slider_body = cv_instafade_sliders.getBool();
    bool instafade_slider_head = cv_instafade.getBool();
    if(!instafade_slider_body && this->fEndSliderBodyFadeAnimation > 0.0f &&
       this->fEndSliderBodyFadeAnimation != 1.0f && !(this->bm->getModsLegacy() & LegacyFlags::Hidden)) {
        std::vector<Vector2> emptyVector;
        std::vector<Vector2> alwaysPoints;
        alwaysPoints.push_back(this->bm->osuCoords2Pixels(this->curve->pointAt(this->fSlidePercent)));
        if(!cv_slider_shrink.getBool())
            this->drawBody(1.0f - this->fEndSliderBodyFadeAnimation, 0, 1);
        else if(cv_slider_body_lazer_fadeout_style.getBool())
            SliderRenderer::draw(emptyVector, alwaysPoints, this->bm->fHitcircleDiameter, 0.0f, 0.0f,
                                 this->bm->getSkin()->getComboColorForCounter(this->iColorCounter, this->iColorOffset),
                                 1.0f, 1.0f - this->fEndSliderBodyFadeAnimation, this->click_time);
    }

    if(!instafade_slider_head && cv_slider_sliderhead_fadeout.getBool() && this->fStartHitAnimation > 0.0f &&
       this->fStartHitAnimation != 1.0f && !(this->bm->getModsLegacy() & LegacyFlags::Hidden)) {
        float alpha = 1.0f - this->fStartHitAnimation;

        float scale = this->fStartHitAnimation;
        scale = -scale * (scale - 2.0f);  // quad out scale

        bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && this->iCurRepeat < 1;

        g->pushTransform();
        {
            g->scale((1.0f + scale * foscale), (1.0f + scale * foscale));
            if(this->iCurRepeat < 1) {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(skin->getAnimationSpeed(),
                                                                     !this->bm->isInMafhamRenderChunk()
                                                                         ? this->click_time - this->iApproachTime
                                                                         : this->bm->getCurMusicPosWithOffsets());
                skin->getSliderStartCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(), !this->bm->isInMafhamRenderChunk()
                                                   ? this->click_time - this->iApproachTime
                                                   : this->bm->getCurMusicPosWithOffsets());

                Circle::drawSliderStartCircle(this->bm, this->curve->pointAt(0.0f), this->combo_number,
                                              this->iColorCounter, this->iColorOffset, 1.0f, 1.0f, alpha, alpha,
                                              drawNumber);
            } else {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !this->bm->isInMafhamRenderChunk() ? this->click_time : this->bm->getCurMusicPosWithOffsets());
                skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !this->bm->isInMafhamRenderChunk() ? this->click_time : this->bm->getCurMusicPosWithOffsets());

                Circle::drawSliderEndCircle(this->bm, this->curve->pointAt(0.0f), this->combo_number,
                                            this->iColorCounter, this->iColorOffset, 1.0f, 1.0f, alpha, alpha,
                                            drawNumber);
            }
        }
        g->popTransform();
    }

    if(!instafade_slider_head && this->fEndHitAnimation > 0.0f && this->fEndHitAnimation != 1.0f &&
       !(this->bm->getModsLegacy() & LegacyFlags::Hidden)) {
        float alpha = 1.0f - this->fEndHitAnimation;

        float scale = this->fEndHitAnimation;
        scale = -scale * (scale - 2.0f);  // quad out scale

        g->pushTransform();
        {
            g->scale((1.0f + scale * foscale), (1.0f + scale * foscale));
            {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(skin->getAnimationSpeed(),
                                                                     !this->bm->isInMafhamRenderChunk()
                                                                         ? this->click_time - this->iFadeInTime
                                                                         : this->bm->getCurMusicPosWithOffsets());
                skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(skin->getAnimationSpeed(),
                                                                           !this->bm->isInMafhamRenderChunk()
                                                                               ? this->click_time - this->iFadeInTime
                                                                               : this->bm->getCurMusicPosWithOffsets());

                Circle::drawSliderEndCircle(this->bm, this->curve->pointAt(1.0f), this->combo_number,
                                            this->iColorCounter, this->iColorOffset, 1.0f, 1.0f, alpha, 0.0f, false);
            }
        }
        g->popTransform();
    }

    HitObject::draw();
}

void Slider::draw2() { this->draw2(true, false); }

void Slider::draw2(bool drawApproachCircle, bool drawOnlyApproachCircle) {
    HitObject::draw2();

    Skin *skin = this->bm->getSkin();

    // HACKHACK: so much code duplication aaaaaaah
    if((this->bVisible || (this->bStartFinished && !this->bFinished)) &&
       drawApproachCircle)  // extra possibility to avoid flicker between HitObject::m_bVisible delay and the fadeout
                            // animation below this if block
    {
        if(this->points.size() > 1) {
            // HACKHACK: very dirty code
            bool sliderRepeatStartCircleFinished = this->iRepeat < 2;
            for(int i = 0; i < this->clicks.size(); i++) {
                if(this->clicks[i].type == 0) {
                    if(!this->clicks[i].sliderend) sliderRepeatStartCircleFinished = this->clicks[i].finished;
                }
            }

            // start circle
            if(!this->bStartFinished || !sliderRepeatStartCircleFinished ||
               (!this->bEndFinished && this->iRepeat % 2 == 0)) {
                Circle::drawApproachCircle(this->bm, this->curve->pointAt(0.0f), this->combo_number,
                                           this->iColorCounter, this->iColorOffset,
                                           this->fHittableDimRGBColorMultiplierPercent, this->fApproachScale,
                                           this->fAlphaForApproachCircle, this->bOverrideHDApproachCircle);
            }
        }
    }

    if(drawApproachCircle && drawOnlyApproachCircle) return;

    // draw followcircle
    // HACKHACK: this is not entirely correct (due to m_bHeldTillEnd, if held within 300 range but then released, will
    // flash followcircle at the end)
    if((this->bVisible && this->bCursorInside &&
        (this->isClickHeldSlider() || (this->bm->getModsLegacy() & LegacyFlags::Autoplay) ||
         (this->bm->getModsLegacy() & LegacyFlags::Relax))) ||
       (this->bFinished && this->fFollowCircleAnimationAlpha > 0.0f && this->bHeldTillEnd)) {
        Vector2 point = this->bm->osuCoords2Pixels(this->vCurPointRaw);

        // HACKHACK: this is shit
        float tickAnimation =
            (this->fFollowCircleTickAnimationScale < 0.1f ? this->fFollowCircleTickAnimationScale / 0.1f
                                                          : (1.0f - this->fFollowCircleTickAnimationScale) / 0.9f);
        if(this->fFollowCircleTickAnimationScale < 0.1f) {
            tickAnimation = -tickAnimation * (tickAnimation - 2.0f);
            tickAnimation = std::clamp<float>(tickAnimation / 0.02f, 0.0f, 1.0f);
        }
        float tickAnimationScale = 1.0f + tickAnimation * cv_slider_followcircle_tick_pulse_scale.getFloat();

        g->setColor(0xffffffff);
        g->setAlpha(this->fFollowCircleAnimationAlpha);
        skin->getSliderFollowCircle2()->setAnimationTimeOffset(skin->getAnimationSpeed(), this->click_time);
        skin->getSliderFollowCircle2()->drawRaw(
            point,
            (this->bm->fSliderFollowCircleDiameter / skin->getSliderFollowCircle2()->getSizeBaseRaw().x) *
                tickAnimationScale * this->fFollowCircleAnimationScale *
                0.85f);  // this is a bit strange, but seems to work perfectly with 0.85
    }

    const bool isCompletelyFinished = this->bStartFinished && this->bEndFinished && this->bFinished;

    // draw sliderb on top of everything
    if((this->bVisible || (this->bStartFinished && !this->bFinished)) &&
       !isCompletelyFinished)  // extra possibility in the if-block to avoid flicker between HitObject::m_bVisible
                               // delay and the fadeout animation below this if-block
    {
        if(this->fSlidePercent > 0.0f) {
            // draw sliderb
            Vector2 point = this->bm->osuCoords2Pixels(this->vCurPointRaw);
            Vector2 c1 = this->bm->osuCoords2Pixels(this->curve->pointAt(
                this->fSlidePercent + 0.01f <= 1.0f ? this->fSlidePercent : this->fSlidePercent - 0.01f));
            Vector2 c2 = this->bm->osuCoords2Pixels(this->curve->pointAt(
                this->fSlidePercent + 0.01f <= 1.0f ? this->fSlidePercent + 0.01f : this->fSlidePercent));
            float ballAngle = glm::degrees(std::atan2(c2.y - c1.y, c2.x - c1.x));
            if(skin->getSliderBallFlip()) ballAngle += (this->iCurRepeat % 2 == 0) ? 0 : 180;

            g->setColor(skin->getAllowSliderBallTint()
                            ? (cv_slider_ball_tint_combo_color.getBool()
                                   ? skin->getComboColorForCounter(this->iColorCounter, this->iColorOffset)
                                   : skin->getSliderBallColor())
                            : rgb(255, 255, 255));
            g->pushTransform();
            {
                g->rotate(ballAngle);
                skin->getSliderb()->setAnimationTimeOffset(skin->getAnimationSpeed(), this->click_time);
                skin->getSliderb()->drawRaw(point,
                                            this->bm->fHitcircleDiameter / skin->getSliderb()->getSizeBaseRaw().x);
            }
            g->popTransform();
        }
    }
}

void Slider::drawStartCircle(float alpha) {
    Skin *skin = this->bm->getSkin();

    if(this->bStartFinished) {
        skin->getHitCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(),
            !this->bm->isInMafhamRenderChunk() ? this->click_time : this->bm->getCurMusicPosWithOffsets());
        skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(),
            !this->bm->isInMafhamRenderChunk() ? this->click_time : this->bm->getCurMusicPosWithOffsets());

        Circle::drawSliderEndCircle(this->bm, this->curve->pointAt(0.0f), this->combo_number, this->iColorCounter,
                                    this->iColorOffset, this->fHittableDimRGBColorMultiplierPercent, 1.0f, this->fAlpha,
                                    0.0f, false, false);
    } else {
        skin->getHitCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(), !this->bm->isInMafhamRenderChunk() ? this->click_time - this->iApproachTime
                                                                          : this->bm->getCurMusicPosWithOffsets());
        skin->getSliderStartCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(), !this->bm->isInMafhamRenderChunk() ? this->click_time - this->iApproachTime
                                                                          : this->bm->getCurMusicPosWithOffsets());

        Circle::drawSliderStartCircle(this->bm, this->curve->pointAt(0.0f), this->combo_number, this->iColorCounter,
                                      this->iColorOffset, this->fHittableDimRGBColorMultiplierPercent,
                                      this->fApproachScale, this->fAlpha, this->fAlpha,
                                      !this->bHideNumberAfterFirstRepeatHit, this->bOverrideHDApproachCircle);
    }
}

void Slider::drawEndCircle(float alpha, float sliderSnake) {
    Skin *skin = this->bm->getSkin();

    skin->getHitCircleOverlay2()->setAnimationTimeOffset(
        skin->getAnimationSpeed(), !this->bm->isInMafhamRenderChunk() ? this->click_time - this->iFadeInTime
                                                                      : this->bm->getCurMusicPosWithOffsets());
    skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
        skin->getAnimationSpeed(), !this->bm->isInMafhamRenderChunk() ? this->click_time - this->iFadeInTime
                                                                      : this->bm->getCurMusicPosWithOffsets());

    Circle::drawSliderEndCircle(this->bm, this->curve->pointAt(sliderSnake), this->combo_number, this->iColorCounter,
                                this->iColorOffset, this->fHittableDimRGBColorMultiplierPercent, 1.0f, this->fAlpha,
                                0.0f, false, false);
}

void Slider::drawBody(float alpha, float from, float to) {
    // smooth begin/end while snaking/shrinking
    std::vector<Vector2> alwaysPoints;
    if(cv_slider_body_smoothsnake.getBool()) {
        if(cv_slider_shrink.getBool() && this->fSliderSnakePercent > 0.999f) {
            alwaysPoints.push_back(this->bm->osuCoords2Pixels(this->curve->pointAt(this->fSlidePercent)));  // curpoint
            alwaysPoints.push_back(this->bm->osuCoords2Pixels(this->getRawPosAt(
                this->click_time + this->duration + 1)));  // endpoint (because setDrawPercent() causes the last
                                                           // circle mesh to become invisible too quickly)
        }
        if(cv_snaking_sliders.getBool() && this->fSliderSnakePercent < 1.0f)
            alwaysPoints.push_back(this->bm->osuCoords2Pixels(
                this->curve->pointAt(this->fSliderSnakePercent)));  // snakeoutpoint (only while snaking out)
    }

    const Color undimmedComboColor =
        this->bm->getSkin()->getComboColorForCounter(this->iColorCounter, this->iColorOffset);

    if(osu->shouldFallBackToLegacySliderRenderer()) {
        std::vector<Vector2> screenPoints = this->curve->getPoints();
        for(int p = 0; p < screenPoints.size(); p++) {
            screenPoints[p] = this->bm->osuCoords2Pixels(screenPoints[p]);
        }

        // peppy sliders
        SliderRenderer::draw(screenPoints, alwaysPoints, this->bm->fHitcircleDiameter, from, to, undimmedComboColor,
                             this->fHittableDimRGBColorMultiplierPercent, alpha, this->click_time);
    } else {
        // vertex buffered sliders
        // as the base mesh is centered at (0, 0, 0) and in raw osu coordinates, we have to scale and translate it to
        // make it fit the actual desktop playfield
        const float scale = GameRules::getPlayfieldScaleFactor();
        Vector2 translation = GameRules::getPlayfieldCenter();

        if(this->bm->hasFailed())
            translation =
                this->bm->osuCoords2Pixels(Vector2(GameRules::OSU_COORD_WIDTH / 2, GameRules::OSU_COORD_HEIGHT / 2));

        if(cv_mod_fps.getBool()) translation += this->bm->getFirstPersonCursorDelta();

        SliderRenderer::draw(this->vao, alwaysPoints, translation, scale, this->bm->fHitcircleDiameter, from, to,
                             undimmedComboColor, this->fHittableDimRGBColorMultiplierPercent, alpha, this->click_time);
    }
}

void Slider::update(long curPos, f64 frame_time) {
    HitObject::update(curPos, frame_time);

    if(this->bm != NULL) {
        // stop slide sound while paused
        if(this->bm->isPaused() || !this->bm->isPlaying() || this->bm->hasFailed()) {
            this->bm->getSkin()->stopSliderSlideSound();
        }

        // animations must be updated even if we are finished
        this->updateAnimations(curPos);
    }

    // all further calculations are only done while we are active
    if(this->bFinished) return;

    // slider slide percent
    this->fSlidePercent = 0.0f;
    if(curPos > this->click_time)
        this->fSlidePercent = std::clamp<float>(
            std::clamp<long>((curPos - (this->click_time)), 0, (long)this->fSliderTime) / this->fSliderTime, 0.0f,
            1.0f);

    this->fActualSlidePercent = this->fSlidePercent;

    const float sliderSnakeDuration =
        (1.0f / 3.0f) * this->iApproachTime * cv_slider_snake_duration_multiplier.getFloat();
    this->fSliderSnakePercent =
        std::min(1.0f, (curPos - (this->click_time - this->iApproachTime)) / (sliderSnakeDuration));

    const long reverseArrowFadeInStart =
        this->click_time -
        (cv_snaking_sliders.getBool() ? (this->iApproachTime - sliderSnakeDuration) : this->iApproachTime);
    const long reverseArrowFadeInEnd = reverseArrowFadeInStart + cv_slider_reverse_arrow_fadein_duration.getInt();
    this->fReverseArrowAlpha = 1.0f - std::clamp<float>(((float)(reverseArrowFadeInEnd - curPos) /
                                                         (float)(reverseArrowFadeInEnd - reverseArrowFadeInStart)),
                                                        0.0f, 1.0f);
    this->fReverseArrowAlpha *= cv_slider_reverse_arrow_alpha_multiplier.getFloat();

    this->fBodyAlpha = this->fAlpha;
    if((this->bi->getModsLegacy() & LegacyFlags::Hidden))  // hidden modifies the body alpha
    {
        this->fBodyAlpha = this->fAlphaWithoutHidden;  // fade in as usual

        // fade out over the duration of the slider, starting exactly when the default fadein finishes
        const long hiddenSliderBodyFadeOutStart =
            std::min(this->click_time,
                     this->click_time - this->iApproachTime +
                         this->iFadeInTime);  // std::min() ensures that the fade always starts at click_time
                                              // (even if the fadeintime is longer than the approachtime)
        const float fade_percent = cv_mod_hd_slider_fade_percent.getFloat();
        const long hiddenSliderBodyFadeOutEnd = this->click_time + (long)(fade_percent * this->fSliderTime);
        if(curPos >= hiddenSliderBodyFadeOutStart) {
            this->fBodyAlpha = std::clamp<float>(((float)(hiddenSliderBodyFadeOutEnd - curPos) /
                                                  (float)(hiddenSliderBodyFadeOutEnd - hiddenSliderBodyFadeOutStart)),
                                                 0.0f, 1.0f);
            this->fBodyAlpha *= this->fBodyAlpha;  // quad in body fadeout
        }
    }

    // if this slider is active, recalculate sliding/curve position and general state
    if(this->fSlidePercent > 0.0f || this->bVisible) {
        // handle reverse sliders
        this->bInReverse = false;
        this->bHideNumberAfterFirstRepeatHit = false;
        if(this->iRepeat > 1) {
            if(this->fSlidePercent > 0.0f && this->bStartFinished) this->bHideNumberAfterFirstRepeatHit = true;

            float part = 1.0f / (float)this->iRepeat;
            this->iCurRepeat = (int)(this->fSlidePercent * this->iRepeat);
            float baseSlidePercent = part * this->iCurRepeat;
            float partSlidePercent = (this->fSlidePercent - baseSlidePercent) / part;
            if(this->iCurRepeat % 2 == 0) {
                this->fSlidePercent = partSlidePercent;
                this->iReverseArrowPos = 2;
            } else {
                this->fSlidePercent = 1.0f - partSlidePercent;
                this->iReverseArrowPos = 1;
                this->bInReverse = true;
            }

            // no reverse arrow on the last repeat
            if(this->iCurRepeat == this->iRepeat - 1) this->iReverseArrowPos = 0;

            // osu style: immediately show all coming reverse arrows (even on the circle we just started from)
            if(this->iCurRepeat < this->iRepeat - 2 && this->fSlidePercent > 0.0f && this->iRepeat > 2)
                this->iReverseArrowPos = 3;
        }

        this->vCurPointRaw = this->curve->pointAt(this->fSlidePercent);
        this->vCurPoint = this->bi->osuCoords2Pixels(this->vCurPointRaw);
    } else {
        this->vCurPointRaw = this->curve->pointAt(0.0f);
        this->vCurPoint = this->bi->osuCoords2Pixels(this->vCurPointRaw);
    }

    // When fat finger key is released, remove isClickHeldSlider() restrictions
    if(this->iFatFingerKey == 1 && !this->bi->isKey1Down()) this->iFatFingerKey = 0;
    if(this->iFatFingerKey == 2 && !this->bi->isKey2Down()) this->iFatFingerKey = 0;

    // handle dynamic followradius
    float followRadius =
        this->bCursorLeft ? this->bi->fHitcircleDiameter / 2.0f : this->bi->fSliderFollowCircleDiameter / 2.0f;
    const bool isBeatmapCursorInside = ((this->bi->getCursorPos() - this->vCurPoint).length() < followRadius);
    const bool isAutoCursorInside =
        ((this->bi->getModsLegacy() & LegacyFlags::Autoplay) &&
         (!cv_auto_cursordance.getBool() || ((this->bi->getCursorPos() - this->vCurPoint).length() < followRadius)));
    this->bCursorInside = (isAutoCursorInside || isBeatmapCursorInside);
    this->bCursorLeft = !this->bCursorInside;

    // handle slider start
    if(!this->bStartFinished) {
        if((this->bi->getModsLegacy() & LegacyFlags::Autoplay)) {
            if(curPos >= this->click_time) {
                this->onHit(LiveScore::HIT::HIT_300, 0, false);
                this->bi->holding_slider = true;
            }
        } else {
            long delta = curPos - this->click_time;

            if((this->bi->getModsLegacy() & LegacyFlags::Relax)) {
                if(curPos >= this->click_time + (long)cv_relax_offset.getInt() && !this->bi->isPaused() &&
                   !this->bi->isContinueScheduled()) {
                    const Vector2 pos = this->bi->osuCoords2Pixels(this->curve->pointAt(0.0f));
                    const float cursorDelta = (this->bi->getCursorPos() - pos).length();
                    if((cursorDelta < this->bi->fHitcircleDiameter / 2.0f &&
                        (this->bi->getModsLegacy() & LegacyFlags::Relax))) {
                        LiveScore::HIT result = this->bi->getHitResult(delta);

                        if(result != LiveScore::HIT::HIT_NULL) {
                            const float targetDelta = cursorDelta / (this->bi->fHitcircleDiameter / 2.0f);
                            const float targetAngle = glm::degrees(
                                std::atan2(this->bi->getCursorPos().y - pos.y, this->bi->getCursorPos().x - pos.x));

                            this->startResult = result;
                            this->onHit(this->startResult, delta, false, targetDelta, targetAngle);
                            this->bi->holding_slider = true;
                        }
                    }
                }
            }

            // wait for a miss
            if(delta >= 0) {
                // if this is a miss after waiting
                if(delta > (long)this->bi->getHitWindow50()) {
                    this->startResult = LiveScore::HIT::HIT_MISS;
                    this->onHit(this->startResult, delta, false);
                    this->bi->holding_slider = false;
                }
            }
        }
    }

    // handle slider end, repeats, ticks
    if(!this->bEndFinished) {
        // NOTE: we have 2 timing conditions after which we start checking for strict tracking: 1) startcircle was
        // clicked, 2) slider has started timing wise it is easily possible to hit the startcircle way before the
        // sliderball would become active, which is why the first check exists. even if the sliderball has not yet
        // started sliding, you will be punished for leaving the (still invisible) followcircle area after having
        // clicked the startcircle, always.
        const bool isTrackingStrictTrackingMod =
            ((this->bStartFinished || curPos >= this->click_time) && cv_mod_strict_tracking.getBool());

        // slider tail lenience bullshit: see
        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Objects/Slider.cs#L123 being "inside the slider"
        // (for the end of the slider) is NOT checked at the exact end of the slider, but somewhere random before,
        // because fuck you
        const long offset = (long)cv_slider_end_inside_check_offset.getInt();
        const long lenienceHackEndTime =
            std::max(this->click_time + this->duration / 2, (this->click_time + this->duration) - offset);
        const bool isTrackingCorrectly =
            (this->isClickHeldSlider() || (this->bi->getModsLegacy() & LegacyFlags::Relax)) && this->bCursorInside;
        if(isTrackingCorrectly) {
            if(isTrackingStrictTrackingMod) {
                this->iStrictTrackingModLastClickHeldTime = curPos;
                if(this->iStrictTrackingModLastClickHeldTime ==
                   0)  // (prevent frame perfect inputs from not triggering the strict tracking miss because we use != 0
                       // comparison to detect if tracking correctly at least once)
                    this->iStrictTrackingModLastClickHeldTime = 1;
            }

            // only check it at the exact point in time ...
            if(curPos >= lenienceHackEndTime) {
                // ... once (like a tick)
                if(!this->bHeldTillEndForLenienceHackCheck) {
                    // player was correctly clicking/holding inside slider at lenienceHackEndTime
                    this->bHeldTillEndForLenienceHackCheck = true;
                    this->bHeldTillEndForLenienceHack = true;
                }
            }
        } else {
            // do not allow empty clicks outside of the circle radius to prevent the
            // m_bCursorInside flag from resetting
            this->bCursorLeft = true;
        }

        // can't be "inside the slider" after lenienceHackEndTime
        // (even though the slider is still going, which is madness)
        if(curPos >= lenienceHackEndTime) this->bHeldTillEndForLenienceHackCheck = true;

        // handle strict tracking mod
        if(isTrackingStrictTrackingMod) {
            const bool wasTrackingCorrectlyAtLeastOnce = (this->iStrictTrackingModLastClickHeldTime != 0);
            if(wasTrackingCorrectlyAtLeastOnce && !isTrackingCorrectly) {
                // if past lenience end time then don't trigger a strict tracking miss,
                // since the slider is then already considered fully finished gameplay wise
                if(!this->bHeldTillEndForLenienceHack) {
                    // force miss the end once, if it has not already been force missed by notelock
                    if(this->endResult == LiveScore::HIT::HIT_NULL) {
                        // force miss endcircle
                        this->onSliderBreak();

                        this->bHeldTillEnd = false;
                        this->bHeldTillEndForLenienceHack = false;
                        this->bHeldTillEndForLenienceHackCheck = true;
                        this->endResult = LiveScore::HIT::HIT_MISS;

                        // end of combo, ignore in hiterrorbar, ignore combo, subtract health
                        this->addHitResult(this->endResult, 0, this->is_end_of_combo,
                                           this->getRawPosAt(this->click_time + this->duration), -1.0f, 0.0f, true,
                                           true, false);
                    }
                }
            }
        }

        // handle repeats and ticks
        for(int i = 0; i < this->clicks.size(); i++) {
            if(!this->clicks[i].finished && curPos >= this->clicks[i].time) {
                this->clicks[i].finished = true;
                this->clicks[i].successful = (this->isClickHeldSlider() && this->bCursorInside) ||
                                             (this->bi->getModsLegacy() & LegacyFlags::Autoplay) ||
                                             ((this->bi->getModsLegacy() & LegacyFlags::Relax) && this->bCursorInside);

                if(this->clicks[i].type == 0) {
                    this->onRepeatHit(this->clicks[i].successful, this->clicks[i].sliderend);
                } else {
                    this->onTickHit(this->clicks[i].successful, this->clicks[i].tickIndex);
                }
            }
        }

        // handle auto, and the last circle
        if((this->bi->getModsLegacy() & LegacyFlags::Autoplay)) {
            if(curPos >= this->click_time + this->duration) {
                this->bHeldTillEnd = true;
                this->onHit(LiveScore::HIT::HIT_300, 0, true);
                this->bi->holding_slider = false;
            }
        } else {
            if(curPos >= this->click_time + this->duration) {
                // handle leftover startcircle
                {
                    // this may happen (if the slider time is shorter than the miss window of the startcircle)
                    if(this->startResult == LiveScore::HIT::HIT_NULL) {
                        // we still want to cause a sliderbreak in this case!
                        this->onSliderBreak();

                        // special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS
                        // health)
                        this->bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true,
                                               true,
                                               false);  // only decrease health

                        this->startResult = LiveScore::HIT::HIT_MISS;
                    }
                }

                // handle endcircle
                bool isEndResultComingFromStrictTrackingMod = false;
                if(this->endResult == LiveScore::HIT::HIT_NULL) {
                    this->bHeldTillEnd = this->bHeldTillEndForLenienceHack;
                    this->endResult = this->bHeldTillEnd ? LiveScore::HIT::HIT_300 : LiveScore::HIT::HIT_MISS;

                    // handle total slider result (currently startcircle + repeats + ticks + endcircle)
                    // clicks = (repeats + ticks)
                    const float numMaxPossibleHits = 1 + this->clicks.size() + 1;
                    float numActualHits = 0;

                    if(this->startResult != LiveScore::HIT::HIT_MISS) numActualHits++;
                    if(this->endResult != LiveScore::HIT::HIT_MISS) numActualHits++;

                    for(int i = 0; i < this->clicks.size(); i++) {
                        if(this->clicks[i].successful) numActualHits++;
                    }

                    const float percent = numActualHits / numMaxPossibleHits;

                    const bool allow300 = (this->bi->getModsLegacy() & LegacyFlags::ScoreV2)
                                              ? (this->startResult == LiveScore::HIT::HIT_300)
                                              : true;
                    const bool allow100 = (this->bi->getModsLegacy() & LegacyFlags::ScoreV2)
                                              ? (this->startResult == LiveScore::HIT::HIT_300 ||
                                                 this->startResult == LiveScore::HIT::HIT_100)
                                              : true;

                    // rewrite m_endResult as the whole slider result, then use it for the final onHit()
                    if(percent >= 0.999f && allow300)
                        this->endResult = LiveScore::HIT::HIT_300;
                    else if(percent >= 0.5f && allow100 && !this->bi->mod_ming3012 && !this->bi->mod_no100s)
                        this->endResult = LiveScore::HIT::HIT_100;
                    else if(percent > 0.0f && !this->bi->mod_no100s && !this->bi->mod_no50s)
                        this->endResult = LiveScore::HIT::HIT_50;
                    else
                        this->endResult = LiveScore::HIT::HIT_MISS;

                    // debugLog("percent = %f\n", percent);

                    if(!this->bHeldTillEnd && cv_slider_end_miss_breaks_combo.getBool()) this->onSliderBreak();
                } else
                    isEndResultComingFromStrictTrackingMod = true;

                this->onHit(this->endResult, 0, true, 0.0f, 0.0f, isEndResultComingFromStrictTrackingMod);
                this->bi->holding_slider = false;
            }
        }

        // handle sliderslide sound
        if(this->bm != NULL) {
            bool sliding = this->bStartFinished && !this->bEndFinished && this->bCursorInside && this->iDelta <= 0;
            sliding &= (this->isClickHeldSlider() || (this->bm->getModsLegacy() & LegacyFlags::Autoplay) ||
                        (this->bm->getModsLegacy() & LegacyFlags::Relax));
            sliding &= !this->bm->isPaused() && !this->bm->isWaiting() && this->bm->isPlaying();

            if(sliding) {
                const Vector2 osuCoords = this->bm->pixels2OsuCoords(this->bm->osuCoords2Pixels(this->vCurPointRaw));

                this->bm->getSkin()->playSliderSlideSound(GameRules::osuCoords2Pan(osuCoords.x));
                this->iPrevSliderSlideSoundSampleSet = this->bm->getSkin()->getSampleSet();
            } else {
                this->bm->getSkin()->stopSliderSlideSound(this->iPrevSliderSlideSoundSampleSet);
                this->iPrevSliderSlideSoundSampleSet = -1;
            }
        }
    }
}

void Slider::updateAnimations(long curPos) {
    float animation_multiplier = this->bi->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();

    float fadein_fade_time = cv_slider_followcircle_fadein_fade_time.getFloat() * animation_multiplier;
    float fadeout_fade_time = cv_slider_followcircle_fadeout_fade_time.getFloat() * animation_multiplier;
    float fadein_scale_time = cv_slider_followcircle_fadein_scale_time.getFloat() * animation_multiplier;
    float fadeout_scale_time = cv_slider_followcircle_fadeout_scale_time.getFloat() * animation_multiplier;

    // handle followcircle animations
    this->fFollowCircleAnimationAlpha =
        std::clamp<float>((float)((curPos - this->click_time)) / 1000.0f /
                              std::clamp<float>(fadein_fade_time, 0.0f, this->duration / 1000.0f),
                          0.0f, 1.0f);
    if(this->bFinished) {
        this->fFollowCircleAnimationAlpha =
            1.0f -
            std::clamp<float>((float)((curPos - (this->click_time + this->duration))) / 1000.0f / fadeout_fade_time,
                              0.0f, 1.0f);
        this->fFollowCircleAnimationAlpha *= this->fFollowCircleAnimationAlpha;  // quad in
    }

    this->fFollowCircleAnimationScale =
        std::clamp<float>((float)((curPos - this->click_time)) / 1000.0f /
                              std::clamp<float>(fadein_scale_time, 0.0f, this->duration / 1000.0f),
                          0.0f, 1.0f);
    if(this->bFinished) {
        this->fFollowCircleAnimationScale = std::clamp<float>(
            (float)((curPos - (this->click_time + this->duration))) / 1000.0f / fadeout_scale_time, 0.0f, 1.0f);
    }
    this->fFollowCircleAnimationScale =
        -this->fFollowCircleAnimationScale * (this->fFollowCircleAnimationScale - 2.0f);  // quad out

    if(!this->bFinished)
        this->fFollowCircleAnimationScale =
            cv_slider_followcircle_fadein_scale.getFloat() +
            (1.0f - cv_slider_followcircle_fadein_scale.getFloat()) * this->fFollowCircleAnimationScale;
    else
        this->fFollowCircleAnimationScale =
            1.0f - (1.0f - cv_slider_followcircle_fadeout_scale.getFloat()) * this->fFollowCircleAnimationScale;
}

void Slider::updateStackPosition(float stackOffset) {
    if(this->curve != NULL)
        this->curve->updateStackPosition(this->iStack * stackOffset,
                                         (this->bi->getModsLegacy() & LegacyFlags::HardRock));
}

void Slider::miss(long curPos) {
    if(this->bFinished) return;

    const long delta = curPos - this->click_time;

    // startcircle
    if(!this->bStartFinished) {
        this->startResult = LiveScore::HIT::HIT_MISS;
        this->onHit(this->startResult, delta, false);
        this->bi->holding_slider = false;
    }

    // endcircle, repeats, ticks
    if(!this->bEndFinished) {
        // repeats, ticks
        {
            for(int i = 0; i < this->clicks.size(); i++) {
                if(!this->clicks[i].finished) {
                    this->clicks[i].finished = true;
                    this->clicks[i].successful = false;

                    if(this->clicks[i].type == 0)
                        this->onRepeatHit(this->clicks[i].successful, this->clicks[i].sliderend);
                    else
                        this->onTickHit(this->clicks[i].successful, this->clicks[i].tickIndex);
                }
            }
        }

        // endcircle
        {
            this->bHeldTillEnd = this->bHeldTillEndForLenienceHack;

            if(!this->bHeldTillEnd && cv_slider_end_miss_breaks_combo.getBool()) this->onSliderBreak();

            this->endResult = LiveScore::HIT::HIT_MISS;
            this->onHit(this->endResult, 0, true);
            this->bi->holding_slider = false;
        }
    }
}

Vector2 Slider::getRawPosAt(long pos) {
    if(this->curve == NULL) return Vector2(0, 0);

    if(pos <= this->click_time)
        return this->curve->pointAt(0.0f);
    else if(pos >= this->click_time + this->fSliderTime) {
        if(this->iRepeat % 2 == 0)
            return this->curve->pointAt(0.0f);
        else
            return this->curve->pointAt(1.0f);
    } else
        return this->curve->pointAt(this->getT(pos, false));
}

Vector2 Slider::getOriginalRawPosAt(long pos) {
    if(this->curve == NULL) return Vector2(0, 0);

    if(pos <= this->click_time)
        return this->curve->originalPointAt(0.0f);
    else if(pos >= this->click_time + this->fSliderTime) {
        if(this->iRepeat % 2 == 0)
            return this->curve->originalPointAt(0.0f);
        else
            return this->curve->originalPointAt(1.0f);
    } else
        return this->curve->originalPointAt(this->getT(pos, false));
}

float Slider::getT(long pos, bool raw) {
    float t = (float)((long)pos - (long)this->click_time) / this->fSliderTimeWithoutRepeats;
    if(raw)
        return t;
    else {
        float floorVal = (float)std::floor(t);
        return ((int)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
    }
}

void Slider::onClickEvent(std::vector<Click> &clicks) {
    if(this->points.size() == 0 || this->bBlocked)
        return;  // also handle note blocking here (doesn't need fancy shake logic, since sliders don't shake in
                 // osu!stable)

    if(!this->bStartFinished) {
        const Vector2 cursorPos = clicks[0].pos;
        const Vector2 pos = this->bi->osuCoords2Pixels(this->curve->pointAt(0.0f));
        const float cursorDelta = (cursorPos - pos).length();

        if(cursorDelta < this->bi->fHitcircleDiameter / 2.0f) {
            const long delta = clicks[0].click_time - (long)this->click_time;

            LiveScore::HIT result = this->bi->getHitResult(delta);
            if(result != LiveScore::HIT::HIT_NULL) {
                const float targetDelta = cursorDelta / (this->bi->fHitcircleDiameter / 2.0f);
                const float targetAngle = glm::degrees(std::atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

                clicks.erase(clicks.begin());
                this->startResult = result;
                this->onHit(this->startResult, delta, false, targetDelta, targetAngle);
                this->bi->holding_slider = true;
            }
        }
    }
}

void Slider::onHit(LiveScore::HIT result, long delta, bool startOrEnd, float targetDelta, float targetAngle,
                   bool isEndResultFromStrictTrackingMod) {
    if(this->points.size() == 0) return;

    // start + end of a slider add +30 points, if successful

    // debugLog("startOrEnd = %i,    m_iCurRepeat = %i\n", (int)startOrEnd, this->iCurRepeat);

    // sound and hit animation and also sliderbreak combo drop
    {
        if(result == LiveScore::HIT::HIT_MISS) {
            if(!isEndResultFromStrictTrackingMod) this->onSliderBreak();
        } else if(this->bm != NULL) {
            if(cv_timingpoints_force.getBool())
                this->bm->updateTimingPoints(this->click_time + (!startOrEnd ? 0 : this->duration));

            const Vector2 osuCoords = this->bm->pixels2OsuCoords(this->bm->osuCoords2Pixels(this->vCurPointRaw));

            this->bm->getSkin()->playHitCircleSound(this->iCurRepeatCounterForHitSounds < this->hitSounds.size()
                                                        ? this->hitSounds[this->iCurRepeatCounterForHitSounds]
                                                        : this->iSampleType,
                                                    GameRules::osuCoords2Pan(osuCoords.x), delta);

            if(!startOrEnd) {
                this->fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                anim->moveQuadOut(&this->fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(this->bm), true);
            } else {
                if(this->iRepeat % 2 != 0) {
                    this->fEndHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                    anim->moveQuadOut(&this->fEndHitAnimation, 1.0f, GameRules::getFadeOutTime(this->bm), true);
                } else {
                    this->fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                    anim->moveQuadOut(&this->fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(this->bm), true);
                }
            }
        }

        // end body fadeout
        if(this->bm != NULL && startOrEnd) {
            this->fEndSliderBodyFadeAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&this->fEndSliderBodyFadeAnimation, 1.0f,
                              GameRules::getFadeOutTime(this->bm) * cv_slider_body_fade_out_time_multiplier.getFloat(),
                              true);

            this->bm->getSkin()->stopSliderSlideSound();
        }
    }

    // add score, and we are finished
    if(!startOrEnd) {
        // startcircle

        this->bStartFinished = true;

        // The player entered the slider by fat fingering, so...
        if(this->bi->isKey1Down() && this->bi->isKey2Down()) {
            if(this->bi->bPrevKeyWasKey1) {
                // Player pressed K1 last: "fat finger" key is K2
                this->iFatFingerKey = 2;
            } else {
                // Player pressed K2 last: "fat finger" key is K1
                this->iFatFingerKey = 1;
            }
        }

        if(this->bi->getModsLegacy() & LegacyFlags::Target) {
            // not end of combo, show in hiterrorbar, use for accuracy, increase combo, increase
            // score, ignore for health, don't add object duration to result anim
            this->addHitResult(result, delta, false, this->curve->pointAt(0.0f), targetDelta, targetAngle, false, false,
                               true, false);
        } else {
            // not end of combo, show in hiterrorbar, ignore for accuracy, increase combo,
            // don't count towards score, depending on scorev2 ignore for health or not
            this->bi->addHitResult(this, result, delta, false, false, true, false, true, true);
        }

        // add bonus score + health manually
        if(result != LiveScore::HIT::HIT_MISS) {
            LiveScore::HIT resultForHealth = LiveScore::HIT::HIT_SLIDER30;

            this->bi->addHitResult(this, resultForHealth, 0, false, true, true, true, true,
                                   false);  // only increase health
            this->bi->addScorePoints(30);
        } else {
            // special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
            this->bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                                   false);  // only decrease health
        }
    } else {
        // endcircle

        this->bStartFinished = true;
        this->bEndFinished = true;
        this->bFinished = true;

        if(!isEndResultFromStrictTrackingMod) {
            // special case: osu!lazer 2020 only returns 1 judgement for the whole slider, but via the startcircle. i.e.
            // we are not allowed to drain again here in neosu logic (because startcircle judgement is handled at the
            // end here)
            // XXX: remove this
            const bool isLazer2020Drain = false;

            this->addHitResult(
                result, delta, this->is_end_of_combo, this->getRawPosAt(this->click_time + this->duration), -1.0f, 0.0f,
                true, !this->bHeldTillEnd,
                isLazer2020Drain);  // end of combo, ignore in hiterrorbar, depending on heldTillEnd increase
                                    // combo or not, increase score, increase health depending on drain type

            // add bonus score + extra health manually
            if(this->bHeldTillEnd) {
                this->bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER30, 0, false, true, true, true, true,
                                       false);  // only increase health
                this->bi->addScorePoints(30);
            } else {
                // special case: missing the endcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
                // NOTE: yes, this will drain twice for the end of a slider (once for the judgement of the whole slider
                // above, and once for the endcircle here)
                this->bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                                       false);  // only decrease health
            }
        }
    }

    this->iCurRepeatCounterForHitSounds++;
}

void Slider::onRepeatHit(bool successful, bool sliderend) {
    if(this->points.size() == 0) return;

    // repeat hit of a slider adds +30 points, if successful

    // sound and hit animation
    if(!successful) {
        this->onSliderBreak();
    } else if(this->bm != NULL) {
        if(cv_timingpoints_force.getBool())
            this->bm->updateTimingPoints(this->click_time + (long)((float)this->duration * this->fActualSlidePercent));

        const Vector2 osuCoords = this->bm->pixels2OsuCoords(this->bm->osuCoords2Pixels(this->vCurPointRaw));

        this->bm->getSkin()->playHitCircleSound(this->iCurRepeatCounterForHitSounds < this->hitSounds.size()
                                                    ? this->hitSounds[this->iCurRepeatCounterForHitSounds]
                                                    : this->iSampleType,
                                                GameRules::osuCoords2Pan(osuCoords.x), 0);

        float animation_multiplier = this->bm->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
        float tick_pulse_time = cv_slider_followcircle_tick_pulse_time.getFloat() * animation_multiplier;

        this->fFollowCircleTickAnimationScale = 0.0f;
        anim->moveLinear(&this->fFollowCircleTickAnimationScale, 1.0f, tick_pulse_time, true);

        if(sliderend) {
            this->fEndHitAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&this->fEndHitAnimation, 1.0f, GameRules::getFadeOutTime(this->bm), true);
        } else {
            this->fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&this->fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(this->bm), true);
        }
    }

    // add score
    if(!successful) {
        // add health manually
        // special case: missing a repeat drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
        this->bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                               false);  // only decrease health
    } else {
        this->bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER30, 0, false, true, true, false, true,
                               false);  // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase
                                        // combo, don't count towards score, increase health

        // add bonus score manually
        this->bi->addScorePoints(30);
    }

    this->iCurRepeatCounterForHitSounds++;
}

void Slider::onTickHit(bool successful, int tickIndex) {
    if(this->points.size() == 0) return;

    // tick hit of a slider adds +10 points, if successful

    // tick drawing visibility
    int numMissingTickClicks = 0;
    for(int i = 0; i < this->clicks.size(); i++) {
        if(this->clicks[i].type == 1 && this->clicks[i].tickIndex == tickIndex && !this->clicks[i].finished)
            numMissingTickClicks++;
    }
    if(numMissingTickClicks == 0) this->ticks[tickIndex].finished = true;

    // sound and hit animation
    if(!successful) {
        this->onSliderBreak();
    } else if(this->bm != NULL) {
        if(cv_timingpoints_force.getBool())
            this->bm->updateTimingPoints(this->click_time + (long)((float)this->duration * this->fActualSlidePercent));

        const Vector2 osuCoords = this->bm->pixels2OsuCoords(this->bm->osuCoords2Pixels(this->vCurPointRaw));

        this->bm->getSkin()->playSliderTickSound(GameRules::osuCoords2Pan(osuCoords.x));

        float animation_multiplier = this->bm->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
        float tick_pulse_time = cv_slider_followcircle_tick_pulse_time.getFloat() * animation_multiplier;

        this->fFollowCircleTickAnimationScale = 0.0f;
        anim->moveLinear(&this->fFollowCircleTickAnimationScale, 1.0f, tick_pulse_time, true);
    }

    // add score
    if(!successful) {
        // add health manually
        // special case: missing a tick drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
        this->bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                               false);  // only decrease health
    } else {
        this->bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER10, 0, false, true, true, false, true,
                               false);  // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase
                                        // combo, don't count towards score, increase health

        // add bonus score manually
        this->bi->addScorePoints(10);
    }
}

void Slider::onSliderBreak() { this->bi->addSliderBreak(); }

void Slider::onReset(long curPos) {
    HitObject::onReset(curPos);

    if(this->bm != NULL) {
        this->bm->getSkin()->stopSliderSlideSound();

        anim->deleteExistingAnimation(&this->fFollowCircleTickAnimationScale);
        anim->deleteExistingAnimation(&this->fStartHitAnimation);
        anim->deleteExistingAnimation(&this->fEndHitAnimation);
        anim->deleteExistingAnimation(&this->fEndSliderBodyFadeAnimation);
    }

    this->iStrictTrackingModLastClickHeldTime = 0;
    this->iFatFingerKey = 0;
    this->iPrevSliderSlideSoundSampleSet = -1;
    this->bCursorLeft = true;
    this->bHeldTillEnd = false;
    this->bHeldTillEndForLenienceHack = false;
    this->bHeldTillEndForLenienceHackCheck = false;
    this->startResult = LiveScore::HIT::HIT_NULL;
    this->endResult = LiveScore::HIT::HIT_NULL;

    this->iCurRepeatCounterForHitSounds = 0;

    if(this->click_time > curPos) {
        this->bStartFinished = false;
        this->fStartHitAnimation = 0.0f;
        this->bEndFinished = false;
        this->bFinished = false;
        this->fEndHitAnimation = 0.0f;
        this->fEndSliderBodyFadeAnimation = 0.0f;
    } else if(curPos < this->click_time + this->duration) {
        this->bStartFinished = true;
        this->fStartHitAnimation = 1.0f;

        this->bEndFinished = false;
        this->bFinished = false;
        this->fEndHitAnimation = 0.0f;
        this->fEndSliderBodyFadeAnimation = 0.0f;
    } else {
        this->bStartFinished = true;
        this->fStartHitAnimation = 1.0f;
        this->bEndFinished = true;
        this->bFinished = true;
        this->fEndHitAnimation = 1.0f;
        this->fEndSliderBodyFadeAnimation = 1.0f;
    }

    for(int i = 0; i < this->clicks.size(); i++) {
        if(curPos > this->clicks[i].time) {
            this->clicks[i].finished = true;
            this->clicks[i].successful = true;
        } else {
            this->clicks[i].finished = false;
            this->clicks[i].successful = false;
        }
    }

    for(int i = 0; i < this->ticks.size(); i++) {
        int numMissingTickClicks = 0;
        for(int c = 0; c < this->clicks.size(); c++) {
            if(this->clicks[c].type == 1 && this->clicks[c].tickIndex == i && !this->clicks[c].finished)
                numMissingTickClicks++;
        }
        this->ticks[i].finished = numMissingTickClicks == 0;
    }
}

void Slider::rebuildVertexBuffer(bool useRawCoords) {
    // base mesh (background) (raw unscaled, size in raw osu coordinates centered at (0, 0, 0))
    // this mesh needs to be scaled and translated appropriately since we are not 1:1 with the playfield
    std::vector<Vector2> osuCoordPoints = this->curve->getPoints();
    if(!useRawCoords) {
        for(int p = 0; p < osuCoordPoints.size(); p++) {
            osuCoordPoints[p] = this->bi->osuCoords2LegacyPixels(osuCoordPoints[p]);
        }
    }
    SAFE_DELETE(this->vao);
    this->vao = SliderRenderer::generateVAO(osuCoordPoints, this->bi->fRawHitcircleDiameter);
}

bool Slider::isClickHeldSlider() {
    // osu! has a weird slider quirk, that I'll explain in detail here.
    // When holding K1 before the slider, tapping K2 on slider head, and releasing K2 later,
    // the slider is no longer considered being "held" until K2 is pressed again, or K1 is released and pressed again.

    // The reason this exists is to prevent people from holding K1 the whole map and tapping with K2.
    // Holding is part of the rhythm flow, and this is a rhythm game right?

    if(this->iFatFingerKey == 0) return this->bi->isClickHeld();
    if(this->iFatFingerKey == 1) return this->bi->isKey2Down();
    if(this->iFatFingerKey == 2) return this->bi->isKey1Down();

    return false;  // unreachable
}
