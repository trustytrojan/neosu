#include "SimulatedBeatmap.h"

#include "Circle.h"
#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "Engine.h"
#include "Environment.h"
#include "GameRules.h"
#include "HitObject.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "ModFPoSu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "Slider.h"
#include "Spinner.h"

using namespace std;

SimulatedBeatmap::SimulatedBeatmap(DatabaseBeatmap *diff2) {
    m_selectedDifficulty2 = diff2;

    nb_hitobjects = diff2->getNumObjects();

    m_iNPS = 0;
    m_iND = 0;
    m_iCurrentHitObjectIndex = 0;
    m_iCurrentNumCircles = 0;
    m_iCurrentNumSliders = 0;
    m_iCurrentNumSpinners = 0;

    m_bIsSpinnerActive = false;

    m_fPlayfieldRotation = 0.0f;

    m_fXMultiplier = 1.0f;
    m_fRawHitcircleDiameter = 27.35f * 2.0f;
    m_fSliderFollowCircleDiameter = 0.0f;
}

SimulatedBeatmap::~SimulatedBeatmap() {
    for(int i = 0; i < m_hitobjects.size(); i++) {
        delete m_hitobjects[i];
    }
}

void SimulatedBeatmap::simulate_to(i32 music_pos) {
    Replay::Frame current_frame = spectated_replay[current_frame_idx];
    Replay::Frame next_frame = spectated_replay[current_frame_idx + 1];

    while(next_frame.cur_music_pos <= music_pos) {
        if(current_frame_idx + 2 >= spectated_replay.size()) break;

        last_keys = current_keys;

        current_frame_idx++;
        current_frame = spectated_replay[current_frame_idx];
        next_frame = spectated_replay[current_frame_idx + 1];
        current_keys = current_frame.key_flags;

        Click click;
        click.tms = current_frame.cur_music_pos;
        click.pos.x = current_frame.x;
        click.pos.y = current_frame.y;

        // Flag fix to simplify logic (stable sets both K1 and M1 when K1 is pressed)
        if(current_keys & Replay::K1) current_keys &= ~Replay::M1;
        if(current_keys & Replay::K2) current_keys &= ~Replay::M2;

        // Pressed key 1
        if(!(last_keys & Replay::K1) && current_keys & Replay::K1) {
            m_bPrevKeyWasKey1 = true;
            m_clicks.push_back(click);
            if(!m_bInBreak) live_score.addKeyCount(1);
        }
        if(!(last_keys & Replay::M1) && current_keys & Replay::M1) {
            m_bPrevKeyWasKey1 = true;
            m_clicks.push_back(click);
            if(!m_bInBreak) live_score.addKeyCount(3);
        }

        // Pressed key 2
        if(!(last_keys & Replay::K2) && current_keys & Replay::K2) {
            m_bPrevKeyWasKey1 = false;
            m_clicks.push_back(click);
            if(!m_bInBreak) live_score.addKeyCount(2);
        }
        if(!(last_keys & Replay::M2) && current_keys & Replay::M2) {
            m_bPrevKeyWasKey1 = false;
            m_clicks.push_back(click);
            if(!m_bInBreak) live_score.addKeyCount(4);
        }

        m_interpolatedMousePos = {current_frame.x, current_frame.y};
        m_iCurMusicPos = current_frame.cur_music_pos;

        update();
    }
}

bool SimulatedBeatmap::start() {
    // reset everything, including deleting any previously loaded hitobjects from another diff which we might just have
    // played
    resetScore();

    // some hitobjects already need this information to be up-to-date before their constructor is called
    updatePlayfieldMetrics();
    updateHitobjectMetrics();

    // actually load the difficulty (and the hitobjects)
    DatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = DatabaseBeatmap::loadGameplay(m_selectedDifficulty2, this);
    if(result.errorCode != 0) {
        return false;
    }
    m_hitobjects = std::move(result.hitobjects);
    m_breaks = std::move(result.breaks);

    // sort hitobjects by endtime
    m_hitobjectsSortedByEndTime = m_hitobjects;
    struct HitObjectSortComparator {
        bool operator()(HitObject const *a, HitObject const *b) const {
            // strict weak ordering!
            if((a->getTime() + a->getDuration()) == (b->getTime() + b->getDuration()))
                return a->getSortHack() < b->getSortHack();
            else
                return (a->getTime() + a->getDuration()) < (b->getTime() + b->getDuration());
        }
    };
    std::sort(m_hitobjectsSortedByEndTime.begin(), m_hitobjectsSortedByEndTime.end(), HitObjectSortComparator());

    // after the hitobjects have been loaded we can calculate the stacks
    calculateStacks();
    computeDrainRate();

    m_bInBreak = false;

    // NOTE: loading failures are handled dynamically in update(), so temporarily assume everything has worked in here
    return true;
}

void SimulatedBeatmap::fail() { m_bFailed = true; }

void SimulatedBeatmap::cancelFailing() { m_bFailed = false; }

u32 SimulatedBeatmap::getScoreV1DifficultyMultiplier() const {
    // NOTE: We intentionally get CS/HP/OD from beatmap data, not "real" CS/HP/OD
    //       Since this multiplier is only used for ScoreV1
    u32 breakTimeMS = getBreakDurationTotal();
    f32 drainLength = max(getLengthPlayable() - min(breakTimeMS, getLengthPlayable()), (u32)1000) / 1000;
    return std::round((m_selectedDifficulty2->getCS() + m_selectedDifficulty2->getHP() +
                       m_selectedDifficulty2->getOD() +
                       clamp<f32>((f32)m_selectedDifficulty2->getNumObjects() / drainLength * 8.0f, 0.0f, 16.0f)) /
                      38.0f * 5.0f);
}

