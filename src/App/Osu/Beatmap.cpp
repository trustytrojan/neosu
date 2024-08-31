#include "Beatmap.h"

#include <string.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <future>
#include <limits>
#include <sstream>

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "BanchoSubmitter.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "Chat.h"
#include "Circle.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "Engine.h"
#include "Environment.h"
#include "GameRules.h"
#include "HUD.h"
#include "HitObject.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "LegacyReplay.h"
#include "MainMenu.h"
#include "ModFPoSu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "PauseMenu.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "RoomScreen.h"
#include "Skin.h"
#include "SkinImage.h"
#include "Slider.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "Spinner.h"
#include "UIModSelectorModButton.h"

using namespace std;

Beatmap::Beatmap() {
    // vars
    m_bIsPlaying = false;
    m_bIsPaused = false;
    m_bIsWaiting = false;
    m_bIsRestartScheduled = false;
    m_bIsRestartScheduledQuick = false;

    m_bIsInSkippableSection = false;
    m_bShouldFlashWarningArrows = false;
    m_fShouldFlashSectionPass = 0.0f;
    m_fShouldFlashSectionFail = 0.0f;
    m_bContinueScheduled = false;
    m_iContinueMusicPos = 0;
    m_fWaitTime = 0.0f;

    m_selectedDifficulty2 = NULL;

    m_music = NULL;

    m_fMusicFrequencyBackup = 0.f;
    m_iCurMusicPos = 0;
    m_iCurMusicPosWithOffsets = 0;
    m_bWasSeekFrame = false;
    m_fInterpolatedMusicPos = 0.0;
    m_fLastAudioTimeAccurateSet = 0.0;
    m_fLastRealTimeForInterpolationDelta = 0.0;
    m_iResourceLoadUpdateDelayHack = 0;
    m_bForceStreamPlayback = true;  // if this is set to true here, then the music will always be loaded as a stream
                                    // (meaning slow disk access could cause audio stalling/stuttering)
    m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
    m_bIsFirstMissSound = true;

    m_bFailed = false;
    m_fFailAnim = 1.0f;
    m_fHealth = 1.0;
    m_fHealth2 = 1.0f;

    m_fDrainRate = 0.0;

    m_fBreakBackgroundFade = 0.0f;
    m_bInBreak = false;
    m_currentHitObject = NULL;
    m_iNextHitObjectTime = 0;
    m_iPreviousHitObjectTime = 0;
    m_iPreviousSectionPassFailTime = -1;

    m_bClick1Held = false;
    m_bClick2Held = false;
    m_bClickedContinue = false;
    m_bPrevKeyWasKey1 = false;
    m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;

    m_iNPS = 0;
    m_iND = 0;
    m_iCurrentHitObjectIndex = 0;
    m_iCurrentNumCircles = 0;
    m_iCurrentNumSliders = 0;
    m_iCurrentNumSpinners = 0;

    m_iPreviousFollowPointObjectIndex = -1;

    m_bIsSpinnerActive = false;

    m_fPlayfieldRotation = 0.0f;
    m_fScaleFactor = 1.0f;

    m_fXMultiplier = 1.0f;
    m_fNumberScale = 1.0f;
    m_fHitcircleOverlapScale = 1.0f;
    m_fRawHitcircleDiameter = 27.35f * 2.0f;
    m_fSliderFollowCircleDiameter = 0.0f;

    m_iAutoCursorDanceIndex = 0;

    m_fAimStars = 0.0f;
    m_fAimSliderFactor = 0.0f;
    m_fSpeedStars = 0.0f;
    m_fSpeedNotes = 0.0f;

    m_bWasHREnabled = false;
    m_fPrevHitCircleDiameter = 0.0f;
    m_bWasHorizontalMirrorEnabled = false;
    m_bWasVerticalMirrorEnabled = false;
    m_bWasEZEnabled = false;
    m_bWasMafhamEnabled = false;
    m_fPrevPlayfieldRotationFromConVar = 0.0f;
    m_bIsPreLoading = true;
    m_iPreLoadingIndex = 0;

    m_mafhamActiveRenderTarget = NULL;
    m_mafhamFinishedRenderTarget = NULL;
    m_bMafhamRenderScheduled = true;
    m_iMafhamHitObjectRenderIndex = 0;
    m_iMafhamPrevHitObjectIndex = 0;
    m_iMafhamActiveRenderHitObjectIndex = 0;
    m_iMafhamFinishedRenderHitObjectIndex = 0;
    m_bInMafhamRenderChunk = false;
}

Beatmap::~Beatmap() {
    anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
    anim->deleteExistingAnimation(&m_fHealth2);
    anim->deleteExistingAnimation(&m_fFailAnim);

    unloadObjects();
}

void Beatmap::drawDebug(Graphics *g) {
    if(cv_debug_draw_timingpoints.getBool()) {
        McFont *debugFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
        g->setColor(0xffffffff);
        g->pushTransform();
        g->translate(5, debugFont->getHeight() + 5 - getMousePos().y);
        {
            for(const DatabaseBeatmap::TIMINGPOINT &t : m_selectedDifficulty2->getTimingpoints()) {
                g->drawString(debugFont, UString::format("%li,%f,%i,%i,%i", t.offset, t.msPerBeat, t.sampleType,
                                                         t.sampleSet, t.volume));
                g->translate(0, debugFont->getHeight());
            }
        }
        g->popTransform();
    }
}

