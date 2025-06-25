#include "AnimationHandler.h"

#include "ConVar.h"
#include "Engine.h"

using namespace std;

AnimationHandler *anim = NULL;

AnimationHandler::AnimationHandler() { anim = this; }

AnimationHandler::~AnimationHandler() {
    this->vAnimations.clear();

    anim = NULL;
}

void AnimationHandler::update() {
    for(size_t i = 0; i < this->vAnimations.size(); i++) {
        // start animation
        Animation &animation = this->vAnimations[i];
        if(engine->getTime() < animation.fStartTime)
            continue;
        else if(!animation.bStarted) {
            // after our delay, take the current value as startValue, then start animating to the target
            animation.fStartValue = *animation.fBase;
            animation.bStarted = true;
        }

        // calculate percentage
        float percent = clamp<float>((engine->getTime() - animation.fStartTime) / (animation.fDuration), 0.0f, 1.0f);

        if(cv_debug_anim.getBool()) debugLog("animation #%i, percent = %f\n", i, percent);

        // check if finished
        if(percent >= 1.0f) {
            *animation.fBase = animation.fTarget;

            if(cv_debug_anim.getBool())
                debugLog("removing animation #%i, dtime = %f\n", i, engine->getTime() - animation.fStartTime);

            this->vAnimations.erase(this->vAnimations.begin() + i);
            i--;

            continue;
        }

        // modify percentage
        switch(animation.animType) {
            case ANIMATION_TYPE::MOVE_SMOOTH_END:
                percent = clamp<float>(1.0f - pow(1.0f - percent, animation.fFactor), 0.0f, 1.0f);
                if((int)(percent * (animation.fTarget - animation.fStartValue) + animation.fStartValue) ==
                   (int)animation.fTarget)
                    percent = 1.0f;
                break;

            case ANIMATION_TYPE::MOVE_QUAD_IN:
                percent = percent * percent;
                break;

            case ANIMATION_TYPE::MOVE_QUAD_OUT:
                percent = -percent * (percent - 2.0f);
                break;

            case ANIMATION_TYPE::MOVE_QUAD_INOUT:
                if((percent *= 2.0f) < 1.0f)
                    percent = 0.5f * percent * percent;
                else {
                    percent -= 1.0f;
                    percent = -0.5f * ((percent) * (percent - 2.0f) - 1.0f);
                }
                break;

            case ANIMATION_TYPE::MOVE_CUBIC_IN:
                percent = percent * percent * percent;
                break;

            case ANIMATION_TYPE::MOVE_CUBIC_OUT:
                percent = percent - 1.0f;
                percent = percent * percent * percent + 1.0f;
                break;

            case ANIMATION_TYPE::MOVE_QUART_IN:
                percent = percent * percent * percent * percent;
                break;

            case ANIMATION_TYPE::MOVE_QUART_OUT:
                percent = percent - 1.0f;
                percent = 1.0f - percent * percent * percent * percent;
                break;
            default:
                break;
        }

        // set new value
        *animation.fBase = animation.fStartValue + percent * (animation.fTarget - animation.fStartValue);
    }

    if(this->vAnimations.size() > 512)
        debugLog("WARNING: AnimationHandler has %i animations!\n", this->vAnimations.size());

    // printf("AnimStackSize = %i\n", this->vAnimations.size());
}

void AnimationHandler::moveLinear(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_LINEAR);
}

void AnimationHandler::moveQuadIn(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_IN);
}

void AnimationHandler::moveQuadOut(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_OUT);
}

void AnimationHandler::moveQuadInOut(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_INOUT);
}

void AnimationHandler::moveCubicIn(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_CUBIC_IN);
}

void AnimationHandler::moveCubicOut(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_CUBIC_OUT);
}

void AnimationHandler::moveQuartIn(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUART_IN);
}

void AnimationHandler::moveQuartOut(float *base, float target, float duration, float delay, bool overrideExisting) {
    this->addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUART_OUT);
}

void AnimationHandler::moveSmoothEnd(float *base, float target, float duration, int smoothFactor, float delay) {
    this->addAnimation(base, target, duration, delay, true, ANIMATION_TYPE::MOVE_SMOOTH_END, smoothFactor);
}

void AnimationHandler::addAnimation(float *base, float target, float duration, float delay, bool overrideExisting,
                                    AnimationHandler::ANIMATION_TYPE type, float smoothFactor) {
    if(base == NULL) return;

    if(overrideExisting) this->overrideExistingAnimation(base);

    Animation anim;

    anim.fBase = base;
    anim.fTarget = target;
    anim.fDuration = duration;
    anim.fStartValue = *base;
    anim.fStartTime = engine->getTime() + delay;
    anim.animType = type;
    anim.fFactor = smoothFactor;
    anim.bStarted = (delay == 0.0f);

    this->vAnimations.push_back(anim);
}

void AnimationHandler::overrideExistingAnimation(float *base) { this->deleteExistingAnimation(base); }

void AnimationHandler::deleteExistingAnimation(float *base) {
    for(size_t i = 0; i < this->vAnimations.size(); i++) {
        if(this->vAnimations[i].fBase == base) {
            this->vAnimations.erase(this->vAnimations.begin() + i);
            i--;
        }
    }
}

float AnimationHandler::getRemainingDuration(float *base) const {
    for(size_t i = 0; i < this->vAnimations.size(); i++) {
        if(this->vAnimations[i].fBase == base)
            return max(0.0f,
                       (this->vAnimations[i].fStartTime + this->vAnimations[i].fDuration) - (float)engine->getTime());
    }

    return 0.0f;
}

bool AnimationHandler::isAnimating(float *base) const {
    for(size_t i = 0; i < this->vAnimations.size(); i++) {
        if(this->vAnimations[i].fBase == base) return true;
    }

    return false;
}
