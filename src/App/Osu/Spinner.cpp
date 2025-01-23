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
    this->type = HitObjectType::SPINNER;

    this->vOriginalRawPos = Vector2(x, y);
    this->vRawPos = this->vOriginalRawPos;
    this->duration = endTime - time;
    this->bClickedOnce = false;
    this->fRotationsNeeded = -1.0f;

    int minVel = 12;
    int maxVel = 48;
    int minTime = 2000;
    int maxTime = 5000;
    this->iMaxStoredDeltaAngles = clamp<int>(
        (int)((endTime - time - minTime) * (maxVel - minVel) / (maxTime - minTime) + minVel), minVel, maxVel);
    this->storedDeltaAngles = new float[this->iMaxStoredDeltaAngles];
    for(int i = 0; i < this->iMaxStoredDeltaAngles; i++) {
        this->storedDeltaAngles[i] = 0.0f;
    }
    this->iDeltaAngleIndex = 0;

    this->fPercent = 0.0f;

    this->fDrawRot = 0.0f;
    this->fRotations = 0.0f;
    this->fDeltaOverflow = 0.0f;
    this->fSumDeltaAngle = 0.0f;
    this->fDeltaAngleOverflow = 0.0f;
    this->fRPM = 0.0f;
    this->fLastMouseAngle = 0.0f;
    this->fRatio = 0.0f;

    // spinners don't need misaims
    this->bMisAim = true;

    // spinners don't use AR-dependent fadein, instead they always fade in with hardcoded 400 ms (see
    // GameRules::getFadeInTime())
    this->bUseFadeInTimeAsApproachTime = !cv_spinner_use_ar_fadein.getBool();
}

Spinner::~Spinner() {
    this->onReset(0);

    delete[] this->storedDeltaAngles;
    this->storedDeltaAngles = NULL;
}

