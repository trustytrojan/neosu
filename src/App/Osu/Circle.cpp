#include "Circle.h"

#include "AnimationHandler.h"
#include "Beatmap.h"
#include "Camera.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "ModFPoSu.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"
#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SoundEngine.h"

int Circle::rainbowNumber = 0;
int Circle::rainbowColorCounter = 0;

void Circle::drawApproachCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                bool overrideHDApproachCircle) {
    rainbowNumber = number;
    rainbowColorCounter = colorCounter;

    Color comboColor = beatmap->getSkin()->getComboColorForCounter(colorCounter, colorOffset);
    comboColor =
        COLOR(255, (int)(COLOR_GET_Ri(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Gi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Bi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()));

    drawApproachCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), comboColor,
                       beatmap->m_fHitcircleDiameter, approachScale, alpha, osu->getModHD(), overrideHDApproachCircle);
}

void Circle::drawCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                        float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber,
                        bool overrideHDApproachCircle) {
    drawCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->m_fHitcircleDiameter,
               beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset,
               colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void Circle::drawCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                        float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier,
                        float approachScale, float alpha, float numberAlpha, bool drawNumber,
                        bool overrideHDApproachCircle) {
    if(alpha <= 0.0f || !cv_draw_circles.getBool()) return;

    rainbowNumber = number;
    rainbowColorCounter = colorCounter;

    Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
    comboColor =
        COLOR(255, (int)(COLOR_GET_Ri(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Gi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Bi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()));

    // approach circle
    /// drawApproachCircle(g, skin, pos, comboColor, hitcircleDiameter, approachScale, alpha, modHD,
    /// overrideHDApproachCircle); // they are now drawn separately in draw2()

    // circle
    const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
    drawHitCircle(g, skin->getHitCircle(), pos, comboColor, circleImageScale, alpha);

    // overlay
    const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
    if(!skin->getHitCircleOverlayAboveNumber())
        drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);

    // number
    if(drawNumber)
        drawHitCircleNumber(g, skin, numberScale, overlapScale, pos, number, numberAlpha, colorRGBMultiplier);

    // overlay
    if(skin->getHitCircleOverlayAboveNumber())
        drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
}

void Circle::drawCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha) {
    // this function is only used by the target practice heatmap

    // circle
    const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
    drawHitCircle(g, skin->getHitCircle(), pos, color, circleImageScale, alpha);

    // overlay
    const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
    drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, 1.0f);
}

void Circle::drawSliderStartCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                   int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                   float numberAlpha, bool drawNumber, bool overrideHDApproachCircle) {
    drawSliderStartCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->m_fHitcircleDiameter,
                          beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter,
                          colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber,
                          overrideHDApproachCircle);
}

void Circle::drawSliderStartCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                   float hitcircleOverlapScale, int number, int colorCounter, int colorOffset,
                                   float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                                   bool drawNumber, bool overrideHDApproachCircle) {
    if(alpha <= 0.0f || !cv_draw_circles.getBool()) return;

    // if no sliderstartcircle image is preset, fallback to default circle
    if(skin->getSliderStartCircle() == skin->getMissingTexture()) {
        drawCircle(g, skin, pos, hitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter,
                   colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber,
                   overrideHDApproachCircle);  // normal
        return;
    }

    rainbowNumber = number;
    rainbowColorCounter = colorCounter;

    Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
    comboColor =
        COLOR(255, (int)(COLOR_GET_Ri(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Gi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Bi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()));

    // circle
    const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderStartCircle2x() ? 2.0f : 1.0f));
    drawHitCircle(g, skin->getSliderStartCircle(), pos, comboColor, circleImageScale, alpha);

    // overlay
    const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderStartCircleOverlay2()->getSizeBaseRaw().x;
    if(skin->getSliderStartCircleOverlay() != skin->getMissingTexture()) {
        if(!skin->getHitCircleOverlayAboveNumber())
            drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha,
                                 colorRGBMultiplier);
    }

    // number
    if(drawNumber)
        drawHitCircleNumber(g, skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha, colorRGBMultiplier);

    // overlay
    if(skin->getSliderStartCircleOverlay() != skin->getMissingTexture()) {
        if(skin->getHitCircleOverlayAboveNumber())
            drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha,
                                 colorRGBMultiplier);
    }
}

