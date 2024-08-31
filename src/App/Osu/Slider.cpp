#include "Slider.h"

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

using namespace std;

Slider::Slider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds,
               std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType,
               int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, BeatmapInterface *beatmap)
    : HitObject(time, sampleType, comboNumber, isEndOfCombo, colorCounter, colorOffset, beatmap) {
    m_cType = type;
    m_iRepeat = repeat;
    m_fPixelLength = pixelLength;
    m_points = points;
    m_hitSounds = hitSounds;
    m_fSliderTime = sliderTime;
    m_fSliderTimeWithoutRepeats = sliderTimeWithoutRepeats;

    // build raw ticks
    for(int i = 0; i < ticks.size(); i++) {
        SLIDERTICK st;
        st.finished = false;
        st.percent = ticks[i];
        m_ticks.push_back(st);
    }

    // build curve
    m_curve = SliderCurve::createCurve(m_cType, m_points, m_fPixelLength);

    // build repeats
    for(int i = 0; i < (m_iRepeat - 1); i++) {
        SLIDERCLICK sc;

        sc.finished = false;
        sc.successful = false;
        sc.type = 0;
        sc.sliderend = ((i % 2) == 0);  // for hit animation on repeat hit
        sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * (i + 1));

        m_clicks.push_back(sc);
    }

    // build ticks
    for(int i = 0; i < m_iRepeat; i++) {
        for(int t = 0; t < m_ticks.size(); t++) {
            // NOTE: repeat ticks are not necessarily symmetric.
            //
            // e.g. this slider: [1]=======*==[2]
            //
            // the '*' is where the tick is, let's say percent = 0.75
            // on repeat 0, the tick is at: m_iTime + 0.75*m_fSliderTimeWithoutRepeats
            // but on repeat 1, the tick is at: m_iTime + 1*m_fSliderTimeWithoutRepeats + (1.0 -
            // 0.75)*m_fSliderTimeWithoutRepeats this gives better readability at the cost of invalid rhythms: ticks are
            // guaranteed to always be at the same position, even in repeats so, depending on which repeat we are in
            // (even or odd), we either do (percent) or (1.0 - percent)

            const float tickPercentRelativeToRepeatFromStartAbs =
                (((i + 1) % 2) != 0 ? m_ticks[t].percent : 1.0f - m_ticks[t].percent);

            SLIDERCLICK sc;

            sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * i) +
                      (long)(tickPercentRelativeToRepeatFromStartAbs * m_fSliderTimeWithoutRepeats);
            sc.finished = false;
            sc.successful = false;
            sc.type = 1;
            sc.tickIndex = t;

            m_clicks.push_back(sc);
        }
    }

    m_iObjectDuration = (long)m_fSliderTime;
    m_iObjectDuration = m_iObjectDuration >= 0 ? m_iObjectDuration : 1;  // force clamp to positive range

    m_fSlidePercent = 0.0f;
    m_fActualSlidePercent = 0.0f;
    m_fSliderSnakePercent = 0.0f;
    m_fReverseArrowAlpha = 0.0f;
    m_fBodyAlpha = 0.0f;

    m_startResult = LiveScore::HIT::HIT_NULL;
    m_endResult = LiveScore::HIT::HIT_NULL;
    m_bStartFinished = false;
    m_fStartHitAnimation = 0.0f;
    m_bEndFinished = false;
    m_fEndHitAnimation = 0.0f;
    m_fEndSliderBodyFadeAnimation = 0.0f;
    m_iStrictTrackingModLastClickHeldTime = 0;
    m_iFatFingerKey = 0;
    m_iPrevSliderSlideSoundSampleSet = -1;
    m_bCursorLeft = true;
    m_bCursorInside = false;
    m_bHeldTillEnd = false;
    m_bHeldTillEndForLenienceHack = false;
    m_bHeldTillEndForLenienceHackCheck = false;
    m_fFollowCircleTickAnimationScale = 0.0f;
    m_fFollowCircleAnimationScale = 0.0f;
    m_fFollowCircleAnimationAlpha = 0.0f;

    m_iReverseArrowPos = 0;
    m_iCurRepeat = 0;
    m_iCurRepeatCounterForHitSounds = 0;
    m_bInReverse = false;
    m_bHideNumberAfterFirstRepeatHit = false;

    m_fSliderBreakRapeTime = 0.0f;

    m_vao = NULL;
}

Slider::~Slider() {
    onReset(0);

    SAFE_DELETE(m_curve);
    SAFE_DELETE(m_vao);
}

