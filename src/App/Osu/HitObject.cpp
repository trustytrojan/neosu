#include "HitObject.h"

#include "AnimationHandler.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "HUD.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SkinImage.h"

using namespace std;

unsigned long long HitObject::sortHackCounter = 0;

void HitObject::drawHitResult(Graphics *g, Beatmap *beatmap, Vector2 rawPos, LiveScore::HIT result,
                              float animPercentInv, float hitDeltaRangePercent) {
    drawHitResult(g, beatmap->getSkin(), beatmap->m_fHitcircleDiameter, beatmap->m_fRawHitcircleDiameter, rawPos,
                  result, animPercentInv, hitDeltaRangePercent);
}

void HitObject::drawHitResult(Graphics *g, Skin *skin, float hitcircleDiameter, float rawHitcircleDiameter,
                              Vector2 rawPos, LiveScore::HIT result, float animPercentInv, float hitDeltaRangePercent) {
    if(animPercentInv <= 0.0f) return;

    const float animPercent = 1.0f - animPercentInv;

    const float fadeInEndPercent = cv_hitresult_fadein_duration.getFloat() / cv_hitresult_duration.getFloat();

    // determine color/transparency
    {
        if(!cv_hitresult_delta_colorize.getBool() || result == LiveScore::HIT::HIT_MISS)
            g->setColor(0xffffffff);
        else {
            // NOTE: hitDeltaRangePercent is within -1.0f to 1.0f
            // -1.0f means early miss
            // 1.0f means late miss
            // -0.999999999f means early 50
            // 0.999999999f means late 50
            // percentage scale is linear with respect to the entire hittable 50s range in both directions (contrary to
            // OD brackets which are nonlinear of course)
            if(hitDeltaRangePercent != 0.0f) {
                hitDeltaRangePercent =
                    clamp<float>(hitDeltaRangePercent * cv_hitresult_delta_colorize_multiplier.getFloat(), -1.0f, 1.0f);

                const float rf = lerp3f(cv_hitresult_delta_colorize_early_r.getFloat() / 255.0f, 1.0f,
                                        cv_hitresult_delta_colorize_late_r.getFloat() / 255.0f,
                                        cv_hitresult_delta_colorize_interpolate.getBool()
                                            ? hitDeltaRangePercent / 2.0f + 0.5f
                                            : (hitDeltaRangePercent < 0.0f ? -1.0f : 1.0f));
                const float gf = lerp3f(cv_hitresult_delta_colorize_early_g.getFloat() / 255.0f, 1.0f,
                                        cv_hitresult_delta_colorize_late_g.getFloat() / 255.0f,
                                        cv_hitresult_delta_colorize_interpolate.getBool()
                                            ? hitDeltaRangePercent / 2.0f + 0.5f
                                            : (hitDeltaRangePercent < 0.0f ? -1.0f : 1.0f));
                const float bf = lerp3f(cv_hitresult_delta_colorize_early_b.getFloat() / 255.0f, 1.0f,
                                        cv_hitresult_delta_colorize_late_b.getFloat() / 255.0f,
                                        cv_hitresult_delta_colorize_interpolate.getBool()
                                            ? hitDeltaRangePercent / 2.0f + 0.5f
                                            : (hitDeltaRangePercent < 0.0f ? -1.0f : 1.0f));

                g->setColor(COLORf(1.0f, rf, gf, bf));
            }
        }

        const float fadeOutStartPercent = cv_hitresult_fadeout_start_time.getFloat() / cv_hitresult_duration.getFloat();
        const float fadeOutDurationPercent =
            cv_hitresult_fadeout_duration.getFloat() / cv_hitresult_duration.getFloat();

        g->setAlpha(clamp<float>(animPercent < fadeInEndPercent
                                     ? animPercent / fadeInEndPercent
                                     : 1.0f - ((animPercent - fadeOutStartPercent) / fadeOutDurationPercent),
                                 0.0f, 1.0f));
    }

    g->pushTransform();
    {
        const float osuCoordScaleMultiplier = hitcircleDiameter / rawHitcircleDiameter;

        bool doScaleOrRotateAnim = true;
        bool hasParticle = true;
        float hitImageScale = 1.0f;
        switch(result) {
            case LiveScore::HIT::HIT_MISS:
                doScaleOrRotateAnim = skin->getHit0()->getNumImages() == 1;
                hitImageScale = (rawHitcircleDiameter / skin->getHit0()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_50:
                doScaleOrRotateAnim = skin->getHit50()->getNumImages() == 1;
                hasParticle = skin->getParticle50() != skin->getMissingTexture();
                hitImageScale = (rawHitcircleDiameter / skin->getHit50()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_100:
                doScaleOrRotateAnim = skin->getHit100()->getNumImages() == 1;
                hasParticle = skin->getParticle100() != skin->getMissingTexture();
                hitImageScale =
                    (rawHitcircleDiameter / skin->getHit100()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_300:
                doScaleOrRotateAnim = skin->getHit300()->getNumImages() == 1;
                hasParticle = skin->getParticle300() != skin->getMissingTexture();
                hitImageScale =
                    (rawHitcircleDiameter / skin->getHit300()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_100K:
                doScaleOrRotateAnim = skin->getHit100k()->getNumImages() == 1;
                hasParticle = skin->getParticle100() != skin->getMissingTexture();
                hitImageScale =
                    (rawHitcircleDiameter / skin->getHit100k()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_300K:
                doScaleOrRotateAnim = skin->getHit300k()->getNumImages() == 1;
                hasParticle = skin->getParticle300() != skin->getMissingTexture();
                hitImageScale =
                    (rawHitcircleDiameter / skin->getHit300k()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            case LiveScore::HIT::HIT_300G:
                doScaleOrRotateAnim = skin->getHit300g()->getNumImages() == 1;
                hasParticle = skin->getParticle300() != skin->getMissingTexture();
                hitImageScale =
                    (rawHitcircleDiameter / skin->getHit300g()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
                break;

            default:
                break;
        }

        // non-misses have a special scale animation (the type of which depends on hasParticle)
        float scale = 1.0f;
        if(doScaleOrRotateAnim && cv_hitresult_animated.getBool()) {
            if(!hasParticle) {
                if(animPercent < fadeInEndPercent * 0.8f)
                    scale = lerp<float>(0.6f, 1.1f, clamp<float>(animPercent / (fadeInEndPercent * 0.8f), 0.0f, 1.0f));
                else if(animPercent < fadeInEndPercent * 1.2f)
                    scale = lerp<float>(1.1f, 0.9f,
                                        clamp<float>((animPercent - fadeInEndPercent * 0.8f) /
                                                         (fadeInEndPercent * 1.2f - fadeInEndPercent * 0.8f),
                                                     0.0f, 1.0f));
                else if(animPercent < fadeInEndPercent * 1.4f)
                    scale = lerp<float>(0.9f, 1.0f,
                                        clamp<float>((animPercent - fadeInEndPercent * 1.2f) /
                                                         (fadeInEndPercent * 1.4f - fadeInEndPercent * 1.2f),
                                                     0.0f, 1.0f));
            } else
                scale = lerp<float>(0.9f, 1.05f, clamp<float>(animPercent, 0.0f, 1.0f));

            // TODO: osu draws an additive copy of the hitresult on top (?) with 0.5 alpha anim and negative timing, if
            // the skin hasParticle. in this case only the copy does the wobble anim, while the main result just scales
        }

        switch(result) {
            case LiveScore::HIT::HIT_MISS: {
                // special case: animated misses don't move down, and skins with version <= 1 also don't move down
                Vector2 downAnim;
                if(skin->getHit0()->getNumImages() < 2 && skin->getVersion() > 1.0f)
                    downAnim.y =
                        lerp<float>(-5.0f, 40.0f, clamp<float>(animPercent * animPercent * animPercent, 0.0f, 1.0f)) *
                        osuCoordScaleMultiplier;

                float missScale = 1.0f + clamp<float>((1.0f - (animPercent / fadeInEndPercent)), 0.0f, 1.0f) *
                                             (cv_hitresult_miss_fadein_scale.getFloat() - 1.0f);
                if(!cv_hitresult_animated.getBool()) missScale = 1.0f;

                // TODO: rotation anim (only for all non-animated skins), rot = rng(-0.15f, 0.15f), anim1 = 120 ms to
                // rot, anim2 = rest to rot*2, all ease in

                skin->getHit0()->drawRaw(
                    g, rawPos + downAnim,
                    (doScaleOrRotateAnim ? missScale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
            } break;

            case LiveScore::HIT::HIT_50:
                skin->getHit50()->drawRaw(
                    g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                break;

            case LiveScore::HIT::HIT_100:
                skin->getHit100()->drawRaw(
                    g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                break;

            case LiveScore::HIT::HIT_300:
                if(cv_hitresult_draw_300s.getBool()) {
                    skin->getHit300()->drawRaw(
                        g, rawPos,
                        (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                }
                break;

            case LiveScore::HIT::HIT_100K:
                skin->getHit100k()->drawRaw(
                    g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                break;

            case LiveScore::HIT::HIT_300K:
                if(cv_hitresult_draw_300s.getBool()) {
                    skin->getHit300k()->drawRaw(
                        g, rawPos,
                        (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                }
                break;

            case LiveScore::HIT::HIT_300G:
                if(cv_hitresult_draw_300s.getBool()) {
                    skin->getHit300g()->drawRaw(
                        g, rawPos,
                        (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * cv_hitresult_scale.getFloat());
                }
                break;

            default:
                break;
        }
    }
    g->popTransform();
}

HitObject::HitObject(long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset,
                     BeatmapInterface *beatmap) {
    click_time = time;
    m_iSampleType = sampleType;
    m_iComboNumber = comboNumber;
    m_bIsEndOfCombo = isEndOfCombo;
    m_iColorCounter = colorCounter;
    m_iColorOffset = colorOffset;

    m_fAlpha = 0.0f;
    m_fAlphaWithoutHidden = 0.0f;
    m_fAlphaForApproachCircle = 0.0f;
    m_fApproachScale = 0.0f;
    m_fHittableDimRGBColorMultiplierPercent = 1.0f;
    m_iApproachTime = 0;
    m_iFadeInTime = 0;
    duration = 0;
    m_iDelta = 0;
    duration = 0;

    m_bVisible = false;
    m_bFinished = false;
    m_bBlocked = false;
    m_bMisAim = false;
    m_iAutopilotDelta = 0;
    m_bOverrideHDApproachCircle = false;
    m_bUseFadeInTimeAsApproachTime = false;

    m_iStack = 0;

    m_hitresultanim1.time = -9999.0f;
    m_hitresultanim2.time = -9999.0f;

    m_iSortHack = sortHackCounter++;

    bi = beatmap;
    bm = dynamic_cast<Beatmap *>(beatmap);  // should be NULL if SimulatedBeatmap
}

void HitObject::draw2(Graphics *g) {
    drawHitResultAnim(g, m_hitresultanim1);
    drawHitResultAnim(g, m_hitresultanim2);
}

void HitObject::drawHitResultAnim(Graphics *g, const HITRESULTANIM &hitresultanim) {
    if((hitresultanim.time - cv_hitresult_duration.getFloat()) <
           engine->getTime()  // NOTE: this is written like that on purpose, don't change it ("future" results can be
                              // scheduled with it, e.g. for slider end)
       && (hitresultanim.time + cv_hitresult_duration_max.getFloat() * (1.0f / osu->getAnimationSpeedMultiplier())) >
              engine->getTime()) {
        Skin *skin = bm->getSkin();
        {
            const long skinAnimationTimeStartOffset =
                click_time + (hitresultanim.addObjectDurationToSkinAnimationTimeStartOffset ? duration : 0) +
                hitresultanim.delta;

            skin->getHit0()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit0()->setAnimationFrameClampUp();
            skin->getHit50()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit50()->setAnimationFrameClampUp();
            skin->getHit100()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit100()->setAnimationFrameClampUp();
            skin->getHit100k()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit100k()->setAnimationFrameClampUp();
            skin->getHit300()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit300()->setAnimationFrameClampUp();
            skin->getHit300g()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit300g()->setAnimationFrameClampUp();
            skin->getHit300k()->setAnimationTimeOffset(skin->getAnimationSpeed(), skinAnimationTimeStartOffset);
            skin->getHit300k()->setAnimationFrameClampUp();

            const float animPercentInv =
                1.0f - (((engine->getTime() - hitresultanim.time) * osu->getAnimationSpeedMultiplier()) /
                        cv_hitresult_duration.getFloat());

            drawHitResult(g, bm, bm->osuCoords2Pixels(hitresultanim.rawPos), hitresultanim.result, animPercentInv,
                          clamp<float>((float)hitresultanim.delta / bi->getHitWindow50(), -1.0f, 1.0f));
        }
    }
}

void HitObject::update(long curPos, f64 frame_time) {
    m_fAlphaForApproachCircle = 0.0f;
    m_fHittableDimRGBColorMultiplierPercent = 1.0f;

    const auto mods = bi->getMods();

    double animationSpeedMultipler = mods.speed / osu->getAnimationSpeedMultiplier();
    m_iApproachTime = (m_bUseFadeInTimeAsApproachTime ? (GameRules::getFadeInTime() * animationSpeedMultipler)
                                                      : (long)bi->getApproachTime());
    m_iFadeInTime = GameRules::getFadeInTime() * animationSpeedMultipler;

    m_iDelta = click_time - curPos;

    // 1 ms fudge by using >=, shouldn't really be a problem
    if(curPos >= (click_time - m_iApproachTime) && curPos < (click_time + duration)) {
        // approach circle scale
        const float scale = clamp<float>((float)m_iDelta / (float)m_iApproachTime, 0.0f, 1.0f);
        m_fApproachScale = 1 + (scale * cv_approach_scale_multiplier.getFloat());
        if(cv_mod_approach_different.getBool()) {
            const float back_const = 1.70158;

            float time = 1.0f - scale;
            {
                switch(cv_mod_approach_different_style.getInt()) {
                    default:  // "Linear"
                        break;
                    case 1:  // "Gravity" / InBack
                        time = time * time * ((back_const + 1.0f) * time - back_const);
                        break;
                    case 2:  // "InOut1" / InOutCubic
                        if(time < 0.5f)
                            time = time * time * time * 4.0f;
                        else {
                            --time;
                            time = time * time * time * 4.0f + 1.0f;
                        }
                        break;
                    case 3:  // "InOut2" / InOutQuint
                        if(time < 0.5f)
                            time = time * time * time * time * time * 16.0f;
                        else {
                            --time;
                            time = time * time * time * time * time * 16.0f + 1.0f;
                        }
                        break;
                    case 4:  // "Accelerate1" / In
                        time = time * time;
                        break;
                    case 5:  // "Accelerate2" / InCubic
                        time = time * time * time;
                        break;
                    case 6:  // "Accelerate3" / InQuint
                        time = time * time * time * time * time;
                        break;
                    case 7:  // "Decelerate1" / Out
                        time = time * (2.0f - time);
                        break;
                    case 8:  // "Decelerate2" / OutCubic
                        --time;
                        time = time * time * time + 1.0f;
                        break;
                    case 9:  // "Decelerate3" / OutQuint
                        --time;
                        time = time * time * time * time * time + 1.0f;
                        break;
                }
                // NOTE: some of the easing functions will overflow/underflow, don't clamp and instead allow it on
                // purpose
            }
            m_fApproachScale = 1 + lerp<float>(cv_mod_approach_different_initial_size.getFloat() - 1.0f, 0.0f, time);
        }

        // hitobject body fadein
        const long fadeInStart = click_time - m_iApproachTime;
        const long fadeInEnd =
            min(click_time,
                click_time - m_iApproachTime + m_iFadeInTime);  // min() ensures that the fade always finishes at click_time
                                                             // (even if the fadeintime is longer than the approachtime)
        m_fAlpha = clamp<float>(1.0f - ((float)(fadeInEnd - curPos) / (float)(fadeInEnd - fadeInStart)), 0.0f, 1.0f);
        m_fAlphaWithoutHidden = m_fAlpha;

        if(mods.flags & Replay::ModFlags::Hidden) {
            // hidden hitobject body fadein
            const float fin_start_percent = cv_mod_hd_circle_fadein_start_percent.getFloat();
            const float fin_end_percent = cv_mod_hd_circle_fadein_end_percent.getFloat();
            const float fout_start_percent = cv_mod_hd_circle_fadeout_start_percent.getFloat();
            const float fout_end_percent = cv_mod_hd_circle_fadeout_end_percent.getFloat();
            const long hiddenFadeInStart = click_time - (long)(m_iApproachTime * fin_start_percent);
            const long hiddenFadeInEnd = click_time - (long)(m_iApproachTime * fin_end_percent);
            m_fAlpha = clamp<float>(
                1.0f - ((float)(hiddenFadeInEnd - curPos) / (float)(hiddenFadeInEnd - hiddenFadeInStart)), 0.0f, 1.0f);

            // hidden hitobject body fadeout
            const long hiddenFadeOutStart = click_time - (long)(m_iApproachTime * fout_start_percent);
            const long hiddenFadeOutEnd = click_time - (long)(m_iApproachTime * fout_end_percent);
            if(curPos >= hiddenFadeOutStart)
                m_fAlpha = clamp<float>(
                    ((float)(hiddenFadeOutEnd - curPos) / (float)(hiddenFadeOutEnd - hiddenFadeOutStart)), 0.0f, 1.0f);
        }

        // approach circle fadein (doubled fadeintime)
        const long approachCircleFadeStart = click_time - m_iApproachTime;
        const long approachCircleFadeEnd =
            min(click_time, click_time - m_iApproachTime +
                             2 * m_iFadeInTime);  // min() ensures that the fade always finishes at click_time (even
                                                  // if the fadeintime is longer than the approachtime)
        m_fAlphaForApproachCircle = clamp<float>(
            1.0f - ((float)(approachCircleFadeEnd - curPos) / (float)(approachCircleFadeEnd - approachCircleFadeStart)),
            0.0f, 1.0f);

        // hittable dim, see https://github.com/ppy/osu/pull/20572
        if(cv_hitobject_hittable_dim.getBool() &&
           (!(bi->getMods().flags & Replay::ModFlags::Mafham) || !cv_mod_mafham_ignore_hittable_dim.getBool())) {
            const long hittableDimFadeStart = click_time - (long)GameRules::getHitWindowMiss();

            // yes, this means the un-dim animation cuts into the already clickable range
            const long hittableDimFadeEnd = hittableDimFadeStart + (long)cv_hitobject_hittable_dim_duration.getInt();

            m_fHittableDimRGBColorMultiplierPercent =
                lerp<float>(cv_hitobject_hittable_dim_start_percent.getFloat(), 1.0f,
                            clamp<float>(1.0f - (float)(hittableDimFadeEnd - curPos) /
                                                    (float)(hittableDimFadeEnd - hittableDimFadeStart),
                                         0.0f, 1.0f));
        }

        m_bVisible = true;
    } else {
        m_fApproachScale = 1.0f;
        m_bVisible = false;
    }
}

void HitObject::addHitResult(LiveScore::HIT result, long delta, bool isEndOfCombo, Vector2 posRaw, float targetDelta,
                             float targetAngle, bool ignoreOnHitErrorBar, bool ignoreCombo, bool ignoreHealth,
                             bool addObjectDurationToSkinAnimationTimeStartOffset) {
    if(bm != NULL && osu->getModTarget() && result != LiveScore::HIT::HIT_MISS && targetDelta >= 0.0f) {
        const float p300 = cv_mod_target_300_percent.getFloat();
        const float p100 = cv_mod_target_100_percent.getFloat();
        const float p50 = cv_mod_target_50_percent.getFloat();

        if(targetDelta < p300 && (result == LiveScore::HIT::HIT_300 || result == LiveScore::HIT::HIT_100))
            result = LiveScore::HIT::HIT_300;
        else if(targetDelta < p100)
            result = LiveScore::HIT::HIT_100;
        else if(targetDelta < p50)
            result = LiveScore::HIT::HIT_50;
        else
            result = LiveScore::HIT::HIT_MISS;

        osu->getHUD()->addTarget(targetDelta, targetAngle);
    }

    const LiveScore::HIT returnedHit = bi->addHitResult(this, result, delta, isEndOfCombo, ignoreOnHitErrorBar, false,
                                                        ignoreCombo, false, ignoreHealth);
    if(bm == NULL) return;

    HITRESULTANIM hitresultanim;
    {
        hitresultanim.result = (returnedHit != LiveScore::HIT::HIT_MISS ? returnedHit : result);
        hitresultanim.rawPos = posRaw;
        hitresultanim.delta = delta;
        hitresultanim.time = engine->getTime();
        hitresultanim.addObjectDurationToSkinAnimationTimeStartOffset = addObjectDurationToSkinAnimationTimeStartOffset;
    }

    // currently a maximum of 2 simultaneous results are supported (for drawing, per hitobject)
    if(engine->getTime() >
       m_hitresultanim1.time + cv_hitresult_duration_max.getFloat() * (1.0f / osu->getAnimationSpeedMultiplier()))
        m_hitresultanim1 = hitresultanim;
    else
        m_hitresultanim2 = hitresultanim;
}

void HitObject::onReset(long curPos) {
    m_bMisAim = false;
    m_iAutopilotDelta = 0;

    m_hitresultanim1.time = -9999.0f;
    m_hitresultanim2.time = -9999.0f;
}

float HitObject::lerp3f(float a, float b, float c, float percent) {
    if(percent <= 0.5f)
        return lerp<float>(a, b, percent * 2.0f);
    else
        return lerp<float>(b, c, (percent - 0.5f) * 2.0f);
}