void Beatmap::drawBackground(Graphics *g) {
    if(!canDraw()) return;

    // draw beatmap background image
    {
        Image *backgroundImage = osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_selectedDifficulty2);
        if(cv_draw_beatmap_background_image.getBool() && backgroundImage != NULL &&
           (cv_background_dim.getFloat() < 1.0f || m_fBreakBackgroundFade > 0.0f)) {
            const f32 scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());
            const Vector2 centerTrans = (osu->getScreenSize() / 2);

            const f32 backgroundFadeDimMultiplier =
                clamp<f32>(1.0f - (cv_background_dim.getFloat() - 0.3f), 0.0f, 1.0f);
            const short dim =
                clamp<f32>((1.0f - cv_background_dim.getFloat()) + m_fBreakBackgroundFade * backgroundFadeDimMultiplier,
                           0.0f, 1.0f) *
                255.0f;

            g->setColor(COLOR(255, dim, dim, dim));
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate((int)centerTrans.x, (int)centerTrans.y);
                g->drawImage(backgroundImage);
            }
            g->popTransform();
        }
    }

    // draw background
    if(cv_background_brightness.getFloat() > 0.0f) {
        const f32 brightness = cv_background_brightness.getFloat();
        const short red = clamp<f32>(brightness * cv_background_color_r.getFloat(), 0.0f, 255.0f);
        const short green = clamp<f32>(brightness * cv_background_color_g.getFloat(), 0.0f, 255.0f);
        const short blue = clamp<f32>(brightness * cv_background_color_b.getFloat(), 0.0f, 255.0f);
        const short alpha = clamp<f32>(1.0f - m_fBreakBackgroundFade, 0.0f, 1.0f) * 255.0f;
        g->setColor(COLOR(alpha, red, green, blue));
        g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());
    }

    // draw scorebar-bg
    if(cv_draw_hud.getBool() && cv_draw_scorebarbg.getBool() &&
       (!cv_mod_fposu.getBool() || (!cv_fposu_draw_scorebarbg_on_top.getBool())))  // NOTE: special case for FPoSu
        osu->getHUD()->drawScorebarBg(
            g, cv_hud_scorebar_hide_during_breaks.getBool() ? (1.0f - m_fBreakBackgroundFade) : 1.0f,
            osu->getHUD()->getScoreBarBreakAnim());

    if(cv_debug.getBool()) {
        int y = 50;

        if(m_bIsPaused) {
            g->setColor(0xffffffff);
            g->fillRect(50, y, 15, 50);
            g->fillRect(50 + 50 - 15, y, 15, 50);
        }

        y += 100;

        if(m_bIsWaiting) {
            g->setColor(0xff00ff00);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(m_bIsPlaying) {
            g->setColor(0xff0000ff);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(m_bForceStreamPlayback) {
            g->setColor(0xffff0000);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(m_hitobjectsSortedByEndTime.size() > 0) {
            HitObject *lastHitObject = m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1];
            if(lastHitObject->isFinished() && m_iCurMusicPos > lastHitObject->getTime() + lastHitObject->getDuration() +
                                                                   (long)cv_end_skip_time.getInt()) {
                g->setColor(0xff00ffff);
                g->fillRect(50, y, 50, 50);
            }

            y += 100;
        }
    }
}

void Beatmap::onKeyDown(KeyboardEvent &e) {
    if(e == KEY_O && engine->getKeyboard()->isControlDown()) {
        osu->toggleOptionsMenu();
        e.consume();
    }
}

void Beatmap::skipEmptySection() {
    if(!m_bIsInSkippableSection) return;
    m_bIsInSkippableSection = false;
    osu->m_chat->updateVisibility();

    const f32 offset = 2500.0f;
    f32 offsetMultiplier = osu->getSpeedMultiplier();
    {
        // only compensate if not within "normal" osu mod range (would make the game feel too different regarding time
        // from skip until first hitobject)
        if(offsetMultiplier >= 0.74f && offsetMultiplier <= 1.51f) offsetMultiplier = 1.0f;

        // don't compensate speed increases at all actually
        if(offsetMultiplier > 1.0f) offsetMultiplier = 1.0f;

        // and cap slowdowns at sane value (~ spinner fadein start)
        if(offsetMultiplier <= 0.2f) offsetMultiplier = 0.2f;
    }

    const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;

    if(!cv_end_skip.getBool() && nextHitObjectDelta < 0) {
        m_music->setPositionMS(max(m_music->getLengthMS(), (u32)1) - 1);
        m_bWasSeekFrame = true;
    } else {
        m_music->setPositionMS(max(m_iNextHitObjectTime - (long)(offset * offsetMultiplier), (long)0));
        m_bWasSeekFrame = true;
    }

    engine->getSound()->play(osu->getSkin()->getMenuHit());

    if(!bancho.spectators.empty()) {
        // TODO @kiwec: how does skip work? does client jump to latest time or just skip beginning?
        //              is signaling skip even necessary?
        broadcast_spectator_frames();

        Packet packet;
        packet.id = OUT_SPECTATE_FRAMES;
        write<i32>(&packet, 0);
        write<u16>(&packet, 0);
        write<u8>(&packet, LiveReplayBundle::Action::SKIP);
        write<ScoreFrame>(&packet, ScoreFrame::get());
        write<u16>(&packet, spectator_sequence++);
        send_packet(packet);
    }
}

void Beatmap::keyPressed1(bool mouse) {
    if(is_watching || is_spectating) return;

    if(m_bContinueScheduled) m_bClickedContinue = !osu->getModSelector()->isMouseInside();

    if(cv_mod_fullalternate.getBool() && m_bPrevKeyWasKey1) {
        if(m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex) {
            engine->getSound()->play(getSkin()->getCombobreak());
            return;
        }
    }

    // key overlay & counter
    osu->getHUD()->animateInputoverlay(mouse ? 3 : 1, true);

    if(m_bFailed) return;

    if(!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying) osu->getScore()->addKeyCount(mouse ? 3 : 1);

    m_bPrevKeyWasKey1 = true;
    m_bClick1Held = true;

    if((!osu->getModAuto() && !osu->getModRelax()) || !cv_auto_and_relax_block_user_input.getBool())
        m_clicks.push_back(Click{
            .tms = m_iCurMusicPosWithOffsets,
            .pos = getCursorPos(),
        });

    if(mouse) {
        current_keys = current_keys | LegacyReplay::M1;
    } else {
        current_keys = current_keys | LegacyReplay::M1 | LegacyReplay::K1;
    }
}

void Beatmap::keyPressed2(bool mouse) {
    if(is_watching || is_spectating) return;

    if(m_bContinueScheduled) m_bClickedContinue = !osu->getModSelector()->isMouseInside();

    if(cv_mod_fullalternate.getBool() && !m_bPrevKeyWasKey1) {
        if(m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex) {
            engine->getSound()->play(getSkin()->getCombobreak());
            return;
        }
    }

    // key overlay & counter
    osu->getHUD()->animateInputoverlay(mouse ? 4 : 2, true);

    if(m_bFailed) return;

    if(!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying) osu->getScore()->addKeyCount(mouse ? 4 : 2);

    m_bPrevKeyWasKey1 = false;
    m_bClick2Held = true;

    if((!osu->getModAuto() && !osu->getModRelax()) || !cv_auto_and_relax_block_user_input.getBool())
        m_clicks.push_back(Click{
            .tms = m_iCurMusicPosWithOffsets,
            .pos = getCursorPos(),
        });

    if(mouse) {
        current_keys = current_keys | LegacyReplay::M2;
    } else {
        current_keys = current_keys | LegacyReplay::M2 | LegacyReplay::K2;
    }
}

void Beatmap::keyReleased1(bool mouse) {
    if(is_watching || is_spectating) return;

    // key overlay
    osu->getHUD()->animateInputoverlay(1, false);
    osu->getHUD()->animateInputoverlay(3, false);

    m_bClick1Held = false;

    current_keys = current_keys & ~(LegacyReplay::M1 | LegacyReplay::K1);
}

void Beatmap::keyReleased2(bool mouse) {
    if(is_watching || is_spectating) return;

    // key overlay
    osu->getHUD()->animateInputoverlay(2, false);
    osu->getHUD()->animateInputoverlay(4, false);

    m_bClick2Held = false;

    current_keys = current_keys & ~(LegacyReplay::M2 | LegacyReplay::K2);
}

void Beatmap::select() {
    // if possible, continue playing where we left off
    if(m_music != NULL && (m_music->isPlaying())) m_iContinueMusicPos = m_music->getPositionMS();

    selectDifficulty2(m_selectedDifficulty2);

    loadMusic();
    handlePreviewPlay();
}

void Beatmap::selectDifficulty2(DatabaseBeatmap *difficulty2) {
    if(difficulty2 != NULL) {
        m_selectedDifficulty2 = difficulty2;

        nb_hitobjects = difficulty2->getNumObjects();

        // need to recheck/reload the music here since every difficulty might be using a different sound file
        loadMusic();
        handlePreviewPlay();
    }

    if(cv_beatmap_preview_mods_live.getBool()) onModUpdate();
}

void Beatmap::deselect() {
    m_iContinueMusicPos = 0;
    m_selectedDifficulty2 = NULL;
    unloadObjects();
}

bool Beatmap::play() {
    is_watching = false;

    if(start()) {
        last_spectator_broadcast = engine->getTime();
        RichPresence::onPlayStart();
        if(!bancho.spectators.empty()) {
            Packet packet;
            packet.id = OUT_SPECTATE_FRAMES;
            write<i32>(&packet, 0);
            write<u16>(&packet, 0);  // 0 frames, we're just signaling map start
            write<u8>(&packet, LiveReplayBundle::Action::NEW_SONG);
            write<ScoreFrame>(&packet, ScoreFrame::get());
            write<u16>(&packet, spectator_sequence++);
            send_packet(packet);
        }

        return true;
    }

    return false;
}

bool Beatmap::watch(FinishedScore score, f64 start_percent) {
    if(score.replay.size() < 3) {
        // Replay is invalid
        return false;
    }

    m_bIsPlaying = false;
    m_bIsPaused = false;
    m_bContinueScheduled = false;
    unloadObjects();

    osu->previous_mods = osu->getModSelector()->getModSelection();

    osu->watched_user_name = score.playerName.c_str();
    osu->watched_user_id = score.player_id;
    is_watching = true;

    osu->useMods(&score);

    if(!start()) {
        // Map failed to load
        return false;
    }

    spectated_replay = score.replay;

    osu->m_songBrowser2->m_bHasSelectedAndIsPlaying = true;
    osu->m_songBrowser2->setVisible(false);

    // Don't seek to 0%, since it feels really bad to start immediately
    if(start_percent > 0.f) {
        seekPercent(start_percent);
    }

    return true;
}

bool Beatmap::spectate() {
    m_bIsPlaying = false;
    m_bIsPaused = false;
    m_bContinueScheduled = false;
    unloadObjects();

    auto user_info = get_user_info(bancho.spectated_player_id);
    osu->watched_user_id = bancho.spectated_player_id;
    osu->watched_user_name = user_info->name;
    is_spectating = true;

    osu->previous_mods = osu->getModSelector()->getModSelection();

    FinishedScore score;
    score.client = "peppy-unknown";
    score.server = bancho.endpoint.toUtf8();
    score.modsLegacy = user_info->mods;
    osu->useMods(&score);

    if(!start()) {
        // Map failed to load
        return false;
    }

    spectated_replay.clear();

    osu->m_songBrowser2->m_bHasSelectedAndIsPlaying = true;
    osu->m_songBrowser2->setVisible(false);
    return true;
}

bool Beatmap::start() {
    if(m_selectedDifficulty2 == NULL) return false;

    engine->getSound()->play(osu->m_skin->getMenuHit());

    osu->updateMods();

    // mp hack
    {
        osu->m_mainMenu->setVisible(false);
        osu->m_modSelector->setVisible(false);
        osu->m_optionsMenu->setVisible(false);
        osu->m_pauseMenu->setVisible(false);
    }

    // HACKHACK: stuck key quickfix
    {
        osu->m_bKeyboardKey1Down = false;
        osu->m_bKeyboardKey12Down = false;
        osu->m_bKeyboardKey2Down = false;
        osu->m_bKeyboardKey22Down = false;
        osu->m_bMouseKey1Down = false;
        osu->m_bMouseKey2Down = false;

        keyReleased1(false);
        keyReleased1(true);
        keyReleased2(false);
        keyReleased2(true);
    }

    static const int OSU_COORD_WIDTH = 512;
    static const int OSU_COORD_HEIGHT = 384;
    osu->flashlight_position = Vector2{OSU_COORD_WIDTH / 2, OSU_COORD_HEIGHT / 2};

    // reset everything, including deleting any previously loaded hitobjects from another diff which we might just have
    // played
    unloadObjects();
    resetScore();

    // some hitobjects already need this information to be up-to-date before their constructor is called
    updatePlayfieldMetrics();
    updateHitobjectMetrics();
    m_bIsPreLoading = false;

    // actually load the difficulty (and the hitobjects)
    {
        DatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = DatabaseBeatmap::loadGameplay(m_selectedDifficulty2, this);
        if(result.errorCode != 0) {
            switch(result.errorCode) {
                case 1: {
                    UString errorMessage = "Error: Couldn't load beatmap metadata :(";
                    debugLog("Osu Error: Couldn't load beatmap metadata %s\n",
                             m_selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
                } break;

                case 2: {
                    UString errorMessage = "Error: Couldn't load beatmap file :(";
                    debugLog("Osu Error: Couldn't load beatmap file %s\n",
                             m_selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
                } break;

                case 3: {
                    UString errorMessage = "Error: No timingpoints in beatmap :(";
                    debugLog("Osu Error: No timingpoints in beatmap %s\n",
                             m_selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
                } break;

                case 4: {
                    UString errorMessage = "Error: No hitobjects in beatmap :(";
                    debugLog("Osu Error: No hitobjects in beatmap %s\n", m_selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
                } break;

                case 5: {
                    UString errorMessage = "Error: Too many hitobjects in beatmap :(";
                    debugLog("Osu Error: Too many hitobjects in beatmap %s\n",
                             m_selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
                } break;
            }

            return false;
        }

        // move temp result data into beatmap
        m_hitobjects = std::move(result.hitobjects);
        m_breaks = std::move(result.breaks);
        osu->getSkin()->setBeatmapComboColors(std::move(result.combocolors));  // update combo colors in skin

        // load beatmap skin
        osu->getSkin()->loadBeatmapOverride(m_selectedDifficulty2->getFolder());
    }

    // the drawing order is different from the playing/input order.
    // for drawing, if multiple hitobjects occupy the exact same time (duration) then they get drawn on top of the
    // active hitobject
    m_hitobjectsSortedByEndTime = m_hitobjects;

    // sort hitobjects by endtime
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

    // start preloading (delays the play start until it's set to false, see isLoading())
    m_bIsPreLoading = true;
    m_iPreLoadingIndex = 0;

    // live pp/stars
    resetLiveStarsTasks();

    // load music
    if(cv_restart_sound_engine_before_playing.getBool()) {
        // HACKHACK: Reload sound engine before starting the song, as it starts lagging after a while
        //           (i haven't figured out the root cause yet)
        engine->getSound()->restart();

        // Restarting sound engine already reloads the music
    } else {
        unloadMusic();  // need to reload in case of speed/pitch changes (just to be sure)
        loadMusic(false);
    }

    m_music->setLoop(false);
    m_bIsPaused = false;
    m_bContinueScheduled = false;

    m_bInBreak = cv_background_fade_after_load.getBool();
    anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
    m_fBreakBackgroundFade = cv_background_fade_after_load.getBool() ? 1.0f : 0.0f;
    m_iPreviousSectionPassFailTime = -1;
    m_fShouldFlashSectionPass = 0.0f;
    m_fShouldFlashSectionFail = 0.0f;

    m_music->setPositionMS(0);
    m_iCurMusicPos = 0;

    // we are waiting for an asynchronous start of the beatmap in the next update()
    m_bIsPlaying = true;
    m_bIsWaiting = true;
    m_fWaitTime = engine->getTimeReal();

    cv_snd_change_check_interval.setValue(0.0f);

    if(osu->m_bModAuto || osu->m_bModAutopilot || is_watching || is_spectating) {
        osu->m_bShouldCursorBeVisible = true;
        env->setCursorVisible(osu->m_bShouldCursorBeVisible);
    }

    if(getSelectedDifficulty2()->getLocalOffset() != 0)
        osu->m_notificationOverlay->addNotification(
            UString::format("Using local beatmap offset (%ld ms)", getSelectedDifficulty2()->getLocalOffset()),
            0xffffffff, false, 0.75f);

    osu->m_fQuickSaveTime = 0.0f;  // reset

    osu->updateConfineCursor();
    osu->updateWindowsKeyDisable();

    engine->getSound()->play(osu->getSkin()->m_expand);

    // NOTE: loading failures are handled dynamically in update(), so temporarily assume everything has worked in here
    return true;
}

void Beatmap::restart(bool quick) {
    engine->getSound()->stop(getSkin()->getFailsound());

    if(!m_bIsWaiting) {
        m_bIsRestartScheduled = true;
        m_bIsRestartScheduledQuick = quick;
    } else if(m_bIsPaused) {
        pause(false);
    }
}

void Beatmap::actualRestart() {
    // reset everything
    resetScore();
    resetHitObjects(-1000);

    // we are waiting for an asynchronous start of the beatmap in the next update()
    m_bIsWaiting = true;
    m_fWaitTime = engine->getTimeReal();

    // if the first hitobject starts immediately, add artificial wait time before starting the music
    if(m_hitobjects.size() > 0) {
        if(m_hitobjects[0]->getTime() < (long)cv_early_note_time.getInt()) {
            m_bIsWaiting = true;
            m_fWaitTime = engine->getTimeReal() + cv_early_note_time.getFloat() / 1000.0f;
        }
    }

    // pause temporarily if playing
    if(m_music->isPlaying()) engine->getSound()->pause(m_music);

    // reset/restore frequency (from potential fail before)
    m_music->setFrequency(0);

    m_music->setLoop(false);
    m_bIsPaused = false;
    m_bContinueScheduled = false;

    m_bInBreak = false;
    anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
    m_fBreakBackgroundFade = 0.0f;
    m_iPreviousSectionPassFailTime = -1;
    m_fShouldFlashSectionPass = 0.0f;
    m_fShouldFlashSectionFail = 0.0f;

    onModUpdate();  // sanity

    // reset position
    m_music->setPositionMS(0);
    m_bWasSeekFrame = true;
    m_iCurMusicPos = 0;

    m_bIsPlaying = true;
    is_spectating = false;
    is_watching = false;
}

void Beatmap::pause(bool quitIfWaiting) {
    if(m_selectedDifficulty2 == NULL) return;
    if(is_spectating) {
        stop_spectating();
        return;
    }

    const bool isFirstPause = !m_bContinueScheduled;

    // NOTE: this assumes that no beatmap ever goes far beyond the end of the music
    // NOTE: if pure virtual audio time is ever supported (playing without SoundEngine) then this needs to be adapted
    // fix pausing after music ends breaking beatmap state (by just not allowing it to be paused)
    if(m_fAfterMusicIsFinishedVirtualAudioTimeStart >= 0.0f) {
        const f32 delta = engine->getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart;
        if(delta < 5.0f)  // WARNING: sanity limit, always allow escaping after 5 seconds of overflow time
            return;
    }

    if(m_bIsPlaying) {
        if(m_bIsWaiting && quitIfWaiting) {
            // if we are still m_bIsWaiting, pausing the game via the escape key is the
            // same as stopping playing
            stop();
        } else {
            // first time pause pauses the music
            // case 1: the beatmap is already "finished", jump to the ranking screen if some small amount of time past
            //         the last objects endTime
            // case 2: in the middle somewhere, pause as usual
            HitObject *lastHitObject = m_hitobjectsSortedByEndTime.size() > 0
                                           ? m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]
                                           : NULL;
            if(lastHitObject != NULL && lastHitObject->isFinished() &&
               (m_iCurMusicPos >
                lastHitObject->getTime() + lastHitObject->getDuration() + (long)cv_end_skip_time.getInt()) &&
               cv_end_skip.getBool()) {
                stop(false);
            } else {
                engine->getSound()->pause(m_music);
                m_bIsPlaying = false;
                m_bIsPaused = true;
            }
        }
    } else if(m_bIsPaused && !m_bContinueScheduled) {
        // if this is the first time unpausing
        if(osu->getModAuto() || osu->getModAutopilot() || m_bIsInSkippableSection || is_watching) {
            if(!m_bIsWaiting) {
                // only force play() if we were not early waiting
                engine->getSound()->play(m_music);
            }

            m_bIsPlaying = true;
            m_bIsPaused = false;
        } else {
            // otherwise, schedule a continue (wait for user to click, handled in update())
            // first time unpause schedules a continue
            m_bIsPaused = false;
            m_bContinueScheduled = true;
        }
    } else {
        // if this is not the first time pausing/unpausing, then just toggle the pause state (the visibility of the
        // pause menu is handled in the Osu class, a bit shit)
        m_bIsPaused = !m_bIsPaused;
    }

    if(m_bIsPaused && isFirstPause) {
        if(!cv_submit_after_pause.getBool()) {
            debugLog("Disabling score submission due to pausing\n");
            vanilla = false;
        }

        m_vContinueCursorPoint = getMousePos();
        if(cv_mod_fps.getBool()) {
            m_vContinueCursorPoint = GameRules::getPlayfieldCenter();
        }
    }

    // if we have failed, and the user early exits to the pause menu, stop the failing animation
    if(m_bFailed) anim->deleteExistingAnimation(&m_fFailAnim);
}

void Beatmap::pausePreviewMusic(bool toggle) {
    if(m_music != NULL) {
        if(m_music->isPlaying())
            engine->getSound()->pause(m_music);
        else if(toggle)
            engine->getSound()->play(m_music);
    }
}

bool Beatmap::isPreviewMusicPlaying() {
    if(m_music != NULL) return m_music->isPlaying();

    return false;
}

void Beatmap::stop(bool quit) {
    if(m_selectedDifficulty2 == NULL) return;

    if(getSkin()->getFailsound()->isPlaying()) engine->getSound()->stop(getSkin()->getFailsound());

    m_bIsPlaying = false;
    m_bIsPaused = false;
    m_bContinueScheduled = false;

    auto score = saveAndSubmitScore(quit);

    // Auto mod was "temporary" since it was set from Ctrl+Clicking a map, not from the mod selector
    if(osu->m_bModAutoTemp) {
        if(osu->m_modSelector->m_modButtonAuto->isOn()) {
            osu->m_modSelector->m_modButtonAuto->click();
        }
        osu->m_bModAutoTemp = false;
    }

    if(is_watching || is_spectating) {
        osu->getModSelector()->restoreMods(osu->previous_mods);
    }

    is_spectating = false;
    is_watching = false;
    spectated_replay.clear();

    unloadObjects();

    if(bancho.is_playing_a_multi_map()) {
        if(quit) {
            osu->onPlayEnd(score, true);
            osu->m_room->ragequit();
        } else {
            osu->m_room->onClientScoreChange(true);
            Packet packet;
            packet.id = FINISH_MATCH;
            send_packet(packet);
        }
    } else {
        osu->onPlayEnd(score, quit);
    }
}

void Beatmap::fail() {
    if(m_bFailed) return;
    if(is_watching) return;

    // Change behavior of relax mod when online
    if(bancho.is_online() && osu->getModRelax()) return;

    if(!bancho.is_playing_a_multi_map() && cv_drain_kill.getBool()) {
        engine->getSound()->play(getSkin()->getFailsound());

        m_bFailed = true;
        m_fFailAnim = 1.0f;
        anim->moveLinear(&m_fFailAnim, 0.0f, cv_fail_time.getFloat(),
                         true);  // trigger music slowdown and delayed menu, see update()
    } else if(!osu->getScore()->isDead()) {
        debugLog("Disabling score submission due to death\n");
        vanilla = false;

        anim->deleteExistingAnimation(&m_fHealth2);
        m_fHealth = 0.0;
        m_fHealth2 = 0.0f;

        if(cv_drain_kill_notification_duration.getFloat() > 0.0f) {
            if(!osu->getScore()->hasDied())
                osu->getNotificationOverlay()->addNotification("You have failed, but you can keep playing!", 0xffffffff,
                                                               false, cv_drain_kill_notification_duration.getFloat());
        }
    }

    if(!osu->getScore()->isDead()) osu->getScore()->setDead(true);
}

void Beatmap::cancelFailing() {
    if(!m_bFailed || m_fFailAnim <= 0.0f) return;

    m_bFailed = false;

    anim->deleteExistingAnimation(&m_fFailAnim);
    m_fFailAnim = 1.0f;

    if(m_music != NULL) m_music->setFrequency(0.0f);

    if(getSkin()->getFailsound()->isPlaying()) engine->getSound()->stop(getSkin()->getFailsound());
}

f32 Beatmap::getIdealVolume() {
    if(m_music == NULL) return 1.f;

    f32 volume = cv_volume_music.getFloat();
    if(!cv_normalize_loudness.getBool()) {
        return volume;
    }

    static std::string last_song = "";
    static f32 modifier = 1.f;

    if(m_selectedDifficulty2->getFullSoundFilePath() == last_song) {
        // We already did the calculation
        return volume * modifier;
    }

    auto decoder = BASS_StreamCreateFile(false, m_selectedDifficulty2->getFullSoundFilePath().c_str(), 0, 0,
                                         BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT);
    if(!decoder) {
        debugLog("BASS_StreamCreateFile() returned error %d on file %s\n", BASS_ErrorGetCode(),
                 m_selectedDifficulty2->getFullSoundFilePath().c_str());
        return volume;
    }

    f32 preview_point = m_selectedDifficulty2->getPreviewTime();
    if(preview_point < 0) preview_point = 0;
    const QWORD position = BASS_ChannelSeconds2Bytes(decoder, preview_point / 1000.0);
    if(!BASS_ChannelSetPosition(decoder, position, BASS_POS_BYTE)) {
        debugLog("BASS_ChannelSetPosition() returned error %d\n", BASS_ErrorGetCode());
        BASS_ChannelFree(decoder);
        return volume;
    }

    auto loudness = BASS_Loudness_Start(decoder, MAKELONG(BASS_LOUDNESS_INTEGRATED, 3000), 0);
    if(!loudness) {
        debugLog("BASS_Loudness_Start() returned error %d\n", BASS_ErrorGetCode());
        BASS_ChannelFree(decoder);
        return volume;
    }

    // Process 4 seconds of audio, should be enough for the loudness measurement
    f32 buf[44100 * 4];
    BASS_ChannelGetData(decoder, buf, sizeof(buf));

    f32 level = -13.f;
    BASS_Loudness_GetLevel(loudness, BASS_LOUDNESS_INTEGRATED, &level);
    if(level == -HUGE_VAL) {
        debugLog("No loudness information available (silent song?)\n");
    }

    BASS_Loudness_Stop(loudness);
    BASS_ChannelFree(decoder);

    last_song = m_selectedDifficulty2->getFullSoundFilePath();
    modifier = (level / -16.f);

    if(cv_debug.getBool()) {
        debugLog("Volume set to %.2fx for this song\n", modifier);
    }

    return volume * modifier;
}

void Beatmap::setSpeed(f32 speed) {
    if(m_music != NULL) m_music->setSpeed(speed);
}

void Beatmap::seekPercent(f64 percent) {
    if(m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

    m_bWasSeekFrame = true;
    m_fWaitTime = 0.0f;

    m_music->setPosition(percent);
    m_music->setVolume(getIdealVolume());
    m_music->setSpeed(osu->getSpeedMultiplier());

    resetHitObjects(m_music->getPositionMS());
    resetScore();

    m_iPreviousSectionPassFailTime = -1;

    if(m_bIsWaiting) {
        m_bIsWaiting = false;
        m_bIsPlaying = true;
        m_bIsRestartScheduledQuick = false;

        engine->getSound()->play(m_music);

        // if there are calculations in there that need the hitobjects to be loaded, also applies speed/pitch
        onModUpdate(false, false);
    }

    if(!is_watching && !is_spectating) {  // score submission already disabled when watching replay
        debugLog("Disabling score submission due to seeking\n");
        vanilla = false;
    }
}

void Beatmap::seekPercentPlayable(f64 percent) {
    if(m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

    m_bWasSeekFrame = true;
    m_fWaitTime = 0.0f;

    f64 actualPlayPercent = percent;
    if(m_hitobjects.size() > 0)
        actualPlayPercent = (((f64)m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                              (f64)m_hitobjects[m_hitobjects.size() - 1]->getDuration()) *
                             percent) /
                            (f64)m_music->getLengthMS();

    seekPercent(actualPlayPercent);
}

u32 Beatmap::getTime() const {
    if(m_music != NULL && m_music->isAsyncReady())
        return m_music->getPositionMS();
    else
        return 0;
}

u32 Beatmap::getStartTimePlayable() const {
    if(m_hitobjects.size() > 0)
        return (u32)m_hitobjects[0]->getTime();
    else
        return 0;
}

u32 Beatmap::getLength() const {
    if(m_music != NULL && m_music->isAsyncReady())
        return m_music->getLengthMS();
    else if(m_selectedDifficulty2 != NULL)
        return m_selectedDifficulty2->getLengthMS();
    else
        return 0;
}

u32 Beatmap::getLengthPlayable() const {
    if(m_hitobjects.size() > 0)
        return (u32)((m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                      m_hitobjects[m_hitobjects.size() - 1]->getDuration()) -
                     m_hitobjects[0]->getTime());
    else
        return getLength();
}

f32 Beatmap::getPercentFinished() const {
    if(m_music != NULL)
        return (f32)m_iCurMusicPos / (f32)m_music->getLengthMS();
    else
        return 0.0f;
}

f32 Beatmap::getPercentFinishedPlayable() const {
    if(m_bIsWaiting) return 1.0f - (m_fWaitTime - engine->getTimeReal()) / (cv_early_note_time.getFloat() / 1000.0f);

    if(m_hitobjects.size() > 0)
        return (f32)m_iCurMusicPos / ((f32)m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                                      (f32)m_hitobjects[m_hitobjects.size() - 1]->getDuration());
    else
        return (f32)m_iCurMusicPos / (f32)m_music->getLengthMS();
}

int Beatmap::getMostCommonBPM() const {
    if(m_selectedDifficulty2 != NULL) {
        if(m_music != NULL)
            return (int)(m_selectedDifficulty2->getMostCommonBPM() * m_music->getSpeed());
        else
            return (int)(m_selectedDifficulty2->getMostCommonBPM() * osu->getSpeedMultiplier());
    } else
        return 0;
}

f32 Beatmap::getSpeedMultiplier() const {
    if(m_music != NULL)
        return max(m_music->getSpeed(), 0.05f);
    else
        return 1.0f;
}

Skin *Beatmap::getSkin() const { return osu->getSkin(); }

u32 Beatmap::getScoreV1DifficultyMultiplier() const {
    // NOTE: We intentionally get CS/HP/OD from beatmap data, not "real" CS/HP/OD
    //       Since this multiplier is only used for ScoreV1
    u32 breakTimeMS = getBreakDurationTotal();
    f32 drainLength = max(getLengthPlayable() - min(breakTimeMS, getLengthPlayable()), (u32)1000) / 1000;
    return std::round((m_selectedDifficulty2->getCS() + m_selectedDifficulty2->getHP() +
                       m_selectedDifficulty2->getOD() +
                       clamp<f32>((f32)m_selectedDifficulty2->getNumObjects() / drainLength * 8.0f, 0.0f, 16.0f)) /
                      38.0f * 5.0f);
}

f32 Beatmap::getRawAR() const {
    if(m_selectedDifficulty2 == NULL) return 5.0f;

    return clamp<f32>(m_selectedDifficulty2->getAR() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

f32 Beatmap::getAR() const {
    if(m_selectedDifficulty2 == NULL) return 5.0f;

    f32 AR = getRawAR();

    if(cv_ar_override.getFloat() >= 0.0f) AR = cv_ar_override.getFloat();

    if(cv_ar_overridenegative.getFloat() < 0.0f) AR = cv_ar_overridenegative.getFloat();

    if(cv_ar_override_lock.getBool())
        AR = GameRules::getRawConstantApproachRateForSpeedMultiplier(
            GameRules::getRawApproachTime(AR),
            (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : osu->getSpeedMultiplier()));

    if(cv_mod_artimewarp.getBool() && m_hitobjects.size() > 0) {
        const f32 percent =
            1.0f - ((f64)(m_iCurMusicPos - m_hitobjects[0]->getTime()) /
                    (f64)(m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                          m_hitobjects[m_hitobjects.size() - 1]->getDuration() - m_hitobjects[0]->getTime())) *
                       (1.0f - cv_mod_artimewarp_multiplier.getFloat());
        AR *= percent;
    }

    if(cv_mod_arwobble.getBool())
        AR += std::sin((m_iCurMusicPos / 1000.0f) * cv_mod_arwobble_interval.getFloat()) *
              cv_mod_arwobble_strength.getFloat();

    return AR;
}

f32 Beatmap::getCS() const {
    if(m_selectedDifficulty2 == NULL) return 5.0f;

    f32 CS = clamp<f32>(m_selectedDifficulty2->getCS() * osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);

    if(cv_cs_override.getFloat() >= 0.0f) CS = cv_cs_override.getFloat();

    if(cv_cs_overridenegative.getFloat() < 0.0f) CS = cv_cs_overridenegative.getFloat();

    if(cv_mod_minimize.getBool() && m_hitobjects.size() > 0) {
        if(m_hitobjects.size() > 0) {
            const f32 percent =
                1.0f + ((f64)(m_iCurMusicPos - m_hitobjects[0]->getTime()) /
                        (f64)(m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                              m_hitobjects[m_hitobjects.size() - 1]->getDuration() - m_hitobjects[0]->getTime())) *
                           cv_mod_minimize_multiplier.getFloat();
            CS *= percent;
        }
    }

    if(cv_cs_cap_sanity.getBool()) CS = min(CS, 12.1429f);

    return CS;
}

f32 Beatmap::getHP() const {
    if(m_selectedDifficulty2 == NULL) return 5.0f;

    f32 HP = clamp<f32>(m_selectedDifficulty2->getHP() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
    if(cv_hp_override.getFloat() >= 0.0f) HP = cv_hp_override.getFloat();

    return HP;
}

f32 Beatmap::getRawOD() const {
    if(m_selectedDifficulty2 == NULL) return 5.0f;

    return clamp<f32>(m_selectedDifficulty2->getOD() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

f32 Beatmap::getOD() const {
    f32 OD = getRawOD();

    if(cv_od_override.getFloat() >= 0.0f) OD = cv_od_override.getFloat();

    if(cv_od_override_lock.getBool())
        OD = GameRules::getRawConstantOverallDifficultyForSpeedMultiplier(
            GameRules::getRawHitWindow300(OD),
            (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : osu->getSpeedMultiplier()));

    return OD;
}

bool Beatmap::isKey1Down() const {
    if(is_watching || is_spectating) {
        return current_keys & (LegacyReplay::M1 | LegacyReplay::K1);
    } else {
        return m_bClick1Held;
    }
}

bool Beatmap::isKey2Down() const {
    if(is_watching || is_spectating) {
        return current_keys & (LegacyReplay::M2 | LegacyReplay::K2);
    } else {
        return m_bClick2Held;
    }
}

bool Beatmap::isClickHeld() const { return isKey1Down() || isKey2Down(); }

std::string Beatmap::getTitle() const {
    if(m_selectedDifficulty2 != NULL)
        return m_selectedDifficulty2->getTitle();
    else
        return "NULL";
}

std::string Beatmap::getArtist() const {
    if(m_selectedDifficulty2 != NULL)
        return m_selectedDifficulty2->getArtist();
    else
        return "NULL";
}

u32 Beatmap::getBreakDurationTotal() const {
    u32 breakDurationTotal = 0;
    for(int i = 0; i < m_breaks.size(); i++) {
        breakDurationTotal += (u32)(m_breaks[i].endTime - m_breaks[i].startTime);
    }

    return breakDurationTotal;
}

DatabaseBeatmap::BREAK Beatmap::getBreakForTimeRange(long startMS, long positionMS, long endMS) const {
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

LiveScore::HIT Beatmap::addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo,
                                     bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore,
                                     bool ignoreHealth) {
    // Frames are already written on every keypress/release.
    // For some edge cases, we need to write extra frames to avoid replaybugs.
    {
        bool should_write_frame = false;

        // Slider interactions
        // Surely buzz sliders won't be an issue... Clueless
        should_write_frame |= (hit == LiveScore::HIT::HIT_SLIDER10);
        should_write_frame |= (hit == LiveScore::HIT::HIT_SLIDER30);
        should_write_frame |= (hit == LiveScore::HIT::HIT_MISS_SLIDERBREAK);

        // Relax: no keypresses, instead we write on every hitresult
        if(osu->getModRelax()) {
            should_write_frame |= (hit == LiveScore::HIT::HIT_50);
            should_write_frame |= (hit == LiveScore::HIT::HIT_100);
            should_write_frame |= (hit == LiveScore::HIT::HIT_300);
            should_write_frame |= (hit == LiveScore::HIT::HIT_MISS);
        }

        Beatmap *beatmap = (Beatmap *)osu->getSelectedBeatmap();
        if(should_write_frame && !hitErrorBarOnly && beatmap != NULL) {
            beatmap->write_frame();
        }
    }

    // handle perfect & sudden death
    if(osu->getModSS()) {
        if(!hitErrorBarOnly && hit != LiveScore::HIT::HIT_300 && hit != LiveScore::HIT::HIT_300G &&
           hit != LiveScore::HIT::HIT_SLIDER10 && hit != LiveScore::HIT::HIT_SLIDER30 &&
           hit != LiveScore::HIT::HIT_SPINNERSPIN && hit != LiveScore::HIT::HIT_SPINNERBONUS) {
            restart();
            return LiveScore::HIT::HIT_MISS;
        }
    } else if(osu->getModSD()) {
        if(hit == LiveScore::HIT::HIT_MISS) {
            if(cv_mod_suddendeath_restart.getBool())
                restart();
            else
                fail();

            return LiveScore::HIT::HIT_MISS;
        }
    }

    // miss sound
    if(hit == LiveScore::HIT::HIT_MISS) playMissSound();

    // score
    osu->getScore()->addHitResult(this, hitObject, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo,
                                  ignoreScore);

    // health
    LiveScore::HIT returnedHit = LiveScore::HIT::HIT_MISS;
    if(!ignoreHealth) {
        addHealth(osu->getScore()->getHealthIncrease(this, hit), true);

        // geki/katu handling
        if(isEndOfCombo) {
            const int comboEndBitmask = osu->getScore()->getComboEndBitmask();

            if(comboEndBitmask == 0) {
                returnedHit = LiveScore::HIT::HIT_300G;
                addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                osu->getScore()->addHitResultComboEnd(returnedHit);
            } else if((comboEndBitmask & 2) == 0) {
                if(hit == LiveScore::HIT::HIT_100) {
                    returnedHit = LiveScore::HIT::HIT_100K;
                    addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                    osu->getScore()->addHitResultComboEnd(returnedHit);
                } else if(hit == LiveScore::HIT::HIT_300) {
                    returnedHit = LiveScore::HIT::HIT_300K;
                    addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                    osu->getScore()->addHitResultComboEnd(returnedHit);
                }
            } else if(hit != LiveScore::HIT::HIT_MISS)
                addHealth(osu->getScore()->getHealthIncrease(this, LiveScore::HIT::HIT_MU), true);

            osu->getScore()->setComboEndBitmask(0);
        }
    }

    return returnedHit;
}

void Beatmap::addSliderBreak() {
    // handle perfect & sudden death
    if(osu->getModSS()) {
        restart();
        return;
    } else if(osu->getModSD()) {
        if(cv_mod_suddendeath_restart.getBool())
            restart();
        else
            fail();

        return;
    }

    // miss sound
    playMissSound();

    // score
    osu->getScore()->addSliderBreak();
}

void Beatmap::addScorePoints(int points, bool isSpinner) { osu->getScore()->addPoints(points, isSpinner); }

void Beatmap::addHealth(f64 percent, bool isFromHitResult) {
    // never drain before first hitobject
    if(m_hitobjects.size() > 0 && m_iCurMusicPosWithOffsets < m_hitobjects[0]->getTime()) return;

    // never drain after last hitobject
    if(m_hitobjectsSortedByEndTime.size() > 0 &&
       m_iCurMusicPosWithOffsets > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() +
                                    m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration()))
        return;

    if(m_bFailed) {
        anim->deleteExistingAnimation(&m_fHealth2);

        m_fHealth = 0.0;
        m_fHealth2 = 0.0f;

        return;
    }

    if(isFromHitResult && percent > 0.0) {
        osu->getHUD()->animateKiBulge();

        if(m_fHealth > 0.9) osu->getHUD()->animateKiExplode();
    }

    m_fHealth = clamp<f64>(m_fHealth + percent, 0.0, 1.0);

    // handle generic fail state (2)
    const bool isDead = m_fHealth < 0.001;
    if(isDead && !osu->getModNF()) {
        if(osu->getModEZ() && osu->getScore()->getNumEZRetries() > 0)  // retries with ez
        {
            osu->getScore()->setNumEZRetries(osu->getScore()->getNumEZRetries() - 1);

            // special case: set health to 160/200 (osu!stable behavior, seems fine for all drains)
            m_fHealth = 160.0f / 200.f;
            m_fHealth2 = (f32)m_fHealth;

            anim->deleteExistingAnimation(&m_fHealth2);
        } else if(isFromHitResult && percent < 0.0)  // judgement fail
        {
            fail();
        }
    }
}

void Beatmap::updateTimingPoints(long curPos) {
    if(curPos < 0) return;  // aspire pls >:(

    /// debugLog("updateTimingPoints( %ld )\n", curPos);

    const DatabaseBeatmap::TIMING_INFO t =
        m_selectedDifficulty2->getTimingInfoForTime(curPos + (long)cv_timingpoints_offset.getInt());
    osu->getSkin()->setSampleSet(
        t.sampleType);  // normal/soft/drum is stored in the sample type! the sample set number is for custom sets
    osu->getSkin()->setSampleVolume(clamp<f32>(t.volume / 100.0f, 0.0f, 1.0f));
}

long Beatmap::getPVS() {
    // this is an approximation with generous boundaries, it doesn't need to be exact (just good enough to filter 10000
    // hitobjects down to a few hundred or so) it will be used in both positive and negative directions (previous and
    // future hitobjects) to speed up loops which iterate over all hitobjects
    return getApproachTime() + GameRules::getFadeInTime() + (long)GameRules::getHitWindowMiss() + 1500;  // sanity
}

bool Beatmap::canDraw() {
    if(!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting) return false;
    if(m_selectedDifficulty2 == NULL || m_music == NULL)  // sanity check
        return false;

    return true;
}

bool Beatmap::canUpdate() { return m_bIsPlaying || m_bIsPaused || m_bContinueScheduled; }

void Beatmap::handlePreviewPlay() {
    if(m_music == NULL) return;

    if((!m_music->isPlaying() || m_music->getPosition() > 0.95f) && m_selectedDifficulty2 != NULL) {
        // this is an assumption, but should be good enough for most songs
        // reset playback position when the song has nearly reached the end (when the user switches back to the results
        // screen or the songbrowser after playing)
        if(m_music->getPosition() > 0.95f) m_iContinueMusicPos = 0;

        engine->getSound()->stop(m_music);

        if(engine->getSound()->play(m_music)) {
            if(m_music->getFrequency() < m_fMusicFrequencyBackup)  // player has died, reset frequency
                m_music->setFrequency(m_fMusicFrequencyBackup);

            // When neosu is initialized, it starts playing a random song in the main menu.
            // Users can set a convar to make it start at its preview point instead.
            // The next songs will start at the beginning regardless.
            static bool should_start_song_at_preview_point = cv_start_first_main_menu_song_at_preview_point.getBool();
            bool start_at_song_beginning = osu->getMainMenu()->isVisible() && !should_start_song_at_preview_point;
            should_start_song_at_preview_point = false;

            if(start_at_song_beginning) {
                m_music->setPositionMS_fast(0);
                m_bWasSeekFrame = true;
            } else if(m_iContinueMusicPos != 0) {
                m_music->setPositionMS_fast(m_iContinueMusicPos);
                m_bWasSeekFrame = true;
            } else {
                m_music->setPositionMS_fast(m_selectedDifficulty2->getPreviewTime() < 0
                                                ? (u32)(m_music->getLengthMS() * 0.40f)
                                                : m_selectedDifficulty2->getPreviewTime());
                m_bWasSeekFrame = true;
            }

            m_music->setVolume(getIdealVolume());
            m_music->setSpeed(osu->getSpeedMultiplier());
        }
    }

    // always loop during preview
    m_music->setLoop(cv_beatmap_preview_music_loop.getBool());
}

void Beatmap::loadMusic(bool stream) {
    stream = stream || m_bForceStreamPlayback;
    m_iResourceLoadUpdateDelayHack = 0;

    // load the song (again)
    if(m_selectedDifficulty2 != NULL &&
       (m_music == NULL || m_selectedDifficulty2->getFullSoundFilePath() != m_music->getFilePath() ||
        !m_music->isReady())) {
        unloadMusic();

        // if it's not a stream then we are loading the entire song into memory for playing
        if(!stream) engine->getResourceManager()->requestNextLoadAsync();

        m_music = engine->getResourceManager()->loadSoundAbs(m_selectedDifficulty2->getFullSoundFilePath(),
                                                             "OSU_BEATMAP_MUSIC", stream, false, false);
        m_music->setVolume(getIdealVolume());
        m_fMusicFrequencyBackup = m_music->getFrequency();
        m_music->setSpeed(osu->getSpeedMultiplier());
    }
}

void Beatmap::unloadMusic() {
    engine->getResourceManager()->destroyResource(m_music);
    m_music = NULL;
}

void Beatmap::unloadObjects() {
    m_currentHitObject = NULL;
    for(int i = 0; i < m_hitobjects.size(); i++) {
        delete m_hitobjects[i];
    }
    m_hitobjects = std::vector<HitObject *>();
    m_hitobjectsSortedByEndTime = std::vector<HitObject *>();
    m_misaimObjects = std::vector<HitObject *>();
    m_breaks = std::vector<DatabaseBeatmap::BREAK>();
    m_clicks = std::vector<Click>();
}

void Beatmap::resetHitObjects(long curPos) {
    for(int i = 0; i < m_hitobjects.size(); i++) {
        m_hitobjects[i]->onReset(curPos);
        m_hitobjects[i]->update(curPos);
        m_hitobjects[i]->onReset(curPos);
    }
    osu->getHUD()->resetHitErrorBar();
}

void Beatmap::resetScore() {
    vanilla = convar->isVanilla();

    live_replay.clear();
    live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = -1,
        .milliseconds_since_last_frame = 0,
        .x = 256,
        .y = -500,
        .key_flags = 0,
    });
    live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = -1,
        .milliseconds_since_last_frame = -1,
        .x = 256,
        .y = -500,
        .key_flags = 0,
    });

    last_event_time = engine->getTimeReal();
    last_event_ms = -1;
    current_keys = 0;
    last_keys = 0;
    current_frame_idx = 0;
    m_iCurMusicPos = 0;
    m_iCurMusicPosWithOffsets = 0;

    m_fHealth = 1.0;
    m_fHealth2 = 1.0f;
    m_bFailed = false;
    m_fFailAnim = 1.0f;
    anim->deleteExistingAnimation(&m_fFailAnim);

    osu->getScore()->reset();
    osu->m_hud->resetScoreboard();

    holding_slider = false;
    m_bIsFirstMissSound = true;
}

void Beatmap::playMissSound() {
    if((m_bIsFirstMissSound && osu->getScore()->getCombo() > 0) ||
       osu->getScore()->getCombo() > cv_combobreak_sound_combo.getInt()) {
        m_bIsFirstMissSound = false;
        engine->getSound()->play(getSkin()->getCombobreak());
    }
}

u32 Beatmap::getMusicPositionMSInterpolated() {
    if(!cv_interpolate_music_pos.getBool() || isLoading()) {
        return m_music->getPositionMS();
    } else {
        const f64 interpolationMultiplier = 1.0;

        // TODO: fix snapping at beginning for maps with instant start

        u32 returnPos = 0;
        const f64 curPos = (f64)m_music->getPositionMS();
        const f32 speed = m_music->getSpeed();

        // not reinventing the wheel, the interpolation magic numbers here are (c) peppy

        const f64 realTime = engine->getTimeReal();
        const f64 interpolationDelta = (realTime - m_fLastRealTimeForInterpolationDelta) * 1000.0 * speed;
        const f64 interpolationDeltaLimit =
            ((realTime - m_fLastAudioTimeAccurateSet) * 1000.0 < 1500 || speed < 1.0f ? 11 : 33) *
            interpolationMultiplier;

        if(m_music->isPlaying() && !m_bWasSeekFrame) {
            f64 newInterpolatedPos = m_fInterpolatedMusicPos + interpolationDelta;
            f64 delta = newInterpolatedPos - curPos;

            // debugLog("delta = %ld\n", (long)delta);

            // approach and recalculate delta
            newInterpolatedPos -= delta / 8.0 / interpolationMultiplier;
            delta = newInterpolatedPos - curPos;

            if(std::abs(delta) > interpolationDeltaLimit * 2)  // we're fucked, snap back to curPos
            {
                m_fInterpolatedMusicPos = (f64)curPos;
            } else if(delta < -interpolationDeltaLimit)  // undershot
            {
                m_fInterpolatedMusicPos += interpolationDelta * 2;
                m_fLastAudioTimeAccurateSet = realTime;
            } else if(delta < interpolationDeltaLimit)  // normal
            {
                m_fInterpolatedMusicPos = newInterpolatedPos;
            } else  // overshot
            {
                m_fInterpolatedMusicPos += interpolationDelta / 2;
                m_fLastAudioTimeAccurateSet = realTime;
            }

            // calculate final return value
            returnPos = (u32)std::round(m_fInterpolatedMusicPos);

            bool nightcoring = osu->getModNC() || osu->getModDC();
            if(speed < 1.0f && cv_compensate_music_speed.getBool() && !nightcoring) {
                returnPos += (u32)(((1.0f - speed) / 0.75f) * 5);
            }
        } else  // no interpolation
        {
            returnPos = curPos;
            m_fInterpolatedMusicPos = (u32)returnPos;
            m_fLastAudioTimeAccurateSet = realTime;
        }

        m_fLastRealTimeForInterpolationDelta =
            realTime;  // this is more accurate than engine->getFrameTime() for the delta calculation, since it
                       // correctly handles all possible delays inbetween

        // debugLog("returning %lu \n", returnPos);
        // debugLog("delta = %lu\n", (long)returnPos - m_iCurMusicPos);
        // debugLog("raw delta = %ld\n", (long)returnPos - (long)curPos);

        return returnPos;
    }
}

void Beatmap::draw(Graphics *g) {
    if(!canDraw()) return;

    // draw background
    drawBackground(g);

    // draw loading circle
    if(isLoading()) {
        if(isBuffering()) {
            f32 leeway = clamp<i32>(last_frame_ms - last_event_ms, 0, 2500);
            f32 pct = leeway / 2500.f * 100.f;
            auto loadingMessage = UString::format("Buffering ... (%.2f%%)", pct);
            osu->getHUD()->drawLoadingSmall(g, loadingMessage);
        } else if(bancho.is_playing_a_multi_map() && !bancho.room.all_players_loaded) {
            osu->getHUD()->drawLoadingSmall(g, "Waiting for players ...");
        } else {
            osu->getHUD()->drawLoadingSmall(g, "Loading ...");
        }

        // only start drawing the rest of the playfield if everything has loaded
        return;
    }

    // draw playfield border
    if(cv_draw_playfield_border.getBool() && !cv_mod_fps.getBool()) {
        osu->getHUD()->drawPlayfieldBorder(g, m_vPlayfieldCenter, m_vPlayfieldSize, m_fHitcircleDiameter);
    }

    // draw hiterrorbar
    if(!cv_mod_fposu.getBool()) osu->getHUD()->drawHitErrorBar(g, this);

    // draw first person crosshair
    if(cv_mod_fps.getBool()) {
        const int length = 15;
        Vector2 center = osuCoords2Pixels(Vector2(GameRules::OSU_COORD_WIDTH / 2, GameRules::OSU_COORD_HEIGHT / 2));
        g->setColor(0xff777777);
        g->drawLine(center.x, (int)(center.y - length), center.x, (int)(center.y + length + 1));
        g->drawLine((int)(center.x - length), center.y, (int)(center.x + length + 1), center.y);
    }

    // draw followpoints
    if(cv_draw_followpoints.getBool() && !cv_mod_mafham.getBool()) drawFollowPoints(g);

    // draw all hitobjects in reverse
    if(cv_draw_hitobjects.getBool()) drawHitObjects(g);

    // debug stuff
    if(cv_debug_hiterrorbar_misaims.getBool()) {
        for(int i = 0; i < m_misaimObjects.size(); i++) {
            g->setColor(0xbb00ff00);
            Vector2 pos = osuCoords2Pixels(m_misaimObjects[i]->getRawPosAt(0));
            g->fillRect(pos.x - 50, pos.y - 50, 100, 100);
        }
    }
}

void Beatmap::drawFollowPoints(Graphics *g) {
    Skin *skin = osu->getSkin();

    const long curPos = m_iCurMusicPosWithOffsets;

    // I absolutely hate this, followpoints can be abused for cheesing high AR reading since they always fade in with a
    // fixed 800 ms custom approach time. Capping it at the current approach rate seems sensible, but unfortunately
    // that's not what osu is doing. It was non-osu-compliant-clamped since this client existed, but let's see how many
    // people notice a change after all this time (26.02.2020)

    // 0.7x means animation lasts only 0.7 of it's time
    const f64 animationMutiplier = osu->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
    const long followPointApproachTime =
        animationMutiplier * (cv_followpoints_clamp.getBool()
                                  ? min((long)getApproachTime(), (long)cv_followpoints_approachtime.getFloat())
                                  : (long)cv_followpoints_approachtime.getFloat());
    const bool followPointsConnectCombos = cv_followpoints_connect_combos.getBool();
    const bool followPointsConnectSpinners = cv_followpoints_connect_spinners.getBool();
    const f32 followPointSeparationMultiplier = max(cv_followpoints_separation_multiplier.getFloat(), 0.1f);
    const f32 followPointPrevFadeTime = animationMutiplier * cv_followpoints_prevfadetime.getFloat();
    const f32 followPointScaleMultiplier = cv_followpoints_scale_multiplier.getFloat();

    // include previous object in followpoints
    int lastObjectIndex = -1;

    for(int index = m_iPreviousFollowPointObjectIndex; index < m_hitobjects.size(); index++) {
        lastObjectIndex = index - 1;

        // ignore future spinners
        Spinner *spinnerPointer = dynamic_cast<Spinner *>(m_hitobjects[index]);
        if(spinnerPointer != NULL && !followPointsConnectSpinners)  // if this is a spinner
        {
            lastObjectIndex = -1;
            continue;
        }

        // NOTE: "m_hitobjects[index]->getComboNumber() != 1" breaks (not literally) on new combos
        // NOTE: the "getComboNumber()" call has been replaced with isEndOfCombo() because of
        // osu_ignore_beatmap_combo_numbers and osu_number_max
        const bool isCurrentHitObjectNewCombo =
            (lastObjectIndex >= 0 ? m_hitobjects[lastObjectIndex]->isEndOfCombo() : false);
        const bool isCurrentHitObjectSpinner = (lastObjectIndex >= 0 && followPointsConnectSpinners
                                                    ? dynamic_cast<Spinner *>(m_hitobjects[lastObjectIndex]) != NULL
                                                    : false);
        if(lastObjectIndex >= 0 && (!isCurrentHitObjectNewCombo || followPointsConnectCombos ||
                                    (isCurrentHitObjectSpinner && followPointsConnectSpinners))) {
            // ignore previous spinners
            spinnerPointer = dynamic_cast<Spinner *>(m_hitobjects[lastObjectIndex]);
            if(spinnerPointer != NULL && !followPointsConnectSpinners)  // if this is a spinner
            {
                lastObjectIndex = -1;
                continue;
            }

            // get time & pos of the last and current object
            const long lastObjectEndTime =
                m_hitobjects[lastObjectIndex]->getTime() + m_hitobjects[lastObjectIndex]->getDuration() + 1;
            const long objectStartTime = m_hitobjects[index]->getTime();
            const long timeDiff = objectStartTime - lastObjectEndTime;

            const Vector2 startPoint = osuCoords2Pixels(m_hitobjects[lastObjectIndex]->getRawPosAt(lastObjectEndTime));
            const Vector2 endPoint = osuCoords2Pixels(m_hitobjects[index]->getRawPosAt(objectStartTime));

            const f32 xDiff = endPoint.x - startPoint.x;
            const f32 yDiff = endPoint.y - startPoint.y;
            const Vector2 diff = endPoint - startPoint;
            const f32 dist =
                std::round(diff.length() * 100.0f) / 100.0f;  // rounded to avoid flicker with playfield rotations

            // draw all points between the two objects
            const int followPointSeparation = Osu::getUIScale(32) * followPointSeparationMultiplier;
            for(int j = (int)(followPointSeparation * 1.5f); j < (dist - followPointSeparation);
                j += followPointSeparation) {
                const f32 animRatio = ((f32)j / dist);

                const Vector2 animPosStart = startPoint + (animRatio - 0.1f) * diff;
                const Vector2 finalPos = startPoint + animRatio * diff;

                const long fadeInTime = (long)(lastObjectEndTime + animRatio * timeDiff) - followPointApproachTime;
                const long fadeOutTime = (long)(lastObjectEndTime + animRatio * timeDiff);

                // draw
                f32 alpha = 1.0f;
                f32 followAnimPercent =
                    clamp<f32>((f32)(curPos - fadeInTime) / (f32)followPointPrevFadeTime, 0.0f, 1.0f);
                followAnimPercent = -followAnimPercent * (followAnimPercent - 2.0f);  // quad out

                // NOTE: only internal osu default skin uses scale + move transforms here, it is impossible to achieve
                // this effect with user skins
                const f32 scale = cv_followpoints_anim.getBool() ? 1.5f - 0.5f * followAnimPercent : 1.0f;
                const Vector2 followPos = cv_followpoints_anim.getBool()
                                              ? animPosStart + (finalPos - animPosStart) * followAnimPercent
                                              : finalPos;

                // bullshit performance optimization: only draw followpoints if within screen bounds (plus a bit of a
                // margin) there is only one beatmap where this matters currently: https://osu.ppy.sh/b/1145513
                if(followPos.x < -osu->getScreenWidth() || followPos.x > osu->getScreenWidth() * 2 ||
                   followPos.y < -osu->getScreenHeight() || followPos.y > osu->getScreenHeight() * 2)
                    continue;

                // calculate trail alpha
                if(curPos >= fadeInTime && curPos < fadeOutTime) {
                    // future trail
                    const f32 delta = curPos - fadeInTime;
                    alpha = (f32)delta / (f32)followPointApproachTime;
                } else if(curPos >= fadeOutTime && curPos < (fadeOutTime + (long)followPointPrevFadeTime)) {
                    // previous trail
                    const long delta = curPos - fadeOutTime;
                    alpha = 1.0f - (f32)delta / (f32)(followPointPrevFadeTime);
                } else
                    alpha = 0.0f;

                // draw it
                g->setColor(0xffffffff);
                g->setAlpha(alpha);
                g->pushTransform();
                {
                    g->rotate(rad2deg(std::atan2(yDiff, xDiff)));

                    skin->getFollowPoint2()->setAnimationTimeOffset(skin->getAnimationSpeed(), fadeInTime);

                    // NOTE: getSizeBaseRaw() depends on the current animation time being set correctly beforehand!
                    // (otherwise you get incorrect scales, e.g. for animated elements with inconsistent @2x mixed in)
                    // the followpoints are scaled by one eighth of the hitcirclediameter (not the raw diameter, but the
                    // scaled diameter)
                    const f32 followPointImageScale =
                        ((m_fHitcircleDiameter / 8.0f) / skin->getFollowPoint2()->getSizeBaseRaw().x) *
                        followPointScaleMultiplier;

                    skin->getFollowPoint2()->drawRaw(g, followPos, followPointImageScale * scale);
                }
                g->popTransform();
            }
        }

        // store current index as previous index
        lastObjectIndex = index;

        // iterate up until the "nextest" element
        if(m_hitobjects[index]->getTime() >= curPos + followPointApproachTime) break;
    }
}

void Beatmap::drawHitObjects(Graphics *g) {
    const long curPos = m_iCurMusicPosWithOffsets;
    const long pvs = getPVS();
    const bool usePVS = cv_pvs.getBool();

    if(!cv_mod_mafham.getBool()) {
        if(!cv_draw_reverse_order.getBool()) {
            for(int i = m_hitobjectsSortedByEndTime.size() - 1; i >= 0; i--) {
                // PVS optimization (reversed)
                if(usePVS) {
                    if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                           m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                        break;
                    if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                        continue;
                }

                m_hitobjectsSortedByEndTime[i]->draw(g);
            }
        } else {
            for(int i = 0; i < m_hitobjectsSortedByEndTime.size(); i++) {
                // PVS optimization
                if(usePVS) {
                    if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                           m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                        continue;
                    if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                        break;
                }

                m_hitobjectsSortedByEndTime[i]->draw(g);
            }
        }
        for(int i = 0; i < m_hitobjectsSortedByEndTime.size(); i++) {
            // NOTE: to fix mayday simultaneous sliders with increasing endtime getting culled here, would have to
            // switch from m_hitobjectsSortedByEndTime to m_hitobjects PVS optimization
            if(usePVS) {
                if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                   (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                       m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                    continue;
                if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                    break;
            }

            m_hitobjectsSortedByEndTime[i]->draw2(g);
        }
    } else {
        const int mafhamRenderLiveSize = cv_mod_mafham_render_livesize.getInt();

        if(m_mafhamActiveRenderTarget == NULL) m_mafhamActiveRenderTarget = osu->getFrameBuffer();

        if(m_mafhamFinishedRenderTarget == NULL) m_mafhamFinishedRenderTarget = osu->getFrameBuffer2();

        // if we have a chunk to render into the scene buffer
        const bool shouldDrawBuffer =
            (m_hitobjectsSortedByEndTime.size() - m_iCurrentHitObjectIndex) > mafhamRenderLiveSize;
        bool shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
        if(shouldRenderChunk) {
            m_bInMafhamRenderChunk = true;

            m_mafhamActiveRenderTarget->setClearColorOnDraw(m_iMafhamHitObjectRenderIndex == 0);
            m_mafhamActiveRenderTarget->setClearDepthOnDraw(m_iMafhamHitObjectRenderIndex == 0);

            m_mafhamActiveRenderTarget->enable();
            {
                g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
                {
                    int chunkCounter = 0;
                    for(int i = m_hitobjectsSortedByEndTime.size() - 1 - m_iMafhamHitObjectRenderIndex; i >= 0;
                        i--, m_iMafhamHitObjectRenderIndex++) {
                        chunkCounter++;
                        if(chunkCounter > cv_mod_mafham_render_chunksize.getInt())
                            break;  // continue chunk render in next frame

                        if(i <= m_iCurrentHitObjectIndex + mafhamRenderLiveSize)  // skip live objects
                        {
                            m_iMafhamHitObjectRenderIndex = m_hitobjectsSortedByEndTime.size();  // stop chunk render
                            break;
                        }

                        // PVS optimization (reversed)
                        if(usePVS) {
                            if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                               (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                                   m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                            {
                                m_iMafhamHitObjectRenderIndex =
                                    m_hitobjectsSortedByEndTime.size();  // stop chunk render
                                break;
                            }
                            if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                                continue;
                        }

                        m_hitobjectsSortedByEndTime[i]->draw(g);

                        m_iMafhamActiveRenderHitObjectIndex = i;
                    }
                }
                g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
            }
            m_mafhamActiveRenderTarget->disable();

            m_bInMafhamRenderChunk = false;
        }
        shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
        if(!shouldRenderChunk && m_bMafhamRenderScheduled) {
            // finished, we can now swap the active framebuffer with the one we just finished
            m_bMafhamRenderScheduled = false;

            RenderTarget *temp = m_mafhamFinishedRenderTarget;
            m_mafhamFinishedRenderTarget = m_mafhamActiveRenderTarget;
            m_mafhamActiveRenderTarget = temp;

            m_iMafhamFinishedRenderHitObjectIndex = m_iMafhamActiveRenderHitObjectIndex;
            m_iMafhamActiveRenderHitObjectIndex = m_hitobjectsSortedByEndTime.size();  // reset
        }

        // draw scene buffer
        if(shouldDrawBuffer) {
            g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_COLOR);
            { m_mafhamFinishedRenderTarget->draw(g, 0, 0); }
            g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
        }

        // draw followpoints
        if(cv_draw_followpoints.getBool()) drawFollowPoints(g);

        // draw live hitobjects (also, code duplication yay)
        {
            for(int i = m_hitobjectsSortedByEndTime.size() - 1; i >= 0; i--) {
                // PVS optimization (reversed)
                if(usePVS) {
                    if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                           m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                        break;
                    if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                        continue;
                }

                if(i > m_iCurrentHitObjectIndex + mafhamRenderLiveSize ||
                   (i > m_iMafhamFinishedRenderHitObjectIndex - 1 && shouldDrawBuffer))  // skip non-live objects
                    continue;

                m_hitobjectsSortedByEndTime[i]->draw(g);
            }

            for(int i = 0; i < m_hitobjectsSortedByEndTime.size(); i++) {
                // PVS optimization
                if(usePVS) {
                    if(m_hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() +
                                           m_hitobjectsSortedByEndTime[i]->getDuration()))  // past objects
                        continue;
                    if(m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs)  // future objects
                        break;
                }

                if(i >= m_iCurrentHitObjectIndex + mafhamRenderLiveSize ||
                   (i >= m_iMafhamFinishedRenderHitObjectIndex - 1 && shouldDrawBuffer))  // skip non-live objects
                    break;

                m_hitobjectsSortedByEndTime[i]->draw2(g);
            }
        }
    }
}

void Beatmap::update() {
    if(!canUpdate()) return;

    // some things need to be updated before loading has finished, so control flow is a bit weird here.

    // live update hitobject and playfield metrics
    updateHitobjectMetrics();
    updatePlayfieldMetrics();

    // @PPV3: also calculate live ppv3
    if(cv_draw_statistics_pp.getBool() || cv_draw_statistics_livestars.getBool()) {
        auto info = m_ppv2_calc.get();
        osu->getHUD()->live_pp = info.pp;
        osu->getHUD()->live_stars = info.total_stars;

        if(last_calculated_hitobject < m_iCurrentHitObjectIndex) {
            last_calculated_hitobject = m_iCurrentHitObjectIndex;

            auto CS = getCS();
            auto AR = getAR();
            auto OD = getOD();
            auto speedMultiplier = osu->getSpeedMultiplier();
            auto osufile_path = m_selectedDifficulty2->getFilePath();
            auto nb_circles = m_selectedDifficulty2->m_iNumCircles;
            auto nb_sliders = m_selectedDifficulty2->m_iNumSliders;
            auto nb_spinners = m_selectedDifficulty2->m_iNumSpinners;
            auto modsLegacy = osu->getScore()->getModsLegacy();
            auto relax = osu->getModRelax();
            auto td = osu->getModTD();
            auto highestCombo = osu->getScore()->getComboMax();
            auto numMisses = osu->getScore()->getNumMisses();
            auto num300s = osu->getScore()->getNum300s();
            auto num100s = osu->getScore()->getNum100s();
            auto num50s = osu->getScore()->getNum50s();

            m_ppv2_calc.enqueue([=]() {
                pp_info info;

                // XXX: slow
                auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(osufile_path.c_str(), AR, CS, speedMultiplier);

                std::vector<f64> aimStrains;
                std::vector<f64> speedStrains;

                info.total_stars = DifficultyCalculator::calculateStarDiffForHitObjects(
                    diffres.diffobjects, CS, OD, speedMultiplier, relax, td, &info.aim_stars, &info.aim_slider_factor,
                    &info.speed_stars, &info.speed_notes, -1, &aimStrains, &speedStrains);

                info.pp = DifficultyCalculator::calculatePPv2(
                    modsLegacy, speedMultiplier, AR, OD, info.aim_stars, info.aim_slider_factor, info.speed_stars,
                    info.speed_notes, nb_circles, nb_sliders, nb_spinners, diffres.maxPossibleCombo, highestCombo,
                    numMisses, num300s, num100s, num50s);

                return info;
            });
        }
    }

    // wobble mod
    if(cv_mod_wobble.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
        m_fPlayfieldRotation =
            (m_iCurMusicPos / 1000.0f) * 30.0f * speedMultiplierCompensation * cv_mod_wobble_rotation_speed.getFloat();
        m_fPlayfieldRotation = std::fmod(m_fPlayfieldRotation, 360.0f);
    } else
        m_fPlayfieldRotation = 0.0f;

    // do hitobject updates among other things
    // yes, this needs to happen after updating metrics and playfield rotation
    update2();

    // handle preloading (only for distributed slider vertexbuffer generation atm)
    if(m_bIsPreLoading) {
        if(cv_debug.getBool() && m_iPreLoadingIndex == 0) debugLog("Beatmap: Preloading slider vertexbuffers ...\n");

        f64 startTime = engine->getTimeReal();
        f64 delta = 0.0;

        // hardcoded deadline of 10 ms, will temporarily bring us down to 45fps on average (better than freezing)
        while(delta < 0.010 && m_bIsPreLoading) {
            if(m_iPreLoadingIndex >= m_hitobjects.size()) {
                m_bIsPreLoading = false;
                debugLog("Beatmap: Preloading done.\n");
                break;
            } else {
                Slider *sliderPointer = dynamic_cast<Slider *>(m_hitobjects[m_iPreLoadingIndex]);
                if(sliderPointer != NULL) sliderPointer->rebuildVertexBuffer();
            }

            m_iPreLoadingIndex++;
            delta = engine->getTimeReal() - startTime;
        }
    }

    // notify all other players (including ourself) once we've finished loading
    if(bancho.is_playing_a_multi_map()) {
        if(!isActuallyLoading()) {
            if(!bancho.room.player_loaded) {
                bancho.room.player_loaded = true;

                Packet packet;
                packet.id = MATCH_LOAD_COMPLETE;
                send_packet(packet);
            }
        }
    }

    if(isLoading()) {
        // only continue if we have loaded everything
        return;
    }

    // update auto (after having updated the hitobjects)
    if(osu->getModAuto() || osu->getModAutopilot()) updateAutoCursorPos();

    // spinner detection (used by osu!stable drain, and by HUD for not drawing the hiterrorbar)
    if(m_currentHitObject != NULL) {
        Spinner *spinnerPointer = dynamic_cast<Spinner *>(m_currentHitObject);
        if(spinnerPointer != NULL && m_iCurMusicPosWithOffsets > m_currentHitObject->getTime() &&
           m_iCurMusicPosWithOffsets < m_currentHitObject->getTime() + m_currentHitObject->getDuration())
            m_bIsSpinnerActive = true;
        else
            m_bIsSpinnerActive = false;
    }

    // scene buffering logic
    if(cv_mod_mafham.getBool()) {
        if(!m_bMafhamRenderScheduled &&
           m_iCurrentHitObjectIndex !=
               m_iMafhamPrevHitObjectIndex)  // if we are not already rendering and the index changed
        {
            m_iMafhamPrevHitObjectIndex = m_iCurrentHitObjectIndex;
            m_iMafhamHitObjectRenderIndex = 0;
            m_bMafhamRenderScheduled = true;
        }
    }

    // full alternate mod lenience
    if(cv_mod_fullalternate.getBool()) {
        if(m_bInBreak || m_bIsInSkippableSection || m_bIsSpinnerActive || m_iCurrentHitObjectIndex < 1)
            m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = m_iCurrentHitObjectIndex + 1;
    }

    if(last_keys != current_keys) {
        write_frame();
    } else if(last_event_time + 0.01666666666 <= engine->getTimeReal()) {
        write_frame();
    }
}

void Beatmap::update2() {
    if(m_bContinueScheduled) {
        // If we paused while m_bIsWaiting (green progressbar), then we have to let the 'if (m_bIsWaiting)' block handle
        // the sound play() call
        bool isEarlyNoteContinue = (!m_bIsPaused && m_bIsWaiting);
        if(m_bClickedContinue || isEarlyNoteContinue) {
            m_bClickedContinue = false;
            m_bContinueScheduled = false;
            m_bIsPaused = false;

            if(!isEarlyNoteContinue) {
                engine->getSound()->play(m_music);
            }

            m_bIsPlaying = true;  // usually this should be checked with the result of the above play() call, but since
                                  // we are continuing we can assume that everything works

            // for nightmare mod, to avoid a miss because of the continue click
            m_clicks.clear();
        }
    }

    // handle restarts
    if(m_bIsRestartScheduled) {
        m_bIsRestartScheduled = false;
        actualRestart();
        return;
    }

    // update current music position (this variable does not include any offsets!)
    m_iCurMusicPos = getMusicPositionMSInterpolated();
    m_iContinueMusicPos = m_music->getPositionMS();
    const bool wasSeekFrame = m_bWasSeekFrame;
    m_bWasSeekFrame = false;

    // handle timewarp
    if(cv_mod_timewarp.getBool()) {
        if(m_hitobjects.size() > 0 && m_iCurMusicPos > m_hitobjects[0]->getTime()) {
            const f32 percentFinished =
                ((f64)(m_iCurMusicPos - m_hitobjects[0]->getTime()) /
                 (f64)(m_hitobjects[m_hitobjects.size() - 1]->getTime() +
                       m_hitobjects[m_hitobjects.size() - 1]->getDuration() - m_hitobjects[0]->getTime()));
            f32 warp_multiplier = max(cv_mod_timewarp_multiplier.getFloat(), 1.f);
            const f32 speed =
                osu->getSpeedMultiplier() + percentFinished * osu->getSpeedMultiplier() * (warp_multiplier - 1.0f);
            m_music->setSpeed(speed);
        }
    }

    // HACKHACK: clean this mess up
    // waiting to start (file loading, retry)
    // NOTE: this is dependent on being here AFTER m_iCurMusicPos has been set above, because it modifies it to fake a
    // negative start (else everything would just freeze for the waiting period)
    if(m_bIsWaiting) {
        if(isLoading()) {
            m_fWaitTime = engine->getTimeReal();

            // if the first hitobject starts immediately, add artificial wait time before starting the music
            if(!m_bIsRestartScheduledQuick && m_hitobjects.size() > 0) {
                if(m_hitobjects[0]->getTime() < (long)cv_early_note_time.getInt())
                    m_fWaitTime = engine->getTimeReal() + cv_early_note_time.getFloat() / 1000.0f;
            }
        } else {
            if(engine->getTimeReal() > m_fWaitTime) {
                if(!m_bIsPaused) {
                    m_bIsWaiting = false;
                    m_bIsPlaying = true;

                    engine->getSound()->play(m_music);
                    m_music->setLoop(false);
                    m_music->setPositionMS(0);
                    m_bWasSeekFrame = true;
                    m_music->setVolume(getIdealVolume());
                    m_music->setSpeed(osu->getSpeedMultiplier());

                    // if we are quick restarting, jump just before the first hitobject (even if there is a long waiting
                    // period at the beginning with nothing etc.)
                    if(m_bIsRestartScheduledQuick && m_hitobjects.size() > 0 &&
                       m_hitobjects[0]->getTime() > (long)cv_quick_retry_time.getInt())
                        m_music->setPositionMS(
                            max((long)0, m_hitobjects[0]->getTime() - (long)cv_quick_retry_time.getInt()));
                    m_bWasSeekFrame = true;

                    m_bIsRestartScheduledQuick = false;

                    // if there are calculations in there that need the hitobjects to be loaded, also applies
                    // speed/pitch
                    onModUpdate(false, false);
                }
            } else
                m_iCurMusicPos = (engine->getTimeReal() - m_fWaitTime) * 1000.0f * osu->getSpeedMultiplier();
        }

        // ugh. force update all hitobjects while waiting (necessary because of pvs optimization)
        long curPos = m_iCurMusicPos + (long)(cv_universal_offset.getFloat() * osu->getSpeedMultiplier()) +
                      cv_universal_offset_hardcoded.getInt() - m_selectedDifficulty2->getLocalOffset() -
                      m_selectedDifficulty2->getOnlineOffset() -
                      (m_selectedDifficulty2->getVersion() < 5 ? cv_old_beatmap_offset.getInt() : 0);
        if(curPos > -1)  // otherwise auto would already click elements that start at exactly 0 (while the map has not
                         // even started)
            curPos = -1;

        for(int i = 0; i < m_hitobjects.size(); i++) {
            m_hitobjects[i]->update(curPos);
        }
    }

    // only continue updating hitobjects etc. if we have loaded everything
    if(isLoading()) return;

    // handle music loading fail
    if(!m_music->isReady()) {
        m_iResourceLoadUpdateDelayHack++;  // HACKHACK: async loading takes 1 additional engine update() until both
                                           // isAsyncReady() and isReady() return true
        if(m_iResourceLoadUpdateDelayHack > 1 &&
           !m_bForceStreamPlayback)  // first: try loading a stream version of the music file
        {
            m_bForceStreamPlayback = true;
            unloadMusic();
            loadMusic(true);

            // we are waiting for an asynchronous start of the beatmap in the next update()
            m_bIsWaiting = true;
            m_fWaitTime = engine->getTimeReal();
        } else if(m_iResourceLoadUpdateDelayHack >
                  3)  // second: if that still doesn't work, stop and display an error message
        {
            osu->getNotificationOverlay()->addNotification("Couldn't load music file :(", 0xffff0000);
            stop(true);
        }
    }

    // detect and handle music end
    if(!m_bIsWaiting && m_music->isReady()) {
        const bool isMusicFinished = m_music->isFinished();

        // trigger virtual audio time after music finishes
        if(!isMusicFinished)
            m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
        else if(m_fAfterMusicIsFinishedVirtualAudioTimeStart < 0.0f)
            m_fAfterMusicIsFinishedVirtualAudioTimeStart = engine->getTimeReal();

        if(isMusicFinished) {
            // continue with virtual audio time until the last hitobject is done (plus sanity offset given via
            // osu_end_delay_time) because some beatmaps have hitobjects going until >= the exact end of the music ffs
            // NOTE: this overwrites m_iCurMusicPos for the rest of the update loop
            m_iCurMusicPos = (long)m_music->getLengthMS() +
                             (long)((engine->getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart) * 1000.0f);
        }

        const bool hasAnyHitObjects = (m_hitobjects.size() > 0);
        const bool isTimePastLastHitObjectPlusLenience =
            (m_iCurMusicPos > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() +
                               m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration() +
                               (long)cv_end_delay_time.getInt()));
        if(!hasAnyHitObjects || (cv_end_skip.getBool() && isTimePastLastHitObjectPlusLenience) ||
           (!cv_end_skip.getBool() && isMusicFinished)) {
            if(!m_bFailed) {
                stop(false);
                return;
            }
        }
    }

    // update timing (points)
    m_iCurMusicPosWithOffsets = m_iCurMusicPos + (long)(cv_universal_offset.getFloat() * osu->getSpeedMultiplier()) +
                                cv_universal_offset_hardcoded.getInt() - m_selectedDifficulty2->getLocalOffset() -
                                m_selectedDifficulty2->getOnlineOffset() -
                                (m_selectedDifficulty2->getVersion() < 5 ? cv_old_beatmap_offset.getInt() : 0);
    updateTimingPoints(m_iCurMusicPosWithOffsets);

    // Make sure we're not too far behind the liveplay
    if(is_spectating) {
        if(m_iCurMusicPos + 5000 < last_frame_ms) {
            debugLog("Spectator is too far behind, seeking to catch up to player...\n");
            i32 target = last_frame_ms - 2500;
            f32 percent = (f32)target / (f32)getLength();
            seekPercent(percent);
            return;
        }
    }

    if(is_watching || is_spectating) {
        LegacyReplay::Frame current_frame = spectated_replay[current_frame_idx];
        LegacyReplay::Frame next_frame = spectated_replay[current_frame_idx + 1];

        while(next_frame.cur_music_pos <= m_iCurMusicPosWithOffsets) {
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
            click.pos *= GameRules::getPlayfieldScaleFactor();
            click.pos += GameRules::getPlayfieldOffset();

            // Flag fix to simplify logic (stable sets both K1 and M1 when K1 is pressed)
            if(current_keys & LegacyReplay::K1) current_keys &= ~LegacyReplay::M1;
            if(current_keys & LegacyReplay::K2) current_keys &= ~LegacyReplay::M2;

            // Released key 1
            if(last_keys & LegacyReplay::K1 && !(current_keys & LegacyReplay::K1)) {
                osu->getHUD()->animateInputoverlay(1, false);
            }
            if(last_keys & LegacyReplay::M1 && !(current_keys & LegacyReplay::M1)) {
                osu->getHUD()->animateInputoverlay(3, false);
            }

            // Released key 2
            if(last_keys & LegacyReplay::K2 && !(current_keys & LegacyReplay::K2)) {
                osu->getHUD()->animateInputoverlay(2, false);
            }
            if(last_keys & LegacyReplay::M2 && !(current_keys & LegacyReplay::M2)) {
                osu->getHUD()->animateInputoverlay(4, false);
            }

            // Pressed key 1
            if(!(last_keys & LegacyReplay::K1) && current_keys & LegacyReplay::K1) {
                m_bPrevKeyWasKey1 = true;
                osu->getHUD()->animateInputoverlay(1, true);
                m_clicks.push_back(click);
                if(!m_bInBreak && !m_bIsInSkippableSection) osu->getScore()->addKeyCount(1);
            }
            if(!(last_keys & LegacyReplay::M1) && current_keys & LegacyReplay::M1) {
                m_bPrevKeyWasKey1 = true;
                osu->getHUD()->animateInputoverlay(3, true);
                m_clicks.push_back(click);
                if(!m_bInBreak && !m_bIsInSkippableSection) osu->getScore()->addKeyCount(3);
            }

            // Pressed key 2
            if(!(last_keys & LegacyReplay::K2) && current_keys & LegacyReplay::K2) {
                m_bPrevKeyWasKey1 = false;
                osu->getHUD()->animateInputoverlay(2, true);
                m_clicks.push_back(click);
                if(!m_bInBreak && !m_bIsInSkippableSection) osu->getScore()->addKeyCount(2);
            }
            if(!(last_keys & LegacyReplay::M2) && current_keys & LegacyReplay::M2) {
                m_bPrevKeyWasKey1 = false;
                osu->getHUD()->animateInputoverlay(4, true);
                m_clicks.push_back(click);
                if(!m_bInBreak && !m_bIsInSkippableSection) osu->getScore()->addKeyCount(4);
            }
        }

        f32 percent = 0.f;
        if(next_frame.milliseconds_since_last_frame > 0) {
            long ms_since_last_frame = m_iCurMusicPosWithOffsets - current_frame.cur_music_pos;
            percent = (f32)ms_since_last_frame / (f32)next_frame.milliseconds_since_last_frame;
        }

        m_interpolatedMousePos =
            Vector2{lerp(current_frame.x, next_frame.x, percent), lerp(current_frame.y, next_frame.y, percent)};

        if(cv_playfield_mirror_horizontal.getBool())
            m_interpolatedMousePos.y = GameRules::OSU_COORD_HEIGHT - m_interpolatedMousePos.y;
        if(cv_playfield_mirror_vertical.getBool())
            m_interpolatedMousePos.x = GameRules::OSU_COORD_WIDTH - m_interpolatedMousePos.x;

        if(cv_playfield_rotation.getFloat() != 0.0f) {
            m_interpolatedMousePos.x -= GameRules::OSU_COORD_WIDTH / 2;
            m_interpolatedMousePos.y -= GameRules::OSU_COORD_HEIGHT / 2;
            Vector3 coords3 = Vector3(m_interpolatedMousePos.x, m_interpolatedMousePos.y, 0);
            Matrix4 rot;
            rot.rotateZ(cv_playfield_rotation.getFloat());
            coords3 = coords3 * rot;
            coords3.x += GameRules::OSU_COORD_WIDTH / 2;
            coords3.y += GameRules::OSU_COORD_HEIGHT / 2;
            m_interpolatedMousePos.x = coords3.x;
            m_interpolatedMousePos.y = coords3.y;
        }

        m_interpolatedMousePos *= GameRules::getPlayfieldScaleFactor();
        m_interpolatedMousePos += GameRules::getPlayfieldOffset();
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
    m_iPreviousFollowPointObjectIndex = 0;
    m_iNPS = 0;
    m_iND = 0;
    m_iCurrentNumCircles = 0;
    m_iCurrentNumSliders = 0;
    m_iCurrentNumSpinners = 0;
    {
        bool blockNextNotes = false;

        const long pvs =
            !cv_mod_mafham.getBool()
                ? getPVS()
                : (m_hitobjects.size() > 0
                       ? (m_hitobjects[clamp<int>(m_iCurrentHitObjectIndex + cv_mod_mafham_render_livesize.getInt() + 1,
                                                  0, m_hitobjects.size() - 1)]
                              ->getTime() -
                          m_iCurMusicPosWithOffsets + 1500)
                       : getPVS());
        const bool usePVS = cv_pvs.getBool();

        const int notelockType = cv_notelock_type.getInt();
        const long tolerance2B = (long)cv_notelock_stable_tolerance2b.getInt();

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
            const bool isCircle = m_hitobjects[i]->type == HitObjectType::CIRCLE;
            const bool isSlider = m_hitobjects[i]->type == HitObjectType::SLIDER;
            const bool isSpinner = m_hitobjects[i]->type == HitObjectType::SPINNER;
            // ************ live pp block end ************** //

            // determine previous & next object time, used for auto + followpoints + warning arrows + empty section
            // skipping
            if(m_iNextHitObjectTime == 0) {
                if(m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets)
                    m_iNextHitObjectTime = m_hitobjects[i]->getTime();
                else {
                    m_currentHitObject = m_hitobjects[i];
                    const long actualPrevHitObjectTime = m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration();
                    m_iPreviousHitObjectTime = actualPrevHitObjectTime;

                    if(m_iCurMusicPosWithOffsets >
                       actualPrevHitObjectTime + (long)cv_followpoints_prevfadetime.getFloat())
                        m_iPreviousFollowPointObjectIndex = i;
                }
            }

            // PVS optimization
            if(usePVS) {
                if(m_hitobjects[i]->isFinished() &&
                   (m_iCurMusicPosWithOffsets - pvs >
                    m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration()))  // past objects
                {
                    // ************ live pp block start ************ //
                    if(isCircle) m_iCurrentNumCircles++;
                    if(isSlider) m_iCurrentNumSliders++;
                    if(isSpinner) m_iCurrentNumSpinners++;

                    m_iCurrentHitObjectIndex = i;
                    // ************ live pp block end ************** //

                    continue;
                }
                if(m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets + pvs)  // future objects
                    break;
            }

            // ************ live pp block start ************ //
            if(m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())
                m_iCurrentHitObjectIndex = i;
            // ************ live pp block end ************** //

            // main hitobject update
            m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);

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
                            blockNextNotes = (m_iCurMusicPosWithOffsets <= m_hitobjects[i]->getTime());

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
                                    m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
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
                                if(m_iCurMusicPosWithOffsets > m_hitobjects[m]->getTime()) {
                                    if(m_hitobjects[i]->getTime() >
                                       (m_hitobjects[m]->getTime() +
                                        m_hitobjects[m]->getDuration()))  // NOTE: 2b exception. only force miss if
                                                                          // objects are not overlapping.
                                        m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
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
            const long npsHalfGateSizeMS = (long)(500.0f * getSpeedMultiplier());
            if(m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets - npsHalfGateSizeMS &&
               m_hitobjects[i]->getTime() < m_iCurMusicPosWithOffsets + npsHalfGateSizeMS)
                m_iNPS++;

            // note density
            if(m_hitobjects[i]->isVisible()) m_iND++;
        }

        // miss hiterrorbar slots
        // this gets the closest previous unfinished hitobject, as well as all following hitobjects which are in 50
        // range and could be clicked
        if(cv_hiterrorbar_misaims.getBool()) {
            m_misaimObjects.clear();
            HitObject *lastUnfinishedHitObject = NULL;
            const long hitWindow50 = (long)getHitWindow50();
            for(int i = 0; i < m_hitobjects.size(); i++)  // this shouldn't hurt performance too much, since no
                                                          // expensive operations are happening within the loop
            {
                if(!m_hitobjects[i]->isFinished()) {
                    if(m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime())
                        lastUnfinishedHitObject = m_hitobjects[i];
                    else if(std::abs(m_hitobjects[i]->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
                        m_misaimObjects.push_back(m_hitobjects[i]);
                    else
                        break;
                }
            }
            if(lastUnfinishedHitObject != NULL &&
               std::abs(lastUnfinishedHitObject->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
                m_misaimObjects.insert(m_misaimObjects.begin(), lastUnfinishedHitObject);

            // now, go through the remaining clicks, and go through the unfinished hitobjects.
            // handle misaim clicks sequentially (setting the misaim flag on the hitobjects to only allow 1 entry in the
            // hiterrorbar for misses per object) clicks don't have to be consumed here, as they are deleted below
            // anyway
            for(int c = 0; c < m_clicks.size(); c++) {
                for(int i = 0; i < m_misaimObjects.size(); i++) {
                    if(m_misaimObjects[i]->hasMisAimed())  // only 1 slot per object!
                        continue;

                    m_misaimObjects[i]->misAimed();
                    const long delta = m_clicks[c].tms - (long)m_misaimObjects[i]->getTime();
                    osu->getHUD()->addHitError(delta, false, true);

                    break;  // the current click has been dealt with (and the hitobject has been misaimed)
                }
            }
        }

        // all remaining clicks which have not been consumed by any hitobjects can safely be deleted
        if(m_clicks.size() > 0) {
            if(cv_play_hitsound_on_click_while_playing.getBool()) osu->getSkin()->playHitCircleSound(0, 0.f, 0);

            // nightmare mod: extra clicks = sliderbreak
            if((osu->getModNightmare() || cv_mod_jigsaw1.getBool()) && !m_bIsInSkippableSection && !m_bInBreak &&
               m_iCurrentHitObjectIndex > 0) {
                addSliderBreak();
                addHitResult(NULL, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                             false);  // only decrease health
            }

            m_clicks.clear();
        }
    }

    // empty section detection & skipping
    if(m_hitobjects.size() > 0) {
        const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000);  // Mc
        const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;
        if(nextHitObjectDelta > 0 && nextHitObjectDelta > (long)cv_skip_time.getInt() &&
           m_iCurMusicPosWithOffsets > (m_iPreviousHitObjectTime + legacyOffset))
            m_bIsInSkippableSection = true;
        else if(!cv_end_skip.getBool() && nextHitObjectDelta < 0)
            m_bIsInSkippableSection = true;
        else
            m_bIsInSkippableSection = false;

        osu->m_chat->updateVisibility();

        // While we want to allow the chat to pop up during breaks, we don't
        // want to be able to skip after the start in multiplayer rooms
        if(bancho.is_playing_a_multi_map() && m_iCurrentHitObjectIndex > 0) {
            m_bIsInSkippableSection = false;
        }
    }

    // warning arrow logic
    if(m_hitobjects.size() > 0) {
        const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000);  // Mc
        const long minGapSize = 1000;
        const long lastVisibleMin = 400;
        const long blinkDelta = 100;

        const long gapSize = m_iNextHitObjectTime - (m_iPreviousHitObjectTime + legacyOffset);
        const long nextDelta = (m_iNextHitObjectTime - m_iCurMusicPosWithOffsets);
        const bool drawWarningArrows = gapSize > minGapSize && nextDelta > 0;
        if(drawWarningArrows &&
           ((nextDelta <= lastVisibleMin + blinkDelta * 13 && nextDelta > lastVisibleMin + blinkDelta * 12) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 11 && nextDelta > lastVisibleMin + blinkDelta * 10) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 9 && nextDelta > lastVisibleMin + blinkDelta * 8) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 7 && nextDelta > lastVisibleMin + blinkDelta * 6) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 5 && nextDelta > lastVisibleMin + blinkDelta * 4) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 3 && nextDelta > lastVisibleMin + blinkDelta * 2) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 1 && nextDelta > lastVisibleMin)))
            m_bShouldFlashWarningArrows = true;
        else
            m_bShouldFlashWarningArrows = false;
    }

    // break time detection, and background fade during breaks
    const DatabaseBeatmap::BREAK breakEvent =
        getBreakForTimeRange(m_iPreviousHitObjectTime, m_iCurMusicPosWithOffsets, m_iNextHitObjectTime);
    const bool isInBreak = ((int)m_iCurMusicPosWithOffsets >= breakEvent.startTime &&
                            (int)m_iCurMusicPosWithOffsets <= breakEvent.endTime);
    if(isInBreak != m_bInBreak) {
        m_bInBreak = !m_bInBreak;

        if(!cv_background_dont_fade_during_breaks.getBool() || m_fBreakBackgroundFade != 0.0f) {
            if(m_bInBreak && !cv_background_dont_fade_during_breaks.getBool()) {
                const int breakDuration = breakEvent.endTime - breakEvent.startTime;
                if(breakDuration > (int)(cv_background_fade_min_duration.getFloat() * 1000.0f))
                    anim->moveLinear(&m_fBreakBackgroundFade, 1.0f, cv_background_fade_in_duration.getFloat(), true);
            } else
                anim->moveLinear(&m_fBreakBackgroundFade, 0.0f, cv_background_fade_out_duration.getFloat(), true);
        }
    }

    // section pass/fail logic
    if(m_hitobjects.size() > 0) {
        const long minGapSize = 2880;
        const long fadeStart = 1280;
        const long fadeEnd = 1480;

        const long gapSize = m_iNextHitObjectTime - m_iPreviousHitObjectTime;
        const long start =
            (gapSize / 2 > minGapSize ? m_iPreviousHitObjectTime + (gapSize / 2) : m_iNextHitObjectTime - minGapSize);
        const long nextDelta = m_iCurMusicPosWithOffsets - start;
        const bool inSectionPassFail =
            (gapSize > minGapSize && nextDelta > 0) && m_iCurMusicPosWithOffsets > m_hitobjects[0]->getTime() &&
            m_iCurMusicPosWithOffsets <
                (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() +
                 m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration()) &&
            !m_bFailed && m_bInBreak && (breakEvent.endTime - breakEvent.startTime) > minGapSize;

        const bool passing = (m_fHealth >= 0.5);

        // draw logic
        if(passing) {
            if(inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280) ||
                                     (nextDelta <= 230 && nextDelta >= 160) || (nextDelta <= 100 && nextDelta >= 20))) {
                const f32 fadeAlpha = 1.0f - (f32)(nextDelta - fadeStart) / (f32)(fadeEnd - fadeStart);
                m_fShouldFlashSectionPass = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
            } else
                m_fShouldFlashSectionPass = 0.0f;
        } else {
            if(inSectionPassFail &&
               ((nextDelta <= fadeEnd && nextDelta >= 280) || (nextDelta <= 230 && nextDelta >= 130))) {
                const f32 fadeAlpha = 1.0f - (f32)(nextDelta - fadeStart) / (f32)(fadeEnd - fadeStart);
                m_fShouldFlashSectionFail = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
            } else
                m_fShouldFlashSectionFail = 0.0f;
        }

        // sound logic
        if(inSectionPassFail) {
            if(m_iPreviousSectionPassFailTime != start &&
               ((passing && nextDelta >= 20) || (!passing && nextDelta >= 130))) {
                m_iPreviousSectionPassFailTime = start;

                if(!wasSeekFrame) {
                    if(passing)
                        engine->getSound()->play(osu->getSkin()->getSectionPassSound());
                    else
                        engine->getSound()->play(osu->getSkin()->getSectionFailSound());
                }
            }
        }
    }

    // hp drain & failing
    // handle constant drain
    {
        if(m_fDrainRate > 0.0) {
            if(m_bIsPlaying                  // not paused
               && !m_bInBreak                // not in a break
               && !m_bIsInSkippableSection)  // not in a skippable section
            {
                // special case: break drain edge cases
                bool drainAfterLastHitobjectBeforeBreakStart = (m_selectedDifficulty2->getVersion() < 8);

                const bool isBetweenHitobjectsAndBreak = (int)m_iPreviousHitObjectTime <= breakEvent.startTime &&
                                                         (int)m_iNextHitObjectTime >= breakEvent.endTime &&
                                                         m_iCurMusicPosWithOffsets > m_iPreviousHitObjectTime;
                const bool isLastHitobjectBeforeBreakStart =
                    isBetweenHitobjectsAndBreak && (int)m_iCurMusicPosWithOffsets <= breakEvent.startTime;

                if(!isBetweenHitobjectsAndBreak ||
                   (drainAfterLastHitobjectBeforeBreakStart && isLastHitobjectBeforeBreakStart)) {
                    // special case: spinner nerf
                    f64 spinnerDrainNerf = isSpinnerActive() ? 0.25 : 1.0;
                    addHealth(-m_fDrainRate * engine->getFrameTime() * (f64)getSpeedMultiplier() * spinnerDrainNerf,
                              false);
                }
            }
        }
    }

    // revive in mp
    if(m_fHealth > 0.999 && osu->getScore()->isDead()) osu->getScore()->setDead(false);

    // handle fail animation
    if(m_bFailed) {
        if(m_fFailAnim <= 0.0f) {
            if(m_music->isPlaying() || !osu->getPauseMenu()->isVisible()) {
                engine->getSound()->pause(m_music);
                m_bIsPaused = true;

                osu->getPauseMenu()->setVisible(true);
                osu->updateConfineCursor();
            }
        } else
            m_music->setFrequency(m_fMusicFrequencyBackup * m_fFailAnim > 100 ? m_fMusicFrequencyBackup * m_fFailAnim
                                                                              : 100);
    }
}