f32 SimulatedBeatmap::getCS() const {
    f32 CS = clamp<f32>(m_selectedDifficulty2->getCS() * osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);

    if(osu_cs_override >= 0.0f) CS = osu_cs_override;
    if(osu_cs_overridenegative < 0.0f) CS = osu_cs_overridenegative;

    if(osu_mod_minimize) {
        const f32 percent =
            1.0f + ((f64)(m_iCurMusicPos - m_hitobjects[0]->getTime()) /
                    (f64)(m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                          m_hitobjects[m_hitobjects.size() - 1]->getDuration() - m_hitobjects[0]->getTime())) *
                       osu_mod_minimize_multiplier;
        CS *= percent;
    }

    CS = min(CS, 12.1429f);

    return CS;
}

f32 SimulatedBeatmap::getHP() const {
    f32 HP = clamp<f32>(m_selectedDifficulty2->getHP() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
    if(osu_hp_override >= 0.0f) HP = osu_hp_override;

    return HP;
}

f32 SimulatedBeatmap::getRawOD() const {
    return clamp<f32>(m_selectedDifficulty2->getOD() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

f32 SimulatedBeatmap::getOD() const {
    f32 OD = getRawOD();

    if(osu_od_override >= 0.0f) OD = osu_od_override;

    if(osu_od_override_lock)
        OD = GameRules::getRawConstantOverallDifficultyForSpeedMultiplier(GameRules::getRawHitWindow300(OD), speed);

    return OD;
}

bool SimulatedBeatmap::isKey1Down() const { return current_keys & (Replay::M1 | Replay::K1); }
bool SimulatedBeatmap::isKey2Down() const { return current_keys & (Replay::M2 | Replay::K2); }
bool SimulatedBeatmap::isClickHeld() const { return isKey1Down() || isKey2Down(); }

DatabaseBeatmap::BREAK SimulatedBeatmap::getBreakForTimeRange(long startMS, long positionMS, long endMS) const {
    DatabaseBeatmap::BREAK curBreak;

    curBreak.startTime = -1;
    curBreak.endTime = -1;

    for(int i = 0; i < m_breaks.size(); i++) {
        if(m_breaks[i].startTime >= (int)startMS && m_breaks[i].endTime <= (int)endMS) {
            if((int)positionMS >= curBreak.startTime) curBreak = m_breaks[i];
        }
    }

    return curBreak;
}

LiveScore::HIT SimulatedBeatmap::addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo,
                                              bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo,
                                              bool ignoreScore, bool ignoreHealth) {
    // handle perfect & sudden death
    if(mod_flags & ModFlags::Perfect) {
        if(!hitErrorBarOnly && hit != LiveScore::HIT::HIT_300 && hit != LiveScore::HIT::HIT_300G &&
           hit != LiveScore::HIT::HIT_SLIDER10 && hit != LiveScore::HIT::HIT_SLIDER30 &&
           hit != LiveScore::HIT::HIT_SPINNERSPIN && hit != LiveScore::HIT::HIT_SPINNERBONUS) {
            fail();
            return LiveScore::HIT::HIT_MISS;
        }
    } else if(mod_flags & ModFlags::SuddenDeath) {
        if(hit == LiveScore::HIT::HIT_MISS) {
            fail();
            return LiveScore::HIT::HIT_MISS;
        }
    }

    // score
    live_score.addHitResult(this, hitObject, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo,
                            ignoreScore);

    // health
    LiveScore::HIT returnedHit = LiveScore::HIT::HIT_MISS;
    if(!ignoreHealth) {
        addHealth(live_score.getHealthIncrease((BeatmapInterface *)this, hit), true);

        // geki/katu handling
        if(isEndOfCombo) {
            const int comboEndBitmask = live_score.getComboEndBitmask();

            if(comboEndBitmask == 0) {
                returnedHit = LiveScore::HIT::HIT_300G;
                addHealth(live_score.getHealthIncrease(this, returnedHit), true);
                live_score.addHitResultComboEnd(returnedHit);
            } else if((comboEndBitmask & 2) == 0) {
                if(hit == LiveScore::HIT::HIT_100) {
                    returnedHit = LiveScore::HIT::HIT_100K;
                    addHealth(live_score.getHealthIncrease(this, returnedHit), true);
                    live_score.addHitResultComboEnd(returnedHit);
                } else if(hit == LiveScore::HIT::HIT_300) {
                    returnedHit = LiveScore::HIT::HIT_300K;
                    addHealth(live_score.getHealthIncrease(this, returnedHit), true);
                    live_score.addHitResultComboEnd(returnedHit);
                }
            } else if(hit != LiveScore::HIT::HIT_MISS)
                addHealth(live_score.getHealthIncrease(this, LiveScore::HIT::HIT_MU), true);

            live_score.setComboEndBitmask(0);
        }
    }

    return returnedHit;
}

void SimulatedBeatmap::addSliderBreak() {
    // handle perfect & sudden death
    if(mod_flags & ModFlags::Perfect) {
        fail();
        return;
    } else if(mod_flags & ModFlags::SuddenDeath) {
        fail();
        return;
    }

    // score
    live_score.addSliderBreak();
}

void SimulatedBeatmap::addScorePoints(int points, bool isSpinner) { live_score.addPoints(points, isSpinner); }

void SimulatedBeatmap::addHealth(f64 percent, bool isFromHitResult) {
    // never drain before first hitobject
    if(m_iCurMusicPos < m_hitobjects[0]->getTime()) return;

    // never drain after last hitobject
    if(m_iCurMusicPos > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() +
                         m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration()))
        return;

    if(m_bFailed) {
        m_fHealth = 0.0;

        return;
    }

    m_fHealth = clamp<f64>(m_fHealth + percent, 0.0, 1.0);

    // handle generic fail state (2)
    const bool isDead = m_fHealth < 0.001;
    if(isDead && !osu->getModNF()) {
        if(osu->getModEZ() && live_score.getNumEZRetries() > 0)  // retries with ez
        {
            live_score.setNumEZRetries(live_score.getNumEZRetries() - 1);

            // special case: set health to 160/200 (osu!stable behavior, seems fine for all drains)
            m_fHealth = 160.0f / 200.f;
        } else if(isFromHitResult && percent < 0.0)  // judgement fail
        {
            fail();
        }
    }
}

long SimulatedBeatmap::getPVS() {
    // this is an approximation with generous boundaries, it doesn't need to be exact (just good enough to filter 10000
    // hitobjects down to a few hundred or so) it will be used in both positive and negative directions (previous and
    // future hitobjects) to speed up loops which iterate over all hitobjects
    return getApproachTime() + GameRules::getFadeInTime() + (long)GameRules::getHitWindowMiss() + 1500;  // sanity
}

void SimulatedBeatmap::resetScore() {
    vanilla = convar->isVanilla();

    current_keys = 0;
    last_keys = 0;
    current_frame_idx = 0;

    m_fHealth = 1.0;
    m_bFailed = false;

    live_score.reset();
}

void SimulatedBeatmap::update() {
    if(m_hitobjects.empty()) return;

    auto last_hitobject = m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1];
    const bool isAfterLastHitObject = (m_iCurMusicPos > (last_hitobject->getTime() + last_hitobject->getDuration()));
    if(isAfterLastHitObject) {
        return;
    }

    // live update hitobject and playfield metrics
    updateHitobjectMetrics();
    updatePlayfieldMetrics();

    // wobble mod
    if(osu_mod_wobble) {
        const f32 speedMultiplierCompensation = 1.0f / speed;
        m_fPlayfieldRotation =
            (m_iCurMusicPos / 1000.0f) * 30.0f * speedMultiplierCompensation * osu_mod_wobble_rotation_speed;
        m_fPlayfieldRotation = std::fmod(m_fPlayfieldRotation, 360.0f);
    } else {
        m_fPlayfieldRotation = 0.0f;
    }

    // for performance reasons, a lot of operations are crammed into 1 loop over all hitobjects:
    // update all hitobjects,
    // handle click events,
    // also get the time of the next/previous hitobject and their indices for later,
    // and get the current hitobject,
    // also handle miss hiterrorbar slots,
    // also calculate nps and nd,
    // also handle note blocking
    m_currentHitObject = NULL;
    m_iNextHitObjectTime = 0;
    m_iPreviousHitObjectTime = 0;
    m_iNPS = 0;
    m_iND = 0;
    m_iCurrentNumCircles = 0;
    m_iCurrentNumSliders = 0;
    m_iCurrentNumSpinners = 0;
    {
        bool blockNextNotes = false;

        const long pvs = getPVS();
        const int notelockType = osu_notelock_type;
        const long tolerance2B = 3;

        m_iCurrentHitObjectIndex = 0;  // reset below here, since it's needed for mafham pvs

        for(int i = 0; i < m_hitobjects.size(); i++) {
            // the order must be like this:
            // 0) miscellaneous stuff (minimal performance impact)
            // 1) prev + next time vars
            // 2) PVS optimization
            // 3) main hitobject update
            // 4) note blocking
            // 5) click events
            //
            // (because the hitobjects need to know about note blocking before handling the click events)

            // ************ live pp block start ************ //
            const bool isCircle = m_hitobjects[i]->isCircle();
            const bool isSlider = m_hitobjects[i]->isSlider();
            const bool isSpinner = m_hitobjects[i]->isSpinner();
            // ************ live pp block end ************** //

            // determine previous & next object time, used for auto + followpoints + warning arrows + empty section
            // skipping
            if(m_iNextHitObjectTime == 0) {
                if(m_hitobjects[i]->getTime() > m_iCurMusicPos)
                    m_iNextHitObjectTime = m_hitobjects[i]->getTime();
                else {
                    m_currentHitObject = m_hitobjects[i];
                    const long actualPrevHitObjectTime = m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration();
                    m_iPreviousHitObjectTime = actualPrevHitObjectTime;
                }
            }

            // PVS optimization
            if(m_hitobjects[i]->isFinished() &&
               (m_iCurMusicPos - pvs > m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration()))  // past objects
            {
                // ************ live pp block start ************ //
                if(isCircle) m_iCurrentNumCircles++;
                if(isSlider) m_iCurrentNumSliders++;
                if(isSpinner) m_iCurrentNumSpinners++;

                m_iCurrentHitObjectIndex = i;
                // ************ live pp block end ************** //

                continue;
            }
            if(m_hitobjects[i]->getTime() > m_iCurMusicPos + pvs)  // future objects
                break;

            // ************ live pp block start ************ //
            if(m_iCurMusicPos >= m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())
                m_iCurrentHitObjectIndex = i;
            // ************ live pp block end ************** //

            // main hitobject update
            m_hitobjects[i]->update(m_iCurMusicPos);

            // note blocking / notelock (1)
            const Slider *currentSliderPointer = dynamic_cast<Slider *>(m_hitobjects[i]);
            if(notelockType > 0) {
                m_hitobjects[i]->setBlocked(blockNextNotes);

                if(notelockType == 1)  // neosu
                {
                    // (nothing, handled in (2) block)
                } else if(notelockType == 2)  // osu!stable
                {
                    if(!m_hitobjects[i]->isFinished()) {
                        blockNextNotes = true;

                        // Sliders are "finished" after they end
                        // Extra handling for simultaneous/2b hitobjects, as these would otherwise get blocked
                        // NOTE: this will still unlock some simultaneous/2b patterns too early
                        //       (slider slider circle [circle]), but nobody from that niche has complained so far
                        {
                            const bool isSlider = (currentSliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(isSlider || isSpinner) {
                                if((i + 1) < m_hitobjects.size()) {
                                    if((isSpinner || currentSliderPointer->isStartCircleFinished()) &&
                                       (m_hitobjects[i + 1]->getTime() <=
                                        (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
                                        blockNextNotes = false;
                                }
                            }
                        }
                    }
                } else if(notelockType == 3)  // osu!lazer 2020
                {
                    if(!m_hitobjects[i]->isFinished()) {
                        const bool isSlider = (currentSliderPointer != NULL);
                        const bool isSpinner = (!isSlider && !isCircle);

                        if(!isSpinner)  // spinners are completely ignored (transparent)
                        {
                            blockNextNotes = (m_iCurMusicPos <= m_hitobjects[i]->getTime());

                            // sliders are "finished" after their startcircle
                            {
                                // sliders with finished startcircles do not block
                                if(currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished())
                                    blockNextNotes = false;
                            }
                        }
                    }
                }
            } else
                m_hitobjects[i]->setBlocked(false);

            // click events (this also handles hitsounds!)
            const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents =
                (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());
            const bool isCurrentHitObjectFinishedBeforeClickEvents = m_hitobjects[i]->isFinished();
            {
                if(m_clicks.size() > 0) m_hitobjects[i]->onClickEvent(m_clicks);
            }
            const bool isCurrentHitObjectFinishedAfterClickEvents = m_hitobjects[i]->isFinished();
            const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents =
                (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());

            // note blocking / notelock (2.1)
            if(!isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents &&
               isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents) {
                // in here if a slider had its startcircle clicked successfully in this update iteration

                if(notelockType == 2)  // osu!stable
                {
                    // edge case: frame perfect double tapping on overlapping sliders would incorrectly eat the second
                    // input, because the isStartCircleFinished() 2b edge case check handling happens before
                    // m_hitobjects[i]->onClickEvent(m_clicks); so, we check if the currentSliderPointer got its
                    // isStartCircleFinished() within this m_hitobjects[i]->onClickEvent(m_clicks); and unlock
                    // blockNextNotes if that is the case note that we still only unlock within duration + tolerance2B
                    // (same as in (1))
                    if((i + 1) < m_hitobjects.size()) {
                        if((m_hitobjects[i + 1]->getTime() <=
                            (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
                            blockNextNotes = false;
                    }
                }
            }

            // note blocking / notelock (2.2)
            if(!isCurrentHitObjectFinishedBeforeClickEvents && isCurrentHitObjectFinishedAfterClickEvents) {
                // in here if a hitobject has been clicked (and finished completely) successfully in this update
                // iteration

                blockNextNotes = false;

                if(notelockType == 1)  // neosu
                {
                    // auto miss all previous unfinished hitobjects, always
                    // (can stop reverse iteration once we get to the first finished hitobject)

                    for(int m = i - 1; m >= 0; m--) {
                        if(!m_hitobjects[m]->isFinished()) {
                            const Slider *sliderPointer = dynamic_cast<Slider *>(m_hitobjects[m]);

                            const bool isSlider = (sliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(!isSpinner)  // spinners are completely ignored (transparent)
                            {
                                if(m_hitobjects[i]->getTime() >
                                   (m_hitobjects[m]->getTime() +
                                    m_hitobjects[m]->getDuration()))  // NOTE: 2b exception. only force miss if objects
                                                                      // are not overlapping.
                                    m_hitobjects[m]->miss(m_iCurMusicPos);
                            }
                        } else
                            break;
                    }
                } else if(notelockType == 2)  // osu!stable
                {
                    // (nothing, handled in (1) and (2.1) blocks)
                } else if(notelockType == 3)  // osu!lazer 2020
                {
                    // auto miss all previous unfinished hitobjects if the current music time is > their time (center)
                    // (can stop reverse iteration once we get to the first finished hitobject)

                    for(int m = i - 1; m >= 0; m--) {
                        if(!m_hitobjects[m]->isFinished()) {
                            const Slider *sliderPointer = dynamic_cast<Slider *>(m_hitobjects[m]);

                            const bool isSlider = (sliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(!isSpinner)  // spinners are completely ignored (transparent)
                            {
                                if(m_iCurMusicPos > m_hitobjects[m]->getTime()) {
                                    if(m_hitobjects[i]->getTime() >
                                       (m_hitobjects[m]->getTime() +
                                        m_hitobjects[m]->getDuration()))  // NOTE: 2b exception. only force miss if
                                                                          // objects are not overlapping.
                                        m_hitobjects[m]->miss(m_iCurMusicPos);
                                }
                            }
                        } else
                            break;
                    }
                }
            }

            // ************ live pp block start ************ //
            if(isCircle && m_hitobjects[i]->isFinished()) m_iCurrentNumCircles++;
            if(isSlider && m_hitobjects[i]->isFinished()) m_iCurrentNumSliders++;
            if(isSpinner && m_hitobjects[i]->isFinished()) m_iCurrentNumSpinners++;

            if(m_hitobjects[i]->isFinished()) m_iCurrentHitObjectIndex = i;
            // ************ live pp block end ************** //

            // notes per second
            const long npsHalfGateSizeMS = (long)(500.0f * speed);
            if(m_hitobjects[i]->getTime() > m_iCurMusicPos - npsHalfGateSizeMS &&
               m_hitobjects[i]->getTime() < m_iCurMusicPos + npsHalfGateSizeMS)
                m_iNPS++;

            // note density
            if(m_hitobjects[i]->isVisible()) m_iND++;
        }

        // all remaining clicks which have not been consumed by any hitobjects can safely be deleted
        if(m_clicks.size() > 0) {
            // nightmare mod: extra clicks = sliderbreak
            if((osu->getModNightmare() || osu_mod_jigsaw1) && !m_bInBreak && m_iCurrentHitObjectIndex > 0) {
                addSliderBreak();
                addHitResult(NULL, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                             false);  // only decrease health
            }

            m_clicks.clear();
        }
    }

    // break time detection
    const DatabaseBeatmap::BREAK breakEvent =
        getBreakForTimeRange(m_iPreviousHitObjectTime, m_iCurMusicPos, m_iNextHitObjectTime);
    const bool isInBreak = ((int)m_iCurMusicPos >= breakEvent.startTime && (int)m_iCurMusicPos <= breakEvent.endTime);
    if(isInBreak != m_bInBreak) {
        m_bInBreak = !m_bInBreak;
    }

    // hp drain & failing
    // handle constant drain
    {
        if(m_fDrainRate > 0.0) {
            if(!m_bInBreak) {
                // special case: break drain edge cases
                bool drainAfterLastHitobjectBeforeBreakStart = (m_selectedDifficulty2->getVersion() < 8);

                const bool isBetweenHitobjectsAndBreak = (int)m_iPreviousHitObjectTime <= breakEvent.startTime &&
                                                         (int)m_iNextHitObjectTime >= breakEvent.endTime &&
                                                         m_iCurMusicPos > m_iPreviousHitObjectTime;
                const bool isLastHitobjectBeforeBreakStart =
                    isBetweenHitobjectsAndBreak && (int)m_iCurMusicPos <= breakEvent.startTime;

                if(!isBetweenHitobjectsAndBreak ||
                   (drainAfterLastHitobjectBeforeBreakStart && isLastHitobjectBeforeBreakStart)) {
                    // special case: spinner nerf
                    f64 spinnerDrainNerf = m_bIsSpinnerActive ? 0.25 : 1.0;
                    addHealth(-m_fDrainRate * engine->getFrameTime() * (f64)speed * spinnerDrainNerf, false);
                }
            }
        }
    }

    // revive in mp
    if(m_fHealth > 0.999 && live_score.isDead()) live_score.setDead(false);

    // update auto (after having updated the hitobjects)
    if(osu->getModAuto() || osu->getModAutopilot()) updateAutoCursorPos();

    // spinner detection (used by osu!stable drain, and by HUD for not drawing the hiterrorbar)
    if(m_currentHitObject != NULL) {
        Spinner *spinnerPointer = dynamic_cast<Spinner *>(m_currentHitObject);
        if(spinnerPointer != NULL && m_iCurMusicPos > m_currentHitObject->getTime() &&
           m_iCurMusicPos < m_currentHitObject->getTime() + m_currentHitObject->getDuration())
            m_bIsSpinnerActive = true;
        else
            m_bIsSpinnerActive = false;
    }

    // full alternate mod lenience
    if(osu_mod_fullalternate) {
        if(m_bInBreak || m_bIsSpinnerActive || m_iCurrentHitObjectIndex < 1)
            m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = m_iCurrentHitObjectIndex + 1;
    }
}

Vector2 SimulatedBeatmap::pixels2OsuCoords(Vector2 pixelCoords) const { return pixelCoords; }

Vector2 SimulatedBeatmap::osuCoords2Pixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;

    // wobble
    if(osu_mod_wobble) {
        const f32 speedMultiplierCompensation = 1.0f / speed;
        coords.x += std::sin((m_iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation * osu_mod_wobble_frequency) *
                    osu_mod_wobble_strength;
        coords.y += std::sin((m_iCurMusicPos / 1000.0f) * 4 * speedMultiplierCompensation * osu_mod_wobble_frequency) *
                    osu_mod_wobble_strength;
    }

    // wobble2
    if(osu_mod_wobble2) {
        const f32 speedMultiplierCompensation = 1.0f / speed;
        Vector2 centerDelta = coords - Vector2(GameRules::OSU_COORD_WIDTH, GameRules::OSU_COORD_HEIGHT) / 2;
        coords.x += centerDelta.x * 0.25f *
                    std::sin((m_iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation * osu_mod_wobble_frequency) *
                    osu_mod_wobble_strength;
        coords.y += centerDelta.y * 0.25f *
                    std::sin((m_iCurMusicPos / 1000.0f) * 3 * speedMultiplierCompensation * osu_mod_wobble_frequency) *
                    osu_mod_wobble_strength;
    }

    // if wobble, clamp coordinates
    if(osu_mod_wobble || osu_mod_wobble2) {
        coords.x = clamp<f32>(coords.x, 0.0f, GameRules::OSU_COORD_WIDTH);
        coords.y = clamp<f32>(coords.y, 0.0f, GameRules::OSU_COORD_HEIGHT);
    }

    return coords;
}

Vector2 SimulatedBeatmap::osuCoords2RawPixels(Vector2 coords) const { return coords; }

Vector2 SimulatedBeatmap::osuCoords2LegacyPixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;

    // VR center
    coords.x -= GameRules::OSU_COORD_WIDTH / 2;
    coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

    return coords;
}

Vector2 SimulatedBeatmap::getCursorPos() const {
    // TODO @kiwec
    return m_interpolatedMousePos;
}

Vector2 SimulatedBeatmap::getFirstPersonCursorDelta() const {
    return m_vPlayfieldCenter - (osu->getModAuto() || osu->getModAutopilot() ? m_vAutoCursorPos : getCursorPos());
}

void SimulatedBeatmap::updateAutoCursorPos() {
    m_vAutoCursorPos = m_vPlayfieldCenter;
    m_vAutoCursorPos.y *= 2.5f;  // start moving in offscreen from bottom

    if(m_hitobjects.size() < 1) {
        m_vAutoCursorPos = m_vPlayfieldCenter;
        return;
    }

    // general
    long prevTime = 0;
    long nextTime = m_hitobjects[0]->getTime();
    Vector2 prevPos = m_vAutoCursorPos;
    Vector2 curPos = m_vAutoCursorPos;
    Vector2 nextPos = m_vAutoCursorPos;
    bool haveCurPos = false;

    if(osu->getModAuto()) {
        for(int i = 0; i < m_hitobjects.size(); i++) {
            HitObject *o = m_hitobjects[i];

            // get previous object
            if(o->getTime() <= m_iCurMusicPos) {
                prevTime = o->getTime() + o->getDuration();
                prevPos = o->getAutoCursorPos(m_iCurMusicPos);
                if(o->getDuration() > 0 && m_iCurMusicPos - o->getTime() <= o->getDuration()) {
                    haveCurPos = true;
                    curPos = prevPos;
                    break;
                }
            }

            // get next object
            if(o->getTime() > m_iCurMusicPos) {
                nextPos = o->getAutoCursorPos(m_iCurMusicPos);
                nextTime = o->getTime();
                break;
            }
        }
    } else if(osu->getModAutopilot()) {
        for(int i = 0; i < m_hitobjects.size(); i++) {
            HitObject *o = m_hitobjects[i];

            // get previous object
            if(o->isFinished() ||
               (m_iCurMusicPos > o->getTime() + o->getDuration() + (long)(getHitWindow50() * osu_autopilot_lenience))) {
                prevTime = o->getTime() + o->getDuration() + o->getAutopilotDelta();
                prevPos = o->getAutoCursorPos(m_iCurMusicPos);
            } else if(!o->isFinished())  // get next object
            {
                nextPos = o->getAutoCursorPos(m_iCurMusicPos);
                nextTime = o->getTime();

                // wait for the user to click
                if(m_iCurMusicPos >= nextTime + o->getDuration()) {
                    haveCurPos = true;
                    curPos = nextPos;

                    // long delta = m_iCurMusicPos - (nextTime + o->getDuration());
                    o->setAutopilotDelta(m_iCurMusicPos - (nextTime + o->getDuration()));
                } else if(o->getDuration() > 0 && m_iCurMusicPos >= nextTime)  // handle objects with duration
                {
                    haveCurPos = true;
                    curPos = nextPos;
                    o->setAutopilotDelta(0);
                }

                break;
            }
        }
    }

    if(haveCurPos)  // in active hitObject
        m_vAutoCursorPos = curPos;
    else {
        // interpolation
        f32 percent = 1.0f;
        if((nextTime == 0 && prevTime == 0) || (nextTime - prevTime) == 0)
            percent = 1.0f;
        else
            percent = (f32)((long)m_iCurMusicPos - prevTime) / (f32)(nextTime - prevTime);

        percent = clamp<f32>(percent, 0.0f, 1.0f);

        // scaled distance (not osucoords)
        f32 distance = (nextPos - prevPos).length();
        if(distance > m_fHitcircleDiameter * 1.05f)  // snap only if not in a stream (heuristic)
        {
            percent = 1.f;
        }

        m_vAutoCursorPos = prevPos + (nextPos - prevPos) * percent;
    }
}

void SimulatedBeatmap::updatePlayfieldMetrics() {
    m_vPlayfieldSize = GameRules::getPlayfieldSize();
    m_vPlayfieldCenter = GameRules::getPlayfieldCenter();
}

void SimulatedBeatmap::updateHitobjectMetrics() {
    m_fRawHitcircleDiameter = GameRules::getRawHitCircleDiameter(getCS());
    m_fXMultiplier = GameRules::getHitCircleXMultiplier();
    m_fHitcircleDiameter = GameRules::getRawHitCircleDiameter(getCS()) * GameRules::getHitCircleXMultiplier();

    const f32 followcircle_size_multiplier = 2.4f;
    const f32 sliderFollowCircleDiameterMultiplier =
        (osu->getModNightmare() || osu_mod_jigsaw2
             ? (1.0f * (1.0f - osu_mod_jigsaw_followcircle_radius_factor) +
                osu_mod_jigsaw_followcircle_radius_factor * followcircle_size_multiplier)
             : followcircle_size_multiplier);
    m_fSliderFollowCircleDiameter = m_fHitcircleDiameter * sliderFollowCircleDiameterMultiplier;
}

void SimulatedBeatmap::calculateStacks() {
    updateHitobjectMetrics();

    debugLog("Beatmap: Calculating stacks ...\n");

    // reset
    for(int i = 0; i < m_hitobjects.size(); i++) {
        m_hitobjects[i]->setStack(0);
    }

    const f32 STACK_LENIENCE = 3.0f;
    const f32 STACK_OFFSET = 0.05f;

    const f32 approachTime = GameRules::mapDifficultyRange(
        getAR(), GameRules::getMinApproachTime(), GameRules::getMidApproachTime(), GameRules::getMaxApproachTime());
    const f32 stackLeniency = m_selectedDifficulty2->getStackLeniency();

    if(m_selectedDifficulty2->getVersion() > 5) {
        // peppy's algorithm
        // https://gist.github.com/peppy/1167470

        for(int i = m_hitobjects.size() - 1; i >= 0; i--) {
            int n = i;

            HitObject *objectI = m_hitobjects[i];

            bool isSpinner = dynamic_cast<Spinner *>(objectI) != NULL;

            if(objectI->getStack() != 0 || isSpinner) continue;

            bool isHitCircle = dynamic_cast<Circle *>(objectI) != NULL;
            bool isSlider = dynamic_cast<Slider *>(objectI) != NULL;

            if(isHitCircle) {
                while(--n >= 0) {
                    HitObject *objectN = m_hitobjects[n];

                    bool isSpinnerN = dynamic_cast<Spinner *>(objectN);

                    if(isSpinnerN) continue;

                    if(objectI->getTime() - (approachTime * stackLeniency) >
                       (objectN->getTime() + objectN->getDuration()))
                        break;

                    Vector2 objectNEndPosition =
                        objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration());
                    if(objectN->getDuration() != 0 &&
                       (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->getTime())).length() <
                           STACK_LENIENCE) {
                        int offset = objectI->getStack() - objectN->getStack() + 1;
                        for(int j = n + 1; j <= i; j++) {
                            if((objectNEndPosition - m_hitobjects[j]->getOriginalRawPosAt(m_hitobjects[j]->getTime()))
                                   .length() < STACK_LENIENCE)
                                m_hitobjects[j]->setStack(m_hitobjects[j]->getStack() - offset);
                        }

                        break;
                    }

                    if((objectN->getOriginalRawPosAt(objectN->getTime()) -
                        objectI->getOriginalRawPosAt(objectI->getTime()))
                           .length() < STACK_LENIENCE) {
                        objectN->setStack(objectI->getStack() + 1);
                        objectI = objectN;
                    }
                }
            } else if(isSlider) {
                while(--n >= 0) {
                    HitObject *objectN = m_hitobjects[n];

                    bool isSpinner = dynamic_cast<Spinner *>(objectN) != NULL;

                    if(isSpinner) continue;

                    if(objectI->getTime() - (approachTime * stackLeniency) > objectN->getTime()) break;

                    if(((objectN->getDuration() != 0
                             ? objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration())
                             : objectN->getOriginalRawPosAt(objectN->getTime())) -
                        objectI->getOriginalRawPosAt(objectI->getTime()))
                           .length() < STACK_LENIENCE) {
                        objectN->setStack(objectI->getStack() + 1);
                        objectI = objectN;
                    }
                }
            }
        }
    } else  // getSelectedDifficulty()->version < 6
    {
        // old stacking algorithm for old beatmaps
        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Beatmaps/BeatmapProcessor.cs

        for(int i = 0; i < m_hitobjects.size(); i++) {
            HitObject *currHitObject = m_hitobjects[i];
            Slider *sliderPointer = dynamic_cast<Slider *>(currHitObject);

            const bool isSlider = (sliderPointer != NULL);

            if(currHitObject->getStack() != 0 && !isSlider) continue;

            long startTime = currHitObject->getTime() + currHitObject->getDuration();
            int sliderStack = 0;

            for(int j = i + 1; j < m_hitobjects.size(); j++) {
                HitObject *objectJ = m_hitobjects[j];

                if(objectJ->getTime() - (approachTime * stackLeniency) > startTime) break;

                // "The start position of the hitobject, or the position at the end of the path if the hitobject is a
                // slider"
                Vector2 position2 =
                    isSlider
                        ? sliderPointer->getOriginalRawPosAt(sliderPointer->getTime() + sliderPointer->getDuration())
                        : currHitObject->getOriginalRawPosAt(currHitObject->getTime());

                if((objectJ->getOriginalRawPosAt(objectJ->getTime()) -
                    currHitObject->getOriginalRawPosAt(currHitObject->getTime()))
                       .length() < 3) {
                    currHitObject->setStack(currHitObject->getStack() + 1);
                    startTime = objectJ->getTime() + objectJ->getDuration();
                } else if((objectJ->getOriginalRawPosAt(objectJ->getTime()) - position2).length() < 3) {
                    // "Case for sliders - bump notes down and right, rather than up and left."
                    sliderStack++;
                    objectJ->setStack(objectJ->getStack() - sliderStack);
                    startTime = objectJ->getTime() + objectJ->getDuration();
                }
            }
        }
    }

    // update hitobject positions
    f32 stackOffset = m_fRawHitcircleDiameter * STACK_OFFSET;
    for(int i = 0; i < m_hitobjects.size(); i++) {
        if(m_hitobjects[i]->getStack() != 0) m_hitobjects[i]->updateStackPosition(stackOffset);
    }
}

void SimulatedBeatmap::computeDrainRate() {
    m_fDrainRate = 0.0;
    m_fHpMultiplierNormal = 1.0;
    m_fHpMultiplierComboEnd = 1.0;

    if(m_hitobjects.size() < 1) return;

    debugLog("Beatmap: Calculating drain ...\n");

    {
        // see https://github.com/ppy/osu-iPhone/blob/master/Classes/OsuPlayer.m
        // see calcHPDropRate() @ https://github.com/ppy/osu-iPhone/blob/master/Classes/OsuFiletype.m#L661

        // NOTE: all drain changes between 2014 and today have been fixed here (the link points to an old version of the
        // algorithm!) these changes include: passive spinner nerf (drain * 0.25 while spinner is active), and clamping
        // the object length drain to 0 + an extra check for that (see maxLongObjectDrop) see
        // https://osu.ppy.sh/home/changelog/stable40/20190513.2

        struct TestPlayer {
            TestPlayer(f64 hpBarMaximum) {
                this->hpBarMaximum = hpBarMaximum;

                hpMultiplierNormal = 1.0;
                hpMultiplierComboEnd = 1.0;

                resetHealth();
            }

            void resetHealth() {
                health = hpBarMaximum;
                healthUncapped = hpBarMaximum;
            }

            void increaseHealth(f64 amount) {
                healthUncapped += amount;
                health += amount;

                if(health > hpBarMaximum) health = hpBarMaximum;

                if(health < 0.0) health = 0.0;

                if(healthUncapped < 0.0) healthUncapped = 0.0;
            }

            void decreaseHealth(f64 amount) {
                health -= amount;

                if(health < 0.0) health = 0.0;

                if(health > hpBarMaximum) health = hpBarMaximum;

                healthUncapped -= amount;

                if(healthUncapped < 0.0) healthUncapped = 0.0;
            }

            f64 hpBarMaximum;

            f64 health;
            f64 healthUncapped;

            f64 hpMultiplierNormal;
            f64 hpMultiplierComboEnd;
        };
        TestPlayer testPlayer(200.f);

        const f64 HP = getHP();
        const int version = m_selectedDifficulty2->getVersion();

        f64 testDrop = 0.05;

        const f64 lowestHpEver = GameRules::mapDifficultyRangeDouble(HP, 195.0, 160.0, 60.0);
        const f64 lowestHpComboEnd = GameRules::mapDifficultyRangeDouble(HP, 198.0, 170.0, 80.0);
        const f64 lowestHpEnd = GameRules::mapDifficultyRangeDouble(HP, 198.0, 180.0, 80.0);
        const f64 HpRecoveryAvailable = GameRules::mapDifficultyRangeDouble(HP, 8.0, 4.0, 0.0);

        bool fail = false;

        do {
            testPlayer.resetHealth();

            f64 lowestHp = testPlayer.health;
            int lastTime = (int)(m_hitobjects[0]->getTime() - (long)getApproachTime());
            fail = false;

            const int breakCount = m_breaks.size();
            int breakNumber = 0;

            int comboTooLowCount = 0;

            for(int i = 0; i < m_hitobjects.size(); i++) {
                const HitObject *h = m_hitobjects[i];
                const Slider *sliderPointer = dynamic_cast<const Slider *>(h);
                const Spinner *spinnerPointer = dynamic_cast<const Spinner *>(h);

                const int localLastTime = lastTime;

                int breakTime = 0;
                if(breakCount > 0 && breakNumber < breakCount) {
                    const DatabaseBeatmap::BREAK &e = m_breaks[breakNumber];
                    if(e.startTime >= localLastTime && e.endTime <= h->getTime()) {
                        // consider break start equal to object end time for version 8+ since drain stops during this
                        // time
                        breakTime = (version < 8) ? (e.endTime - e.startTime) : (e.endTime - localLastTime);
                        breakNumber++;
                    }
                }

                testPlayer.decreaseHealth(testDrop * (h->getTime() - lastTime - breakTime));

                lastTime = (int)(h->getTime() + h->getDuration());

                if(testPlayer.health < lowestHp) lowestHp = testPlayer.health;

                if(testPlayer.health > lowestHpEver) {
                    const f64 longObjectDrop = testDrop * (f64)h->getDuration();
                    const f64 maxLongObjectDrop = max(0.0, longObjectDrop - testPlayer.health);

                    testPlayer.decreaseHealth(longObjectDrop);

                    // nested hitobjects
                    if(sliderPointer != NULL) {
                        // startcircle
                        testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                            LiveScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal,
                            testPlayer.hpMultiplierComboEnd, 1.0));  // slider30

                        // ticks + repeats + repeat ticks
                        const std::vector<Slider::SLIDERCLICK> &clicks = sliderPointer->getClicks();
                        for(int c = 0; c < clicks.size(); c++) {
                            switch(clicks[c].type) {
                                case 0:  // repeat
                                    testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                                        LiveScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal,
                                        testPlayer.hpMultiplierComboEnd, 1.0));  // slider30
                                    break;
                                case 1:  // tick
                                    testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                                        LiveScore::HIT::HIT_SLIDER10, HP, testPlayer.hpMultiplierNormal,
                                        testPlayer.hpMultiplierComboEnd, 1.0));  // slider10
                                    break;
                            }
                        }

                        // endcircle
                        testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                            LiveScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal,
                            testPlayer.hpMultiplierComboEnd, 1.0));  // slider30
                    } else if(spinnerPointer != NULL) {
                        const int rotationsNeeded = (int)((f32)spinnerPointer->getDuration() / 1000.0f *
                                                          GameRules::getSpinnerSpinsPerSecond(this));
                        for(int r = 0; r < rotationsNeeded; r++) {
                            testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                                LiveScore::HIT::HIT_SPINNERSPIN, HP, testPlayer.hpMultiplierNormal,
                                testPlayer.hpMultiplierComboEnd, 1.0));  // spinnerspin
                        }
                    }

                    if(!(maxLongObjectDrop > 0.0) || (testPlayer.health - maxLongObjectDrop) > lowestHpEver) {
                        // regular hit (for every hitobject)
                        testPlayer.increaseHealth(
                            LiveScore::getHealthIncrease(LiveScore::HIT::HIT_300, HP, testPlayer.hpMultiplierNormal,
                                                         testPlayer.hpMultiplierComboEnd, 1.0));  // 300

                        // end of combo (new combo starts at next hitobject)
                        if((i == m_hitobjects.size() - 1) || m_hitobjects[i]->isEndOfCombo()) {
                            testPlayer.increaseHealth(LiveScore::getHealthIncrease(
                                LiveScore::HIT::HIT_300G, HP, testPlayer.hpMultiplierNormal,
                                testPlayer.hpMultiplierComboEnd, 1.0));  // geki

                            if(testPlayer.health < lowestHpComboEnd) {
                                if(++comboTooLowCount > 2) {
                                    testPlayer.hpMultiplierComboEnd *= 1.07;
                                    testPlayer.hpMultiplierNormal *= 1.03;
                                    fail = true;
                                    break;
                                }
                            }
                        }

                        continue;
                    }

                    fail = true;
                    testDrop *= 0.96;
                    break;
                }

                fail = true;
                testDrop *= 0.96;
                break;
            }

            if(!fail && testPlayer.health < lowestHpEnd) {
                fail = true;
                testDrop *= 0.94;
                testPlayer.hpMultiplierComboEnd *= 1.01;
                testPlayer.hpMultiplierNormal *= 1.01;
            }

            const f64 recovery = (testPlayer.healthUncapped - testPlayer.hpBarMaximum) / (f64)m_hitobjects.size();
            if(!fail && recovery < HpRecoveryAvailable) {
                fail = true;
                testDrop *= 0.96;
                testPlayer.hpMultiplierComboEnd *= 1.02;
                testPlayer.hpMultiplierNormal *= 1.01;
            }
        } while(fail);

        m_fDrainRate =
            (testDrop / testPlayer.hpBarMaximum) * 1000.0;  // from [0, 200] to [0, 1], and from ms to seconds
        m_fHpMultiplierComboEnd = testPlayer.hpMultiplierComboEnd;
        m_fHpMultiplierNormal = testPlayer.hpMultiplierNormal;
    }
}