void Slider::draw(Graphics *g) {
    if(m_points.size() <= 0) return;

    const float foscale = cv_circle_fade_out_scale.getFloat();
    Skin *skin = bm->getSkin();

    const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

    if((m_bVisible || (m_bStartFinished && !m_bFinished)) &&
       !isCompletelyFinished)  // extra possibility to avoid flicker between HitObject::m_bVisible delay and the
                               // fadeout animation below this if block
    {
        float alpha = (cv_mod_hd_slider_fast_fade.getBool() ? m_fAlpha : m_fBodyAlpha);
        float sliderSnake = cv_snaking_sliders.getBool() ? m_fSliderSnakePercent : 1.0f;

        // shrinking sliders
        float sliderSnakeStart = 0.0f;
        if(cv_slider_shrink.getBool() && m_iReverseArrowPos == 0) {
            sliderSnakeStart = (m_bInReverse ? 0.0f : m_fSlidePercent);
            if(m_bInReverse) sliderSnake = m_fSlidePercent;
        }

        // draw slider body
        if(alpha > 0.0f && cv_slider_draw_body.getBool()) drawBody(g, alpha, sliderSnakeStart, sliderSnake);

        // draw slider ticks
        Color tickColor = 0xffffffff;
        tickColor = COLOR(255, (int)(COLOR_GET_Ri(tickColor) * m_fHittableDimRGBColorMultiplierPercent),
                          (int)(COLOR_GET_Gi(tickColor) * m_fHittableDimRGBColorMultiplierPercent),
                          (int)(COLOR_GET_Bi(tickColor) * m_fHittableDimRGBColorMultiplierPercent));
        const float tickImageScale =
            (bm->m_fHitcircleDiameter / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f))) * 0.125f;
        for(int t = 0; t < m_ticks.size(); t++) {
            if(m_ticks[t].finished || m_ticks[t].percent > sliderSnake) continue;

            Vector2 pos = bm->osuCoords2Pixels(m_curve->pointAt(m_ticks[t].percent));

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
        if(m_points.size() > 1) {
            // HACKHACK: very dirty code
            bool sliderRepeatStartCircleFinished = (m_iRepeat < 2);
            bool sliderRepeatEndCircleFinished = false;
            bool endCircleIsAtActualSliderEnd = true;
            for(int i = 0; i < m_clicks.size(); i++) {
                // repeats
                if(m_clicks[i].type == 0) {
                    endCircleIsAtActualSliderEnd = m_clicks[i].sliderend;

                    if(endCircleIsAtActualSliderEnd)
                        sliderRepeatEndCircleFinished = m_clicks[i].finished;
                    else
                        sliderRepeatStartCircleFinished = m_clicks[i].finished;
                }
            }

            const bool ifStrictTrackingModShouldDrawEndCircle =
                (!cv_mod_strict_tracking.getBool() || m_endResult != LiveScore::HIT::HIT_MISS);

            // end circle
            if((!m_bEndFinished && m_iRepeat % 2 != 0 && ifStrictTrackingModShouldDrawEndCircle) ||
               (!sliderRepeatEndCircleFinished &&
                (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && endCircleIsAtActualSliderEnd) ||
                 (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))))
                drawEndCircle(g, alpha, sliderSnake);

            // start circle
            if(!m_bStartFinished ||
               (!sliderRepeatStartCircleFinished &&
                (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && !endCircleIsAtActualSliderEnd) ||
                 (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))) ||
               (!m_bEndFinished && m_iRepeat % 2 == 0 && ifStrictTrackingModShouldDrawEndCircle))
                drawStartCircle(g, alpha);

            // reverse arrows
            if(m_fReverseArrowAlpha > 0.0f) {
                // if the combo color is nearly white, blacken the reverse arrow
                Color comboColor = skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset);
                Color reverseArrowColor = 0xffffffff;
                if((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor)) / 3.0f >
                   cv_slider_reverse_arrow_black_threshold.getFloat())
                    reverseArrowColor = 0xff000000;

                reverseArrowColor =
                    COLOR(255, (int)(COLOR_GET_Ri(reverseArrowColor) * m_fHittableDimRGBColorMultiplierPercent),
                          (int)(COLOR_GET_Gi(reverseArrowColor) * m_fHittableDimRGBColorMultiplierPercent),
                          (int)(COLOR_GET_Bi(reverseArrowColor) * m_fHittableDimRGBColorMultiplierPercent));

                float div = 0.30f;
                float pulse = (div - fmod(std::abs(bm->getCurMusicPos()) / 1000.0f, div)) / div;
                pulse *= pulse;  // quad in

                if(!cv_slider_reverse_arrow_animated.getBool() || bm->isInMafhamRenderChunk()) pulse = 0.0f;

                // end
                if(m_iReverseArrowPos == 2 || m_iReverseArrowPos == 3) {
                    Vector2 pos = bm->osuCoords2Pixels(m_curve->pointAt(1.0f));
                    float rotation =
                        m_curve->getEndAngle() - cv_playfield_rotation.getFloat() - bm->getPlayfieldRotation();
                    if((bm->getModsLegacy() & LegacyFlags::HardRock)) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_horizontal.getBool()) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_vertical.getBool()) rotation = 180.0f - rotation;

                    const float osuCoordScaleMultiplier = bm->m_fHitcircleDiameter / bm->m_fRawHitcircleDiameter;
                    float reverseArrowImageScale =
                        (bm->m_fRawHitcircleDiameter / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) *
                        osuCoordScaleMultiplier;

                    reverseArrowImageScale *= 1.0f + pulse * 0.30f;

                    g->setColor(reverseArrowColor);
                    g->setAlpha(m_fReverseArrowAlpha);
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
                if(m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3) {
                    Vector2 pos = bm->osuCoords2Pixels(m_curve->pointAt(0.0f));
                    float rotation =
                        m_curve->getStartAngle() - cv_playfield_rotation.getFloat() - bm->getPlayfieldRotation();
                    if((bm->getModsLegacy() & LegacyFlags::HardRock)) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_horizontal.getBool()) rotation = 360.0f - rotation;
                    if(cv_playfield_mirror_vertical.getBool()) rotation = 180.0f - rotation;

                    const float osuCoordScaleMultiplier = bm->m_fHitcircleDiameter / bm->m_fRawHitcircleDiameter;
                    float reverseArrowImageScale =
                        (bm->m_fRawHitcircleDiameter / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) *
                        osuCoordScaleMultiplier;

                    reverseArrowImageScale *= 1.0f + pulse * 0.30f;

                    g->setColor(reverseArrowColor);
                    g->setAlpha(m_fReverseArrowAlpha);
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
    if(!instafade_slider_body && m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f &&
       !(bm->getModsLegacy() & LegacyFlags::Hidden)) {
        std::vector<Vector2> emptyVector;
        std::vector<Vector2> alwaysPoints;
        alwaysPoints.push_back(bm->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent)));
        if(!cv_slider_shrink.getBool())
            drawBody(g, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);
        else if(cv_slider_body_lazer_fadeout_style.getBool())
            SliderRenderer::draw(g, emptyVector, alwaysPoints, bm->m_fHitcircleDiameter, 0.0f, 0.0f,
                                 bm->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), 1.0f,
                                 1.0f - m_fEndSliderBodyFadeAnimation, getTime());
    }

    if(!instafade_slider_head && cv_slider_sliderhead_fadeout.getBool() && m_fStartHitAnimation > 0.0f &&
       m_fStartHitAnimation != 1.0f && !(bm->getModsLegacy() & LegacyFlags::Hidden)) {
        float alpha = 1.0f - m_fStartHitAnimation;

        float scale = m_fStartHitAnimation;
        scale = -scale * (scale - 2.0f);  // quad out scale

        bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

        g->pushTransform();
        {
            g->scale((1.0f + scale * foscale), (1.0f + scale * foscale));
            if(m_iCurRepeat < 1) {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());
                skin->getSliderStartCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());

                Circle::drawSliderStartCircle(g, bm, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter,
                                              m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
            } else {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime : bm->getCurMusicPosWithOffsets());
                skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime : bm->getCurMusicPosWithOffsets());

                Circle::drawSliderEndCircle(g, bm, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter,
                                            m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
            }
        }
        g->popTransform();
    }

    if(!instafade_slider_head && m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f &&
       !(bm->getModsLegacy() & LegacyFlags::Hidden)) {
        float alpha = 1.0f - m_fEndHitAnimation;

        float scale = m_fEndHitAnimation;
        scale = -scale * (scale - 2.0f);  // quad out scale

        g->pushTransform();
        {
            g->scale((1.0f + scale * foscale), (1.0f + scale * foscale));
            {
                skin->getHitCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : bm->getCurMusicPosWithOffsets());
                skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
                    skin->getAnimationSpeed(),
                    !bm->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : bm->getCurMusicPosWithOffsets());

                Circle::drawSliderEndCircle(g, bm, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter,
                                            m_iColorOffset, 1.0f, 1.0f, alpha, 0.0f, false);
            }
        }
        g->popTransform();
    }

    HitObject::draw(g);
}

void Slider::draw2(Graphics *g) { draw2(g, true, false); }