void Beatmap::broadcast_spectator_frames() {
    if(bancho.spectators.empty()) return;

    Packet packet;
    packet.id = OUT_SPECTATE_FRAMES;
    write<i32>(&packet, 0);
    write<u16>(&packet, frame_batch.size());
    for(auto batch : frame_batch) {
        write<LiveReplayFrame>(&packet, batch);
    }
    write<u8>(&packet, LiveReplayBundle::Action::NONE);
    write<ScoreFrame>(&packet, ScoreFrame::get());
    write<u16>(&packet, spectator_sequence++);
    send_packet(packet);

    frame_batch.clear();
    last_spectator_broadcast = engine->getTime();
}

void Beatmap::write_frame() {
    if(!m_bIsPlaying || m_bFailed || is_watching || is_spectating) return;

    long delta = m_iCurMusicPosWithOffsets - last_event_ms;
    if(delta < 0) return;
    if(delta == 0 && last_keys == current_keys) return;

    Vector2 pos = pixels2OsuCoords(getCursorPos());
    if(cv_playfield_mirror_horizontal.getBool()) pos.y = GameRules::OSU_COORD_HEIGHT - pos.y;
    if(cv_playfield_mirror_vertical.getBool()) pos.x = GameRules::OSU_COORD_WIDTH - pos.x;
    if(cv_playfield_rotation.getFloat() != 0.0f) {
        pos.x -= GameRules::OSU_COORD_WIDTH / 2;
        pos.y -= GameRules::OSU_COORD_HEIGHT / 2;
        Vector3 coords3 = Vector3(pos.x, pos.y, 0);
        Matrix4 rot;
        rot.rotateZ(-cv_playfield_rotation.getFloat());
        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;
        pos.x = coords3.x;
        pos.y = coords3.y;
    }

    live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = m_iCurMusicPosWithOffsets,
        .milliseconds_since_last_frame = delta,
        .x = pos.x,
        .y = pos.y,
        .key_flags = current_keys,
    });

    frame_batch.push_back(LiveReplayFrame{
        .key_flags = current_keys,
        .padding = 0,
        .mouse_x = pos.x,
        .mouse_y = pos.y,
        .time = (i32)m_iCurMusicPos,  // NOTE: might be incorrect
    });

    last_event_time = m_fLastRealTimeForInterpolationDelta;
    last_event_ms = m_iCurMusicPosWithOffsets;
    last_keys = current_keys;

    if(!bancho.spectators.empty() && engine->getTime() > last_spectator_broadcast + 1.0) {
        broadcast_spectator_frames();
    }
}

