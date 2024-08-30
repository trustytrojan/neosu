#include "Spinner.h"

#include "AnimationHandler.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "Mouse.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SoundEngine.h"

using namespace std;

Spinner::Spinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, BeatmapInterface *beatmap)
    : HitObject(time, sampleType, -1, isEndOfCombo, -1, -1, beatmap) {
    m_vOriginalRawPos = Vector2(x, y);
    m_vRawPos = m_vOriginalRawPos;
    m_iObjectDuration = endTime - time;
    m_bClickedOnce = false;
    m_fRotationsNeeded = -1.0f;

    int minVel = 12;
    int maxVel = 48;
    int minTime = 2000;
    int maxTime = 5000;
    m_iMaxStoredDeltaAngles = clamp<int>(
        (int)((endTime - time - minTime) * (maxVel - minVel) / (maxTime - minTime) + minVel), minVel, maxVel);
    m_storedDeltaAngles = new float[m_iMaxStoredDeltaAngles];
    for(int i = 0; i < m_iMaxStoredDeltaAngles; i++) {
        m_storedDeltaAngles[i] = 0.0f;
    }
    m_iDeltaAngleIndex = 0;

    m_fPercent = 0.0f;

    m_fDrawRot = 0.0f;
    m_fRotations = 0.0f;
    m_fDeltaOverflow = 0.0f;
    m_fSumDeltaAngle = 0.0f;
    m_fDeltaAngleOverflow = 0.0f;
    m_fRPM = 0.0f;
    m_fLastMouseAngle = 0.0f;
    m_fRatio = 0.0f;

    // spinners don't need misaims
    m_bMisAim = true;

    // spinners don't use AR-dependent fadein, instead they always fade in with hardcoded 400 ms (see
    // GameRules::getFadeInTime())
    m_bUseFadeInTimeAsApproachTime = !cv_spinner_use_ar_fadein.getBool();
}

Spinner::~Spinner() {
    onReset(0);

    delete[] m_storedDeltaAngles;
    m_storedDeltaAngles = NULL;
}