void Slider::draw2(Graphics *g, bool drawApproachCircle, bool drawOnlyApproachCircle) {
    HitObject::draw2(g);

    Skin *skin = bm->getSkin();

    // HACKHACK: so much code duplication aaaaaaah
    if((m_bVisible || (m_bStartFinished && !m_bFinished)) &&
       drawApproachCircle)  // extra possibility to avoid flicker between HitObject::m_bVisible delay and the fadeout
                            // animation below this if block
    {
        if(m_points.size() > 1) {
            // HACKHACK: very dirty code
            bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
            for(int i = 0; i < m_clicks.size(); i++) {
                if(m_clicks[i].type == 0) {
                    if(!m_clicks[i].sliderend) sliderRepeatStartCircleFinished = m_clicks[i].finished;
                }
            }

            // start circle
            if(!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0)) {
                Circle::drawApproachCircle(g, bm, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter,
                                           m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale,
                                           m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
            }
        }
    }

    if(drawApproachCircle && drawOnlyApproachCircle) return;

    // draw followcircle
    // HACKHACK: this is not entirely correct (due to m_bHeldTillEnd, if held within 300 range but then released, will
    // flash followcircle at the end)
    if((m_bVisible && m_bCursorInside &&
        (isClickHeldSlider() || (bm->getModsLegacy() & LegacyFlags::Autoplay) ||
         (bm->getModsLegacy() & LegacyFlags::Relax))) ||
       (m_bFinished && m_fFollowCircleAnimationAlpha > 0.0f && m_bHeldTillEnd)) {
        Vector2 point = bm->osuCoords2Pixels(m_vCurPointRaw);

        // HACKHACK: this is shit
        float tickAnimation =
            (m_fFollowCircleTickAnimationScale < 0.1f ? m_fFollowCircleTickAnimationScale / 0.1f
                                                      : (1.0f - m_fFollowCircleTickAnimationScale) / 0.9f);
        if(m_fFollowCircleTickAnimationScale < 0.1f) {
            tickAnimation = -tickAnimation * (tickAnimation - 2.0f);
            tickAnimation = clamp<float>(tickAnimation / 0.02f, 0.0f, 1.0f);
        }
        float tickAnimationScale = 1.0f + tickAnimation * cv_slider_followcircle_tick_pulse_scale.getFloat();

        g->setColor(0xffffffff);
        g->setAlpha(m_fFollowCircleAnimationAlpha);
        skin->getSliderFollowCircle2()->setAnimationTimeOffset(skin->getAnimationSpeed(), m_iTime);
        skin->getSliderFollowCircle2()->drawRaw(
            g, point,
            (bm->m_fSliderFollowCircleDiameter / skin->getSliderFollowCircle2()->getSizeBaseRaw().x) *
                tickAnimationScale * m_fFollowCircleAnimationScale *
                0.85f);  // this is a bit strange, but seems to work perfectly with 0.85
    }

    const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

    // draw sliderb on top of everything
    if((m_bVisible || (m_bStartFinished && !m_bFinished)) &&
       !isCompletelyFinished)  // extra possibility in the if-block to avoid flicker between HitObject::m_bVisible
                               // delay and the fadeout animation below this if-block
    {
        if(m_fSlidePercent > 0.0f) {
            // draw sliderb
            Vector2 point = bm->osuCoords2Pixels(m_vCurPointRaw);
            Vector2 c1 = bm->osuCoords2Pixels(
                m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent : m_fSlidePercent - 0.01f));
            Vector2 c2 = bm->osuCoords2Pixels(
                m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent + 0.01f : m_fSlidePercent));
            float ballAngle = rad2deg(atan2(c2.y - c1.y, c2.x - c1.x));
            if(skin->getSliderBallFlip()) ballAngle += (m_iCurRepeat % 2 == 0) ? 0 : 180;

            g->setColor(skin->getAllowSliderBallTint()
                            ? (cv_slider_ball_tint_combo_color.getBool()
                                   ? skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset)
                                   : skin->getSliderBallColor())
                            : 0xffffffff);
            g->pushTransform();
            {
                g->rotate(ballAngle);
                skin->getSliderb()->setAnimationTimeOffset(skin->getAnimationSpeed(), m_iTime);
                skin->getSliderb()->drawRaw(g, point,
                                            bm->m_fHitcircleDiameter / skin->getSliderb()->getSizeBaseRaw().x);
            }
            g->popTransform();
        }
    }
}

void Slider::drawStartCircle(Graphics *g, float alpha) {
    Skin *skin = bm->getSkin();

    if(m_bStartFinished) {
        skin->getHitCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(), !bm->isInMafhamRenderChunk() ? m_iTime : bm->getCurMusicPosWithOffsets());
        skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(), !bm->isInMafhamRenderChunk() ? m_iTime : bm->getCurMusicPosWithOffsets());

        Circle::drawSliderEndCircle(g, bm, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset,
                                    m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
    } else {
        skin->getHitCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(),
            !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());
        skin->getSliderStartCircleOverlay2()->setAnimationTimeOffset(
            skin->getAnimationSpeed(),
            !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());

        Circle::drawSliderStartCircle(g, bm, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset,
                                      m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale, m_fAlpha, m_fAlpha,
                                      !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
    }
}