void Beatmap::onModUpdate(bool rebuildSliderVertexBuffers, bool recomputeDrainRate) {
    if(cv_debug.getBool()) debugLog("Beatmap::onModUpdate() @ %f\n", engine->getTime());

    updatePlayfieldMetrics();
    updateHitobjectMetrics();

    if(recomputeDrainRate) computeDrainRate();

    if(m_music != NULL) {
        m_music->setSpeed(osu->getSpeedMultiplier());
    }

    // recalculate slider vertexbuffers
    if(osu->getModHR() != m_bWasHREnabled ||
       cv_playfield_mirror_horizontal.getBool() != m_bWasHorizontalMirrorEnabled ||
       cv_playfield_mirror_vertical.getBool() != m_bWasVerticalMirrorEnabled) {
        m_bWasHREnabled = osu->getModHR();
        m_bWasHorizontalMirrorEnabled = cv_playfield_mirror_horizontal.getBool();
        m_bWasVerticalMirrorEnabled = cv_playfield_mirror_vertical.getBool();

        calculateStacks();

        if(rebuildSliderVertexBuffers) updateSliderVertexBuffers();
    }
    if(osu->getModEZ() != m_bWasEZEnabled) {
        calculateStacks();

        m_bWasEZEnabled = osu->getModEZ();
        if(rebuildSliderVertexBuffers) updateSliderVertexBuffers();
    }
    if(m_fHitcircleDiameter != m_fPrevHitCircleDiameter && m_hitobjects.size() > 0) {
        calculateStacks();

        m_fPrevHitCircleDiameter = m_fHitcircleDiameter;
        if(rebuildSliderVertexBuffers) updateSliderVertexBuffers();
    }
    if(cv_playfield_rotation.getFloat() != m_fPrevPlayfieldRotationFromConVar) {
        m_fPrevPlayfieldRotationFromConVar = cv_playfield_rotation.getFloat();
        if(rebuildSliderVertexBuffers) updateSliderVertexBuffers();
    }
    if(cv_mod_mafham.getBool() != m_bWasMafhamEnabled) {
        m_bWasMafhamEnabled = cv_mod_mafham.getBool();
        for(int i = 0; i < m_hitobjects.size(); i++) {
            m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);
        }
    }

    resetLiveStarsTasks();
}