void Spinner::draw(Graphics *g) {
    HitObject::draw(g);
    const float fadeOutMultiplier = cv_spinner_fade_out_time_multiplier.getFloat();
    const long fadeOutTimeMS = (long)(GameRules::getFadeOutTime(this->bm) * 1000.0f * fadeOutMultiplier);
    const long deltaEnd = this->iDelta + this->duration;
    if((this->bFinished || !this->bVisible) && (deltaEnd > 0 || (deltaEnd < -fadeOutTimeMS))) return;

    Skin *skin = this->bm->getSkin();
    Vector2 center = this->bm->osuCoords2Pixels(this->vRawPos);

    const float alphaMultiplier =
        clamp<float>((deltaEnd < 0 ? 1.0f - ((float)std::abs(deltaEnd) / (float)fadeOutTimeMS) : 1.0f), 0.0f,
                     1.0f);  // only used for fade out anim atm

    const float globalScale = 1.0f;        // adjustments
    const float globalBaseSkinSize = 667;  // the width of spinner-bottom.png in the default skin
    const float globalBaseSize = this->bm->getPlayfieldSize().y;

    const float clampedRatio = clamp<float>(this->fRatio, 0.0f, 1.0f);
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
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(this->fDrawRot);
                g->scale(spinnerCircleScale * globalScale, spinnerCircleScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerCircle());
            }
            g->popTransform();
        }

        // draw approach circle
        if(!(this->bi->getModsLegacy() & LegacyFlags::Hidden) && this->fPercent > 0.0f) {
            const float spinnerApproachCircleImageScale =
                globalBaseSize / ((globalBaseSkinSize / 2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

            g->setColor(skin->getSpinnerApproachCircleColor());
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->scale(spinnerApproachCircleImageScale * this->fPercent * globalScale,
                         spinnerApproachCircleImageScale * this->fPercent * globalScale);
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
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(this->fDrawRot / 7.0f);
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
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(this->fDrawRot / 2.0f);
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
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(this->fDrawRot);
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

            g->setColor(COLOR(255, 255, (int)(255 * this->fPercent), (int)(255 * this->fPercent)));
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->rotate(this->fDrawRot / 2.0f);  // apparently does not rotate in osu
                g->scale(spinnerMiddleImageScale * finishScale * globalScale,
                         spinnerMiddleImageScale * finishScale * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerMiddle());
            }
            g->popTransform();
        }

        // approach circle
        if(!(this->bi->getModsLegacy() & LegacyFlags::Hidden) && this->fPercent > 0.0f) {
            const float spinnerApproachCircleImageScale =
                globalBaseSize / ((globalBaseSkinSize / 2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

            g->setColor(skin->getSpinnerApproachCircleColor());
            g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
            g->pushTransform();
            {
                g->scale(spinnerApproachCircleImageScale * this->fPercent * globalScale,
                         spinnerApproachCircleImageScale * this->fPercent * globalScale);
                g->translate(center.x, center.y);
                g->drawImage(skin->getSpinnerApproachCircle());
            }
            g->popTransform();
        }
    }

    // "CLEAR!"
    if(this->fRatio >= 1.0f) {
        const float spinnerClearImageScale = Osu::getImageScale(skin->getSpinnerClear(), 80);

        g->setColor(0xffffffff);
        g->setAlpha(alphaMultiplier);
        g->pushTransform();
        {
            g->scale(spinnerClearImageScale, spinnerClearImageScale);
            g->translate(center.x, center.y - this->bm->getPlayfieldSize().y * 0.25f);
            g->drawImage(skin->getSpinnerClear());
        }
        g->popTransform();
    }

    // "SPIN!"
    if(clampedRatio < 0.03f) {
        const float spinerSpinImageScale = Osu::getImageScale(skin->getSpinnerSpin(), 80);

        g->setColor(0xffffffff);
        g->setAlpha(this->fAlphaWithoutHidden * alphaMultiplier);
        g->pushTransform();
        {
            g->scale(spinerSpinImageScale, spinerSpinImageScale);
            g->translate(center.x, center.y + this->bm->getPlayfieldSize().y * 0.30f);
            g->drawImage(skin->getSpinnerSpin());
        }
        g->popTransform();
    }

    // draw RPM
    if(this->iDelta < 0) {
        McFont *rpmFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
        const float stringWidth = rpmFont->getStringWidth("RPM: 477");
        g->setColor(0xffffffff);
        g->setAlpha(this->fAlphaWithoutHidden * this->fAlphaWithoutHidden * this->fAlphaWithoutHidden *
                    alphaMultiplier);
        g->pushTransform();
        {
            g->translate(
                (int)(osu->getScreenWidth() / 2 - stringWidth / 2),
                (int)(osu->getScreenHeight() - 5 + (5 + rpmFont->getHeight()) * (1.0f - this->fAlphaWithoutHidden)));
            g->drawString(rpmFont, UString::format("RPM: %i", (int)(this->fRPM + 0.4f)));
        }
        g->popTransform();
    }
}

void Spinner::update(long curPos, f64 frame_time) {
    HitObject::update(curPos, frame_time);

    // stop spinner sound and don't update() while paused
    if(this->bi->isPaused() || !this->bi->isPlaying() || (this->bm && this->bm->hasFailed())) {
        if(this->bm != NULL) {
            this->bm->getSkin()->stopSpinnerSpinSound();
        }
        return;
    }

    // if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
    if(!this->bFinished) {
        // handle spinner ending
        if(curPos >= this->click_time + this->duration) {
            this->onHit();
            return;
        }

        // Skip calculations
        if(frame_time == 0.0) {
            return;
        }

        this->fRotationsNeeded = GameRules::getSpinnerRotationsForSpeedMultiplier(this->bi, this->duration);

        const float DELTA_UPDATE_TIME = (frame_time * 1000.0f);
        const float AUTO_MULTIPLIER = (1.0f / 20.0f);

        // scale percent calculation
        long delta = (long)this->click_time - (long)curPos;
        this->fPercent = 1.0f - clamp<float>((float)delta / -(float)(this->duration), 0.0f, 1.0f);

        // handle auto, mouse spinning movement
        float angleDiff = 0;
        if((this->bi->getModsLegacy() & LegacyFlags::Autoplay) ||
           (this->bi->getModsLegacy() & LegacyFlags::Autopilot) || (this->bi->getModsLegacy() & LegacyFlags::SpunOut)) {
            angleDiff = frame_time * 1000.0f * AUTO_MULTIPLIER * this->bi->getSpeedMultiplier();
        } else {  // user spin
            Vector2 mouseDelta = this->bi->getCursorPos() - this->bi->osuCoords2Pixels(this->vRawPos);
            const float currentMouseAngle = (float)std::atan2(mouseDelta.y, mouseDelta.x);
            angleDiff = (currentMouseAngle - this->fLastMouseAngle);

            if(std::abs(angleDiff) > 0.001f)
                this->fLastMouseAngle = currentMouseAngle;
            else
                angleDiff = 0;
        }

        // handle spinning
        // HACKHACK: rewrite this
        if(delta <= 0) {
            bool isSpinning = this->bi->isClickHeld() || (this->bi->getModsLegacy() & LegacyFlags::Autoplay) ||
                              (this->bi->getModsLegacy() & LegacyFlags::Relax) ||
                              (this->bi->getModsLegacy() & LegacyFlags::SpunOut);

            this->fDeltaOverflow += frame_time * 1000.0f;

            if(angleDiff < -PI)
                angleDiff += 2 * PI;
            else if(angleDiff > PI)
                angleDiff -= 2 * PI;

            if(isSpinning) this->fDeltaAngleOverflow += angleDiff;

            while(this->fDeltaOverflow >= DELTA_UPDATE_TIME) {
                // spin caused by the cursor
                float deltaAngle = 0;
                if(isSpinning) {
                    deltaAngle = this->fDeltaAngleOverflow * DELTA_UPDATE_TIME / this->fDeltaOverflow;
                    this->fDeltaAngleOverflow -= deltaAngle;
                    // deltaAngle = clamp<float>(deltaAngle, -MAX_ANG_DIFF, MAX_ANG_DIFF);
                }

                this->fDeltaOverflow -= DELTA_UPDATE_TIME;

                this->fSumDeltaAngle -= this->storedDeltaAngles[this->iDeltaAngleIndex];
                this->fSumDeltaAngle += deltaAngle;
                this->storedDeltaAngles[this->iDeltaAngleIndex++] = deltaAngle;
                this->iDeltaAngleIndex %= this->iMaxStoredDeltaAngles;

                float rotationAngle = this->fSumDeltaAngle / this->iMaxStoredDeltaAngles;
                float rotationPerSec = rotationAngle * (1000.0f / DELTA_UPDATE_TIME) / (2.0f * PI);

                f32 decay = pow(0.01f, (f32)frame_time);
                this->fRPM = this->fRPM * decay + (1.0 - decay) * std::abs(rotationPerSec) * 60;
                this->fRPM = min(this->fRPM, 477.0f);

                if(std::abs(rotationAngle) > 0.0001f) this->rotate(rotationAngle);
            }

            this->fRatio = this->fRotations / (this->fRotationsNeeded * 360.0f);
        }
    }
}

void Spinner::onClickEvent(std::vector<Click> &clicks) {
    if(this->bFinished) return;

    // needed for nightmare mod
    if(this->bVisible && !this->bClickedOnce) {
        clicks.erase(clicks.begin());
        this->bClickedOnce = true;
    }
}

void Spinner::onReset(long curPos) {
    HitObject::onReset(curPos);

    if(this->bm != NULL) {
        this->bm->getSkin()->stopSpinnerSpinSound();
    }

    this->bClickedOnce = false;

    this->fRPM = 0.0f;
    this->fDrawRot = 0.0f;
    this->fRotations = 0.0f;
    this->fDeltaOverflow = 0.0f;
    this->fSumDeltaAngle = 0.0f;
    this->iDeltaAngleIndex = 0;
    this->fDeltaAngleOverflow = 0.0f;
    this->fRatio = 0.0f;

    // spinners don't need misaims
    this->bMisAim = true;

    for(int i = 0; i < this->iMaxStoredDeltaAngles; i++) {
        this->storedDeltaAngles[i] = 0.0f;
    }

    if(curPos > this->click_time + this->duration)
        this->bFinished = true;
    else
        this->bFinished = false;
}

void Spinner::onHit() {
    // calculate hit result
    LiveScore::HIT result = LiveScore::HIT::HIT_NULL;
    if(this->fRatio >= 1.0f || (this->bi->getModsLegacy() & LegacyFlags::Autoplay))
        result = LiveScore::HIT::HIT_300;
    else if(this->fRatio >= 0.9f && !cv_mod_ming3012.getBool() && !cv_mod_no100s.getBool())
        result = LiveScore::HIT::HIT_100;
    else if(this->fRatio >= 0.75f && !cv_mod_no100s.getBool() && !cv_mod_no50s.getBool())
        result = LiveScore::HIT::HIT_50;
    else
        result = LiveScore::HIT::HIT_MISS;

    // sound
    if(this->bm != NULL && result != LiveScore::HIT::HIT_MISS) {
        if(cv_timingpoints_force.getBool()) this->bm->updateTimingPoints(this->click_time + this->duration);

        const Vector2 osuCoords = this->bm->pixels2OsuCoords(this->bm->osuCoords2Pixels(this->vRawPos));

        this->bm->getSkin()->playHitCircleSound(this->iSampleType, GameRules::osuCoords2Pan(osuCoords.x), 0);
    }

    // add it, and we are finished
    this->addHitResult(result, 0, this->is_end_of_combo, this->vRawPos, -1.0f);
    this->bFinished = true;

    if(this->bm != NULL) {
        this->bm->getSkin()->stopSpinnerSpinSound();
    }
}

void Spinner::rotate(float rad) {
    this->fDrawRot += rad2deg(rad);

    rad = std::abs(rad);
    const float newRotations = this->fRotations + rad2deg(rad);

    // added one whole rotation
    if(std::floor(newRotations / 360.0f) > this->fRotations / 360.0f) {
        if((int)(newRotations / 360.0f) > (int)(this->fRotationsNeeded) + 1) {
            // extra rotations and bonus sound
            if(this->bm != NULL) {
                this->bm->getSkin()->playSpinnerBonusSound();
            }
            this->bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERBONUS, 0, false, true, true, true, true,
                                   false);  // only increase health
            this->bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERBONUS, 0, false, true, true, true, true,
                                   false);  // HACKHACK: compensating for rotation logic differences
            this->bi->addScorePoints(1100, true);
        } else {
            // normal whole rotation
            this->bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERSPIN, 0, false, true, true, true, true,
                                   false);  // only increase health
            this->bi->addHitResult(this, LiveScore::HIT::HIT_SPINNERSPIN, 0, false, true, true, true, true,
                                   false);  // HACKHACK: compensating for rotation logic differences
            this->bi->addScorePoints(100, true);
        }
    }

    // spinner sound
    if(this->bm != NULL) {
        this->bm->getSkin()->playSpinnerSpinSound();

        const float frequency = 20000.0f + (int)(clamp<float>(this->fRatio, 0.0f, 2.5f) * 40000.0f);
        this->bm->getSkin()->getSpinnerSpinSound()->setFrequency(frequency);
    }

    this->fRotations = newRotations;
}

Vector2 Spinner::getAutoCursorPos(long curPos) {
    // calculate point
    long delta = 0;
    if(curPos <= this->click_time)
        delta = 0;
    else if(curPos >= this->click_time + this->duration)
        delta = this->duration;
    else
        delta = curPos - this->click_time;

    Vector2 actualPos = this->bi->osuCoords2Pixels(this->vRawPos);
    const float AUTO_MULTIPLIER = (1.0f / 20.0f);
    float multiplier =
        ((this->bi->getModsLegacy() & LegacyFlags::Autoplay) || (this->bi->getModsLegacy() & LegacyFlags::Autopilot))
            ? AUTO_MULTIPLIER
            : 1.0f;
    float angle = (delta * multiplier) - PI / 2.0f;
    float r = GameRules::getPlayfieldSize().y / 10.0f;  // XXX: slow?
    return Vector2((float)(actualPos.x + r * std::cos(angle)), (float)(actualPos.y + r * std::sin(angle)));
}