void Spinner::draw(Graphics *g) {
    HitObject::draw(g);
    const float fadeOutMultiplier = cv_spinner_fade_out_time_multiplier.getFloat();
    const long fadeOutTimeMS = (long)(GameRules::getFadeOutTime(bm) * 1000.0f * fadeOutMultiplier);
    const long deltaEnd = m_iDelta + m_iObjectDuration;
    if((m_bFinished || !m_bVisible) && (deltaEnd > 0 || (deltaEnd < -fadeOutTimeMS))) return;

    Skin *skin = bm->getSkin();
    Vector2 center = bm->osuCoords2Pixels(m_vRawPos);

    const float alphaMultiplier =
        clamp<float>((deltaEnd < 0 ? 1.0f - ((float)std::abs(deltaEnd) / (float)fadeOutTimeMS) : 1.0f), 0.0f,
                     1.0f);  // only used for fade out anim atm

    const float globalScale = 1.0f;        // adjustments
    const float globalBaseSkinSize = 667;  // the width of spinner-bottom.png in the default skin
    const float globalBaseSize = bm->getPlayfieldSize().y;

    const float clampedRatio = clamp<float>(m_fRatio, 0.0f, 1.0f);
    float finishScaleRatio = clampedRatio;
    finishScaleRatio = -finishScaleRatio * (finishScaleRatio - 2);
    const float finishScale =
        0.80f +
        finishScaleRatio *
            0.20f;  // the spinner grows until reaching 100% during spinning, depending on how many spins are left

    if(skin->getSpinnerBackground() != skin->getMissingTexture() || skin->getVersion() < 2.0f)  // old style
    {
        // draw spinner circle
        if(skin->getSpinnerCircle() != skin->getMissingTexture()) {
            const float spinnerCircleScale =
                globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerCircle2x() ? 2.0f : 1.0f));

            g->setColor(0xffffffff);
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(m_fDrawRot);
                g->scale(spinnerCircleScale * globalScale, spinnerCircleScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerCircle());
            }
            g->popTransform();
        }

        // draw approach circle
        if(!(bi->getModsLegacy() & ModFlags::Hidden) && m_fPercent > 0.0f) {
            const float spinnerApproachCircleImageScale =
                globalBaseSize / ((globalBaseSkinSize / 2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

            g->setColor(skin->getSpinnerApproachCircleColor());
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->scale(spinnerApproachCircleImageScale * m_fPercent * globalScale,
                         spinnerApproachCircleImageScale * m_fPercent * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerApproachCircle());
            }
            g->popTransform();
        }
    } else  // new style
    {
        // bottom
        if(skin->getSpinnerBottom() != skin->getMissingTexture()) {
            const float spinnerBottomImageScale =
                globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerBottom2x() ? 2.0f : 1.0f));

            g->setColor(0xffffffff);
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(m_fDrawRot / 7.0f);
                g->scale(spinnerBottomImageScale * finishScale * globalScale,
                         spinnerBottomImageScale * finishScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerBottom());
            }
            g->popTransform();
        }

        // top
        if(skin->getSpinnerTop() != skin->getMissingTexture()) {
            const float spinnerTopImageScale =
                globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerTop2x() ? 2.0f : 1.0f));

            g->setColor(0xffffffff);
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(m_fDrawRot / 2.0f);
                g->scale(spinnerTopImageScale * finishScale * globalScale,
                         spinnerTopImageScale * finishScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerTop());
            }
            g->popTransform();
        }

        // middle
        if(skin->getSpinnerMiddle2() != skin->getMissingTexture()) {
            const float spinnerMiddle2ImageScale =
                globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerMiddle22x() ? 2.0f : 1.0f));

            g->setColor(0xffffffff);
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(m_fDrawRot);
                g->scale(spinnerMiddle2ImageScale * finishScale * globalScale,
                         spinnerMiddle2ImageScale * finishScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerMiddle2());
            }
            g->popTransform();
        }
        if(skin->getSpinnerMiddle() != skin->getMissingTexture()) {
            const float spinnerMiddleImageScale =
                globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerMiddle2x() ? 2.0f : 1.0f));

            g->setColor(COLOR(255, 255, (int)(255 * m_fPercent), (int)(255 * m_fPercent)));
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(m_fDrawRot / 2.0f);  // apparently does not rotate in osu
                g->scale(spinnerMiddleImageScale * finishScale * globalScale,
                         spinnerMiddleImageScale * finishScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerMiddle());
            }
            g->popTransform();
        }

        // approach circle
        if(!(bi->getModsLegacy() & ModFlags::Hidden) && m_fPercent > 0.0f) {
            const float spinnerApproachCircleImageScale =
                globalBaseSize / ((globalBaseSkinSize / 2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

            g->setColor(skin->getSpinnerApproachCircleColor());
            g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->scale(spinnerApproachCircleImageScale * m_fPercent * globalScale,
                         spinnerApproachCircleImageScale * m_fPercent * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerApproachCircle());
            }
            g->popTransform();
        }
    }

    // "CLEAR!"
    if(m_fRatio >= 1.0f) {
        const float spinnerClearImageScale = Osu::getImageScale(skin->getSpinnerClear(), 80);

        g->setColor(0xffffffff);
        g->setAlpha(alphaMultiplier);
        g->pushTransform();
        {
            g->scale(spinnerClearImageScale, spinnerClearImageScale);
            g->translate(center.x, center.y - bm->getPlayfieldSize().y * 0.25f);
            g->drawImage(skin->getSpinnerClear());
        }
        g->popTransform();
    }

    // "SPIN!"
    if(clampedRatio < 0.03f) {
        const float spinerSpinImageScale = Osu::getImageScale(skin->getSpinnerSpin(), 80);

        g->setColor(0xffffffff);
        g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
        g->pushTransform();
        {
            g->scale(spinerSpinImageScale, spinerSpinImageScale);
            g->translate(center.x, center.y + bm->getPlayfieldSize().y * 0.30f);
            g->drawImage(skin->getSpinnerSpin());
        }
        g->popTransform();
    }

    // draw RPM
    if(m_iDelta < 0) {
        McFont *rpmFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
        const float stringWidth = rpmFont->getStringWidth("RPM: 477");
        g->setColor(0xffffffff);
        g->setAlpha(m_fAlphaWithoutHidden * m_fAlphaWithoutHidden * m_fAlphaWithoutHidden * alphaMultiplier);
        g->pushTransform();
        {
            g->translate(
                (int)(osu->getScreenWidth() / 2 - stringWidth / 2),
                (int)(osu->getScreenHeight() - 5 + (5 + rpmFont->getHeight()) * (1.0f - m_fAlphaWithoutHidden)));
            g->drawString(rpmFont, UString::format("RPM: %i", (int)(m_fRPM + 0.4f)));
        }
        g->popTransform();
    }
}