void Circle::drawSliderEndCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                 int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                 float numberAlpha, bool drawNumber, bool overrideHDApproachCircle) {
    drawSliderEndCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->m_fHitcircleDiameter,
                        beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter,
                        colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber,
                        overrideHDApproachCircle);
}

void Circle::drawSliderEndCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                 float overlapScale, int number, int colorCounter, int colorOffset,
                                 float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                                 bool drawNumber, bool overrideHDApproachCircle) {
    if(alpha <= 0.0f || !cv_slider_draw_endcircle.getBool() || !cv_draw_circles.getBool()) return;

    // if no sliderendcircle image is preset, fallback to default circle
    if(skin->getSliderEndCircle() == skin->getMissingTexture()) {
        drawCircle(g, skin, pos, hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset,
                   colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
        return;
    }

    rainbowNumber = number;
    rainbowColorCounter = colorCounter;

    Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
    comboColor =
        COLOR(255, (int)(COLOR_GET_Ri(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Gi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()),
              (int)(COLOR_GET_Bi(comboColor) * colorRGBMultiplier * cv_circle_color_saturation.getFloat()));

    // circle
    const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderEndCircle2x() ? 2.0f : 1.0f));
    drawHitCircle(g, skin->getSliderEndCircle(), pos, comboColor, circleImageScale, alpha);

    // overlay
    if(skin->getSliderEndCircleOverlay() != skin->getMissingTexture()) {
        const float circleOverlayImageScale =
            hitcircleDiameter / skin->getSliderEndCircleOverlay2()->getSizeBaseRaw().x;
        drawHitCircleOverlay(g, skin->getSliderEndCircleOverlay2(), pos, circleOverlayImageScale, alpha,
                             colorRGBMultiplier);
    }
}

void Circle::drawApproachCircle(Graphics *g, Skin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter,
                                float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle) {
    if((!modHD || overrideHDApproachCircle) && cv_draw_approach_circles.getBool() && !cv_mod_mafham.getBool()) {
        if(approachScale > 1.0f) {
            const float approachCircleImageScale =
                hitcircleDiameter / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

            g->setColor(comboColor);

            if(cv_circle_rainbow.getBool()) {
                float frequency = 0.3f;
                float time = engine->getTime() * 20;

                char red1 = std::sin(frequency * time + 0 + rainbowNumber * rainbowColorCounter) * 127 + 128;
                char green1 = std::sin(frequency * time + 2 + rainbowNumber * rainbowColorCounter) * 127 + 128;
                char blue1 = std::sin(frequency * time + 4 + rainbowNumber * rainbowColorCounter) * 127 + 128;

                g->setColor(COLOR(255, red1, green1, blue1));
            }

            g->setAlpha(alpha * cv_approach_circle_alpha_multiplier.getFloat());

            g->pushTransform();
            {
                g->scale(approachCircleImageScale * approachScale, approachCircleImageScale * approachScale);
                g->translate(pos.x, pos.y);
                g->drawImage(skin->getApproachCircle());
            }
            g->popTransform();
        }
    }
}

void Circle::drawHitCircleOverlay(Graphics *g, SkinImage *hitCircleOverlayImage, Vector2 pos,
                                  float circleOverlayImageScale, float alpha, float colorRGBMultiplier) {
    g->setColor(COLORf(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier));
    g->setAlpha(alpha);
    hitCircleOverlayImage->drawRaw(g, pos, circleOverlayImageScale);
}

