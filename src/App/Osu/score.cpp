#include "score.h"

#include "Bancho.h"
#include "BanchoProtocol.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "Engine.h"
#include "GameRules.h"
#include "HUD.h"
#include "HitObject.h"
#include "LegacyReplay.h"
#include "Osu.h"
#include "RoomScreen.h"

using namespace std;

LiveScore::LiveScore(bool simulating) {
    m_simulating = simulating;
    reset();
}

void LiveScore::reset() {
    m_hitresults = std::vector<HIT>();
    m_hitdeltas = std::vector<int>();

    m_grade = FinishedScore::Grade::N;
    if(!m_simulating) {
        mods = Replay::Mods::from_cvars();
    }

    m_fStarsTomTotal = 0.0f;
    m_fStarsTomAim = 0.0f;
    m_fStarsTomSpeed = 0.0f;
    m_fPPv2 = 0.0f;
    m_iIndex = -1;

    m_iScoreV1 = 0;
    m_iScoreV2 = 0;
    m_iScoreV2ComboPortion = 0;
    m_iBonusPoints = 0;
    m_iCombo = 0;
    m_iComboMax = 0;
    m_iComboFull = 0;
    m_iComboEndBitmask = 0;
    m_fAccuracy = 1.0f;
    m_fHitErrorAvgMin = 0.0f;
    m_fHitErrorAvgMax = 0.0f;
    m_fHitErrorAvgCustomMin = 0.0f;
    m_fHitErrorAvgCustomMax = 0.0f;
    m_fUnstableRate = 0.0f;

    m_iNumMisses = 0;
    m_iNumSliderBreaks = 0;
    m_iNum50s = 0;
    m_iNum100s = 0;
    m_iNum100ks = 0;
    m_iNum300s = 0;
    m_iNum300gs = 0;

    m_bDead = false;
    m_bDied = false;

    m_iNumK1 = 0;
    m_iNumK2 = 0;
    m_iNumM1 = 0;
    m_iNumM2 = 0;

    m_iNumEZRetries = 2;
    m_bIsUnranked = false;

    onScoreChange();
}

float LiveScore::getScoreMultiplier() {
    bool ez = mods.flags & Replay::ModFlags::Easy;
    bool nf = mods.flags & Replay::ModFlags::NoFail;
    bool sv2 = mods.flags & Replay::ModFlags::ScoreV2;
    bool hr = mods.flags & Replay::ModFlags::HardRock;
    bool fl = mods.flags & Replay::ModFlags::Flashlight;
    bool hd = mods.flags & Replay::ModFlags::Hidden;
    bool so = mods.flags & Replay::ModFlags::SpunOut;
    bool rx = mods.flags & Replay::ModFlags::Relax;
    bool ap = mods.flags & Replay::ModFlags::Autopilot;

    float multiplier = 1.0f;

    // Dumb formula, but the values for HT/DT were dumb to begin with
    if(mods.speed > 1.f) {
        multiplier *= (0.24 * mods.speed) + 0.76;
    } else if(mods.speed < 1.f) {
        multiplier *= 0.008 * std::exp(4.81588 * mods.speed);
    }

    if(ez || (nf && !sv2)) multiplier *= 0.5f;
    if(hr) multiplier *= sv2 ? 1.1f : 1.06f;
    if(fl) multiplier *= 1.12f;
    if(hd) multiplier *= 1.06f;
    if(so) multiplier *= 0.90f;
    if(rx || ap) multiplier *= 0.f;

    return multiplier;
}