void Beatmap::resetLiveStarsTasks() {
    if(cv_debug.getBool()) debugLog("Beatmap::resetLiveStarsTasks() called\n");

    osu->getHUD()->live_pp = 0.0;
    osu->getHUD()->live_stars = 0.0;

    last_calculated_hitobject = -1;
}

// HACK: Updates buffering state and pauses/unpauses the music!
bool Beatmap::isBuffering() {
    if(!is_spectating) return false;

    i32 leeway = last_frame_ms - last_event_ms;
    if(is_buffering) {
        if(leeway >= 2500) {
            debugLog("PAUSING: leeway: %i, last_event: %d, last_frame: %d\n", leeway, last_event_ms, last_frame_ms);
            engine->getSound()->play(m_music);
            m_bIsPlaying = true;
            is_buffering = false;
        }
    } else {
        if(leeway < 1000) {
            debugLog("UNPAUSING: leeway: %i, last_event: %d, last_frame: %d\n", leeway, last_event_ms, last_frame_ms);
            engine->getSound()->pause(m_music);
            m_bIsPlaying = false;
            is_buffering = true;
        }
    }

    return is_buffering;
}

bool Beatmap::isLoading() {
    return (isActuallyLoading() || isBuffering() ||
            (bancho.is_playing_a_multi_map() && !bancho.room.all_players_loaded));
}