void Slider::drawEndCircle(Graphics *g, float alpha, float sliderSnake) {
    Skin *skin = bm->getSkin();

    skin->getHitCircleOverlay2()->setAnimationTimeOffset(
        skin->getAnimationSpeed(),
        !bm->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : bm->getCurMusicPosWithOffsets());
    skin->getSliderEndCircleOverlay2()->setAnimationTimeOffset(
        skin->getAnimationSpeed(),
        !bm->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : bm->getCurMusicPosWithOffsets());

    Circle::drawSliderEndCircle(g, bm, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, m_iColorOffset,
                                m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
}

void Slider::drawBody(Graphics *g, float alpha, float from, float to) {
    // smooth begin/end while snaking/shrinking
    std::vector<Vector2> alwaysPoints;
    if(cv_slider_body_smoothsnake.getBool()) {
        if(cv_slider_shrink.getBool() && m_fSliderSnakePercent > 0.999f) {
            alwaysPoints.push_back(bm->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent)));  // curpoint
            alwaysPoints.push_back(bm->osuCoords2Pixels(
                getRawPosAt(m_iTime + m_iObjectDuration + 1)));  // endpoint (because setDrawPercent() causes the last
                                                                 // circle mesh to become invisible too quickly)
        }
        if(cv_snaking_sliders.getBool() && m_fSliderSnakePercent < 1.0f)
            alwaysPoints.push_back(bm->osuCoords2Pixels(
                m_curve->pointAt(m_fSliderSnakePercent)));  // snakeoutpoint (only while snaking out)
    }

    const Color undimmedComboColor = bm->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset);

    if(osu->shouldFallBackToLegacySliderRenderer()) {
        std::vector<Vector2> screenPoints = m_curve->getPoints();
        for(int p = 0; p < screenPoints.size(); p++) {
            screenPoints[p] = bm->osuCoords2Pixels(screenPoints[p]);
        }

        // peppy sliders
        SliderRenderer::draw(g, screenPoints, alwaysPoints, bm->m_fHitcircleDiameter, from, to, undimmedComboColor,
                             m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
    } else {
        // vertex buffered sliders
        // as the base mesh is centered at (0, 0, 0) and in raw osu coordinates, we have to scale and translate it to
        // make it fit the actual desktop playfield
        const float scale = GameRules::getPlayfieldScaleFactor();
        Vector2 translation = GameRules::getPlayfieldCenter();

        if(bm->hasFailed())
            translation =
                bm->osuCoords2Pixels(Vector2(GameRules::OSU_COORD_WIDTH / 2, GameRules::OSU_COORD_HEIGHT / 2));

        if(cv_mod_fps.getBool()) translation += bm->getFirstPersonCursorDelta();

        SliderRenderer::draw(g, m_vao, alwaysPoints, translation, scale, bm->m_fHitcircleDiameter, from, to,
                             undimmedComboColor, m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
    }
}

void Slider::update(long curPos) {
    HitObject::update(curPos);

    if(m_fSliderBreakRapeTime != 0.0f && engine->getTime() > m_fSliderBreakRapeTime) {
        m_fSliderBreakRapeTime = 0.0f;
        cv_epilepsy.setValue(0.0f);
    }

    if(bm != NULL) {
        // stop slide sound while paused
        if(bm->isPaused() || !bm->isPlaying() || bm->hasFailed()) {
            bm->getSkin()->stopSliderSlideSound();
        }

        // animations must be updated even if we are finished
        updateAnimations(curPos);
    }

    // all further calculations are only done while we are active
    if(m_bFinished) return;

    // slider slide percent
    m_fSlidePercent = 0.0f;
    if(curPos > m_iTime)
        m_fSlidePercent =
            clamp<float>(clamp<long>((curPos - (m_iTime)), 0, (long)m_fSliderTime) / m_fSliderTime, 0.0f, 1.0f);

    m_fActualSlidePercent = m_fSlidePercent;

    const float sliderSnakeDuration = (1.0f / 3.0f) * m_iApproachTime * cv_slider_snake_duration_multiplier.getFloat();
    m_fSliderSnakePercent = min(1.0f, (curPos - (m_iTime - m_iApproachTime)) / (sliderSnakeDuration));

    const long reverseArrowFadeInStart =
        m_iTime - (cv_snaking_sliders.getBool() ? (m_iApproachTime - sliderSnakeDuration) : m_iApproachTime);
    const long reverseArrowFadeInEnd = reverseArrowFadeInStart + cv_slider_reverse_arrow_fadein_duration.getInt();
    m_fReverseArrowAlpha = 1.0f - clamp<float>(((float)(reverseArrowFadeInEnd - curPos) /
                                                (float)(reverseArrowFadeInEnd - reverseArrowFadeInStart)),
                                               0.0f, 1.0f);
    m_fReverseArrowAlpha *= cv_slider_reverse_arrow_alpha_multiplier.getFloat();

    m_fBodyAlpha = m_fAlpha;
    if((bi->getModsLegacy() & LegacyFlags::Hidden))  // hidden modifies the body alpha
    {
        m_fBodyAlpha = m_fAlphaWithoutHidden;  // fade in as usual

        // fade out over the duration of the slider, starting exactly when the default fadein finishes
        const long hiddenSliderBodyFadeOutStart =
            min(m_iTime,
                m_iTime - m_iApproachTime + m_iFadeInTime);  // min() ensures that the fade always starts at m_iTime
                                                             // (even if the fadeintime is longer than the approachtime)
        const float fade_percent = cv_mod_hd_slider_fade_percent.getFloat();
        const long hiddenSliderBodyFadeOutEnd = m_iTime + (long)(fade_percent * m_fSliderTime);
        if(curPos >= hiddenSliderBodyFadeOutStart) {
            m_fBodyAlpha = clamp<float>(((float)(hiddenSliderBodyFadeOutEnd - curPos) /
                                         (float)(hiddenSliderBodyFadeOutEnd - hiddenSliderBodyFadeOutStart)),
                                        0.0f, 1.0f);
            m_fBodyAlpha *= m_fBodyAlpha;  // quad in body fadeout
        }
    }

    // if this slider is active, recalculate sliding/curve position and general state
    if(m_fSlidePercent > 0.0f || m_bVisible) {
        // handle reverse sliders
        m_bInReverse = false;
        m_bHideNumberAfterFirstRepeatHit = false;
        if(m_iRepeat > 1) {
            if(m_fSlidePercent > 0.0f && m_bStartFinished) m_bHideNumberAfterFirstRepeatHit = true;

            float part = 1.0f / (float)m_iRepeat;
            m_iCurRepeat = (int)(m_fSlidePercent * m_iRepeat);
            float baseSlidePercent = part * m_iCurRepeat;
            float partSlidePercent = (m_fSlidePercent - baseSlidePercent) / part;
            if(m_iCurRepeat % 2 == 0) {
                m_fSlidePercent = partSlidePercent;
                m_iReverseArrowPos = 2;
            } else {
                m_fSlidePercent = 1.0f - partSlidePercent;
                m_iReverseArrowPos = 1;
                m_bInReverse = true;
            }

            // no reverse arrow on the last repeat
            if(m_iCurRepeat == m_iRepeat - 1) m_iReverseArrowPos = 0;

            // osu style: immediately show all coming reverse arrows (even on the circle we just started from)
            if(m_iCurRepeat < m_iRepeat - 2 && m_fSlidePercent > 0.0f && m_iRepeat > 2) m_iReverseArrowPos = 3;
        }

        m_vCurPointRaw = m_curve->pointAt(m_fSlidePercent);
        m_vCurPoint = bi->osuCoords2Pixels(m_vCurPointRaw);
    } else {
        m_vCurPointRaw = m_curve->pointAt(0.0f);
        m_vCurPoint = bi->osuCoords2Pixels(m_vCurPointRaw);
    }

    // When fat finger key is released, remove isClickHeldSlider() restrictions
    if(m_iFatFingerKey == 1 && !bi->isKey1Down()) m_iFatFingerKey = 0;
    if(m_iFatFingerKey == 2 && !bi->isKey2Down()) m_iFatFingerKey = 0;

    // handle dynamic followradius
    float followRadius = m_bCursorLeft ? bi->m_fHitcircleDiameter / 2.0f : bi->m_fSliderFollowCircleDiameter / 2.0f;
    const bool isBeatmapCursorInside = ((bi->getCursorPos() - m_vCurPoint).length() < followRadius);
    const bool isAutoCursorInside =
        ((bi->getModsLegacy() & LegacyFlags::Autoplay) &&
         (!cv_auto_cursordance.getBool() || ((bi->getCursorPos() - m_vCurPoint).length() < followRadius)));
    m_bCursorInside = (isAutoCursorInside || isBeatmapCursorInside);
    m_bCursorLeft = !m_bCursorInside;

    // handle slider start
    if(!m_bStartFinished) {
        if((bi->getModsLegacy() & LegacyFlags::Autoplay)) {
            if(curPos >= m_iTime) {
                onHit(LiveScore::HIT::HIT_300, 0, false);
                bi->holding_slider = true;
            }
        } else {
            long delta = curPos - m_iTime;

            if((bi->getModsLegacy() & LegacyFlags::Relax)) {
                if(curPos >= m_iTime + (long)cv_relax_offset.getInt() && !bi->isPaused() &&
                   !bi->isContinueScheduled()) {
                    const Vector2 pos = bi->osuCoords2Pixels(m_curve->pointAt(0.0f));
                    const float cursorDelta = (bi->getCursorPos() - pos).length();
                    if((cursorDelta < bi->m_fHitcircleDiameter / 2.0f && (bi->getModsLegacy() & LegacyFlags::Relax))) {
                        LiveScore::HIT result = bi->getHitResult(delta);

                        if(result != LiveScore::HIT::HIT_NULL) {
                            const float targetDelta = cursorDelta / (bi->m_fHitcircleDiameter / 2.0f);
                            const float targetAngle =
                                rad2deg(atan2(bi->getCursorPos().y - pos.y, bi->getCursorPos().x - pos.x));

                            m_startResult = result;
                            onHit(m_startResult, delta, false, targetDelta, targetAngle);
                            bi->holding_slider = true;
                        }
                    }
                }
            }

            // wait for a miss
            if(delta >= 0) {
                // if this is a miss after waiting
                if(delta > (long)bi->getHitWindow50()) {
                    m_startResult = LiveScore::HIT::HIT_MISS;
                    onHit(m_startResult, delta, false);
                    bi->holding_slider = false;
                }
            }
        }
    }

    // handle slider end, repeats, ticks
    if(!m_bEndFinished) {
        // NOTE: we have 2 timing conditions after which we start checking for strict tracking: 1) startcircle was
        // clicked, 2) slider has started timing wise it is easily possible to hit the startcircle way before the
        // sliderball would become active, which is why the first check exists. even if the sliderball has not yet
        // started sliding, you will be punished for leaving the (still invisible) followcircle area after having
        // clicked the startcircle, always.
        const bool isTrackingStrictTrackingMod =
            ((m_bStartFinished || curPos >= m_iTime) && cv_mod_strict_tracking.getBool());

        // slider tail lenience bullshit: see
        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Objects/Slider.cs#L123 being "inside the slider"
        // (for the end of the slider) is NOT checked at the exact end of the slider, but somewhere random before,
        // because fuck you
        const long offset = (long)cv_slider_end_inside_check_offset.getInt();
        const long lenienceHackEndTime = max(m_iTime + m_iObjectDuration / 2, (m_iTime + m_iObjectDuration) - offset);
        const bool isTrackingCorrectly =
            (isClickHeldSlider() || (bi->getModsLegacy() & LegacyFlags::Relax)) && m_bCursorInside;
        if(isTrackingCorrectly) {
            if(isTrackingStrictTrackingMod) {
                m_iStrictTrackingModLastClickHeldTime = curPos;
                if(m_iStrictTrackingModLastClickHeldTime ==
                   0)  // (prevent frame perfect inputs from not triggering the strict tracking miss because we use != 0
                       // comparison to detect if tracking correctly at least once)
                    m_iStrictTrackingModLastClickHeldTime = 1;
            }

            // only check it at the exact point in time ...
            if(curPos >= lenienceHackEndTime) {
                // ... once (like a tick)
                if(!m_bHeldTillEndForLenienceHackCheck) {
                    m_bHeldTillEndForLenienceHackCheck = true;
                    m_bHeldTillEndForLenienceHack =
                        true;  // player was correctly clicking/holding inside slider at lenienceHackEndTime
                }
            }
        } else
            m_bCursorLeft = true;  // do not allow empty clicks outside of the circle radius to prevent the
                                   // m_bCursorInside flag from resetting

        // can't be "inside the slider" after lenienceHackEndTime (even though the slider is still going, which is
        // madness)
        if(curPos >= lenienceHackEndTime) m_bHeldTillEndForLenienceHackCheck = true;

        // handle strict tracking mod
        if(isTrackingStrictTrackingMod) {
            const bool wasTrackingCorrectlyAtLeastOnce = (m_iStrictTrackingModLastClickHeldTime != 0);
            if(wasTrackingCorrectlyAtLeastOnce && !isTrackingCorrectly) {
                if(!m_bHeldTillEndForLenienceHack)  // if past lenience end time then don't trigger a strict tracking
                                                    // miss, since the slider is then already considered fully finished
                                                    // gameplay wise
                {
                    if(m_endResult == LiveScore::HIT::HIT_NULL)  // force miss the end once, if it has not already been
                                                                 // force missed by notelock
                    {
                        // force miss endcircle
                        {
                            onSliderBreak();

                            m_bHeldTillEnd = false;
                            m_bHeldTillEndForLenienceHack = false;
                            m_bHeldTillEndForLenienceHackCheck = true;
                            m_endResult = LiveScore::HIT::HIT_MISS;

                            addHitResult(m_endResult, 0, m_bIsEndOfCombo, getRawPosAt(m_iTime + m_iObjectDuration),
                                         -1.0f, 0.0f, true, true,
                                         false);  // end of combo, ignore in hiterrorbar, ignore combo, subtract health
                        }
                    }
                }
            }
        }

        // handle repeats and ticks
        for(int i = 0; i < m_clicks.size(); i++) {
            if(!m_clicks[i].finished && curPos >= m_clicks[i].time) {
                m_clicks[i].finished = true;
                m_clicks[i].successful = (isClickHeldSlider() && m_bCursorInside) ||
                                         (bi->getModsLegacy() & LegacyFlags::Autoplay) ||
                                         ((bi->getModsLegacy() & LegacyFlags::Relax) && m_bCursorInside);

                if(m_clicks[i].type == 0)
                    onRepeatHit(m_clicks[i].successful, m_clicks[i].sliderend);
                else
                    onTickHit(m_clicks[i].successful, m_clicks[i].tickIndex);
            }
        }

        // handle auto, and the last circle
        if((bi->getModsLegacy() & LegacyFlags::Autoplay)) {
            if(curPos >= m_iTime + m_iObjectDuration) {
                m_bHeldTillEnd = true;
                onHit(LiveScore::HIT::HIT_300, 0, true);
                bi->holding_slider = false;
            }
        } else {
            if(curPos >= m_iTime + m_iObjectDuration) {
                // handle leftover startcircle
                {
                    // this may happen (if the slider time is shorter than the miss window of the startcircle)
                    if(m_startResult == LiveScore::HIT::HIT_NULL) {
                        // we still want to cause a sliderbreak in this case!
                        onSliderBreak();

                        // special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS
                        // health)
                        bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                                         false);  // only decrease health

                        m_startResult = LiveScore::HIT::HIT_MISS;
                    }
                }

                // handle endcircle
                bool isEndResultComingFromStrictTrackingMod = false;
                if(m_endResult == LiveScore::HIT::HIT_NULL) {
                    m_bHeldTillEnd = m_bHeldTillEndForLenienceHack;
                    m_endResult = m_bHeldTillEnd ? LiveScore::HIT::HIT_300 : LiveScore::HIT::HIT_MISS;

                    // handle total slider result (currently startcircle + repeats + ticks + endcircle)
                    // clicks = (repeats + ticks)
                    const float numMaxPossibleHits = 1 + m_clicks.size() + 1;
                    float numActualHits = 0;

                    if(m_startResult != LiveScore::HIT::HIT_MISS) numActualHits++;
                    if(m_endResult != LiveScore::HIT::HIT_MISS) numActualHits++;

                    for(int i = 0; i < m_clicks.size(); i++) {
                        if(m_clicks[i].successful) numActualHits++;
                    }

                    const float percent = numActualHits / numMaxPossibleHits;

                    const bool allow300 = (bi->getModsLegacy() & LegacyFlags::ScoreV2)
                                              ? (m_startResult == LiveScore::HIT::HIT_300)
                                              : true;
                    const bool allow100 =
                        (bi->getModsLegacy() & LegacyFlags::ScoreV2)
                            ? (m_startResult == LiveScore::HIT::HIT_300 || m_startResult == LiveScore::HIT::HIT_100)
                            : true;

                    // rewrite m_endResult as the whole slider result, then use it for the final onHit()
                    if(percent >= 0.999f && allow300)
                        m_endResult = LiveScore::HIT::HIT_300;
                    else if(percent >= 0.5f && allow100 && !cv_mod_ming3012.getBool() && !cv_mod_no100s.getBool())
                        m_endResult = LiveScore::HIT::HIT_100;
                    else if(percent > 0.0f && !cv_mod_no100s.getBool() && !cv_mod_no50s.getBool())
                        m_endResult = LiveScore::HIT::HIT_50;
                    else
                        m_endResult = LiveScore::HIT::HIT_MISS;

                    // debugLog("percent = %f\n", percent);

                    if(!m_bHeldTillEnd && cv_slider_end_miss_breaks_combo.getBool()) onSliderBreak();
                } else
                    isEndResultComingFromStrictTrackingMod = true;

                onHit(m_endResult, 0, true, 0.0f, 0.0f, isEndResultComingFromStrictTrackingMod);
                bi->holding_slider = false;
            }
        }

        // handle sliderslide sound
        if(bm != NULL) {
            bool sliding = m_bStartFinished && !m_bEndFinished && m_bCursorInside && m_iDelta <= 0;
            sliding &= (isClickHeldSlider() || (bm->getModsLegacy() & LegacyFlags::Autoplay) ||
                        (bm->getModsLegacy() & LegacyFlags::Relax));
            sliding &= !bm->isPaused() && !bm->isWaiting() && bm->isPlaying();

            if(sliding) {
                const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vCurPointRaw));

                bm->getSkin()->playSliderSlideSound(GameRules::osuCoords2Pan(osuCoords.x));
                m_iPrevSliderSlideSoundSampleSet = bm->getSkin()->getSampleSet();
            } else {
                bm->getSkin()->stopSliderSlideSound(m_iPrevSliderSlideSoundSampleSet);
                m_iPrevSliderSlideSoundSampleSet = -1;
            }
        }
    }
}