void LiveScore::addHitResult(BeatmapInterface *beatmap, HitObject *hitObject, HIT hit, long delta,
                             bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore) {
    const int scoreComboMultiplier =
        max(m_iCombo - 1, 0);  // current combo, excluding the current hitobject which caused the addHitResult() call

    // handle hits (and misses)
    if(hit != LiveScore::HIT::HIT_MISS) {
        if(!ignoreOnHitErrorBar) {
            m_hitdeltas.push_back((int)delta);
            if(!m_simulating) osu->getHUD()->addHitError(delta);
        }

        if(!ignoreCombo) {
            m_iCombo++;
            if(!m_simulating) osu->getHUD()->animateCombo();
        }
    } else  // misses
    {
        if(!m_simulating && !ignoreOnHitErrorBar && cv_hiterrorbar_misses.getBool() &&
           delta <= (long)beatmap->getHitWindow50()) {
            osu->getHUD()->addHitError(delta, true);
        }

        m_iCombo = 0;
    }

    // keep track of the maximum possible combo at this time, for live pp calculation
    if(!ignoreCombo) m_iComboFull++;

    // store the result, get hit value
    unsigned long long hitValue = 0;
    if(!hitErrorBarOnly) {
        m_hitresults.push_back(hit);

        switch(hit) {
            case LiveScore::HIT::HIT_MISS:
                m_iNumMisses++;
                m_iComboEndBitmask |= 2;
                break;

            case LiveScore::HIT::HIT_50:
                m_iNum50s++;
                hitValue = 50;
                m_iComboEndBitmask |= 2;
                break;

            case LiveScore::HIT::HIT_100:
                m_iNum100s++;
                hitValue = 100;
                m_iComboEndBitmask |= 1;
                break;

            case LiveScore::HIT::HIT_300:
                m_iNum300s++;
                hitValue = 300;
                break;

            default:
                debugLog("Unexpected hitresult %d\n", hit);
                break;
        }
    }

    // add hitValue to score, recalculate score v1
    if(!ignoreScore) {
        const int difficultyMultiplier = beatmap->getScoreV1DifficultyMultiplier();
        m_iScoreV1 += hitValue;
        m_iScoreV1 +=
            ((hitValue * (u32)((f64)scoreComboMultiplier * (f64)difficultyMultiplier * (f64)getScoreMultiplier())) /
             (u32)25);
    }

    const float totalHitPoints = m_iNum50s * (1.0f / 6.0f) + m_iNum100s * (2.0f / 6.0f) + m_iNum300s;
    const float totalNumHits = m_iNumMisses + m_iNum50s + m_iNum100s + m_iNum300s;

    const float percent300s = m_iNum300s / totalNumHits;
    const float percent50s = m_iNum50s / totalNumHits;

    // recalculate accuracy
    if(((totalHitPoints == 0.0f || totalNumHits == 0.0f) && m_hitresults.size() < 1) || totalNumHits <= 0.0f)
        m_fAccuracy = 1.0f;
    else
        m_fAccuracy = totalHitPoints / totalNumHits;

    // recalculate score v2
    m_iScoreV2ComboPortion += (unsigned long long)((double)hitValue * (1.0 + (double)scoreComboMultiplier / 10.0));
    if(mods.flags & Replay::ModFlags::ScoreV2) {
        const double maximumAccurateHits = beatmap->nb_hitobjects;

        if(totalNumHits > 0) {
            m_iScoreV2 = (u64)(((f64)m_iScoreV2ComboPortion / (f64)beatmap->m_iScoreV2ComboPortionMaximum * 700000.0 +
                                pow((f64)m_fAccuracy, 10.0) * ((f64)totalNumHits / maximumAccurateHits) * 300000.0 +
                                (f64)m_iBonusPoints) *
                               (f64)getScoreMultiplier());
        }
    }

    // recalculate grade
    m_grade = FinishedScore::Grade::D;

    bool hd = mods.flags & Replay::ModFlags::Hidden;
    bool fl = mods.flags & Replay::ModFlags::Flashlight;

    if(percent300s > 0.6f) m_grade = FinishedScore::Grade::C;
    if((percent300s > 0.7f && m_iNumMisses == 0) || (percent300s > 0.8f)) m_grade = FinishedScore::Grade::B;
    if((percent300s > 0.8f && m_iNumMisses == 0) || (percent300s > 0.9f)) m_grade = FinishedScore::Grade::A;
    if(percent300s > 0.9f && percent50s <= 0.01f && m_iNumMisses == 0) {
        m_grade = (hd || fl) ? FinishedScore::Grade::SH : FinishedScore::Grade::S;
    }
    if(m_iNumMisses == 0 && m_iNum50s == 0 && m_iNum100s == 0) {
        m_grade = (hd || fl) ? FinishedScore::Grade::XH : FinishedScore::Grade::X;
    }

    // recalculate unstable rate
    float averageDelta = 0.0f;
    m_fUnstableRate = 0.0f;
    m_fHitErrorAvgMin = 0.0f;
    m_fHitErrorAvgMax = 0.0f;
    m_fHitErrorAvgCustomMin = 0.0f;
    m_fHitErrorAvgCustomMax = 0.0f;
    if(m_hitdeltas.size() > 0) {
        int numPositives = 0;
        int numNegatives = 0;
        int numCustomPositives = 0;
        int numCustomNegatives = 0;

        const int customStartIndex =
            (cv_hud_statistics_hitdelta_chunksize.getInt() < 0
                 ? 0
                 : max(0, (int)m_hitdeltas.size() - cv_hud_statistics_hitdelta_chunksize.getInt())) -
            1;

        for(int i = 0; i < m_hitdeltas.size(); i++) {
            averageDelta += (float)m_hitdeltas[i];

            if(m_hitdeltas[i] > 0) {
                // positive
                m_fHitErrorAvgMax += (float)m_hitdeltas[i];
                numPositives++;

                if(i > customStartIndex) {
                    m_fHitErrorAvgCustomMax += (float)m_hitdeltas[i];
                    numCustomPositives++;
                }
            } else if(m_hitdeltas[i] < 0) {
                // negative
                m_fHitErrorAvgMin += (float)m_hitdeltas[i];
                numNegatives++;

                if(i > customStartIndex) {
                    m_fHitErrorAvgCustomMin += (float)m_hitdeltas[i];
                    numCustomNegatives++;
                }
            } else {
                // perfect
                numPositives++;
                numNegatives++;

                if(i > customStartIndex) {
                    numCustomPositives++;
                    numCustomNegatives++;
                }
            }
        }
        averageDelta /= (float)m_hitdeltas.size();
        m_fHitErrorAvgMin = (numNegatives > 0 ? m_fHitErrorAvgMin / (float)numNegatives : 0.0f);
        m_fHitErrorAvgMax = (numPositives > 0 ? m_fHitErrorAvgMax / (float)numPositives : 0.0f);
        m_fHitErrorAvgCustomMin = (numCustomNegatives > 0 ? m_fHitErrorAvgCustomMin / (float)numCustomNegatives : 0.0f);
        m_fHitErrorAvgCustomMax = (numCustomPositives > 0 ? m_fHitErrorAvgCustomMax / (float)numCustomPositives : 0.0f);

        for(int i = 0; i < m_hitdeltas.size(); i++) {
            m_fUnstableRate += ((float)m_hitdeltas[i] - averageDelta) * ((float)m_hitdeltas[i] - averageDelta);
        }
        m_fUnstableRate /= (float)m_hitdeltas.size();
        m_fUnstableRate = std::sqrt(m_fUnstableRate) * 10;

        // compensate for speed
        m_fUnstableRate /= beatmap->getSpeedMultiplier();
    }

    // recalculate max combo
    if(m_iCombo > m_iComboMax) m_iComboMax = m_iCombo;

    if(!m_simulating) {
        onScoreChange();
    }
}