bool Beatmap::isActuallyLoading() {
    return (!engine->getSound()->isReady() || !m_music->isAsyncReady() || m_bIsPreLoading);
}

Vector2 Beatmap::pixels2OsuCoords(Vector2 pixelCoords) const {
    // un-first-person
    if(cv_mod_fps.getBool()) {
        // HACKHACK: this is the worst hack possible (engine->isDrawing()), but it works
        // the problem is that this same function is called while draw()ing and update()ing
        if(!((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) ||
             !(osu->getModAuto() || osu->getModAutopilot())))
            pixelCoords += getFirstPersonCursorDelta();
    }

    // un-offset and un-scale, reverse order
    pixelCoords -= m_vPlayfieldOffset;
    pixelCoords /= m_fScaleFactor;

    return pixelCoords;
}

Vector2 Beatmap::osuCoords2Pixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv_playfield_mirror_horizontal.getBool()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv_playfield_mirror_vertical.getBool()) coords.x = GameRules::OSU_COORD_WIDTH - coords.x;

    // wobble
    if(cv_mod_wobble.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
        coords.x += std::sin((m_iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                             cv_mod_wobble_frequency.getFloat()) *
                    cv_mod_wobble_strength.getFloat();
        coords.y += std::sin((m_iCurMusicPos / 1000.0f) * 4 * speedMultiplierCompensation *
                             cv_mod_wobble_frequency.getFloat()) *
                    cv_mod_wobble_strength.getFloat();
    }

    // wobble2
    if(cv_mod_wobble2.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
        Vector2 centerDelta = coords - Vector2(GameRules::OSU_COORD_WIDTH, GameRules::OSU_COORD_HEIGHT) / 2;
        coords.x += centerDelta.x * 0.25f *
                    std::sin((m_iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                             cv_mod_wobble_frequency.getFloat()) *
                    cv_mod_wobble_strength.getFloat();
        coords.y += centerDelta.y * 0.25f *
                    std::sin((m_iCurMusicPos / 1000.0f) * 3 * speedMultiplierCompensation *
                             cv_mod_wobble_frequency.getFloat()) *
                    cv_mod_wobble_strength.getFloat();
    }

    // rotation
    if(m_fPlayfieldRotation + cv_playfield_rotation.getFloat() != 0.0f) {
        coords.x -= GameRules::OSU_COORD_WIDTH / 2;
        coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

        Vector3 coords3 = Vector3(coords.x, coords.y, 0);
        Matrix4 rot;
        rot.rotateZ(m_fPlayfieldRotation + cv_playfield_rotation.getFloat());

        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;

        coords.x = coords3.x;
        coords.y = coords3.y;
    }

    // if wobble, clamp coordinates
    if(cv_mod_wobble.getBool() || cv_mod_wobble2.getBool()) {
        coords.x = clamp<f32>(coords.x, 0.0f, GameRules::OSU_COORD_WIDTH);
        coords.y = clamp<f32>(coords.y, 0.0f, GameRules::OSU_COORD_HEIGHT);
    }

    if(m_bFailed) {
        f32 failTimePercentInv = 1.0f - m_fFailAnim;  // goes from 0 to 1 over the duration of osu_fail_time
        failTimePercentInv *= failTimePercentInv;

        coords.x -= GameRules::OSU_COORD_WIDTH / 2;
        coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

        Vector3 coords3 = Vector3(coords.x, coords.y, 0);
        Matrix4 rot;
        rot.rotateZ(failTimePercentInv * 60.0f);

        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;

        coords.x = coords3.x + failTimePercentInv * GameRules::OSU_COORD_WIDTH * 0.25f;
        coords.y = coords3.y + failTimePercentInv * GameRules::OSU_COORD_HEIGHT * 1.25f;
    }

    // scale and offset
    coords *= m_fScaleFactor;
    coords += m_vPlayfieldOffset;  // the offset is already scaled, just add it

    // first person mod, centered cursor
    if(cv_mod_fps.getBool()) {
        // this is the worst hack possible (engine->isDrawing()), but it works
        // the problem is that this same function is called while draw()ing and update()ing
        if((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) ||
           !(osu->getModAuto() || osu->getModAutopilot()))
            coords += getFirstPersonCursorDelta();
    }

    return coords;
}

Vector2 Beatmap::osuCoords2RawPixels(Vector2 coords) const {
    // scale and offset
    coords *= m_fScaleFactor;
    coords += m_vPlayfieldOffset;  // the offset is already scaled, just add it

    return coords;
}

Vector2 Beatmap::osuCoords2LegacyPixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv_playfield_mirror_horizontal.getBool()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv_playfield_mirror_vertical.getBool()) coords.x = GameRules::OSU_COORD_WIDTH - coords.x;

    // rotation
    if(m_fPlayfieldRotation + cv_playfield_rotation.getFloat() != 0.0f) {
        coords.x -= GameRules::OSU_COORD_WIDTH / 2;
        coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

        Vector3 coords3 = Vector3(coords.x, coords.y, 0);
        Matrix4 rot;
        rot.rotateZ(m_fPlayfieldRotation + cv_playfield_rotation.getFloat());

        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;

        coords.x = coords3.x;
        coords.y = coords3.y;
    }

    // VR center
    coords.x -= GameRules::OSU_COORD_WIDTH / 2;
    coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

    return coords;
}