void Spinner::update(long curPos) {
    HitObject::update(curPos);

    // stop spinner sound and don't update() while paused
    if(bi->isPaused() || !bi->isPlaying() || (bm && bm->hasFailed())) {
        if(bm != NULL) {
            bm->getSkin()->stopSpinnerSpinSound();
        }
        return;
    }

    // if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
    if(!m_bFinished) {
        // handle spinner ending
        if(curPos >= m_iTime + m_iObjectDuration) {
            onHit();
            return;
        }

        m_fRotationsNeeded = GameRules::getSpinnerRotationsForSpeedMultiplier(bi, m_iObjectDuration);

        const float fixedRate = engine->getFrameTime();

        const float DELTA_UPDATE_TIME = (fixedRate * 1000.0f);
        const float AUTO_MULTIPLIER = (1.0f / 20.0f);

        // scale percent calculation
        long delta = (long)m_iTime - (long)curPos;
        m_fPercent = 1.0f - clamp<float>((float)delta / -(float)(m_iObjectDuration), 0.0f, 1.0f);

        // handle auto, mouse spinning movement
        float angleDiff = 0;
        if((bi->getModsLegacy() & ModFlags::Autoplay) || (bi->getModsLegacy() & ModFlags::Autopilot) ||
           (bi->getModsLegacy() & ModFlags::SpunOut)) {
            angleDiff = engine->getFrameTime() * 1000.0f * AUTO_MULTIPLIER * bi->getSpeedMultiplier();
        } else {  // user spin
            Vector2 mouseDelta = bi->getCursorPos() - bi->osuCoords2Pixels(m_vRawPos);
            const float currentMouseAngle = (float)std::atan2(mouseDelta.y, mouseDelta.x);
            angleDiff = (currentMouseAngle - m_fLastMouseAngle);

            if(std::abs(angleDiff) > 0.001f)
                m_fLastMouseAngle = currentMouseAngle;
            else
                angleDiff = 0;
        }

        // handle spinning
        // HACKHACK: rewrite this
        if(delta <= 0) {
            bool isSpinning = bi->isClickHeld() || (bi->getModsLegacy() & ModFlags::Autoplay) ||
                              (bi->getModsLegacy() & ModFlags::Relax) || (bi->getModsLegacy() & ModFlags::SpunOut);

            m_fDeltaOverflow += engine->getFrameTime() * 1000.0f;

            if(angleDiff < -PI)
                angleDiff += 2 * PI;
            else if(angleDiff > PI)
                angleDiff -= 2 * PI;

            if(isSpinning) m_fDeltaAngleOverflow += angleDiff;

            while(m_fDeltaOverflow >= DELTA_UPDATE_TIME) {
                // spin caused by the cursor
                float deltaAngle = 0;
                if(isSpinning) {
                    deltaAngle = m_fDeltaAngleOverflow * DELTA_UPDATE_TIME / m_fDeltaOverflow;
                    m_fDeltaAngleOverflow -= deltaAngle;
                    // deltaAngle = clamp<float>(deltaAngle, -MAX_ANG_DIFF, MAX_ANG_DIFF);
                }

                m_fDeltaOverflow -= DELTA_UPDATE_TIME;

                m_fSumDeltaAngle -= m_storedDeltaAngles[m_iDeltaAngleIndex];
                m_fSumDeltaAngle += deltaAngle;
                m_storedDeltaAngles[m_iDeltaAngleIndex++] = deltaAngle;
                m_iDeltaAngleIndex %= m_iMaxStoredDeltaAngles;

                float rotationAngle = m_fSumDeltaAngle / m_iMaxStoredDeltaAngles;
                float rotationPerSec = rotationAngle * (1000.0f / DELTA_UPDATE_TIME) / (2.0f * PI);

                const float decay = pow(0.01f, (float)engine->getFrameTime());
                m_fRPM = m_fRPM * decay + (1.0 - decay) * std::abs(rotationPerSec) * 60;
                m_fRPM = min(m_fRPM, 477.0f);

                if(std::abs(rotationAngle) > 0.0001f) rotate(rotationAngle);
            }

            m_fRatio = m_fRotations / (m_fRotationsNeeded * 360.0f);
        }
    }
}

void Spinner::onClickEvent(std::vector<Click> &clicks) {
    if(m_bFinished) return;

    // needed for nightmare mod
    if(m_bVisible && !m_bClickedOnce) {
        clicks.erase(clicks.begin());
        m_bClickedOnce = true;
    }
}