void Slider::updateAnimations(long curPos) {
    float animation_multiplier = bi->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();

    float fadein_fade_time = cv_slider_followcircle_fadein_fade_time.getFloat() * animation_multiplier;
    float fadeout_fade_time = cv_slider_followcircle_fadeout_fade_time.getFloat() * animation_multiplier;
    float fadein_scale_time = cv_slider_followcircle_fadein_scale_time.getFloat() * animation_multiplier;
    float fadeout_scale_time = cv_slider_followcircle_fadeout_scale_time.getFloat() * animation_multiplier;

    // handle followcircle animations
    m_fFollowCircleAnimationAlpha = clamp<float>(
        (float)((curPos - m_iTime)) / 1000.0f / clamp<float>(fadein_fade_time, 0.0f, m_iObjectDuration / 1000.0f), 0.0f,
        1.0f);
    if(m_bFinished) {
        m_fFollowCircleAnimationAlpha =
            1.0f -
            clamp<float>((float)((curPos - (m_iTime + m_iObjectDuration))) / 1000.0f / fadeout_fade_time, 0.0f, 1.0f);
        m_fFollowCircleAnimationAlpha *= m_fFollowCircleAnimationAlpha;  // quad in
    }

    m_fFollowCircleAnimationScale = clamp<float>(
        (float)((curPos - m_iTime)) / 1000.0f / clamp<float>(fadein_scale_time, 0.0f, m_iObjectDuration / 1000.0f),
        0.0f, 1.0f);
    if(m_bFinished) {
        m_fFollowCircleAnimationScale =
            clamp<float>((float)((curPos - (m_iTime + m_iObjectDuration))) / 1000.0f / fadeout_scale_time, 0.0f, 1.0f);
    }
    m_fFollowCircleAnimationScale =
        -m_fFollowCircleAnimationScale * (m_fFollowCircleAnimationScale - 2.0f);  // quad out

    if(!m_bFinished)
        m_fFollowCircleAnimationScale =
            cv_slider_followcircle_fadein_scale.getFloat() +
            (1.0f - cv_slider_followcircle_fadein_scale.getFloat()) * m_fFollowCircleAnimationScale;
    else
        m_fFollowCircleAnimationScale =
            1.0f - (1.0f - cv_slider_followcircle_fadeout_scale.getFloat()) * m_fFollowCircleAnimationScale;
}