Vector2 Beatmap::getMousePos() const {
    if((is_watching || is_spectating) && !m_bIsPaused) {
        return m_interpolatedMousePos;
    } else {
        return engine->getMouse()->getPos();
    }
}

Vector2 Beatmap::getCursorPos() const {
    if(cv_mod_fps.getBool() && !m_bIsPaused) {
        if(osu->getModAuto() || osu->getModAutopilot())
            return m_vAutoCursorPos;
        else
            return m_vPlayfieldCenter;
    } else if(osu->getModAuto() || osu->getModAutopilot())
        return m_vAutoCursorPos;
    else {
        Vector2 pos = getMousePos();
        if(cv_mod_shirone.getBool() && osu->getScore()->getCombo() > 0)  // <3
            return pos + Vector2(std::sin((m_iCurMusicPos / 20.0f) * 1.15f) *
                                     ((f32)osu->getScore()->getCombo() / cv_mod_shirone_combo.getFloat()),
                                 std::cos((m_iCurMusicPos / 20.0f) * 1.3f) *
                                     ((f32)osu->getScore()->getCombo() / cv_mod_shirone_combo.getFloat()));
        else
            return pos;
    }
}

Vector2 Beatmap::getFirstPersonCursorDelta() const {
    return m_vPlayfieldCenter - (osu->getModAuto() || osu->getModAutopilot() ? m_vAutoCursorPos : getMousePos());
}