void LiveScore::addHitResultComboEnd(LiveScore::HIT hit) {
    switch(hit) {
        case LiveScore::HIT::HIT_100K:
        case LiveScore::HIT::HIT_300K:
            m_iNum100ks++;
            break;

        case LiveScore::HIT::HIT_300G:
            m_iNum300gs++;
            break;

        default:
            break;
    }
}

void LiveScore::addSliderBreak() {
    m_iCombo = 0;
    m_iNumSliderBreaks++;

    onScoreChange();
}

void LiveScore::addPoints(int points, bool isSpinner) {
    m_iScoreV1 += (unsigned long long)points;

    if(isSpinner) m_iBonusPoints += points;  // only used for scorev2 calculation currently

    onScoreChange();
}

void LiveScore::setDead(bool dead) {
    if(m_bDead == dead) return;

    m_bDead = dead;

    if(m_bDead) m_bDied = true;

    onScoreChange();
}

void LiveScore::addKeyCount(int key) {
    switch(key) {
        case 1:
            m_iNumK1++;
            break;
        case 2:
            m_iNumK2++;
            break;
        case 3:
            m_iNumM1++;
            break;
        case 4:
            m_iNumM2++;
            break;
    }
}

double LiveScore::getHealthIncrease(BeatmapInterface *beatmap, HIT hit) {
    return getHealthIncrease(hit, beatmap->getHP(), beatmap->m_fHpMultiplierNormal, beatmap->m_fHpMultiplierComboEnd,
                             200.0);
}