void Slider::updateStackPosition(float stackOffset) {
    if(m_curve != NULL)
        m_curve->updateStackPosition(m_iStack * stackOffset, (bi->getModsLegacy() & LegacyFlags::HardRock));
}

void Slider::miss(long curPos) {
    if(m_bFinished) return;

    const long delta = curPos - m_iTime;

    // startcircle
    if(!m_bStartFinished) {
        m_startResult = LiveScore::HIT::HIT_MISS;
        onHit(m_startResult, delta, false);
        bi->holding_slider = false;
    }

    // endcircle, repeats, ticks
    if(!m_bEndFinished) {
        // repeats, ticks
        {
            for(int i = 0; i < m_clicks.size(); i++) {
                if(!m_clicks[i].finished) {
                    m_clicks[i].finished = true;
                    m_clicks[i].successful = false;

                    if(m_clicks[i].type == 0)
                        onRepeatHit(m_clicks[i].successful, m_clicks[i].sliderend);
                    else
                        onTickHit(m_clicks[i].successful, m_clicks[i].tickIndex);
                }
            }
        }

        // endcircle
        {
            m_bHeldTillEnd = m_bHeldTillEndForLenienceHack;

            if(!m_bHeldTillEnd && cv_slider_end_miss_breaks_combo.getBool()) onSliderBreak();

            m_endResult = LiveScore::HIT::HIT_MISS;
            onHit(m_endResult, 0, true);
            bi->holding_slider = false;
        }
    }
}

Vector2 Slider::getRawPosAt(long pos) {
    if(m_curve == NULL) return Vector2(0, 0);

    if(pos <= m_iTime)
        return m_curve->pointAt(0.0f);
    else if(pos >= m_iTime + m_fSliderTime) {
        if(m_iRepeat % 2 == 0)
            return m_curve->pointAt(0.0f);
        else
            return m_curve->pointAt(1.0f);
    } else
        return m_curve->pointAt(getT(pos, false));
}