FinishedScore Beatmap::saveAndSubmitScore(bool quit) {
    // calculate stars
    f64 aim = 0.0;
    f64 aimSliderFactor = 0.0;
    f64 speed = 0.0;
    f64 speedNotes = 0.0;
    const std::string &osuFilePath = m_selectedDifficulty2->getFilePath();
    const f32 AR = getAR();
    const f32 CS = getCS();
    const f32 OD = getOD();
    const f32 speedMultiplier = osu->getSpeedMultiplier();  // NOTE: not this->getSpeedMultiplier()!
    const bool relax = osu->getModRelax();
    const bool touchDevice = osu->getModTD();

    DatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres =
        DatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, AR, CS, speedMultiplier);
    const f64 totalStars = DifficultyCalculator::calculateStarDiffForHitObjects(
        diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aim, &aimSliderFactor, &speed, &speedNotes,
        -1, &m_aimStrains, &m_speedStrains);

    m_fAimStars = (f32)aim;
    m_fSpeedStars = (f32)speed;

    // calculate final pp
    const int numHitObjects = m_hitobjects.size();
    const int numCircles = m_selectedDifficulty2->getNumCircles();
    const int numSliders = m_selectedDifficulty2->getNumSliders();
    const int numSpinners = m_selectedDifficulty2->getNumSpinners();
    const int highestCombo = osu->getScore()->getComboMax();
    const int numMisses = osu->getScore()->getNumMisses();
    const int num300s = osu->getScore()->getNum300s();
    const int num100s = osu->getScore()->getNum100s();
    const int num50s = osu->getScore()->getNum50s();
    const f32 pp = DifficultyCalculator::calculatePPv2(
        osu->getScore()->getModsLegacy(), speedMultiplier, AR, OD, aim, aimSliderFactor, speed, speedNotes, numCircles,
        numSliders, numSpinners, m_iMaxPossibleCombo, highestCombo, numMisses, num300s, num100s, num50s);
    osu->getScore()->setStarsTomTotal(totalStars);
    osu->getScore()->setStarsTomAim(m_fAimStars);
    osu->getScore()->setStarsTomSpeed(m_fSpeedStars);
    osu->getScore()->setPPv2(pp);

    // save local score, but only under certain conditions
    bool isComplete = (num300s + num100s + num50s + numMisses >= numHitObjects);
    bool isZero = (osu->getScore()->getScore() < 1);
    bool isCheated = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax())) ||
                     osu->getScore()->isUnranked() || is_watching || is_spectating;

    FinishedScore score;
    UString client_ver = "neosu-" OS_NAME "-" NEOSU_STREAM "-";
    client_ver.append(UString::format("%.2f", cv_version.getFloat()));
    score.client = client_ver.toUtf8();

    score.unixTimestamp =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // @neonet: check if we're connected to neonet
    if(bancho.is_online()) {
        score.player_id = bancho.user_id;
        score.playerName = bancho.username.toUtf8();
        score.server = bancho.endpoint.toUtf8();
    } else {
        auto local_name = cv_name.getString();
        score.playerName = local_name.toUtf8();
    }
    score.passed = isComplete && !isZero && !osu->getScore()->hasDied();
    score.grade = score.passed ? osu->getScore()->getGrade() : FinishedScore::Grade::F;
    score.diff2 = m_selectedDifficulty2;
    score.ragequit = quit;
    score.play_time_ms = m_iCurMusicPos / osu->getSpeedMultiplier();

    // osu!stable doesn't submit scores of less than 7 seconds
    isZero |= (score.play_time_ms < 7000);

    score.num300s = osu->getScore()->getNum300s();
    score.num100s = osu->getScore()->getNum100s();
    score.num50s = osu->getScore()->getNum50s();
    score.numGekis = osu->getScore()->getNum300gs();
    score.numKatus = osu->getScore()->getNum100ks();
    score.numMisses = osu->getScore()->getNumMisses();
    score.score = osu->getScore()->getScore();
    score.comboMax = osu->getScore()->getComboMax();
    score.perfect = (m_iMaxPossibleCombo > 0 && score.comboMax > 0 && score.comboMax >= m_iMaxPossibleCombo);
    score.numSliderBreaks = osu->getScore()->getNumSliderBreaks();
    score.unstableRate = osu->getScore()->getUnstableRate();
    score.hitErrorAvgMin = osu->getScore()->getHitErrorAvgMin();
    score.hitErrorAvgMax = osu->getScore()->getHitErrorAvgMax();
    score.speedMultiplier = osu->getSpeedMultiplier();
    score.CS = CS;
    score.AR = AR;
    score.OD = getOD();
    score.HP = getHP();
    score.maxPossibleCombo = m_iMaxPossibleCombo;
    score.numHitObjects = numHitObjects;
    score.numCircles = numCircles;
    score.modsLegacy = osu->getScore()->getModsLegacy();

    std::vector<ConVar *> allExperimentalMods = osu->getExperimentalMods();
    for(int i = 0; i < allExperimentalMods.size(); i++) {
        if(allExperimentalMods[i]->getBool()) {
            score.experimentalModsConVars.append(allExperimentalMods[i]->getName());
            score.experimentalModsConVars.append(";");
        }
    }

    score.beatmap_hash = m_selectedDifficulty2->getMD5Hash();  // NOTE: necessary for "Use Mods"
    score.replay = live_replay;

    // @PPV3: store ppv3 data if not already done. also f64 check replay is marked correctly
    score.ppv2_version = 20220902;
    score.ppv2_score = pp;
    score.ppv2_total_stars = totalStars;
    score.ppv2_aim_stars = aim;
    score.ppv2_speed_stars = speed;

    if(!isCheated) {
        RichPresence::onPlayEnd(quit);

        if(bancho.submit_scores() && !isZero && vanilla) {
            score.server = bancho.endpoint.toUtf8();
            submit_score(score);
            // XXX: Save bancho_score_id after getting submission result
        }

        if(score.passed) {
            int scoreIndex = osu->getSongBrowser()->getDatabase()->addScore(score);
            if(scoreIndex == -1) {
                osu->getNotificationOverlay()->addNotification("Failed saving score!", 0xffff0000, false, 3.0f);
            }
        }
    }

    if(!bancho.spectators.empty()) {
        broadcast_spectator_frames();

        Packet packet;
        packet.id = OUT_SPECTATE_FRAMES;
        write<i32>(&packet, 0);
        write<u16>(&packet, 0);
        write<u8>(&packet, isComplete ? LiveReplayBundle::Action::COMPLETION : LiveReplayBundle::Action::FAIL);
        write<ScoreFrame>(&packet, ScoreFrame::get());
        write<u16>(&packet, spectator_sequence++);
        send_packet(packet);
    }

    // special case: incomplete scores should NEVER show pp, even if auto
    if(!isComplete) {
        osu->getScore()->setPPv2(0.0f);
    }

    return score;
}

void Beatmap::updateAutoCursorPos() {
    m_vAutoCursorPos = m_vPlayfieldCenter;
    m_vAutoCursorPos.y *= 2.5f;  // start moving in offscreen from bottom

    if(!m_bIsPlaying && !m_bIsPaused) {
        m_vAutoCursorPos = m_vPlayfieldCenter;
        return;
    }
    if(m_hitobjects.size() < 1) {
        m_vAutoCursorPos = m_vPlayfieldCenter;
        return;
    }

    const long curMusicPos = m_iCurMusicPosWithOffsets;

    // general
    long prevTime = 0;
    long nextTime = m_hitobjects[0]->getTime();
    Vector2 prevPos = m_vAutoCursorPos;
    Vector2 curPos = m_vAutoCursorPos;
    Vector2 nextPos = m_vAutoCursorPos;
    bool haveCurPos = false;

    // dance
    int nextPosIndex = 0;

    if(m_hitobjects[0]->getTime() < (long)cv_early_note_time.getInt())
        prevTime = -(long)cv_early_note_time.getInt() * getSpeedMultiplier();

    if(osu->getModAuto()) {
        bool autoDanceOverride = false;
        for(int i = 0; i < m_hitobjects.size(); i++) {
            HitObject *o = m_hitobjects[i];

            // get previous object
            if(o->getTime() <= curMusicPos) {
                prevTime = o->getTime() + o->getDuration();
                prevPos = o->getAutoCursorPos(curMusicPos);
                if(o->getDuration() > 0 && curMusicPos - o->getTime() <= o->getDuration()) {
                    if(cv_auto_cursordance.getBool()) {
                        Slider *sliderPointer = dynamic_cast<Slider *>(o);
                        if(sliderPointer != NULL) {
                            const std::vector<Slider::SLIDERCLICK> &clicks = sliderPointer->getClicks();

                            // start
                            prevTime = o->getTime();
                            prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));

                            long biggestPrevious = 0;
                            long smallestNext = (std::numeric_limits<long>::max)();
                            bool allFinished = true;
                            long endTime = 0;

                            // middle clicks
                            for(int c = 0; c < clicks.size(); c++) {
                                // get previous click
                                if(clicks[c].time <= curMusicPos && clicks[c].time > biggestPrevious) {
                                    biggestPrevious = clicks[c].time;
                                    prevTime = clicks[c].time;
                                    prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
                                }

                                // get next click
                                if(clicks[c].time > curMusicPos && clicks[c].time < smallestNext) {
                                    smallestNext = clicks[c].time;
                                    nextTime = clicks[c].time;
                                    nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
                                }

                                // end hack
                                if(!clicks[c].finished)
                                    allFinished = false;
                                else if(clicks[c].time > endTime)
                                    endTime = clicks[c].time;
                            }

                            // end
                            if(allFinished) {
                                // hack for slider without middle clicks
                                if(endTime == 0) endTime = o->getTime();

                                prevTime = endTime;
                                prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
                                nextTime = o->getTime() + o->getDuration();
                                nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
                            }

                            haveCurPos = false;
                            autoDanceOverride = true;
                            break;
                        }
                    }

                    haveCurPos = true;
                    curPos = prevPos;
                    break;
                }
            }

            // get next object
            if(o->getTime() > curMusicPos) {
                nextPosIndex = i;
                if(!autoDanceOverride) {
                    nextPos = o->getAutoCursorPos(curMusicPos);
                    nextTime = o->getTime();
                }
                break;
            }
        }
    } else if(osu->getModAutopilot()) {
        for(int i = 0; i < m_hitobjects.size(); i++) {
            HitObject *o = m_hitobjects[i];

            // get previous object
            if(o->isFinished() || (curMusicPos > o->getTime() + o->getDuration() +
                                                     (long)(getHitWindow50() * cv_autopilot_lenience.getFloat()))) {
                prevTime = o->getTime() + o->getDuration() + o->getAutopilotDelta();
                prevPos = o->getAutoCursorPos(curMusicPos);
            } else if(!o->isFinished())  // get next object
            {
                nextPosIndex = i;
                nextPos = o->getAutoCursorPos(curMusicPos);
                nextTime = o->getTime();

                // wait for the user to click
                if(curMusicPos >= nextTime + o->getDuration()) {
                    haveCurPos = true;
                    curPos = nextPos;

                    // long delta = curMusicPos - (nextTime + o->getDuration());
                    o->setAutopilotDelta(curMusicPos - (nextTime + o->getDuration()));
                } else if(o->getDuration() > 0 && curMusicPos >= nextTime)  // handle objects with duration
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
            percent = (f32)((long)curMusicPos - prevTime) / (f32)(nextTime - prevTime);

        percent = clamp<f32>(percent, 0.0f, 1.0f);

        // scaled distance (not osucoords)
        f32 distance = (nextPos - prevPos).length();
        if(distance > m_fHitcircleDiameter * 1.05f)  // snap only if not in a stream (heuristic)
        {
            int numIterations = clamp<int>(
                osu->getModAutopilot() ? cv_autopilot_snapping_strength.getInt() : cv_auto_snapping_strength.getInt(),
                0, 42);
            for(int i = 0; i < numIterations; i++) {
                percent = (-percent) * (percent - 2.0f);
            }
        } else  // in a stream
        {
            m_iAutoCursorDanceIndex = nextPosIndex;
        }

        m_vAutoCursorPos = prevPos + (nextPos - prevPos) * percent;

        if(cv_auto_cursordance.getBool() && !osu->getModAutopilot()) {
            Vector3 dir = Vector3(nextPos.x, nextPos.y, 0) - Vector3(prevPos.x, prevPos.y, 0);
            Vector3 center = dir * 0.5f;
            Matrix4 worldMatrix;
            worldMatrix.translate(center);
            worldMatrix.rotate((1.0f - percent) * 180.0f * (m_iAutoCursorDanceIndex % 2 == 0 ? 1 : -1), 0, 0, 1);
            Vector3 fancyAutoCursorPos = worldMatrix * center;
            m_vAutoCursorPos =
                prevPos + (nextPos - prevPos) * 0.5f + Vector2(fancyAutoCursorPos.x, fancyAutoCursorPos.y);
        }
    }
}

void Beatmap::updatePlayfieldMetrics() {
    m_fScaleFactor = GameRules::getPlayfieldScaleFactor();
    m_vPlayfieldSize = GameRules::getPlayfieldSize();
    m_vPlayfieldOffset = GameRules::getPlayfieldOffset();
    m_vPlayfieldCenter = GameRules::getPlayfieldCenter();
}

void Beatmap::updateHitobjectMetrics() {
    Skin *skin = osu->getSkin();

    m_fRawHitcircleDiameter = GameRules::getRawHitCircleDiameter(getCS());
    m_fXMultiplier = GameRules::getHitCircleXMultiplier();
    m_fHitcircleDiameter = GameRules::getRawHitCircleDiameter(getCS()) * GameRules::getHitCircleXMultiplier();

    const f32 osuCoordScaleMultiplier = (m_fHitcircleDiameter / m_fRawHitcircleDiameter);
    m_fNumberScale = (m_fRawHitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) *
                     osuCoordScaleMultiplier * cv_number_scale_multiplier.getFloat();
    m_fHitcircleOverlapScale =
        (m_fRawHitcircleDiameter / (160.0f)) * osuCoordScaleMultiplier * cv_number_scale_multiplier.getFloat();

    const f32 followcircle_size_multiplier = 2.4f;
    const f32 sliderFollowCircleDiameterMultiplier =
        (osu->getModNightmare() || cv_mod_jigsaw2.getBool()
             ? (1.0f * (1.0f - cv_mod_jigsaw_followcircle_radius_factor.getFloat()) +
                cv_mod_jigsaw_followcircle_radius_factor.getFloat() * followcircle_size_multiplier)
             : followcircle_size_multiplier);
    m_fSliderFollowCircleDiameter = m_fHitcircleDiameter * sliderFollowCircleDiameterMultiplier;
}

void Beatmap::updateSliderVertexBuffers() {
    updatePlayfieldMetrics();
    updateHitobjectMetrics();

    m_bWasEZEnabled = osu->getModEZ();                // to avoid useless f64 updates in onModUpdate()
    m_fPrevHitCircleDiameter = m_fHitcircleDiameter;  // same here
    m_fPrevPlayfieldRotationFromConVar = cv_playfield_rotation.getFloat();  // same here

    debugLog("Beatmap::updateSliderVertexBuffers() for %i hitobjects ...\n", m_hitobjects.size());

    for(int i = 0; i < m_hitobjects.size(); i++) {
        Slider *sliderPointer = dynamic_cast<Slider *>(m_hitobjects[i]);
        if(sliderPointer != NULL) sliderPointer->rebuildVertexBuffer();
    }
}

void Beatmap::calculateStacks() {
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

    if(getSelectedDifficulty2()->getVersion() > 5) {
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

void Beatmap::computeDrainRate() {
    m_fDrainRate = 0.0;
    m_fHpMultiplierNormal = 1.0;
    m_fHpMultiplierComboEnd = 1.0;

    if(m_hitobjects.size() < 1 || m_selectedDifficulty2 == NULL) return;

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
        TestPlayer testPlayer(200.0);

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

f32 Beatmap::getApproachTime() const {
    return cv_mod_mafham.getBool()
               ? getLength() * 2
               : GameRules::mapDifficultyRange(getAR(), GameRules::getMinApproachTime(),
                                               GameRules::getMidApproachTime(), GameRules::getMaxApproachTime());
}

f32 Beatmap::getRawApproachTime() const {
    return cv_mod_mafham.getBool()
               ? getLength() * 2
               : GameRules::mapDifficultyRange(getRawAR(), GameRules::getMinApproachTime(),
                                               GameRules::getMidApproachTime(), GameRules::getMaxApproachTime());
}

i32 Beatmap::getModsLegacy() const { return osu->getScore()->getModsLegacy(); }
