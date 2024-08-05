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
#include "Osu.h"
#include "Replay.h"
#include "RoomScreen.h"

using namespace std;

ConVar osu_hiterrorbar_misses("osu_hiterrorbar_misses", true, FCVAR_DEFAULT);
ConVar osu_debug_pp("osu_debug_pp", false, FCVAR_DEFAULT);

ConVar osu_hud_statistics_hitdelta_chunksize("osu_hud_statistics_hitdelta_chunksize", 30, FCVAR_DEFAULT,
                                             "how many recent hit deltas to average (-1 = all)");

ConVar osu_drain_vr_multiplier("osu_drain_vr_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar osu_drain_vr_300("osu_drain_vr_300", 0.035f, FCVAR_DEFAULT);
ConVar osu_drain_vr_100("osu_drain_vr_100", -0.10f, FCVAR_DEFAULT);
ConVar osu_drain_vr_50("osu_drain_vr_50", -0.125f, FCVAR_DEFAULT);
ConVar osu_drain_vr_miss("osu_drain_vr_miss", -0.15f, FCVAR_DEFAULT);
ConVar osu_drain_vr_sliderbreak("osu_drain_vr_sliderbreak", -0.10f, FCVAR_DEFAULT);

ConVar use_ppv3("use_ppv3", false, FCVAR_DEFAULT, "use ppv3 instead of ppv2 (experimental)");

ConVar *LiveScore::m_osu_draw_statistics_pp_ref = NULL;

LiveScore::LiveScore() {
    reset();

    if(m_osu_draw_statistics_pp_ref == NULL)
        m_osu_draw_statistics_pp_ref = convar->getConVarByName("osu_draw_statistics_pp");
}

void LiveScore::reset() {
    m_hitresults = std::vector<HIT>();
    m_hitdeltas = std::vector<int>();

    m_grade = FinishedScore::Grade::N;

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

void LiveScore::addHitResult(BeatmapInterface *beatmap, HitObject *hitObject, HIT hit, long delta,
                             bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore) {
    const int scoreComboMultiplier =
        max(m_iCombo - 1, 0);  // current combo, excluding the current hitobject which caused the addHitResult() call

    // handle hits (and misses)
    if(hit != LiveScore::HIT::HIT_MISS) {
        if(!ignoreOnHitErrorBar) {
            m_hitdeltas.push_back((int)delta);
            osu->getHUD()->addHitError(delta);
        }

        if(!ignoreCombo) {
            m_iCombo++;
            osu->getHUD()->animateCombo();
        }
    } else  // misses
    {
        if(osu_hiterrorbar_misses.getBool() && !ignoreOnHitErrorBar && delta <= (long)beatmap->getHitWindow50())
            osu->getHUD()->addHitError(delta, true);

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
        m_iScoreV1 += ((hitValue *
                        (u32)((f64)scoreComboMultiplier * (f64)difficultyMultiplier * (f64)osu->getScoreMultiplier())) /
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
    if(osu->getModScorev2()) {
        const double maximumAccurateHits = beatmap->nb_hitobjects;

        if(totalNumHits > 0)
            m_iScoreV2 = (unsigned long long)(((double)m_iScoreV2ComboPortion /
                                                   (double)beatmap->m_iScoreV2ComboPortionMaximum * 700000.0 +
                                               pow((double)m_fAccuracy, 10.0) *
                                                   ((double)totalNumHits / maximumAccurateHits) * 300000.0 +
                                               (double)m_iBonusPoints) *
                                              (double)osu->getScoreMultiplier());
    }

    // recalculate grade
    m_grade = FinishedScore::Grade::D;
    if(percent300s > 0.6f) m_grade = FinishedScore::Grade::C;
    if((percent300s > 0.7f && m_iNumMisses == 0) || (percent300s > 0.8f)) m_grade = FinishedScore::Grade::B;
    if((percent300s > 0.8f && m_iNumMisses == 0) || (percent300s > 0.9f)) m_grade = FinishedScore::Grade::A;
    if(percent300s > 0.9f && percent50s <= 0.01f && m_iNumMisses == 0)
        m_grade = osu->getModHD() || osu->getModFlashlight() ? FinishedScore::Grade::SH : FinishedScore::Grade::S;
    if(m_iNumMisses == 0 && m_iNum50s == 0 && m_iNum100s == 0)
        m_grade = osu->getModHD() || osu->getModFlashlight() ? FinishedScore::Grade::XH : FinishedScore::Grade::X;

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
            (osu_hud_statistics_hitdelta_chunksize.getInt() < 0
                 ? 0
                 : max(0, (int)m_hitdeltas.size() - osu_hud_statistics_hitdelta_chunksize.getInt())) -
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

    onScoreChange();
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

u32 LiveScore::getModsLegacy() {
    int modsLegacy = 0;

    modsLegacy |= (osu->getModAuto() ? ModFlags::Autoplay : 0);
    modsLegacy |= (osu->getModAutopilot() ? ModFlags::Autopilot : 0);
    modsLegacy |= (osu->getModRelax() ? ModFlags::Relax : 0);
    modsLegacy |= (osu->getModSpunout() ? ModFlags::SpunOut : 0);
    modsLegacy |= (osu->getModTarget() ? ModFlags::Target : 0);
    modsLegacy |= (osu->getModScorev2() ? ModFlags::ScoreV2 : 0);
    modsLegacy |= (osu->getModDT() ? ModFlags::DoubleTime : 0);
    modsLegacy |= (osu->getModNC() ? (ModFlags::DoubleTime | ModFlags::Nightcore) : 0);
    modsLegacy |= (osu->getModNF() ? ModFlags::NoFail : 0);
    modsLegacy |= (osu->getModHT() ? ModFlags::HalfTime : 0);
    modsLegacy |= (osu->getModDC() ? ModFlags::HalfTime : 0);
    modsLegacy |= (osu->getModHD() ? ModFlags::Hidden : 0);
    modsLegacy |= (osu->getModHR() ? ModFlags::HardRock : 0);
    modsLegacy |= (osu->getModEZ() ? ModFlags::Easy : 0);
    modsLegacy |= (osu->getModSD() ? ModFlags::SuddenDeath : 0);
    modsLegacy |= (osu->getModSS() ? ModFlags::Perfect : 0);
    modsLegacy |= (osu->getModNightmare() ? ModFlags::Nightmare : 0);
    modsLegacy |= (osu->getModTD() ? ModFlags::TouchDevice : 0);
    modsLegacy |= (osu->getModFlashlight() ? ModFlags::Flashlight : 0);

    // Set some unused (in osu!std) mod flags for non-vanilla mods
    // (these flags don't seem to cause issues on osu!stable or bancho.py)
    if(convar->getConVarByName("osu_mod_fposu")->getBool()) modsLegacy |= ModFlags::FPoSu;
    if(convar->getConVarByName("osu_playfield_mirror_horizontal")->getBool()) modsLegacy |= ModFlags::Mirror;
    if(convar->getConVarByName("osu_playfield_mirror_vertical")->getBool()) modsLegacy |= ModFlags::Mirror;

    return modsLegacy;
}

UString LiveScore::getModsStringForRichPresence() {
    UString modsString;

    if(osu->getModNF()) modsString.append("NF");
    if(osu->getModEZ()) modsString.append("EZ");
    if(osu->getModHD()) modsString.append("HD");
    if(osu->getModHR()) modsString.append("HR");
    if(osu->getModSD()) modsString.append("SD");
    if(osu->getModDT()) modsString.append("DT");
    if(osu->getModRelax()) modsString.append("RX");
    if(osu->getModHT()) modsString.append("HT");
    if(osu->getModNC()) modsString.append("NC");
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

unsigned long long LiveScore::getScore() { return osu->getModScorev2() ? m_iScoreV2 : m_iScoreV1; }

void LiveScore::onScoreChange() {
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
    if(use_ppv3.getBool() && ppv3_algorithm.size() > 0) {
        return ppv3_score;
    }

    return ppv2_score;
}

FinishedScore::Grade LiveScore::calculateGrade(int num300s, int num100s, int num50s, int numMisses, bool modHidden,
                                               bool modFlashlight) {
    const float totalNumHits = numMisses + num50s + num100s + num300s;

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