Vector2 Slider::getOriginalRawPosAt(long pos) {
    if(m_curve == NULL) return Vector2(0, 0);

    if(pos <= m_iTime)
        return m_curve->originalPointAt(0.0f);
    else if(pos >= m_iTime + m_fSliderTime) {
        if(m_iRepeat % 2 == 0)
            return m_curve->originalPointAt(0.0f);
        else
            return m_curve->originalPointAt(1.0f);
    } else
        return m_curve->originalPointAt(getT(pos, false));
}

float Slider::getT(long pos, bool raw) {
    float t = (float)((long)pos - (long)m_iTime) / m_fSliderTimeWithoutRepeats;
    if(raw)
        return t;
    else {
        float floorVal = (float)std::floor(t);
        return ((int)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
    }
}

void Slider::onClickEvent(std::vector<Click> &clicks) {
    if(m_points.size() == 0 || m_bBlocked)
        return;  // also handle note blocking here (doesn't need fancy shake logic, since sliders don't shake in
                 // osu!stable)

    if(!m_bStartFinished) {
        const Vector2 cursorPos = clicks[0].pos;
        const Vector2 pos = bi->osuCoords2Pixels(m_curve->pointAt(0.0f));
        const float cursorDelta = (cursorPos - pos).length();

        if(cursorDelta < bi->m_fHitcircleDiameter / 2.0f) {
            const long delta = clicks[0].tms - (long)m_iTime;

            LiveScore::HIT result = bi->getHitResult(delta);
            if(result != LiveScore::HIT::HIT_NULL) {
                const float targetDelta = cursorDelta / (bi->m_fHitcircleDiameter / 2.0f);
                const float targetAngle = rad2deg(atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

                clicks.erase(clicks.begin());
                m_startResult = result;
                onHit(m_startResult, delta, false, targetDelta, targetAngle);
                bi->holding_slider = true;
            }
        }
    }
}

void Slider::onHit(LiveScore::HIT result, long delta, bool startOrEnd, float targetDelta, float targetAngle,
                   bool isEndResultFromStrictTrackingMod) {
    if(m_points.size() == 0) return;

    // start + end of a slider add +30 points, if successful

    // debugLog("startOrEnd = %i,    m_iCurRepeat = %i\n", (int)startOrEnd, m_iCurRepeat);

    // sound and hit animation and also sliderbreak combo drop
    {
        if(result == LiveScore::HIT::HIT_MISS) {
            if(!isEndResultFromStrictTrackingMod) onSliderBreak();
        } else if(bm != NULL) {
            if(cv_timingpoints_force.getBool()) bm->updateTimingPoints(m_iTime + (!startOrEnd ? 0 : m_iObjectDuration));

            const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vCurPointRaw));

            const long sound_delta = result == LiveScore::HIT::HIT_300 ? 0 : delta;
            bm->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size()
                                                  ? m_hitSounds[m_iCurRepeatCounterForHitSounds]
                                                  : m_iSampleType,
                                              GameRules::osuCoords2Pan(osuCoords.x), sound_delta);

            if(!startOrEnd) {
                m_fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
            } else {
                if(m_iRepeat % 2 != 0) {
                    m_fEndHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                    anim->moveQuadOut(&m_fEndHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
                } else {
                    m_fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
                    anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
                }
            }
        }

        // end body fadeout
        if(bm != NULL && startOrEnd) {
            m_fEndSliderBodyFadeAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&m_fEndSliderBodyFadeAnimation, 1.0f,
                              GameRules::getFadeOutTime(bm) * cv_slider_body_fade_out_time_multiplier.getFloat(), true);

            bm->getSkin()->stopSliderSlideSound();
        }
    }

    // add score, and we are finished
    if(!startOrEnd) {
        // startcircle

        m_bStartFinished = true;

        // The player entered the slider by fat fingering, so...
        if(bi->isKey1Down() && bi->isKey2Down()) {
            if(bi->m_bPrevKeyWasKey1) {
                // Player pressed K1 last: "fat finger" key is K2
                m_iFatFingerKey = 2;
            } else {
                // Player pressed K2 last: "fat finger" key is K1
                m_iFatFingerKey = 1;
            }
        }

        if(!(bi->getModsLegacy() & LegacyFlags::Target))
            bi->addHitResult(this, result, delta, false, false, true, false, true,
                             true);  // not end of combo, show in hiterrorbar, ignore for accuracy, increase combo,
                                     // don't count towards score, depending on scorev2 ignore for health or not
        else
            addHitResult(result, delta, false, m_curve->pointAt(0.0f), targetDelta, targetAngle, false, false, true,
                         false);  // not end of combo, show in hiterrorbar, use for accuracy, increase combo, increase
                                  // score, ignore for health, don't add object duration to result anim

        // add bonus score + health manually
        if(result != LiveScore::HIT::HIT_MISS) {
            LiveScore::HIT resultForHealth = LiveScore::HIT::HIT_SLIDER30;

            bi->addHitResult(this, resultForHealth, 0, false, true, true, true, true,
                             false);  // only increase health
            bi->addScorePoints(30);
        } else {
            // special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
            bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                             false);  // only decrease health
        }
    } else {
        // endcircle

        m_bStartFinished = true;
        m_bEndFinished = true;
        m_bFinished = true;

        if(!isEndResultFromStrictTrackingMod) {
            // special case: osu!lazer 2020 only returns 1 judgement for the whole slider, but via the startcircle. i.e.
            // we are not allowed to drain again here in neosu logic (because startcircle judgement is handled at the
            // end here)
            // XXX: remove this
            const bool isLazer2020Drain = false;

            addHitResult(result, delta, m_bIsEndOfCombo, getRawPosAt(m_iTime + m_iObjectDuration), -1.0f, 0.0f, true,
                         !m_bHeldTillEnd,
                         isLazer2020Drain);  // end of combo, ignore in hiterrorbar, depending on heldTillEnd increase
                                             // combo or not, increase score, increase health depending on drain type

            // add bonus score + extra health manually
            if(m_bHeldTillEnd) {
                bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER30, 0, false, true, true, true, true,
                                 false);  // only increase health
                bi->addScorePoints(30);
            } else {
                // special case: missing the endcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
                // NOTE: yes, this will drain twice for the end of a slider (once for the judgement of the whole slider
                // above, and once for the endcircle here)
                bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                                 false);  // only decrease health
            }
        }
    }

    m_iCurRepeatCounterForHitSounds++;
}