void Spinner::onReset(long curPos) {
    HitObject::onReset(curPos);

    if(bm != NULL) {
        bm->getSkin()->stopSpinnerSpinSound();
    }

    m_bClickedOnce = false;

    m_fRPM = 0.0f;
    m_fDrawRot = 0.0f;
    m_fRotations = 0.0f;
    m_fDeltaOverflow = 0.0f;
    m_fSumDeltaAngle = 0.0f;
    m_iDeltaAngleIndex = 0;
    m_fDeltaAngleOverflow = 0.0f;
    m_fRatio = 0.0f;

    // spinners don't need misaims
    m_bMisAim = true;

    for(int i = 0; i < m_iMaxStoredDeltaAngles; i++) {
        m_storedDeltaAngles[i] = 0.0f;
    }

    if(curPos > m_iTime + m_iObjectDuration)
        m_bFinished = true;
    else
        m_bFinished = false;
}

void Spinner::onHit() {
    // calculate hit result
    LiveScore::HIT result = LiveScore::HIT::HIT_NULL;
    if(m_fRatio >= 1.0f || (bi->getModsLegacy() & ModFlags::Autoplay))
        result = LiveScore::HIT::HIT_300;
    else if(m_fRatio >= 0.9f && !cv_mod_ming3012.getBool() && !cv_mod_no100s.getBool())
        result = LiveScore::HIT::HIT_100;
    else if(m_fRatio >= 0.75f && !cv_mod_no100s.getBool() && !cv_mod_no50s.getBool())
        result = LiveScore::HIT::HIT_50;
    else
        result = LiveScore::HIT::HIT_MISS;

    // sound
    if(bm != NULL && result != LiveScore::HIT::HIT_MISS) {
        if(cv_timingpoints_force.getBool()) bm->updateTimingPoints(m_iTime + m_iObjectDuration);

        const Vector2 osuCoords = bm->pixels2OsuCoords(bm->osuCoords2Pixels(m_vRawPos));

        bm->getSkin()->playHitCircleSound(m_iSampleType, GameRules::osuCoords2Pan(osuCoords.x), 0);
    }

    // add it, and we are finished
    addHitResult(result, 0, m_bIsEndOfCombo, m_vRawPos, -1.0f);
    m_bFinished = true;

    if(bm != NULL) {
        bm->getSkin()->stopSpinnerSpinSound();
    }
}

void Spinner::rotate(float rad) {
    m_fDrawRot += rad2deg(rad);

    rad = std::abs(rad);
    const float newRotations = m_fRotations + rad2deg(rad);

    // added one whole rotation
    if(std::floor(newRotations / 360.0f) > m_fRotations / 360.0f) {
        if((int)(newRotations / 360.0f) > (int)(m_fRotationsNeeded) + 1) {
            // extra rotations and bonus sound
            if(bm != NULL) {
                bm->getSkin()->playSpinnerBonusSound();
            }
            bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERBONUS, 0, false, true, true, true, true,
                             false);  // only increase health
            bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERBONUS, 0, false, true, true, true, true,
                             false);  // HACKHACK: compensating for rotation logic differences
            bi->addScorePoints(1100, true);
        } else {
            // normal whole rotation
            bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERSPIN, 0, false, true, true, true, true,
                             false);  // only increase health
            bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERSPIN, 0, false, true, true, true, true,
                             false);  // HACKHACK: compensating for rotation logic differences
            bi->addScorePoints(100, true);
        }
    }

    // spinner sound
    if(bm != NULL) {
        bm->getSkin()->playSpinnerSpinSound();

        const float frequency = 20000.0f + (int)(clamp<float>(m_fRatio, 0.0f, 2.5f) * 40000.0f);
        bm->getSkin()->getSpinnerSpinSound()->setFrequency(frequency);
    }

    m_fRotations = newRotations;
}

Vector2 Spinner::getAutoCursorPos(long curPos) {
    // calculate point
    long delta = 0;
    if(curPos <= m_iTime)
        delta = 0;
    else if(curPos >= m_iTime + m_iObjectDuration)
        delta = m_iObjectDuration;
    else
        delta = curPos - m_iTime;

    Vector2 actualPos = bi->osuCoords2Pixels(m_vRawPos);
    const float AUTO_MULTIPLIER = (1.0f / 20.0f);
    float multiplier = ((bi->getModsLegacy() & ModFlags::Autoplay) || (bi->getModsLegacy() & ModFlags::Autopilot))
                           ? AUTO_MULTIPLIER
                           : 1.0f;
    float angle = (delta * multiplier) - PI / 2.0f;
    float r = GameRules::getPlayfieldSize().y / 10.0f;  // XXX: slow?
    return Vector2((float)(actualPos.x + r * std::cos(angle)), (float)(actualPos.y + r * std::sin(angle)));
}