void Circle::drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale,
                           float alpha) {
    g->setColor(comboColor);

    if(cv_circle_rainbow.getBool()) {
        float frequency = 0.3f;
        float time = engine->getTime() * 20;

        char red1 = std::sin(frequency * time + 0 + rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 + 128;
        char green1 = std::sin(frequency * time + 2 + rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 + 128;
        char blue1 = std::sin(frequency * time + 4 + rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 + 128;

        g->setColor(COLOR(255, red1, green1, blue1));
    }

    g->setAlpha(alpha);

    g->pushTransform();
    {
        g->scale(circleImageScale, circleImageScale);
        g->translate(pos.x, pos.y);
        g->drawImage(hitCircleImage);
    }
    g->popTransform();
}

void Circle::drawHitCircleNumber(Graphics *g, Skin *skin, float numberScale, float overlapScale, Vector2 pos,
                                 int number, float numberAlpha, float colorRGBMultiplier) {
    if(!cv_draw_numbers.getBool()) return;

    class DigitWidth {
       public:
        static float getWidth(Skin *skin, int digit) {
            switch(digit) {
                case 0:
                    return skin->getDefault0()->getWidth();
                case 1:
                    return skin->getDefault1()->getWidth();
                case 2:
                    return skin->getDefault2()->getWidth();
                case 3:
                    return skin->getDefault3()->getWidth();
                case 4:
                    return skin->getDefault4()->getWidth();
                case 5:
                    return skin->getDefault5()->getWidth();
                case 6:
                    return skin->getDefault6()->getWidth();
                case 7:
                    return skin->getDefault7()->getWidth();
                case 8:
                    return skin->getDefault8()->getWidth();
                case 9:
                    return skin->getDefault9()->getWidth();
            }

            return skin->getDefault0()->getWidth();
        }
    };

    // generate digits
    std::vector<int> digits;
    while(number >= 10) {
        digits.push_back(number % 10);
        number = number / 10;
    }
    digits.push_back(number);

    // set color
    // g->setColor(COLORf(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // see
    // https://github.com/ppy/osu/issues/24506
    g->setColor(0xffffffff);
    if(cv_circle_number_rainbow.getBool()) {
        float frequency = 0.3f;
        float time = engine->getTime() * 20;

        char red1 =
            std::sin(frequency * time + 0 + rainbowNumber * rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 +
            128;
        char green1 =
            std::sin(frequency * time + 2 + rainbowNumber * rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 +
            128;
        char blue1 =
            std::sin(frequency * time + 4 + rainbowNumber * rainbowNumber * rainbowNumber * rainbowColorCounter) * 127 +
            128;

        g->setColor(COLOR(255, red1, green1, blue1));
    }
    g->setAlpha(numberAlpha);

    // draw digits, start at correct offset
    g->pushTransform();
    {
        g->scale(numberScale, numberScale);
        g->translate(pos.x, pos.y);
        {
            float digitWidthCombined = 0.0f;
            for(size_t i = 0; i < digits.size(); i++) {
                digitWidthCombined += DigitWidth::getWidth(skin, digits[i]);
            }

            const int digitOverlapCount = digits.size() - 1;
            g->translate(
                -(digitWidthCombined * numberScale - skin->getHitCircleOverlap() * digitOverlapCount * overlapScale) *
                        0.5f +
                    DigitWidth::getWidth(skin, (digits.size() > 0 ? digits[digits.size() - 1] : 0)) * numberScale *
                        0.5f,
                0);
        }

        for(int i = digits.size() - 1; i >= 0; i--) {
            switch(digits[i]) {
                case 0:
                    g->drawImage(skin->getDefault0());
                    break;
                case 1:
                    g->drawImage(skin->getDefault1());
                    break;
                case 2:
                    g->drawImage(skin->getDefault2());
                    break;
                case 3:
                    g->drawImage(skin->getDefault3());
                    break;
                case 4:
                    g->drawImage(skin->getDefault4());
                    break;
                case 5:
                    g->drawImage(skin->getDefault5());
                    break;
                case 6:
                    g->drawImage(skin->getDefault6());
                    break;
                case 7:
                    g->drawImage(skin->getDefault7());
                    break;
                case 8:
                    g->drawImage(skin->getDefault8());
                    break;
                case 9:
                    g->drawImage(skin->getDefault9());
                    break;
            }

            float offset = DigitWidth::getWidth(skin, digits[i]) * numberScale;
            if(i > 0) {
                offset += DigitWidth::getWidth(skin, digits[i - 1]) * numberScale;
            }

            g->translate(offset * 0.5f - skin->getHitCircleOverlap() * overlapScale, 0);
        }
    }
    g->popTransform();
}

Circle::Circle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter,
               int colorOffset, BeatmapInterface *beatmap)
    : HitObject(time, sampleType, comboNumber, isEndOfCombo, colorCounter, colorOffset, beatmap) {
    m_vOriginalRawPos = Vector2(x, y);
    m_vRawPos = m_vOriginalRawPos;
    m_bWaiting = false;
    m_fHitAnimation = 0.0f;
    m_fShakeAnimation = 0.0f;
}

Circle::~Circle() { onReset(0); }

void Circle::draw(Graphics *g) {
    HitObject::draw(g);
    Skin *skin = osu->getSkin();

    // draw hit animation
    bool is_instafade = cv_instafade.getBool();
    if(!is_instafade && m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !osu->getModHD()) {
        float alpha = 1.0f - m_fHitAnimation;

        float scale = m_fHitAnimation;
        scale = -scale * (scale - 2.0f);  // quad out scale

        const bool drawNumber = skin->getVersion() > 1.0f ? false : true;
        const float foscale = cv_circle_fade_out_scale.getFloat();

        g->pushTransform();
        {
            g->scale((1.0f + scale * foscale), (1.0f + scale * foscale));
            skin->getHitCircleOverlay2()->setAnimationTimeOffset(
                skin->getAnimationSpeed(),
                !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());
            drawCircle(g, bm, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha,
                       drawNumber);
        }
        g->popTransform();
    }

    if(m_bFinished ||
       (!m_bVisible && !m_bWaiting))  // special case needed for when we are past this objects time, but still within
                                      // not-miss range, because we still need to draw the object
        return;

    // draw circle
    const bool hd = osu->getModHD();
    Vector2 shakeCorrectedPos = m_vRawPos;
    if(engine->getTime() < m_fShakeAnimation && !bm->isInMafhamRenderChunk())  // handle note blocking shaking
    {
        float smooth =
            1.0f - ((m_fShakeAnimation - engine->getTime()) / cv_circle_shake_duration.getFloat());  // goes from 0 to 1
        if(smooth < 0.5f)
            smooth = smooth / 0.5f;
        else
            smooth = (1.0f - smooth) / 0.5f;
        // (now smooth goes from 0 to 1 to 0 linearly)
        smooth = -smooth * (smooth - 2);  // quad out
        smooth = -smooth * (smooth - 2);  // quad out twice
        shakeCorrectedPos.x += std::sin(engine->getTime() * 120) * smooth * cv_circle_shake_strength.getFloat();
    }
    skin->getHitCircleOverlay2()->setAnimationTimeOffset(
        skin->getAnimationSpeed(),
        !bm->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : bm->getCurMusicPosWithOffsets());
    drawCircle(g, bm, shakeCorrectedPos, m_iComboNumber, m_iColorCounter, m_iColorOffset,
               m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale,
               m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true,
               m_bOverrideHDApproachCircle);
}

void Circle::draw2(Graphics *g) {
    HitObject::draw2(g);
    if(m_bFinished || (!m_bVisible && !m_bWaiting))
        return;  // special case needed for when we are past this objects time, but still within not-miss range, because
                 // we still need to draw the object

    // draw approach circle
    const bool hd = osu->getModHD();

    // HACKHACK: don't fucking change this piece of code here, it fixes a heisenbug
    // (https://github.com/McKay42/McOsu/issues/165)
    if(cv_bug_flicker_log.getBool()) {
        const float approachCircleImageScale =
            bm->m_fHitcircleDiameter / (128.0f * (bm->getSkin()->isApproachCircle2x() ? 2.0f : 1.0f));
        debugLog("m_iTime = %ld, aScale = %f, iScale = %f\n", m_iTime, m_fApproachScale, approachCircleImageScale);
    }

    drawApproachCircle(g, bm, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset,
                       m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale,
                       m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void Circle::update(long curPos) {
    HitObject::update(curPos);

    // if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
    if(!m_bFinished) {
        if((bi->getModsLegacy() & LegacyFlags::Autoplay)) {
            if(curPos >= m_iTime) onHit(LiveScore::HIT::HIT_300, 0);
        } else {
            const long delta = curPos - m_iTime;

            if((bi->getModsLegacy() & LegacyFlags::Relax)) {
                if(curPos >= m_iTime + (long)cv_relax_offset.getInt() && !bi->isPaused() &&
                   !bi->isContinueScheduled()) {
                    const Vector2 pos = bi->osuCoords2Pixels(m_vRawPos);
                    const float cursorDelta = (bi->getCursorPos() - pos).length();
                    if((cursorDelta < bi->m_fHitcircleDiameter / 2.0f && (bi->getModsLegacy() & LegacyFlags::Relax))) {
                        LiveScore::HIT result = bi->getHitResult(delta);

                        if(result != LiveScore::HIT::HIT_NULL) {
                            const float targetDelta = cursorDelta / (bi->m_fHitcircleDiameter / 2.0f);
                            const float targetAngle =
                                rad2deg(atan2(bi->getCursorPos().y - pos.y, bi->getCursorPos().x - pos.x));

                            onHit(result, delta, targetDelta, targetAngle);
                        }
                    }
                }
            }

            if(delta >= 0) {
                m_bWaiting = true;

                // if this is a miss after waiting
                if(delta > (long)bi->getHitWindow50()) onHit(LiveScore::HIT::HIT_MISS, delta);
            } else
                m_bWaiting = false;
        }
    }
}

void Circle::updateStackPosition(float stackOffset) {
    m_vRawPos = m_vOriginalRawPos -
                Vector2(m_iStack * stackOffset,
                        m_iStack * stackOffset * ((bi->getModsLegacy() & LegacyFlags::HardRock) ? -1.0f : 1.0f));
}

void Circle::miss(long curPos) {
    if(m_bFinished) return;

    const long delta = curPos - m_iTime;

    onHit(LiveScore::HIT::HIT_MISS, delta);
}

void Circle::onClickEvent(std::vector<Click> &clicks) {
    if(m_bFinished) return;

    const Vector2 cursorPos = clicks[0].pos;
    const Vector2 pos = bi->osuCoords2Pixels(m_vRawPos);
    const float cursorDelta = (cursorPos - pos).length();

    if(cursorDelta < bi->m_fHitcircleDiameter / 2.0f) {
        // note blocking & shake
        if(m_bBlocked) {
            m_fShakeAnimation = engine->getTime() + cv_circle_shake_duration.getFloat();
            return;  // ignore click event completely
        }

        const long delta = clicks[0].tms - (long)m_iTime;

        LiveScore::HIT result = bi->getHitResult(delta);
        if(result != LiveScore::HIT::HIT_NULL) {
            const float targetDelta = cursorDelta / (bi->m_fHitcircleDiameter / 2.0f);
            const float targetAngle = rad2deg(atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

            clicks.erase(clicks.begin());
            onHit(result, delta, targetDelta, targetAngle);
        }
    }
}

void Circle::onHit(LiveScore::HIT result, long delta, float targetDelta, float targetAngle) {
    // sound and hit animation
    if(bm != NULL && result != LiveScore::HIT::HIT_MISS) {
        if(cv_timingpoints_force.getBool()) bm->updateTimingPoints(m_iTime);

        const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vRawPos));

        const long sound_delta = result == LiveScore::HIT::HIT_300 ? 0 : delta;
        bm->getSkin()->playHitCircleSound(m_iSampleType, GameRules::osuCoords2Pan(osuCoords.x), sound_delta);

        m_fHitAnimation = 0.001f;  // quickfix for 1 frame missing images
        anim->moveQuadOut(&m_fHitAnimation, 1.0f, GameRules::getFadeOutTime(bm), true);
    }

    // add it, and we are finished
    addHitResult(result, delta, m_bIsEndOfCombo, m_vRawPos, targetDelta, targetAngle);
    m_bFinished = true;
}

void Circle::onReset(long curPos) {
    HitObject::onReset(curPos);

    m_bWaiting = false;
    m_fShakeAnimation = 0.0f;

    anim->deleteExistingAnimation(&m_fHitAnimation);

    if(m_iTime > curPos) {
        m_bFinished = false;
        m_fHitAnimation = 0.0f;
    } else {
        m_bFinished = true;
        m_fHitAnimation = 1.0f;
    }
}

Vector2 Circle::getAutoCursorPos(long curPos) { return bi->osuCoords2Pixels(m_vRawPos); }