double LiveScore::getHealthIncrease(LiveScore::HIT hit, double HP, double hpMultiplierNormal,
                                    double hpMultiplierComboEnd, double hpBarMaximumForNormalization) {
    switch(hit) {
        case LiveScore::HIT::HIT_MISS:
            return (GameRules::mapDifficultyRangeDouble(HP, -6.0, -25.0, -40.0) / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_50:
            return (hpMultiplierNormal * GameRules::mapDifficultyRangeDouble(HP, 0.4 * 8.0, 0.4, 0.4) /
                    hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_100:
            return (hpMultiplierNormal * GameRules::mapDifficultyRangeDouble(HP, 2.2 * 8.0, 2.2, 2.2) /
                    hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_300:
            return (hpMultiplierNormal * 6.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_MISS_SLIDERBREAK:
            return (GameRules::mapDifficultyRangeDouble(HP, -4.0, -15.0, -28.0) / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_MU:
            return (hpMultiplierComboEnd * 6.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_100K:
            return (hpMultiplierComboEnd * 10.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_300K:
            return (hpMultiplierComboEnd * 10.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_300G:
            return (hpMultiplierComboEnd * 14.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_SLIDER10:
            return (hpMultiplierNormal * 3.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_SLIDER30:
            return (hpMultiplierNormal * 4.0 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_SPINNERSPIN:
            return (hpMultiplierNormal * 1.7 / hpBarMaximumForNormalization);

        case LiveScore::HIT::HIT_SPINNERBONUS:
            return (hpMultiplierNormal * 2.0 / hpBarMaximumForNormalization);

        default:
            return 0.f;
    }
}

int LiveScore::getKeyCount(int key) {
    switch(key) {
        case 1:
            return m_iNumK1;
        case 2:
            return m_iNumK2;
        case 3:
            return m_iNumM1;
        case 4:
            return m_iNumM2;
    }

    return 0;
}

u32 LiveScore::getModsLegacy() { return mods.to_legacy(); }

UString LiveScore::getModsStringForRichPresence() {
    UString modsString;

    if(osu->getModNF()) modsString.append("NF");
    if(osu->getModEZ()) modsString.append("EZ");
    if(osu->getModHD()) modsString.append("HD");
    if(osu->getModHR()) modsString.append("HR");
    if(osu->getModSD()) modsString.append("SD");
    if(osu->getModRelax()) modsString.append("RX");
    if(osu->getModAuto()) modsString.append("AT");
    if(osu->getModSpunout()) modsString.append("SO");
    if(osu->getModAutopilot()) modsString.append("AP");
    if(osu->getModSS()) modsString.append("PF");
    if(osu->getModScorev2()) modsString.append("v2");
    if(osu->getModTarget()) modsString.append("TP");
    if(osu->getModNightmare()) modsString.append("NM");
    if(osu->getModTD()) modsString.append("TD");

    return modsString;
}

unsigned long long LiveScore::getScore() { return mods.flags & Replay::ModFlags::ScoreV2 ? m_iScoreV2 : m_iScoreV1; }

void LiveScore::onScoreChange() {
    if(m_simulating) return;

    osu->m_room->onClientScoreChange();

    // only used to block local scores for people who think they are very clever by quickly disabling auto just before
    // the end of a beatmap
    m_bIsUnranked |= (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax()));

    if(osu->isInPlayMode()) {
        osu->m_hud->updateScoreboard(true);
    }
}

float LiveScore::calculateAccuracy(int num300s, int num100s, int num50s, int numMisses) {
    const float totalHitPoints = num50s * (1.0f / 6.0f) + num100s * (2.0f / 6.0f) + num300s;
    const float totalNumHits = numMisses + num50s + num100s + num300s;

    if(totalNumHits > 0.0f) return (totalHitPoints / totalNumHits);

    return 0.0f;
}

f64 FinishedScore::get_pp() const {
    if(cv_use_ppv3.getBool() && ppv3_algorithm.size() > 0) {
        return ppv3_score;
    }

    if(ppv2_version < DifficultyCalculator::PP_ALGORITHM_VERSION) {
        return -1.0;
    } else {
        return ppv2_score;
    }
}

FinishedScore::Grade FinishedScore::calculate_grade() const {
    float totalNumHits = numMisses + num50s + num100s + num300s;
    bool modHidden = (mods.flags & Replay::ModFlags::Hidden);
    bool modFlashlight = (mods.flags & Replay::ModFlags::Flashlight);

    float percent300s = 0.0f;
    float percent50s = 0.0f;
    if(totalNumHits > 0.0f) {
        percent300s = num300s / totalNumHits;
        percent50s = num50s / totalNumHits;
    }

    FinishedScore::Grade grade = FinishedScore::Grade::D;
    if(percent300s > 0.6f) grade = FinishedScore::Grade::C;
    if((percent300s > 0.7f && numMisses == 0) || (percent300s > 0.8f)) grade = FinishedScore::Grade::B;
    if((percent300s > 0.8f && numMisses == 0) || (percent300s > 0.9f)) grade = FinishedScore::Grade::A;
    if(percent300s > 0.9f && percent50s <= 0.01f && numMisses == 0)
        grade = ((modHidden || modFlashlight) ? FinishedScore::Grade::SH : FinishedScore::Grade::S);
    if(numMisses == 0 && num50s == 0 && num100s == 0)
        grade = ((modHidden || modFlashlight) ? FinishedScore::Grade::XH : FinishedScore::Grade::X);

    return grade;
}