void Slider::onRepeatHit(bool successful, bool sliderend) {
    if(m_points.size() == 0) return;

    // repeat hit of a slider adds +30 points, if successful

    // sound and hit animation
    if(!successful) {
        onSliderBreak();
    } else if(bm != NULL) {
        if(cv_timingpoints_force.getBool())
            bm->updateTimingPoints(m_iTime + (long)((float)m_iObjectDuration * m_fActualSlidePercent));

        const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vCurPointRaw));

        bm->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size()
                                              ? m_hitSounds[m_iCurRepeatCounterForHitSounds]
                                              : m_iSampleType,
                                          GameRules::osuCoords2Pan(osuCoords.x), 0);

        float animation_multiplier = bm->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
        float tick_pulse_time = cv_slider_followcircle_tick_pulse_time.getFloat() * animation_multiplier;

        m_fFollowCircleTickAnimationScale = 0.0f;
        anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, tick_pulse_time, true);

        if(sliderend) {
            m_fEndHitAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&m_fEndHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
        } else {
            m_fStartHitAnimation = 0.001f;  // quickfix for 1 frame missing images
            anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
        }
    }

    // add score
    if(!successful) {
        // add health manually
        // special case: missing a repeat drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
        bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                         false);  // only decrease health
    } else {
        bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER30, 0, false, true, true, false, true,
                         false);  // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase
                                  // combo, don't count towards score, increase health

        // add bonus score manually
        bi->addScorePoints(30);
    }

    m_iCurRepeatCounterForHitSounds++;
}

void Slider::onTickHit(bool successful, int tickIndex) {
    if(m_points.size() == 0) return;

    // tick hit of a slider adds +10 points, if successful

    // tick drawing visibility
    int numMissingTickClicks = 0;
    for(int i = 0; i < m_clicks.size(); i++) {
        if(m_clicks[i].type == 1 && m_clicks[i].tickIndex == tickIndex && !m_clicks[i].finished) numMissingTickClicks++;
    }
    if(numMissingTickClicks == 0) m_ticks[tickIndex].finished = true;

    // sound and hit animation
    if(!successful) {
        onSliderBreak();
    } else if(bm != NULL) {
        if(cv_timingpoints_force.getBool())
            bm->updateTimingPoints(m_iTime + (long)((float)m_iObjectDuration * m_fActualSlidePercent));

        const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vCurPointRaw));

        bm->getSkin()->playSliderTickSound(GameRules::osuCoords2Pan(osuCoords.x));

        float animation_multiplier = bm->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
        float tick_pulse_time = cv_slider_followcircle_tick_pulse_time.getFloat() * animation_multiplier;

        m_fFollowCircleTickAnimationScale = 0.0f;
        anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, tick_pulse_time, true);
    }

    // add score
    if(!successful) {
        // add health manually
        // special case: missing a tick drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
        bi->addHitResult(this, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                         false);  // only decrease health
    } else {
        bi->addHitResult(this, LiveScore::HIT::HIT_SLIDER10, 0, false, true, true, false, true,
                         false);  // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase
                                  // combo, don't count towards score, increase health

        // add bonus score manually
        bi->addScorePoints(10);
    }
}

void Slider::onSliderBreak() {
    bi->addSliderBreak();

    if(cv_slider_break_epilepsy.getBool()) {
        m_fSliderBreakRapeTime = engine->getTime() + 0.15f;
        cv_epilepsy.setValue(1.0f);
    }
}

void Slider::onReset(long curPos) {
    HitObject::onReset(curPos);

    if(bm != NULL) {
        bm->getSkin()->stopSliderSlideSound();
    }

    m_iStrictTrackingModLastClickHeldTime = 0;
    m_iFatFingerKey = 0;
    m_iPrevSliderSlideSoundSampleSet = -1;
    m_bCursorLeft = true;
    m_bHeldTillEnd = false;
    m_bHeldTillEndForLenienceHack = false;
    m_bHeldTillEndForLenienceHackCheck = false;
    m_startResult = LiveScore::HIT::HIT_NULL;
    m_endResult = LiveScore::HIT::HIT_NULL;

    m_iCurRepeatCounterForHitSounds = 0;

    anim->deleteExistingAnimation(&m_fFollowCircleTickAnimationScale);
    anim->deleteExistingAnimation(&m_fStartHitAnimation);
    anim->deleteExistingAnimation(&m_fEndHitAnimation);
    anim->deleteExistingAnimation(&m_fEndSliderBodyFadeAnimation);

    if(m_iTime > curPos) {
        m_bStartFinished = false;
        m_fStartHitAnimation = 0.0f;
        m_bEndFinished = false;
        m_bFinished = false;
        m_fEndHitAnimation = 0.0f;
        m_fEndSliderBodyFadeAnimation = 0.0f;
    } else if(curPos < m_iTime + m_iObjectDuration) {
        m_bStartFinished = true;
        m_fStartHitAnimation = 1.0f;

        m_bEndFinished = false;
        m_bFinished = false;
        m_fEndHitAnimation = 0.0f;
        m_fEndSliderBodyFadeAnimation = 0.0f;
    } else {
        m_bStartFinished = true;
        m_fStartHitAnimation = 1.0f;
        m_bEndFinished = true;
        m_bFinished = true;
        m_fEndHitAnimation = 1.0f;
        m_fEndSliderBodyFadeAnimation = 1.0f;
    }

    for(int i = 0; i < m_clicks.size(); i++) {
        if(curPos > m_clicks[i].time) {
            m_clicks[i].finished = true;
            m_clicks[i].successful = true;
        } else {
            m_clicks[i].finished = false;
            m_clicks[i].successful = false;
        }
    }

    for(int i = 0; i < m_ticks.size(); i++) {
        int numMissingTickClicks = 0;
        for(int c = 0; c < m_clicks.size(); c++) {
            if(m_clicks[c].type == 1 && m_clicks[c].tickIndex == i && !m_clicks[c].finished) numMissingTickClicks++;
        }
        m_ticks[i].finished = numMissingTickClicks == 0;
    }

    m_fSliderBreakRapeTime = 0.0f;
    cv_epilepsy.setValue(0.0f);
}

void Slider::rebuildVertexBuffer(bool useRawCoords) {
    // base mesh (background) (raw unscaled, size in raw osu coordinates centered at (0, 0, 0))
    // this mesh needs to be scaled and translated appropriately since we are not 1:1 with the playfield
    std::vector<Vector2> osuCoordPoints = m_curve->getPoints();
    if(!useRawCoords) {
        for(int p = 0; p < osuCoordPoints.size(); p++) {
            osuCoordPoints[p] = bi->osuCoords2LegacyPixels(osuCoordPoints[p]);
        }
    }
    SAFE_DELETE(m_vao);
    m_vao = SliderRenderer::generateVAO(osuCoordPoints, bi->m_fRawHitcircleDiameter);
}

bool Slider::isClickHeldSlider() {
    // osu! has a weird slider quirk, that I'll explain in detail here.
    // When holding K1 before the slider, tapping K2 on slider head, and releasing K2 later,
    // the slider is no longer considered being "held" until K2 is pressed again, or K1 is released and pressed again.

    // The reason this exists is to prevent people from holding K1 the whole map and tapping with K2.
    // Holding is part of the rhythm flow, and this is a rhythm game right?

    if(m_iFatFingerKey == 0) return bi->isClickHeld();
    if(m_iFatFingerKey == 1) return bi->isKey2Down();
    if(m_iFatFingerKey == 2) return bi->isKey1Down();

    return false;  // unreachable
}
