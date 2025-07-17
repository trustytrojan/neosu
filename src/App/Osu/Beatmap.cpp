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
#include "SimulatedBeatmap.h"
#include "Skin.h"
#include "SkinImage.h"
#include "Slider.h"
#include "SongBrowser/LeaderboardPPCalcThread.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "Spinner.h"
#include "UIModSelectorModButton.h"

namespace proto = BANCHO::Proto;

Beatmap::Beatmap() {
    // vars
    this->bIsPlaying = false;
    this->bIsPaused = false;
    this->bIsWaiting = false;
    this->bIsRestartScheduled = false;
    this->bIsRestartScheduledQuick = false;

    this->bIsInSkippableSection = false;
    this->bShouldFlashWarningArrows = false;
    this->fShouldFlashSectionPass = 0.0f;
    this->fShouldFlashSectionFail = 0.0f;
    this->bContinueScheduled = false;
    this->iContinueMusicPos = 0;
    this->fWaitTime = 0.0f;

    this->selectedDifficulty2 = NULL;

    this->music = NULL;

    this->fMusicFrequencyBackup = 0.f;
    this->iCurMusicPos = 0;
    this->iCurMusicPosWithOffsets = 0;
    this->bWasSeekFrame = false;
    this->fInterpolatedMusicPos = 0.0;
    this->fLastAudioTimeAccurateSet = 0.0;
    this->fLastRealTimeForInterpolationDelta = 0.0;
    this->iResourceLoadUpdateDelayHack = 0;
    this->bForceStreamPlayback = true;  // if this is set to true here, then the music will always be loaded as a stream
                                        // (meaning slow disk access could cause audio stalling/stuttering)
    this->fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
    this->bIsFirstMissSound = true;

    this->bFailed = false;
    this->fFailAnim = 1.0f;
    this->fHealth = 1.0;
    this->fHealth2 = 1.0f;

    this->fDrainRate = 0.0;

    this->fBreakBackgroundFade = 0.0f;
    this->bInBreak = false;
    this->currentHitObject = NULL;
    this->iNextHitObjectTime = 0;
    this->iPreviousHitObjectTime = 0;
    this->iPreviousSectionPassFailTime = -1;

    this->bClick1Held = false;
    this->bClick2Held = false;
    this->bClickedContinue = false;
    this->bPrevKeyWasKey1 = false;
    this->iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;

    this->iNPS = 0;
    this->iND = 0;
    this->iCurrentHitObjectIndex = 0;
    this->iCurrentNumCircles = 0;
    this->iCurrentNumSliders = 0;
    this->iCurrentNumSpinners = 0;

    this->iPreviousFollowPointObjectIndex = -1;

    this->bIsSpinnerActive = false;

    this->fPlayfieldRotation = 0.0f;
    this->fScaleFactor = 1.0f;

    this->fXMultiplier = 1.0f;
    this->fNumberScale = 1.0f;
    this->fHitcircleOverlapScale = 1.0f;
    this->fRawHitcircleDiameter = 27.35f * 2.0f;
    this->fSliderFollowCircleDiameter = 0.0f;

    this->iAutoCursorDanceIndex = 0;

    this->fAimStars = 0.0f;
    this->fAimSliderFactor = 0.0f;
    this->fSpeedStars = 0.0f;
    this->fSpeedNotes = 0.0f;

    this->bWasHREnabled = false;
    this->fPrevHitCircleDiameter = 0.0f;
    this->bWasHorizontalMirrorEnabled = false;
    this->bWasVerticalMirrorEnabled = false;
    this->bWasEZEnabled = false;
    this->bWasMafhamEnabled = false;
    this->fPrevPlayfieldRotationFromConVar = 0.0f;
    this->bIsPreLoading = true;
    this->iPreLoadingIndex = 0;

    this->mafhamActiveRenderTarget = NULL;
    this->mafhamFinishedRenderTarget = NULL;
    this->bMafhamRenderScheduled = true;
    this->iMafhamHitObjectRenderIndex = 0;
    this->iMafhamPrevHitObjectIndex = 0;
    this->iMafhamActiveRenderHitObjectIndex = 0;
    this->iMafhamFinishedRenderHitObjectIndex = 0;
    this->bInMafhamRenderChunk = false;
}

Beatmap::~Beatmap() {
    anim->deleteExistingAnimation(&this->fBreakBackgroundFade);
    anim->deleteExistingAnimation(&this->fHealth2);
    anim->deleteExistingAnimation(&this->fFailAnim);

    this->unloadObjects();
}

void Beatmap::drawDebug() {
    if(cv::debug_draw_timingpoints.getBool()) {
        McFont *debugFont = resourceManager->getFont("FONT_DEFAULT");
        g->setColor(0xffffffff);
        g->pushTransform();
        g->translate(5, debugFont->getHeight() + 5 - this->getMousePos().y);
        {
            for(const DatabaseBeatmap::TIMINGPOINT &t : this->selectedDifficulty2->getTimingpoints()) {
                g->drawString(debugFont, UString::format("%li,%f,%i,%i,%i", t.offset, t.msPerBeat, t.sampleType,
                                                         t.sampleSet, t.volume));
                g->translate(0, debugFont->getHeight());
            }
        }
        g->popTransform();
    }
}

void Beatmap::drawBackground() {
    if(!this->canDraw()) return;

    // draw beatmap background image
    {
        Image *backgroundImage = osu->getBackgroundImageHandler()->getLoadBackgroundImage(this->selectedDifficulty2);
        if(cv::draw_beatmap_background_image.getBool() && backgroundImage != NULL &&
           (cv::background_dim.getFloat() < 1.0f || this->fBreakBackgroundFade > 0.0f)) {
            const f32 scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());
            const Vector2 centerTrans = (osu->getScreenSize() / 2);

            const f32 backgroundFadeDimMultiplier =
                std::clamp<f32>(1.0f - (cv::background_dim.getFloat() - 0.3f), 0.0f, 1.0f);
            const short dim = std::clamp<f32>((1.0f - cv::background_dim.getFloat()) +
                                             this->fBreakBackgroundFade * backgroundFadeDimMultiplier,
                                         0.0f, 1.0f) *
                              255.0f;

            g->setColor(argb(255, dim, dim, dim));
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
    if(cv::background_brightness.getFloat() > 0.0f) {
        const f32 brightness = cv::background_brightness.getFloat();
        const short red = std::clamp<f32>(brightness * cv::background_color_r.getFloat(), 0.0f, 255.0f);
        const short green = std::clamp<f32>(brightness * cv::background_color_g.getFloat(), 0.0f, 255.0f);
        const short blue = std::clamp<f32>(brightness * cv::background_color_b.getFloat(), 0.0f, 255.0f);
        const short alpha = std::clamp<f32>(1.0f - this->fBreakBackgroundFade, 0.0f, 1.0f) * 255.0f;
        g->setColor(argb(alpha, red, green, blue));
        g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());
    }

    // draw scorebar-bg
    if(cv::draw_hud.getBool() && cv::draw_scorebarbg.getBool() &&
       (!cv::mod_fposu.getBool() || (!cv::fposu_draw_scorebarbg_on_top.getBool())))  // NOTE: special case for FPoSu
        osu->getHUD()->drawScorebarBg(
            cv::hud_scorebar_hide_during_breaks.getBool() ? (1.0f - this->fBreakBackgroundFade) : 1.0f,
            osu->getHUD()->getScoreBarBreakAnim());

    if(cv::debug.getBool()) {
        int y = 50;

        if(this->bIsPaused) {
            g->setColor(0xffffffff);
            g->fillRect(50, y, 15, 50);
            g->fillRect(50 + 50 - 15, y, 15, 50);
        }

        y += 100;

        if(this->bIsWaiting) {
            g->setColor(0xff00ff00);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(this->bIsPlaying) {
            g->setColor(0xff0000ff);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(this->bForceStreamPlayback) {
            g->setColor(0xffff0000);
            g->fillRect(50, y, 50, 50);
        }

        y += 100;

        if(this->hitobjectsSortedByEndTime.size() > 0) {
            HitObject *lastHitObject = this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1];
            if(lastHitObject->isFinished() && this->iCurMusicPos > lastHitObject->click_time + lastHitObject->duration +
                                                                       (long)cv::end_skip_time.getInt()) {
                g->setColor(0xff00ffff);
                g->fillRect(50, y, 50, 50);
            }

            y += 100;
        }
    }
}

void Beatmap::onKeyDown(KeyboardEvent &e) {
    if(e == KEY_O && keyboard->isControlDown()) {
        osu->toggleOptionsMenu();
        e.consume();
    }
}

void Beatmap::skipEmptySection() {
    if(!this->bIsInSkippableSection) return;
    this->bIsInSkippableSection = false;
    osu->chat->updateVisibility();

    const f32 offset = 2500.0f;
    f32 offsetMultiplier = this->getSpeedMultiplier();
    {
        // only compensate if not within "normal" osu mod range (would make the game feel too different regarding time
        // from skip until first hitobject)
        if(offsetMultiplier >= 0.74f && offsetMultiplier <= 1.51f) offsetMultiplier = 1.0f;

        // don't compensate speed increases at all actually
        if(offsetMultiplier > 1.0f) offsetMultiplier = 1.0f;

        // and cap slowdowns at sane value (~ spinner fadein start)
        if(offsetMultiplier <= 0.2f) offsetMultiplier = 0.2f;
    }

    const long nextHitObjectDelta = this->iNextHitObjectTime - (long)this->iCurMusicPosWithOffsets;

    if(!cv::end_skip.getBool() && nextHitObjectDelta < 0) {
        this->music->setPositionMS(std::max(this->music->getLengthMS(), (u32)1) - 1);
        this->bWasSeekFrame = true;
    } else {
        this->music->setPositionMS(std::max(this->iNextHitObjectTime - (long)(offset * offsetMultiplier), (long)0));
        this->bWasSeekFrame = true;
    }

    soundEngine->play(osu->getSkin()->getMenuHit());

    if(!bancho->spectators.empty()) {
        // TODO @kiwec: how does skip work? does client jump to latest time or just skip beginning?
        //              is signaling skip even necessary?
        this->broadcast_spectator_frames();

        Packet packet;
        packet.id = OUT_SPECTATE_FRAMES;
        proto::write<i32>(&packet, 0);
        proto::write<u16>(&packet, 0);
        proto::write<u8>(&packet, LiveReplayBundle::Action::SKIP);
        proto::write<ScoreFrame>(&packet, ScoreFrame::get());
        proto::write<u16>(&packet, this->spectator_sequence++);
        BANCHO::Net::send_packet(packet);
    }
}

void Beatmap::keyPressed1(bool mouse) {
    if(this->is_watching || bancho->spectating) return;

    if(this->bContinueScheduled) this->bClickedContinue = !osu->getModSelector()->isMouseInside();

    if(cv::mod_fullalternate.getBool() && this->bPrevKeyWasKey1) {
        if(this->iCurrentHitObjectIndex > this->iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex) {
            soundEngine->play(this->getSkin()->getCombobreak());
            return;
        }
    }

    // key overlay & counter
    osu->getHUD()->animateInputoverlay(mouse ? 3 : 1, true);

    if(this->bFailed) return;

    if(!this->bInBreak && !this->bIsInSkippableSection && this->bIsPlaying) osu->getScore()->addKeyCount(mouse ? 3 : 1);

    this->bPrevKeyWasKey1 = true;
    this->bClick1Held = true;

    if((!osu->getModAuto() && !osu->getModRelax()) || !cv::auto_and_relax_block_user_input.getBool())
        this->clicks.push_back(Click{
            .click_time = this->iCurMusicPosWithOffsets,
            .pos = this->getCursorPos(),
        });

    if(mouse) {
        this->current_keys = this->current_keys | LegacyReplay::M1;
    } else {
        this->current_keys = this->current_keys | LegacyReplay::M1 | LegacyReplay::K1;
    }
}

void Beatmap::keyPressed2(bool mouse) {
    if(this->is_watching || bancho->spectating) return;

    if(this->bContinueScheduled) this->bClickedContinue = !osu->getModSelector()->isMouseInside();

    if(cv::mod_fullalternate.getBool() && !this->bPrevKeyWasKey1) {
        if(this->iCurrentHitObjectIndex > this->iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex) {
            soundEngine->play(this->getSkin()->getCombobreak());
            return;
        }
    }

    // key overlay & counter
    osu->getHUD()->animateInputoverlay(mouse ? 4 : 2, true);

    if(this->bFailed) return;

    if(!this->bInBreak && !this->bIsInSkippableSection && this->bIsPlaying) osu->getScore()->addKeyCount(mouse ? 4 : 2);

    this->bPrevKeyWasKey1 = false;
    this->bClick2Held = true;

    if((!osu->getModAuto() && !osu->getModRelax()) || !cv::auto_and_relax_block_user_input.getBool())
        this->clicks.push_back(Click{
            .click_time = this->iCurMusicPosWithOffsets,
            .pos = this->getCursorPos(),
        });

    if(mouse) {
        this->current_keys = this->current_keys | LegacyReplay::M2;
    } else {
        this->current_keys = this->current_keys | LegacyReplay::M2 | LegacyReplay::K2;
    }
}

void Beatmap::keyReleased1(bool  /*mouse*/) {
    if(this->is_watching || bancho->spectating) return;

    // key overlay
    osu->getHUD()->animateInputoverlay(1, false);
    osu->getHUD()->animateInputoverlay(3, false);

    this->bClick1Held = false;

    this->current_keys = this->current_keys & ~(LegacyReplay::M1 | LegacyReplay::K1);
}

void Beatmap::keyReleased2(bool  /*mouse*/) {
    if(this->is_watching || bancho->spectating) return;

    // key overlay
    osu->getHUD()->animateInputoverlay(2, false);
    osu->getHUD()->animateInputoverlay(4, false);

    this->bClick2Held = false;

    this->current_keys = this->current_keys & ~(LegacyReplay::M2 | LegacyReplay::K2);
}

void Beatmap::select() {
    // if possible, continue playing where we left off
    if(this->music != NULL && (this->music->isPlaying())) this->iContinueMusicPos = this->music->getPositionMS();

    this->selectDifficulty2(this->selectedDifficulty2);

    this->loadMusic();
    this->handlePreviewPlay();
}

void Beatmap::selectDifficulty2(DatabaseBeatmap *difficulty2) {
    lct_set_map(difficulty2);

    if(difficulty2 != NULL) {
        this->selectedDifficulty2 = difficulty2;

        this->nb_hitobjects = difficulty2->getNumObjects();

        // need to recheck/reload the music here since every difficulty might be using a different sound file
        this->loadMusic();
        this->handlePreviewPlay();
    }

    if(cv::beatmap_preview_mods_live.getBool()) this->onModUpdate();
}

void Beatmap::deselect() {
    this->iContinueMusicPos = 0;
    this->selectedDifficulty2 = NULL;
    this->unloadObjects();
}

bool Beatmap::play() {
    this->is_watching = false;

    if(this->start()) {
        this->last_spectator_broadcast = engine->getTime();
        RichPresence::onPlayStart();
        if(!bancho->spectators.empty()) {
            Packet packet;
            packet.id = OUT_SPECTATE_FRAMES;
            proto::write<i32>(&packet, 0);
            proto::write<u16>(&packet, 0);  // 0 frames, we're just signaling map start
            proto::write<u8>(&packet, LiveReplayBundle::Action::NEW_SONG);
            proto::write<ScoreFrame>(&packet, ScoreFrame::get());
            proto::write<u16>(&packet, this->spectator_sequence++);
            BANCHO::Net::send_packet(packet);
        }

        return true;
    }

    return false;
}

bool Beatmap::watch(FinishedScore score, f64 start_percent) {
    SAFE_DELETE(sim);
    if(score.replay.size() < 3) {
        // Replay is invalid
        return false;
    }

    this->bIsPlaying = false;
    this->bIsPaused = false;
    this->bContinueScheduled = false;
    this->unloadObjects();

    osu->previous_mods = Replay::Mods::from_cvars();

    osu->watched_user_name = score.playerName.c_str();
    osu->watched_user_id = score.player_id;
    this->is_watching = true;

    osu->useMods(&score);

    if(!this->start()) {
        // Map failed to load
        return false;
    }

    this->spectated_replay = score.replay;

    osu->songBrowser2->bHasSelectedAndIsPlaying = true;
    osu->songBrowser2->setVisible(false);

    // Don't seek to 0%, since it feels really bad to start immediately
    if(start_percent > 0.f) {
        this->seekPercent(start_percent);
    }

    this->sim = new SimulatedBeatmap(this->selectedDifficulty2, score.mods);
    this->sim->spectated_replay = score.replay;

    return true;
}

bool Beatmap::spectate() {
    this->bIsPlaying = false;
    this->bIsPaused = false;
    this->bContinueScheduled = false;
    this->unloadObjects();

    auto user_info = BANCHO::User::get_user_info(bancho->spectated_player_id, true);
    osu->watched_user_id = bancho->spectated_player_id;
    osu->watched_user_name = user_info->name;

    osu->previous_mods = Replay::Mods::from_cvars();

    FinishedScore score;
    score.client = "peppy-unknown";
    score.server = bancho->endpoint.toUtf8();
    score.mods = Replay::Mods::from_legacy(user_info->mods);
    osu->useMods(&score);

    if(!this->start()) {
        // Map failed to load
        return false;
    }

    this->spectated_replay.clear();
    this->score_frames.clear();

    osu->songBrowser2->bHasSelectedAndIsPlaying = true;
    osu->songBrowser2->setVisible(false);

    SAFE_DELETE(sim);
    score.mods.flags |= Replay::ModFlags::NoFail;
    this->sim = new SimulatedBeatmap(this->selectedDifficulty2, score.mods);
    this->sim->spectated_replay.clear();

    return true;
}

bool Beatmap::start() {
    if(this->selectedDifficulty2 == NULL) return false;

    osu->should_pause_background_threads = true;

    soundEngine->play(osu->skin->getMenuHit());

    osu->updateMods();

    // mp hack
    {
        osu->mainMenu->setVisible(false);
        osu->modSelector->setVisible(false);
        osu->optionsMenu->setVisible(false);
        osu->pauseMenu->setVisible(false);
    }

    // HACKHACK: stuck key quickfix
    {
        osu->bKeyboardKey1Down = false;
        osu->bKeyboardKey12Down = false;
        osu->bKeyboardKey2Down = false;
        osu->bKeyboardKey22Down = false;
        osu->bMouseKey1Down = false;
        osu->bMouseKey2Down = false;

        this->keyReleased1(false);
        this->keyReleased1(true);
        this->keyReleased2(false);
        this->keyReleased2(true);
    }

    static const int OSU_COORD_WIDTH = 512;
    static const int OSU_COORD_HEIGHT = 384;
    osu->flashlight_position = Vector2{OSU_COORD_WIDTH / 2, OSU_COORD_HEIGHT / 2};

    // reset everything, including deleting any previously loaded hitobjects from another diff which we might just have
    // played
    this->unloadObjects();
    this->resetScore();

    // some hitobjects already need this information to be up-to-date before their constructor is called
    this->updatePlayfieldMetrics();
    this->updateHitobjectMetrics();
    this->bIsPreLoading = false;

    // actually load the difficulty (and the hitobjects)
    {
        DatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = DatabaseBeatmap::loadGameplay(this->selectedDifficulty2, this);
        if(result.errorCode != 0) {
            switch(result.errorCode) {
                case 1: {
                    UString errorMessage = "Error: Couldn't load beatmap metadata :(";
                    debugLog("Osu Error: Couldn't load beatmap metadata %s\n",
                             this->selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addToast(errorMessage, 0xffff0000);
                } break;

                case 2: {
                    UString errorMessage = "Error: Couldn't load beatmap file :(";
                    debugLog("Osu Error: Couldn't load beatmap file %s\n",
                             this->selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addToast(errorMessage, 0xffff0000);
                } break;

                case 3: {
                    UString errorMessage = "Error: No timingpoints in beatmap :(";
                    debugLog("Osu Error: No timingpoints in beatmap %s\n",
                             this->selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addToast(errorMessage, 0xffff0000);
                } break;

                case 4: {
                    UString errorMessage = "Error: No hitobjects in beatmap :(";
                    debugLog("Osu Error: No hitobjects in beatmap %s\n",
                             this->selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addToast(errorMessage, 0xffff0000);
                } break;

                case 5: {
                    UString errorMessage = "Error: Too many hitobjects in beatmap :(";
                    debugLog("Osu Error: Too many hitobjects in beatmap %s\n",
                             this->selectedDifficulty2->getFilePath().c_str());

                    osu->getNotificationOverlay()->addToast(errorMessage, 0xffff0000);
                } break;
            }

            osu->should_pause_background_threads = false;
            return false;
        }

        // move temp result data into beatmap
        this->hitobjects = std::move(result.hitobjects);
        this->breaks = std::move(result.breaks);
        osu->getSkin()->setBeatmapComboColors(std::move(result.combocolors));  // update combo colors in skin

        // load beatmap skin
        osu->getSkin()->loadBeatmapOverride(this->selectedDifficulty2->getFolder());
    }

    // the drawing order is different from the playing/input order.
    // for drawing, if multiple hitobjects occupy the exact same time (duration) then they get drawn on top of the
    // active hitobject
    this->hitobjectsSortedByEndTime = this->hitobjects;

    // sort hitobjects by endtime
    struct HitObjectSortComparator {
        bool operator()(HitObject const *a, HitObject const *b) const {
            // strict weak ordering!
            if((a->click_time + a->duration) == (b->click_time + b->duration))
                return a < b;
            else
                return (a->click_time + a->duration) < (b->click_time + b->duration);
        }
    };
    std::sort(this->hitobjectsSortedByEndTime.begin(), this->hitobjectsSortedByEndTime.end(),
              HitObjectSortComparator());

    // after the hitobjects have been loaded we can calculate the stacks
    this->calculateStacks();
    this->computeDrainRate();

    // start preloading (delays the play start until it's set to false, see isLoading())
    this->bIsPreLoading = true;
    this->iPreLoadingIndex = 0;

    // live pp/stars
    this->resetLiveStarsTasks();

    // load music
    if(cv::restart_sound_engine_before_playing.getBool()) {
        // HACKHACK: Reload sound engine before starting the song, as it starts lagging after a while
        //           (i haven't figured out the root cause yet)
        soundEngine->pause(this->music);
        soundEngine->restart();

        // Restarting sound engine already reloads the music
    } else {
        this->unloadMusic();  // need to reload in case of speed/pitch changes (just to be sure)
        this->loadMusic(false);
    }

    this->music->setLoop(false);
    this->spectate_pause = false;
    this->bIsPaused = false;
    this->bContinueScheduled = false;

    this->bInBreak = cv::background_fade_after_load.getBool();
    anim->deleteExistingAnimation(&this->fBreakBackgroundFade);
    this->fBreakBackgroundFade = cv::background_fade_after_load.getBool() ? 1.0f : 0.0f;
    this->iPreviousSectionPassFailTime = -1;
    this->fShouldFlashSectionPass = 0.0f;
    this->fShouldFlashSectionFail = 0.0f;

    this->music->setPositionMS(0);
    this->iCurMusicPos = 0;

    // we are waiting for an asynchronous start of the beatmap in the next update()
    this->bIsPlaying = true;
    this->bIsWaiting = true;
    this->fWaitTime = engine->getTimeReal();

    cv::snd_change_check_interval.setValue(0.0f);

    if(this->getSelectedDifficulty2()->getLocalOffset() != 0)
        osu->notificationOverlay->addNotification(
            UString::format("Using local beatmap offset (%ld ms)", this->getSelectedDifficulty2()->getLocalOffset()),
            0xffffffff, false, 0.75f);

    osu->fQuickSaveTime = 0.0f;  // reset

    osu->updateConfineCursor();
    osu->updateWindowsKeyDisable();

    soundEngine->play(osu->getSkin()->expand);

    // NOTE: loading failures are handled dynamically in update(), so temporarily assume everything has worked in here
    return true;
}

void Beatmap::restart(bool quick) {
    soundEngine->stop(this->getSkin()->getFailsound());

    if(!this->bIsWaiting) {
        this->bIsRestartScheduled = true;
        this->bIsRestartScheduledQuick = quick;
    } else if(this->bIsPaused && !bancho->spectating) {
        this->pause(false);
    }
}

void Beatmap::actualRestart() {
    // reset everything
    this->resetScore();
    this->resetHitObjects(-1000);

    // we are waiting for an asynchronous start of the beatmap in the next update()
    this->bIsWaiting = true;
    this->fWaitTime = engine->getTimeReal();

    // if the first hitobject starts immediately, add artificial wait time before starting the music
    if(this->hitobjects.size() > 0) {
        if(this->hitobjects[0]->click_time < (long)cv::early_note_time.getInt()) {
            this->bIsWaiting = true;
            this->fWaitTime = engine->getTimeReal() + cv::early_note_time.getFloat() / 1000.0f;
        }
    }

    // pause temporarily if playing
    if(this->music->isPlaying()) soundEngine->pause(this->music);

    // reset/restore frequency (from potential fail before)
    this->music->setFrequency(0);

    this->music->setLoop(false);
    this->bIsPaused = false;
    this->bContinueScheduled = false;

    this->bInBreak = false;
    anim->deleteExistingAnimation(&this->fBreakBackgroundFade);
    this->fBreakBackgroundFade = 0.0f;
    this->iPreviousSectionPassFailTime = -1;
    this->fShouldFlashSectionPass = 0.0f;
    this->fShouldFlashSectionFail = 0.0f;

    this->onModUpdate();  // sanity

    // reset position
    this->music->setPositionMS(0);
    this->bWasSeekFrame = true;
    this->iCurMusicPos = 0;

    this->bIsPlaying = true;
    this->is_watching = false;
}

void Beatmap::pause(bool quitIfWaiting) {
    if(this->selectedDifficulty2 == NULL) return;
    if(bancho->spectating) {
        stop_spectating();
        return;
    }

    const bool isFirstPause = !this->bContinueScheduled;

    // NOTE: this assumes that no beatmap ever goes far beyond the end of the music
    // NOTE: if pure virtual audio time is ever supported (playing without SoundEngine) then this needs to be adapted
    // fix pausing after music ends breaking beatmap state (by just not allowing it to be paused)
    if(this->fAfterMusicIsFinishedVirtualAudioTimeStart >= 0.0f) {
        const f32 delta = engine->getTimeReal() - this->fAfterMusicIsFinishedVirtualAudioTimeStart;
        if(delta < 5.0f)  // WARNING: sanity limit, always allow escaping after 5 seconds of overflow time
            return;
    }

    if(this->bIsPlaying) {
        if(this->bIsWaiting && quitIfWaiting) {
            // if we are still m_bIsWaiting, pausing the game via the escape key is the
            // same as stopping playing
            this->stop();
        } else {
            // first time pause pauses the music
            // case 1: the beatmap is already "finished", jump to the ranking screen if some small amount of time past
            //         the last objects endTime
            // case 2: in the middle somewhere, pause as usual
            HitObject *lastHitObject = this->hitobjectsSortedByEndTime.size() > 0
                                           ? this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]
                                           : NULL;
            if(lastHitObject != NULL && lastHitObject->isFinished() &&
               (this->iCurMusicPos >
                lastHitObject->click_time + lastHitObject->duration + (long)cv::end_skip_time.getInt()) &&
               cv::end_skip.getBool()) {
                this->stop(false);
            } else {
                soundEngine->pause(this->music);
                this->bIsPlaying = false;
                this->bIsPaused = true;
            }
        }
    } else if(this->bIsPaused && !this->bContinueScheduled) {
        // if this is the first time unpausing
        if(osu->getModAuto() || osu->getModAutopilot() || this->bIsInSkippableSection || this->is_watching) {
            if(!this->bIsWaiting) {
                // only force play() if we were not early waiting
                soundEngine->play(this->music);
            }

            this->bIsPlaying = true;
            this->bIsPaused = false;
        } else {
            // otherwise, schedule a continue (wait for user to click, handled in update())
            // first time unpause schedules a continue
            this->bIsPaused = false;
            this->bContinueScheduled = true;
        }
    } else {
        // if this is not the first time pausing/unpausing, then just toggle the pause state (the visibility of the
        // pause menu is handled in the Osu class, a bit shit)
        this->bIsPaused = !this->bIsPaused;
    }

    if(this->bIsPaused && isFirstPause) {
        if(!cv::submit_after_pause.getBool()) {
            debugLog("Disabling score submission due to pausing\n");
            this->vanilla = false;
        }

        this->vContinueCursorPoint = this->getMousePos();
        if(cv::mod_fps.getBool()) {
            this->vContinueCursorPoint = GameRules::getPlayfieldCenter();
        }
    }

    // if we have failed, and the user early exits to the pause menu, stop the failing animation
    if(this->bFailed) anim->deleteExistingAnimation(&this->fFailAnim);
}

void Beatmap::pausePreviewMusic(bool toggle) {
    if(this->music != NULL) {
        if(this->music->isPlaying())
            soundEngine->pause(this->music);
        else if(toggle)
            soundEngine->play(this->music);
    }
}

bool Beatmap::isPreviewMusicPlaying() {
    if(this->music != NULL) return this->music->isPlaying();

    return false;
}

void Beatmap::stop(bool quit) {
    if(this->selectedDifficulty2 == NULL) return;

    if(this->getSkin()->getFailsound()->isPlaying()) soundEngine->stop(this->getSkin()->getFailsound());

    this->bIsPlaying = false;
    this->bIsPaused = false;
    this->bContinueScheduled = false;

    auto score = this->saveAndSubmitScore(quit);

    // Auto mod was "temporary" since it was set from Ctrl+Clicking a map, not from the mod selector
    if(osu->bModAutoTemp) {
        if(osu->modSelector->modButtonAuto->isOn()) {
            osu->modSelector->modButtonAuto->click();
        }
        osu->bModAutoTemp = false;
    }

    if(this->is_watching || bancho->spectating) {
        Replay::Mods::use(osu->previous_mods);
    }

    this->is_watching = false;
    this->spectated_replay.clear();
    this->score_frames.clear();
    SAFE_DELETE(sim);

    this->unloadObjects();

    if(bancho->is_playing_a_multi_map()) {
        if(quit) {
            osu->onPlayEnd(score, true);
            osu->room->ragequit();
        } else {
            osu->room->onClientScoreChange(true);
            Packet packet;
            packet.id = FINISH_MATCH;
            BANCHO::Net::send_packet(packet);
        }
    } else {
        osu->onPlayEnd(score, quit);
    }

    osu->should_pause_background_threads = false;
}

void Beatmap::fail(bool force_death) {
    if(this->bFailed) return;
    if((this->is_watching || bancho->spectating) && !force_death) return;

    // Change behavior of relax mod when online
    if(bancho->is_online() && osu->getModRelax()) return;

    if(!bancho->is_playing_a_multi_map() && cv::drain_kill.getBool()) {
        soundEngine->play(this->getSkin()->getFailsound());

        // trigger music slowdown and delayed menu, see update()
        this->bFailed = true;
        this->fFailAnim = 1.0f;
        anim->moveLinear(&this->fFailAnim, 0.0f, cv::fail_time.getFloat(), true);
    } else if(!osu->getScore()->isDead()) {
        debugLog("Disabling score submission due to death\n");
        this->vanilla = false;

        anim->deleteExistingAnimation(&this->fHealth2);
        this->fHealth = 0.0;
        this->fHealth2 = 0.0f;

        if(cv::drain_kill_notification_duration.getFloat() > 0.0f) {
            if(!osu->getScore()->hasDied())
                osu->getNotificationOverlay()->addNotification("You have failed, but you can keep playing!", 0xffffffff,
                                                               false, cv::drain_kill_notification_duration.getFloat());
        }
    }

    if(!osu->getScore()->isDead()) osu->getScore()->setDead(true);
}

void Beatmap::cancelFailing() {
    if(!this->bFailed || this->fFailAnim <= 0.0f) return;

    this->bFailed = false;

    anim->deleteExistingAnimation(&this->fFailAnim);
    this->fFailAnim = 1.0f;

    if(this->music != NULL) this->music->setFrequency(0.0f);

    if(this->getSkin()->getFailsound()->isPlaying()) soundEngine->stop(this->getSkin()->getFailsound());
}

f32 Beatmap::getIdealVolume() {
    if(this->music == NULL) return 1.f;

    f32 volume = cv::volume_music.getFloat();
    f32 modifier = 1.f;

    if(cv::normalize_loudness.getBool()) {
        if(this->selectedDifficulty2->loudness == 0.f) {
            modifier = std::pow(10, (cv::loudness_target.getFloat() - cv::loudness_fallback.getFloat()) / 20);
        } else {
            modifier = std::pow(10, (cv::loudness_target.getFloat() - this->selectedDifficulty2->loudness) / 20);
        }
    }

    return volume * modifier;
}

void Beatmap::setSpeed(f32 speed) {
    if(this->music != NULL) this->music->setSpeed(speed);
}

void Beatmap::seekPercent(f64 percent) {
    if(this->selectedDifficulty2 == NULL || this->music == NULL || this->bFailed) return;

    this->bWasSeekFrame = true;
    this->fWaitTime = 0.0f;

    u32 ms = this->music->setPosition(percent);
    this->music->setVolume(this->getIdealVolume());
    this->music->setSpeed(this->getSpeedMultiplier());

    this->resetHitObjects(ms);
    this->resetScore();

    this->iPreviousSectionPassFailTime = -1;

    if(this->bIsWaiting) {
        this->bIsWaiting = false;
        this->bIsPlaying = true;
        this->bIsRestartScheduledQuick = false;

        soundEngine->play(this->music);

        // if there are calculations in there that need the hitobjects to be loaded, also applies speed/pitch
        this->onModUpdate(false, false);
    }

    if(bancho->spectating) {
        debugLog("After seeking, we are now %dms behind the player.\n", this->last_frame_ms - (i32)ms);
    }

    if(this->is_watching) {
        // When seeking backwards, restart simulation from beginning
        if(ms < this->iCurMusicPos) {
            SAFE_DELETE(sim);
            this->sim = new SimulatedBeatmap(this->selectedDifficulty2, osu->getScore()->mods);
            this->sim->spectated_replay = this->spectated_replay;
            osu->getScore()->reset();
        }
    }

    if(!this->is_watching && !bancho->spectating) {  // score submission already disabled when watching replay
        debugLog("Disabling score submission due to seeking\n");
        this->vanilla = false;
    }
}

void Beatmap::seekPercentPlayable(f64 percent) {
    if(this->selectedDifficulty2 == NULL || (!this->bIsPlaying && !this->bIsPaused) || this->music == NULL ||
       this->bFailed)
        return;

    this->bWasSeekFrame = true;
    this->fWaitTime = 0.0f;

    f64 actualPlayPercent = percent;
    if(this->hitobjects.size() > 0)
        actualPlayPercent = (((f64)this->hitobjects[this->hitobjects.size() - 1]->click_time +
                              (f64)this->hitobjects[this->hitobjects.size() - 1]->duration) *
                             percent) /
                            (f64)this->music->getLengthMS();

    this->seekPercent(actualPlayPercent);
}

u32 Beatmap::getTime() const {
    if(this->music != NULL && this->music->isAsyncReady())
        return this->music->getPositionMS();
    else
        return 0;
}

u32 Beatmap::getStartTimePlayable() const {
    if(this->hitobjects.size() > 0)
        return (u32)this->hitobjects[0]->click_time;
    else
        return 0;
}

u32 Beatmap::getLength() const {
    if(this->music != NULL && this->music->isAsyncReady())
        return this->music->getLengthMS();
    else if(this->selectedDifficulty2 != NULL)
        return this->selectedDifficulty2->getLengthMS();
    else
        return 0;
}

u32 Beatmap::getLengthPlayable() const {
    if(this->hitobjects.size() > 0)
        return (u32)((this->hitobjects[this->hitobjects.size() - 1]->click_time +
                      this->hitobjects[this->hitobjects.size() - 1]->duration) -
                     this->hitobjects[0]->click_time);
    else
        return this->getLength();
}

f32 Beatmap::getPercentFinished() const {
    if(this->music != NULL)
        return (f32)this->iCurMusicPos / (f32)this->music->getLengthMS();
    else
        return 0.0f;
}

f32 Beatmap::getPercentFinishedPlayable() const {
    if(this->bIsWaiting)
        return 1.0f - (this->fWaitTime - engine->getTimeReal()) / (cv::early_note_time.getFloat() / 1000.0f);

    if(this->hitobjects.size() > 0)
        return (f32)this->iCurMusicPos / ((f32)this->hitobjects[this->hitobjects.size() - 1]->click_time +
                                          (f32)this->hitobjects[this->hitobjects.size() - 1]->duration);
    else
        return (f32)this->iCurMusicPos / (f32)this->music->getLengthMS();
}

int Beatmap::getMostCommonBPM() const {
    if(this->selectedDifficulty2 != NULL) {
        if(this->music != NULL)
            return (int)(this->selectedDifficulty2->getMostCommonBPM() * this->music->getSpeed());
        else
            return (int)(this->selectedDifficulty2->getMostCommonBPM() * this->getSpeedMultiplier());
    } else
        return 0;
}

f32 Beatmap::getSpeedMultiplier() const {
    if(cv::speed_override.getFloat() >= 0.0f) {
        return std::max(cv::speed_override.getFloat(), 0.05f);
    } else {
        return 1.f;
    }
}

Skin *Beatmap::getSkin() const { return osu->getSkin(); }

u32 Beatmap::getScoreV1DifficultyMultiplier() const {
    // NOTE: We intentionally get CS/HP/OD from beatmap data, not "real" CS/HP/OD
    //       Since this multiplier is only used for ScoreV1
    u32 breakTimeMS = this->getBreakDurationTotal();
    f32 drainLength = std::max(this->getLengthPlayable() - std::min(breakTimeMS, this->getLengthPlayable()), (u32)1000) / 1000;
    return std::round((this->selectedDifficulty2->getCS() + this->selectedDifficulty2->getHP() +
                       this->selectedDifficulty2->getOD() +
                       std::clamp<f32>((f32)this->selectedDifficulty2->getNumObjects() / drainLength * 8.0f, 0.0f, 16.0f)) /
                      38.0f * 5.0f);
}

f32 Beatmap::getRawAR() const {
    if(this->selectedDifficulty2 == NULL) return 5.0f;

    return std::clamp<f32>(this->selectedDifficulty2->getAR() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

f32 Beatmap::getAR() const {
    if(this->selectedDifficulty2 == NULL) return 5.0f;

    f32 AR = this->getRawAR();

    if(cv::ar_override.getFloat() >= 0.0f) AR = cv::ar_override.getFloat();

    if(cv::ar_overridenegative.getFloat() < 0.0f) AR = cv::ar_overridenegative.getFloat();

    if(cv::ar_override_lock.getBool())
        AR = GameRules::getRawConstantApproachRateForSpeedMultiplier(
            GameRules::getRawApproachTime(AR),
            (this->music != NULL && this->bIsPlaying ? this->getSpeedMultiplier() : this->getSpeedMultiplier()));

    if(cv::mod_artimewarp.getBool() && this->hitobjects.size() > 0) {
        const f32 percent =
            1.0f - ((f64)(this->iCurMusicPos - this->hitobjects[0]->click_time) /
                    (f64)(this->hitobjects[this->hitobjects.size() - 1]->click_time +
                          this->hitobjects[this->hitobjects.size() - 1]->duration - this->hitobjects[0]->click_time)) *
                       (1.0f - cv::mod_artimewarp_multiplier.getFloat());
        AR *= percent;
    }

    if(cv::mod_arwobble.getBool())
        AR += std::sin((this->iCurMusicPos / 1000.0f) * cv::mod_arwobble_interval.getFloat()) *
              cv::mod_arwobble_strength.getFloat();

    return AR;
}

f32 Beatmap::getCS() const {
    if(this->selectedDifficulty2 == NULL) return 5.0f;

    f32 CS = std::clamp<f32>(this->selectedDifficulty2->getCS() * osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);

    if(cv::cs_override.getFloat() >= 0.0f) CS = cv::cs_override.getFloat();

    if(cv::cs_overridenegative.getFloat() < 0.0f) CS = cv::cs_overridenegative.getFloat();

    if(cv::mod_minimize.getBool() && this->hitobjects.size() > 0) {
        if(this->hitobjects.size() > 0) {
            const f32 percent = 1.0f + ((f64)(this->iCurMusicPos - this->hitobjects[0]->click_time) /
                                        (f64)(this->hitobjects[this->hitobjects.size() - 1]->click_time +
                                              this->hitobjects[this->hitobjects.size() - 1]->duration -
                                              this->hitobjects[0]->click_time)) *
                                           cv::mod_minimize_multiplier.getFloat();
            CS *= percent;
        }
    }

    if(cv::cs_cap_sanity.getBool()) CS = std::min(CS, 12.1429f);

    return CS;
}

f32 Beatmap::getHP() const {
    if(this->selectedDifficulty2 == NULL) return 5.0f;

    f32 HP = std::clamp<f32>(this->selectedDifficulty2->getHP() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
    if(cv::hp_override.getFloat() >= 0.0f) HP = cv::hp_override.getFloat();

    return HP;
}

f32 Beatmap::getRawOD() const {
    if(this->selectedDifficulty2 == NULL) return 5.0f;

    return std::clamp<f32>(this->selectedDifficulty2->getOD() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

f32 Beatmap::getOD() const {
    f32 OD = this->getRawOD();

    if(cv::od_override.getFloat() >= 0.0f) OD = cv::od_override.getFloat();

    if(cv::od_override_lock.getBool())
        OD = GameRules::getRawConstantOverallDifficultyForSpeedMultiplier(
            GameRules::getRawHitWindow300(OD),
            (this->music != NULL && this->bIsPlaying ? this->getSpeedMultiplier() : this->getSpeedMultiplier()));

    return OD;
}

bool Beatmap::isKey1Down() const {
    if(this->is_watching || bancho->spectating) {
        return this->current_keys & (LegacyReplay::M1 | LegacyReplay::K1);
    } else {
        return this->bClick1Held;
    }
}

bool Beatmap::isKey2Down() const {
    if(this->is_watching || bancho->spectating) {
        return this->current_keys & (LegacyReplay::M2 | LegacyReplay::K2);
    } else {
        return this->bClick2Held;
    }
}

bool Beatmap::isClickHeld() const { return this->isKey1Down() || this->isKey2Down(); }

std::string Beatmap::getTitle() const {
    if(this->selectedDifficulty2 != NULL)
        return this->selectedDifficulty2->getTitle();
    else
        return "NULL";
}

std::string Beatmap::getArtist() const {
    if(this->selectedDifficulty2 != NULL)
        return this->selectedDifficulty2->getArtist();
    else
        return "NULL";
}

u32 Beatmap::getBreakDurationTotal() const {
    u32 breakDurationTotal = 0;
    for(int i = 0; i < this->breaks.size(); i++) {
        breakDurationTotal += (u32)(this->breaks[i].endTime - this->breaks[i].startTime);
    }

    return breakDurationTotal;
}

DatabaseBeatmap::BREAK Beatmap::getBreakForTimeRange(i64 startMS, i64 positionMS, i64 endMS) const {
    DatabaseBeatmap::BREAK curBreak;

    curBreak.startTime = -1;
    curBreak.endTime = -1;

    for(int i = 0; i < this->breaks.size(); i++) {
        if(this->breaks[i].startTime >= startMS && this->breaks[i].endTime <= endMS) {
            if(positionMS >= curBreak.startTime) curBreak = this->breaks[i];
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

        if(should_write_frame && !hitErrorBarOnly) {
            this->write_frame();
        }
    }

    // handle perfect & sudden death
    if(osu->getModSS()) {
        if(!hitErrorBarOnly && hit != LiveScore::HIT::HIT_300 && hit != LiveScore::HIT::HIT_300G &&
           hit != LiveScore::HIT::HIT_SLIDER10 && hit != LiveScore::HIT::HIT_SLIDER30 &&
           hit != LiveScore::HIT::HIT_SPINNERSPIN && hit != LiveScore::HIT::HIT_SPINNERBONUS) {
            this->restart();
            return LiveScore::HIT::HIT_MISS;
        }
    } else if(osu->getModSD()) {
        if(hit == LiveScore::HIT::HIT_MISS) {
            if(cv::mod_suddendeath_restart.getBool())
                this->restart();
            else
                this->fail();

            return LiveScore::HIT::HIT_MISS;
        }
    }

    // miss sound
    if(hit == LiveScore::HIT::HIT_MISS) this->playMissSound();

    // score
    osu->getScore()->addHitResult(this, hitObject, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo,
                                  ignoreScore);

    // health
    LiveScore::HIT returnedHit = LiveScore::HIT::HIT_MISS;
    if(!ignoreHealth) {
        this->addHealth(osu->getScore()->getHealthIncrease(this, hit), true);

        // geki/katu handling
        if(isEndOfCombo) {
            const int comboEndBitmask = osu->getScore()->getComboEndBitmask();

            if(comboEndBitmask == 0) {
                returnedHit = LiveScore::HIT::HIT_300G;
                this->addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                osu->getScore()->addHitResultComboEnd(returnedHit);
            } else if((comboEndBitmask & 2) == 0) {
                if(hit == LiveScore::HIT::HIT_100) {
                    returnedHit = LiveScore::HIT::HIT_100K;
                    this->addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                    osu->getScore()->addHitResultComboEnd(returnedHit);
                } else if(hit == LiveScore::HIT::HIT_300) {
                    returnedHit = LiveScore::HIT::HIT_300K;
                    this->addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
                    osu->getScore()->addHitResultComboEnd(returnedHit);
                }
            } else if(hit != LiveScore::HIT::HIT_MISS)
                this->addHealth(osu->getScore()->getHealthIncrease(this, LiveScore::HIT::HIT_MU), true);

            osu->getScore()->setComboEndBitmask(0);
        }
    }

    return returnedHit;
}

void Beatmap::addSliderBreak() {
    // handle perfect & sudden death
    if(osu->getModSS()) {
        this->restart();
        return;
    } else if(osu->getModSD()) {
        if(cv::mod_suddendeath_restart.getBool())
            this->restart();
        else
            this->fail();

        return;
    }

    // miss sound
    this->playMissSound();

    // score
    osu->getScore()->addSliderBreak();
}

void Beatmap::addScorePoints(int points, bool isSpinner) { osu->getScore()->addPoints(points, isSpinner); }

void Beatmap::addHealth(f64 percent, bool isFromHitResult) {
    // never drain before first hitobject
    if(this->hitobjects.size() > 0 && this->iCurMusicPosWithOffsets < this->hitobjects[0]->click_time) return;

    // never drain after last hitobject
    if(this->hitobjectsSortedByEndTime.size() > 0 &&
       this->iCurMusicPosWithOffsets >
           (this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->click_time +
            this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->duration))
        return;

    if(this->bFailed) {
        anim->deleteExistingAnimation(&this->fHealth2);

        this->fHealth = 0.0;
        this->fHealth2 = 0.0f;

        return;
    }

    if(isFromHitResult && percent > 0.0) {
        osu->getHUD()->animateKiBulge();

        if(this->fHealth > 0.9) osu->getHUD()->animateKiExplode();
    }

    this->fHealth = std::clamp<f64>(this->fHealth + percent, 0.0, 1.0);

    // handle generic fail state (2)
    const bool isDead = this->fHealth < 0.001;
    if(isDead && !osu->getModNF()) {
        if(osu->getModEZ() && osu->getScore()->getNumEZRetries() > 0)  // retries with ez
        {
            osu->getScore()->setNumEZRetries(osu->getScore()->getNumEZRetries() - 1);

            // special case: set health to 160/200 (osu!stable behavior, seems fine for all drains)
            this->fHealth = 160.0f / 200.f;
            this->fHealth2 = (f32)this->fHealth;

            anim->deleteExistingAnimation(&this->fHealth2);
        } else if(isFromHitResult && percent < 0.0)  // judgement fail
        {
            this->fail();
        }
    }
}

void Beatmap::updateTimingPoints(long curPos) {
    if(curPos < 0) return;  // aspire pls >:(

    /// debugLog("updateTimingPoints( %ld )\n", curPos);

    const DatabaseBeatmap::TIMING_INFO t =
        this->selectedDifficulty2->getTimingInfoForTime(curPos + (long)cv::timingpoints_offset.getInt());
    osu->getSkin()->setSampleSet(
        t.sampleType);  // normal/soft/drum is stored in the sample type! the sample set number is for custom sets
    osu->getSkin()->setSampleVolume(std::clamp<f32>(t.volume / 100.0f, 0.0f, 1.0f));
}

long Beatmap::getPVS() {
    // this is an approximation with generous boundaries, it doesn't need to be exact (just good enough to filter 10000
    // hitobjects down to a few hundred or so) it will be used in both positive and negative directions (previous and
    // future hitobjects) to speed up loops which iterate over all hitobjects
    return this->getApproachTime() + GameRules::getFadeInTime() + (long)GameRules::getHitWindowMiss() + 1500;  // sanity
}

bool Beatmap::canDraw() {
    if(!this->bIsPlaying && !this->bIsPaused && !this->bContinueScheduled && !this->bIsWaiting) return false;
    if(this->selectedDifficulty2 == NULL || this->music == NULL)  // sanity check
        return false;

    return true;
}

bool Beatmap::canUpdate() { return this->bIsPlaying || this->bIsPaused || this->bContinueScheduled; }

void Beatmap::handlePreviewPlay() {
    if(this->music == NULL) return;

    if((!this->music->isPlaying() || this->music->getPosition() > 0.95f) && this->selectedDifficulty2 != NULL) {
        // this is an assumption, but should be good enough for most songs
        // reset playback position when the song has nearly reached the end (when the user switches back to the results
        // screen or the songbrowser after playing)
        if(this->music->getPosition() > 0.95f) this->iContinueMusicPos = 0;

        soundEngine->stop(this->music);

        if(soundEngine->play(this->music)) {
            if(this->music->getFrequency() < this->fMusicFrequencyBackup)  // player has died, reset frequency
                this->music->setFrequency(this->fMusicFrequencyBackup);

            // When neosu is initialized, it starts playing a random song in the main menu.
            // Users can set a convar to make it start at its preview point instead.
            // The next songs will start at the beginning regardless.
            static bool should_start_song_at_preview_point = cv::start_first_main_menu_song_at_preview_point.getBool();
            bool start_at_song_beginning = osu->getMainMenu()->isVisible() && !should_start_song_at_preview_point;
            should_start_song_at_preview_point = false;

            if(start_at_song_beginning) {
                this->music->setPositionMS_fast(0);
                this->bWasSeekFrame = true;
            } else if(this->iContinueMusicPos != 0) {
                this->music->setPositionMS_fast(this->iContinueMusicPos);
                this->bWasSeekFrame = true;
            } else {
                this->music->setPositionMS_fast(this->selectedDifficulty2->getPreviewTime() < 0
                                                    ? (u32)(this->music->getLengthMS() * 0.40f)
                                                    : this->selectedDifficulty2->getPreviewTime());
                this->bWasSeekFrame = true;
            }

            this->music->setVolume(this->getIdealVolume());
            this->music->setSpeed(this->getSpeedMultiplier());
        }
    }

    // always loop during preview
    this->music->setLoop(cv::beatmap_preview_music_loop.getBool());
}

void Beatmap::loadMusic(bool stream) {
    stream = stream || this->bForceStreamPlayback;
    this->iResourceLoadUpdateDelayHack = 0;

    // load the song (again)
    if(this->selectedDifficulty2 != NULL &&
       (this->music == NULL || this->selectedDifficulty2->getFullSoundFilePath() != this->music->getFilePath() ||
        !this->music->isReady())) {
        this->unloadMusic();

        // if it's not a stream then we are loading the entire song into memory for playing
        if(!stream) resourceManager->requestNextLoadAsync();

        this->music = resourceManager->loadSoundAbs(this->selectedDifficulty2->getFullSoundFilePath(),
                                                                 "OSU_BEATMAP_MUSIC", stream, false, false);
        this->music->setVolume(this->getIdealVolume());
        this->fMusicFrequencyBackup = this->music->getFrequency();
        this->music->setSpeed(this->getSpeedMultiplier());
    }
}

void Beatmap::unloadMusic() {
    resourceManager->destroyResource(this->music);
    this->music = NULL;
}

void Beatmap::unloadObjects() {
    this->currentHitObject = NULL;
    for(int i = 0; i < this->hitobjects.size(); i++) {
        delete this->hitobjects[i];
    }
    this->hitobjects = std::vector<HitObject *>();
    this->hitobjectsSortedByEndTime = std::vector<HitObject *>();
    this->misaimObjects = std::vector<HitObject *>();
    this->breaks = std::vector<DatabaseBeatmap::BREAK>();
    this->clicks = std::vector<Click>();
}

void Beatmap::resetHitObjects(long curPos) {
    for(int i = 0; i < this->hitobjects.size(); i++) {
        this->hitobjects[i]->onReset(curPos);
        this->hitobjects[i]->update(curPos, engine->getFrameTime());
        this->hitobjects[i]->onReset(curPos);
    }
    osu->getHUD()->resetHitErrorBar();
}

void Beatmap::resetScore() {
    this->vanilla = convar->isVanilla();

    this->live_replay.clear();
    this->live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = -1,
        .milliseconds_since_last_frame = 0,
        .x = 256,
        .y = -500,
        .key_flags = 0,
    });
    this->live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = -1,
        .milliseconds_since_last_frame = -1,
        .x = 256,
        .y = -500,
        .key_flags = 0,
    });

    this->last_event_time = engine->getTimeReal();
    this->last_event_ms = 0;
    this->current_keys = 0;
    this->last_keys = 0;
    this->current_frame_idx = 0;
    this->iCurMusicPos = 0;
    this->iCurMusicPosWithOffsets = 0;

    this->fHealth = 1.0;
    this->fHealth2 = 1.0f;
    this->bFailed = false;
    this->fFailAnim = 1.0f;
    anim->deleteExistingAnimation(&this->fFailAnim);

    osu->getScore()->reset();
    osu->hud->resetScoreboard();

    this->holding_slider = false;
    this->bIsFirstMissSound = true;
}

void Beatmap::playMissSound() {
    if((this->bIsFirstMissSound && osu->getScore()->getCombo() > 0) ||
       osu->getScore()->getCombo() > cv::combobreak_sound_combo.getInt()) {
        this->bIsFirstMissSound = false;
        soundEngine->play(this->getSkin()->getCombobreak());
    }
}

u32 Beatmap::getMusicPositionMSInterpolated() {
    if(!cv::interpolate_music_pos.getBool() || this->isLoading()) {
        return this->music->getPositionMS();
    } else {
        const f64 interpolationMultiplier = 1.0;

        // TODO: fix snapping at beginning for maps with instant start

        u32 returnPos = 0;
        const f64 curPos = (f64)this->music->getPositionMS();
        const f32 speed = this->music->getSpeed();

        // not reinventing the wheel, the interpolation magic numbers here are (c) peppy

        const f64 realTime = engine->getTimeReal();
        const f64 interpolationDelta = (realTime - this->fLastRealTimeForInterpolationDelta) * 1000.0 * speed;
        const f64 interpolationDeltaLimit =
            ((realTime - this->fLastAudioTimeAccurateSet) * 1000.0 < 1500 || speed < 1.0f ? 11 : 33) *
            interpolationMultiplier;

        if(this->music->isPlaying() && !this->bWasSeekFrame) {
            f64 newInterpolatedPos = this->fInterpolatedMusicPos + interpolationDelta;
            f64 delta = newInterpolatedPos - curPos;

            // debugLog("delta = %ld\n", (long)delta);

            // approach and recalculate delta
            newInterpolatedPos -= delta / 8.0 / interpolationMultiplier;
            delta = newInterpolatedPos - curPos;

            if(std::abs(delta) > interpolationDeltaLimit * 2)  // we're fucked, snap back to curPos
            {
                this->fInterpolatedMusicPos = (f64)curPos;
            } else if(delta < -interpolationDeltaLimit)  // undershot
            {
                this->fInterpolatedMusicPos += interpolationDelta * 2;
                this->fLastAudioTimeAccurateSet = realTime;
            } else if(delta < interpolationDeltaLimit)  // normal
            {
                this->fInterpolatedMusicPos = newInterpolatedPos;
            } else  // overshot
            {
                this->fInterpolatedMusicPos += interpolationDelta / 2;
                this->fLastAudioTimeAccurateSet = realTime;
            }

            // calculate final return value
            returnPos = (u32)std::round(this->fInterpolatedMusicPos);

            if(speed < 1.0f && cv::compensate_music_speed.getBool() && !cv::nightcore_enjoyer.getBool()) {
                returnPos += (u32)(((1.0f - speed) / 0.75f) * 5);
            }
        } else  // no interpolation
        {
            returnPos = curPos;
            this->fInterpolatedMusicPos = (u32)returnPos;
            this->fLastAudioTimeAccurateSet = realTime;
        }

        this->fLastRealTimeForInterpolationDelta =
            realTime;  // this is more accurate than engine->getFrameTime() for the delta calculation, since it
                       // correctly handles all possible delays inbetween

        // debugLog("returning %lu \n", returnPos);
        // debugLog("delta = %lu\n", (long)returnPos - m_iCurMusicPos);
        // debugLog("raw delta = %ld\n", (long)returnPos - (long)curPos);

        return returnPos;
    }
}

void Beatmap::draw() {
    if(!this->canDraw()) return;

    // draw background
    this->drawBackground();

    // draw loading circle
    if(this->isLoading()) {
        if(this->isBuffering()) {
            f32 leeway = std::clamp<i32>(this->last_frame_ms - this->iCurMusicPos, 0, cv::spec_buffer.getInt());
            f32 pct = leeway / (cv::spec_buffer.getFloat()) * 100.f;
            auto loadingMessage = UString::format("Buffering ... (%.2f%%)", pct);
            osu->getHUD()->drawLoadingSmall(loadingMessage);

            // draw the rest of the playfield while buffering/paused
        } else if(bancho->is_playing_a_multi_map() && !bancho->room.all_players_loaded) {
            osu->getHUD()->drawLoadingSmall("Waiting for players ...");

            // only start drawing the rest of the playfield if everything has loaded
            return;
        } else {
            osu->getHUD()->drawLoadingSmall("Loading ...");

            // only start drawing the rest of the playfield if everything has loaded
            return;
        }
    }

    if(this->is_watching && cv::simulate_replays.getBool()) {
        // XXX: while this fixes HUD desyncing, it's not perfect replay playback
        this->sim->simulate_to(this->iCurMusicPosWithOffsets);
        *osu->getScore() = this->sim->live_score;
        osu->getScore()->simulating = false;
        osu->getScore()->setCheated();
    }

    // draw playfield border
    if(cv::draw_playfield_border.getBool() && !cv::mod_fps.getBool()) {
        osu->getHUD()->drawPlayfieldBorder(this->vPlayfieldCenter, this->vPlayfieldSize, this->fHitcircleDiameter);
    }

    // draw hiterrorbar
    if(!cv::mod_fposu.getBool()) osu->getHUD()->drawHitErrorBar(this);

    // draw first person crosshair
    if(cv::mod_fps.getBool()) {
        const int length = 15;
        Vector2 center =
            this->osuCoords2Pixels(Vector2(GameRules::OSU_COORD_WIDTH / 2, GameRules::OSU_COORD_HEIGHT / 2));
        g->setColor(0xff777777);
        g->drawLine(center.x, (int)(center.y - length), center.x, (int)(center.y + length + 1));
        g->drawLine((int)(center.x - length), center.y, (int)(center.x + length + 1), center.y);
    }

    // draw followpoints
    if(cv::draw_followpoints.getBool() && !cv::mod_mafham.getBool()) this->drawFollowPoints();

    // draw all hitobjects in reverse
    if(cv::draw_hitobjects.getBool()) this->drawHitObjects();

    // draw spectator pause message
    if(this->spectate_pause) {
        auto info = BANCHO::User::get_user_info(bancho->spectated_player_id);
        auto pause_msg = UString::format("%s has paused", info->name.toUtf8());
        osu->getHUD()->drawLoadingSmall(pause_msg);
    }

    // debug stuff
    if(cv::debug_hiterrorbar_misaims.getBool()) {
        for(int i = 0; i < this->misaimObjects.size(); i++) {
            g->setColor(0xbb00ff00);
            Vector2 pos = this->osuCoords2Pixels(this->misaimObjects[i]->getRawPosAt(0));
            g->fillRect(pos.x - 50, pos.y - 50, 100, 100);
        }
    }
}

void Beatmap::drawFollowPoints() {
    Skin *skin = osu->getSkin();

    const long curPos = this->iCurMusicPosWithOffsets;

    // I absolutely hate this, followpoints can be abused for cheesing high AR reading since they always fade in with a
    // fixed 800 ms custom approach time. Capping it at the current approach rate seems sensible, but unfortunately
    // that's not what osu is doing. It was non-osu-compliant-clamped since this client existed, but let's see how many
    // people notice a change after all this time (26.02.2020)

    // 0.7x means animation lasts only 0.7 of it's time
    const f64 animationMutiplier = this->getSpeedMultiplier() / osu->getAnimationSpeedMultiplier();
    const long followPointApproachTime =
        animationMutiplier * (cv::followpoints_clamp.getBool()
                                  ? std::min((long)this->getApproachTime(), (long)cv::followpoints_approachtime.getFloat())
                                  : (long)cv::followpoints_approachtime.getFloat());
    const bool followPointsConnectCombos = cv::followpoints_connect_combos.getBool();
    const bool followPointsConnectSpinners = cv::followpoints_connect_spinners.getBool();
    const f32 followPointSeparationMultiplier = std::max(cv::followpoints_separation_multiplier.getFloat(), 0.1f);
    const f32 followPointPrevFadeTime = animationMutiplier * cv::followpoints_prevfadetime.getFloat();
    const f32 followPointScaleMultiplier = cv::followpoints_scale_multiplier.getFloat();

    // include previous object in followpoints
    int lastObjectIndex = -1;

    for(int index = this->iPreviousFollowPointObjectIndex; index < this->hitobjects.size(); index++) {
        lastObjectIndex = index - 1;

        // ignore future spinners
        Spinner *spinnerPointer = dynamic_cast<Spinner *>(this->hitobjects[index]);
        if(spinnerPointer != NULL && !followPointsConnectSpinners)  // if this is a spinner
        {
            lastObjectIndex = -1;
            continue;
        }

        const bool isCurrentHitObjectNewCombo =
            (lastObjectIndex >= 0 ? this->hitobjects[lastObjectIndex]->is_end_of_combo : false);
        const bool isCurrentHitObjectSpinner = (lastObjectIndex >= 0 && followPointsConnectSpinners
                                                    ? dynamic_cast<Spinner *>(this->hitobjects[lastObjectIndex]) != NULL
                                                    : false);
        if(lastObjectIndex >= 0 && (!isCurrentHitObjectNewCombo || followPointsConnectCombos ||
                                    (isCurrentHitObjectSpinner && followPointsConnectSpinners))) {
            // ignore previous spinners
            spinnerPointer = dynamic_cast<Spinner *>(this->hitobjects[lastObjectIndex]);
            if(spinnerPointer != NULL && !followPointsConnectSpinners)  // if this is a spinner
            {
                lastObjectIndex = -1;
                continue;
            }

            // get time & pos of the last and current object
            const long lastObjectEndTime =
                this->hitobjects[lastObjectIndex]->click_time + this->hitobjects[lastObjectIndex]->duration + 1;
            const long objectStartTime = this->hitobjects[index]->click_time;
            const long timeDiff = objectStartTime - lastObjectEndTime;

            const Vector2 startPoint =
                this->osuCoords2Pixels(this->hitobjects[lastObjectIndex]->getRawPosAt(lastObjectEndTime));
            const Vector2 endPoint = this->osuCoords2Pixels(this->hitobjects[index]->getRawPosAt(objectStartTime));

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
                    std::clamp<f32>((f32)(curPos - fadeInTime) / (f32)followPointPrevFadeTime, 0.0f, 1.0f);
                followAnimPercent = -followAnimPercent * (followAnimPercent - 2.0f);  // quad out

                // NOTE: only internal osu default skin uses scale + move transforms here, it is impossible to achieve
                // this effect with user skins
                const f32 scale = cv::followpoints_anim.getBool() ? 1.5f - 0.5f * followAnimPercent : 1.0f;
                const Vector2 followPos = cv::followpoints_anim.getBool()
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
                    g->rotate(glm::degrees(std::atan2(yDiff, xDiff)));

                    skin->getFollowPoint2()->setAnimationTimeOffset(skin->getAnimationSpeed(), fadeInTime);

                    // NOTE: getSizeBaseRaw() depends on the current animation time being set correctly beforehand!
                    // (otherwise you get incorrect scales, e.g. for animated elements with inconsistent @2x mixed in)
                    // the followpoints are scaled by one eighth of the hitcirclediameter (not the raw diameter, but the
                    // scaled diameter)
                    const f32 followPointImageScale =
                        ((this->fHitcircleDiameter / 8.0f) / skin->getFollowPoint2()->getSizeBaseRaw().x) *
                        followPointScaleMultiplier;

                    skin->getFollowPoint2()->drawRaw(followPos, followPointImageScale * scale);
                }
                g->popTransform();
            }
        }

        // store current index as previous index
        lastObjectIndex = index;

        // iterate up until the "nextest" element
        if(this->hitobjects[index]->click_time >= curPos + followPointApproachTime) break;
    }
}

void Beatmap::drawHitObjects() {
    const long curPos = this->iCurMusicPosWithOffsets;
    const long pvs = this->getPVS();
    const bool usePVS = cv::pvs.getBool();

    if(!cv::mod_mafham.getBool()) {
        if(!cv::draw_reverse_order.getBool()) {
            for(int i = this->hitobjectsSortedByEndTime.size() - 1; i >= 0; i--) {
                // PVS optimization (reversed)
                if(usePVS) {
                    if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                           this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                        break;
                    if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                        continue;
                }

                this->hitobjectsSortedByEndTime[i]->draw();
            }
        } else {
            for(int i = 0; i < this->hitobjectsSortedByEndTime.size(); i++) {
                // PVS optimization
                if(usePVS) {
                    if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                           this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                        continue;
                    if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                        break;
                }

                this->hitobjectsSortedByEndTime[i]->draw();
            }
        }
        for(int i = 0; i < this->hitobjectsSortedByEndTime.size(); i++) {
            // NOTE: to fix mayday simultaneous sliders with increasing endtime getting culled here, would have to
            // switch from m_hitobjectsSortedByEndTime to m_hitobjects PVS optimization
            if(usePVS) {
                if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                   (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                       this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                    continue;
                if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                    break;
            }

            this->hitobjectsSortedByEndTime[i]->draw2();
        }
    } else {
        const int mafhamRenderLiveSize = cv::mod_mafham_render_livesize.getInt();

        if(this->mafhamActiveRenderTarget == NULL) this->mafhamActiveRenderTarget = osu->getFrameBuffer();

        if(this->mafhamFinishedRenderTarget == NULL) this->mafhamFinishedRenderTarget = osu->getFrameBuffer2();

        // if we have a chunk to render into the scene buffer
        const bool shouldDrawBuffer =
            (this->hitobjectsSortedByEndTime.size() - this->iCurrentHitObjectIndex) > mafhamRenderLiveSize;
        bool shouldRenderChunk =
            this->iMafhamHitObjectRenderIndex < this->hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
        if(shouldRenderChunk) {
            this->bInMafhamRenderChunk = true;

            this->mafhamActiveRenderTarget->setClearColorOnDraw(this->iMafhamHitObjectRenderIndex == 0);
            this->mafhamActiveRenderTarget->setClearDepthOnDraw(this->iMafhamHitObjectRenderIndex == 0);

            this->mafhamActiveRenderTarget->enable();
            {
                g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
                {
                    int chunkCounter = 0;
                    for(int i = this->hitobjectsSortedByEndTime.size() - 1 - this->iMafhamHitObjectRenderIndex; i >= 0;
                        i--, this->iMafhamHitObjectRenderIndex++) {
                        chunkCounter++;
                        if(chunkCounter > cv::mod_mafham_render_chunksize.getInt())
                            break;  // continue chunk render in next frame

                        if(i <= this->iCurrentHitObjectIndex + mafhamRenderLiveSize)  // skip live objects
                        {
                            this->iMafhamHitObjectRenderIndex =
                                this->hitobjectsSortedByEndTime.size();  // stop chunk render
                            break;
                        }

                        // PVS optimization (reversed)
                        if(usePVS) {
                            if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                               (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                                   this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                            {
                                this->iMafhamHitObjectRenderIndex =
                                    this->hitobjectsSortedByEndTime.size();  // stop chunk render
                                break;
                            }
                            if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                                continue;
                        }

                        this->hitobjectsSortedByEndTime[i]->draw();

                        this->iMafhamActiveRenderHitObjectIndex = i;
                    }
                }
                g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
            }
            this->mafhamActiveRenderTarget->disable();

            this->bInMafhamRenderChunk = false;
        }
        shouldRenderChunk =
            this->iMafhamHitObjectRenderIndex < this->hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
        if(!shouldRenderChunk && this->bMafhamRenderScheduled) {
            // finished, we can now swap the active framebuffer with the one we just finished
            this->bMafhamRenderScheduled = false;

            RenderTarget *temp = this->mafhamFinishedRenderTarget;
            this->mafhamFinishedRenderTarget = this->mafhamActiveRenderTarget;
            this->mafhamActiveRenderTarget = temp;

            this->iMafhamFinishedRenderHitObjectIndex = this->iMafhamActiveRenderHitObjectIndex;
            this->iMafhamActiveRenderHitObjectIndex = this->hitobjectsSortedByEndTime.size();  // reset
        }

        // draw scene buffer
        if(shouldDrawBuffer) {
            g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_COLOR);
            { this->mafhamFinishedRenderTarget->draw(0, 0); }
            g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
        }

        // draw followpoints
        if(cv::draw_followpoints.getBool()) this->drawFollowPoints();

        // draw live hitobjects (also, code duplication yay)
        {
            for(int i = this->hitobjectsSortedByEndTime.size() - 1; i >= 0; i--) {
                // PVS optimization (reversed)
                if(usePVS) {
                    if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                           this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                        break;
                    if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                        continue;
                }

                if(i > this->iCurrentHitObjectIndex + mafhamRenderLiveSize ||
                   (i > this->iMafhamFinishedRenderHitObjectIndex - 1 && shouldDrawBuffer))  // skip non-live objects
                    continue;

                this->hitobjectsSortedByEndTime[i]->draw();
            }

            for(int i = 0; i < this->hitobjectsSortedByEndTime.size(); i++) {
                // PVS optimization
                if(usePVS) {
                    if(this->hitobjectsSortedByEndTime[i]->isFinished() &&
                       (curPos - pvs > this->hitobjectsSortedByEndTime[i]->click_time +
                                           this->hitobjectsSortedByEndTime[i]->duration))  // past objects
                        continue;
                    if(this->hitobjectsSortedByEndTime[i]->click_time > curPos + pvs)  // future objects
                        break;
                }

                if(i >= this->iCurrentHitObjectIndex + mafhamRenderLiveSize ||
                   (i >= this->iMafhamFinishedRenderHitObjectIndex - 1 && shouldDrawBuffer))  // skip non-live objects
                    break;

                this->hitobjectsSortedByEndTime[i]->draw2();
            }
        }
    }
}

void Beatmap::update() {
    if(!this->canUpdate()) return;

    // some things need to be updated before loading has finished, so control flow is a bit weird here.

    // live update hitobject and playfield metrics
    this->updateHitobjectMetrics();
    this->updatePlayfieldMetrics();

    // wobble mod
    if(cv::mod_wobble.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / this->getSpeedMultiplier();
        this->fPlayfieldRotation = (this->iCurMusicPos / 1000.0f) * 30.0f * speedMultiplierCompensation *
                                   cv::mod_wobble_rotation_speed.getFloat();
        this->fPlayfieldRotation = std::fmod(this->fPlayfieldRotation, 360.0f);
    } else {
        this->fPlayfieldRotation = 0.0f;
    }

    // do hitobject updates among other things
    // yes, this needs to happen after updating metrics and playfield rotation
    this->update2();

    // handle preloading (only for distributed slider vertexbuffer generation atm)
    bool was_preloading = this->bIsPreLoading;
    if(this->bIsPreLoading) {
        if(cv::debug.getBool() && this->iPreLoadingIndex == 0)
            debugLog("Beatmap: Preloading slider vertexbuffers ...\n");

        f64 startTime = engine->getTimeReal();
        f64 delta = 0.0;

        // hardcoded deadline of 10 ms, will temporarily bring us down to 45fps on average (better than freezing)
        while(delta < 0.010 && this->bIsPreLoading) {
            if(this->iPreLoadingIndex >= this->hitobjects.size()) {
                this->bIsPreLoading = false;
                debugLog("Beatmap: Preloading done.\n");
                break;
            } else {
                Slider *sliderPointer = dynamic_cast<Slider *>(this->hitobjects[this->iPreLoadingIndex]);
                if(sliderPointer != NULL) sliderPointer->rebuildVertexBuffer();
            }

            this->iPreLoadingIndex++;
            delta = engine->getTimeReal() - startTime;
        }
    }

    // notify all other players (including ourself) once we've finished loading
    if(bancho->is_playing_a_multi_map()) {
        if(!this->isActuallyLoading()) {
            if(!bancho->room.player_loaded) {
                bancho->room.player_loaded = true;

                Packet packet;
                packet.id = MATCH_LOAD_COMPLETE;
                BANCHO::Net::send_packet(packet);
            }
        }
    }

    if(this->isLoading() || was_preloading) {
        // Only continue if we have loaded everything.
        // We also return if we just finished preloading, since we need an extra update loop
        // to correctly initialize the beatmap.
        return;
    }

    // @PPV3: also calculate live ppv3
    if(cv::draw_statistics_pp.getBool() || cv::draw_statistics_livestars.getBool()) {
        auto info = this->ppv2_calc.get();
        osu->getHUD()->live_pp = info.pp;
        osu->getHUD()->live_stars = info.total_stars;

        if(this->last_calculated_hitobject < this->iCurrentHitObjectIndex) {
            this->last_calculated_hitobject = this->iCurrentHitObjectIndex;

            auto current_hitobject = this->iCurrentHitObjectIndex;
            auto CS = this->getCS();
            auto AR = this->getAR();
            auto OD = this->getOD();
            auto speedMultiplier = this->getSpeedMultiplier();
            auto osufile_path = this->selectedDifficulty2->getFilePath();
            auto nb_circles = this->iCurrentNumCircles;
            auto nb_sliders = this->iCurrentNumSliders;
            auto nb_spinners = this->iCurrentNumSpinners;
            auto modsLegacy = osu->getScore()->getModsLegacy();
            auto relax = osu->getModRelax();
            auto td = osu->getModTD();
            auto highestCombo = osu->getScore()->getComboMax();
            auto numMisses = osu->getScore()->getNumMisses();
            auto num300s = osu->getScore()->getNum300s();
            auto num100s = osu->getScore()->getNum100s();
            auto num50s = osu->getScore()->getNum50s();

            this->ppv2_calc.enqueue([=]() {
                pp_info info;

                // XXX: slow
                auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(osufile_path.c_str(), AR, CS, speedMultiplier);

                DifficultyCalculator::StarCalcParams params;
                params.CS = CS;
                params.OD = OD;
                params.speedMultiplier = speedMultiplier;
                params.relax = relax;
                params.touchDevice = td;
                params.upToObjectIndex = current_hitobject;
                params.sortedHitObjects.swap(diffres.diffobjects);

                std::vector<f64> aimStrains;
                std::vector<f64> speedStrains;
                params.aim = &info.aim_stars;
                params.aimSliderFactor = &info.aim_slider_factor;
                params.difficultAimStrains = &info.difficult_aim_strains;
                params.speed = &info.speed_stars;
                params.speedNotes = &info.speed_notes;
                params.difficultSpeedStrains = &info.difficult_speed_strains;
                params.outAimStrains = &aimStrains;
                params.outSpeedStrains = &speedStrains;

                info.total_stars = DifficultyCalculator::calculateStarDiffForHitObjects(params);

                info.pp = DifficultyCalculator::calculatePPv2(
                    modsLegacy, params.speedMultiplier, AR, params.OD, info.aim_stars, info.aim_slider_factor,
                    info.difficult_aim_strains, info.speed_stars, info.speed_notes, info.difficult_speed_strains,
                    nb_circles, nb_sliders, nb_spinners, diffres.maxPossibleCombo, highestCombo, numMisses, num300s,
                    num100s, num50s);

                return info;
            });
        }
    }

    // update auto (after having updated the hitobjects)
    if(osu->getModAuto() || osu->getModAutopilot()) this->updateAutoCursorPos();

    // spinner detection (used by osu!stable drain, and by HUD for not drawing the hiterrorbar)
    if(this->currentHitObject != NULL) {
        this->bIsSpinnerActive = this->currentHitObject->type == HitObjectType::SPINNER;
        this->bIsSpinnerActive &= this->iCurMusicPosWithOffsets > this->currentHitObject->click_time;
        this->bIsSpinnerActive &=
            this->iCurMusicPosWithOffsets < this->currentHitObject->click_time + this->currentHitObject->duration;
    }

    // scene buffering logic
    if(cv::mod_mafham.getBool()) {
        if(!this->bMafhamRenderScheduled &&
           this->iCurrentHitObjectIndex !=
               this->iMafhamPrevHitObjectIndex)  // if we are not already rendering and the index changed
        {
            this->iMafhamPrevHitObjectIndex = this->iCurrentHitObjectIndex;
            this->iMafhamHitObjectRenderIndex = 0;
            this->bMafhamRenderScheduled = true;
        }
    }

    // full alternate mod lenience
    if(cv::mod_fullalternate.getBool()) {
        if(this->bInBreak || this->bIsInSkippableSection || this->bIsSpinnerActive || this->iCurrentHitObjectIndex < 1)
            this->iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = this->iCurrentHitObjectIndex + 1;
    }

    if(this->last_keys != this->current_keys) {
        this->write_frame();
    } else if(this->last_event_time + 0.01666666666 <= engine->getTimeReal()) {
        this->write_frame();
    }
}

void Beatmap::update2() {
    if(this->bContinueScheduled) {
        // If we paused while m_bIsWaiting (green progressbar), then we have to let the 'if (this->bIsWaiting)' block
        // handle the sound play() call
        bool isEarlyNoteContinue = (!this->bIsPaused && this->bIsWaiting);
        if(this->bClickedContinue || isEarlyNoteContinue) {
            this->bClickedContinue = false;
            this->bContinueScheduled = false;
            this->bIsPaused = false;

            if(!isEarlyNoteContinue) {
                soundEngine->play(this->music);
            }

            this->bIsPlaying = true;  // usually this should be checked with the result of the above play() call, but
                                      // since we are continuing we can assume that everything works

            // for nightmare mod, to avoid a miss because of the continue click
            this->clicks.clear();
        }
    }

    // handle restarts
    if(this->bIsRestartScheduled) {
        this->bIsRestartScheduled = false;
        this->actualRestart();
        return;
    }

    // update current music position (this variable does not include any offsets!)
    this->iCurMusicPos = this->getMusicPositionMSInterpolated();
    this->iContinueMusicPos = this->music->getPositionMS();
    const bool wasSeekFrame = this->bWasSeekFrame;
    this->bWasSeekFrame = false;

    // handle timewarp
    if(cv::mod_timewarp.getBool()) {
        if(this->hitobjects.size() > 0 && this->iCurMusicPos > this->hitobjects[0]->click_time) {
            const f32 percentFinished =
                ((f64)(this->iCurMusicPos - this->hitobjects[0]->click_time) /
                 (f64)(this->hitobjects[this->hitobjects.size() - 1]->click_time +
                       this->hitobjects[this->hitobjects.size() - 1]->duration - this->hitobjects[0]->click_time));
            f32 warp_multiplier = std::max(cv::mod_timewarp_multiplier.getFloat(), 1.f);
            const f32 speed =
                this->getSpeedMultiplier() + percentFinished * this->getSpeedMultiplier() * (warp_multiplier - 1.0f);
            this->music->setSpeed(speed);
        }
    }

    // HACKHACK: clean this mess up
    // waiting to start (file loading, retry)
    // NOTE: this is dependent on being here AFTER m_iCurMusicPos has been set above, because it modifies it to fake a
    // negative start (else everything would just freeze for the waiting period)
    if(this->bIsWaiting) {
        if(this->isLoading()) {
            this->fWaitTime = engine->getTimeReal();

            // if the first hitobject starts immediately, add artificial wait time before starting the music
            if(!this->bIsRestartScheduledQuick && this->hitobjects.size() > 0) {
                if(this->hitobjects[0]->click_time < (long)cv::early_note_time.getInt()) {
                    this->fWaitTime = engine->getTimeReal() + cv::early_note_time.getFloat() / 1000.0f;
                }
            }
        } else {
            if(engine->getTimeReal() > this->fWaitTime) {
                if(!this->bIsPaused) {
                    this->bIsWaiting = false;
                    this->bIsPlaying = true;

                    soundEngine->play(this->music);
                    this->music->setLoop(false);
                    this->music->setPositionMS(0);
                    this->bWasSeekFrame = true;
                    this->music->setVolume(this->getIdealVolume());
                    this->music->setSpeed(this->getSpeedMultiplier());

                    // if we are quick restarting, jump just before the first hitobject (even if there is a long waiting
                    // period at the beginning with nothing etc.)
                    bool quick_restarting = this->bIsRestartScheduledQuick;
                    quick_restarting &=
                        this->hitobjects.size() > 0 && this->hitobjects[0]->click_time > cv::quick_retry_time.getInt();
                    if(quick_restarting) {
                        this->music->setPositionMS(
                            std::max((i64)0, this->hitobjects[0]->click_time - cv::quick_retry_time.getInt()));
                    }
                    this->bWasSeekFrame = true;

                    this->bIsRestartScheduledQuick = false;

                    // if there are calculations in there that need the hitobjects to be loaded, also applies
                    // speed/pitch
                    this->onModUpdate(false, false);
                }
            } else {
                this->iCurMusicPos = (engine->getTimeReal() - this->fWaitTime) * 1000.0f * this->getSpeedMultiplier();
            }
        }

        // ugh. force update all hitobjects while waiting (necessary because of pvs optimization)
        long curPos = this->iCurMusicPos + (long)(cv::universal_offset.getFloat() * this->getSpeedMultiplier()) +
                      cv::universal_offset_hardcoded.getInt() - this->selectedDifficulty2->getLocalOffset() -
                      this->selectedDifficulty2->getOnlineOffset() -
                      (this->selectedDifficulty2->getVersion() < 5 ? cv::old_beatmap_offset.getInt() : 0);
        if(curPos > -1)  // otherwise auto would already click elements that start at exactly 0 (while the map has not
                         // even started)
            curPos = -1;

        for(int i = 0; i < this->hitobjects.size(); i++) {
            this->hitobjects[i]->update(curPos, engine->getFrameTime());
        }
    }

    if(bancho->spectating) {
        if(this->spectate_pause && !this->bFailed && this->music->isPlaying()) {
            soundEngine->pause(this->music);
            this->bIsPlaying = false;
            this->bIsPaused = true;
        }
        if(!this->spectate_pause && this->bIsPaused) {
            soundEngine->play(this->music);
            this->bIsPlaying = true;
            this->bIsPaused = false;
        }
    }

    // only continue updating hitobjects etc. if we have loaded everything
    if(this->isLoading()) return;

    // handle music loading fail
    if(!this->music->isReady()) {
        this->iResourceLoadUpdateDelayHack++;  // HACKHACK: async loading takes 1 additional engine update() until both
                                               // isAsyncReady() and isReady() return true
        if(this->iResourceLoadUpdateDelayHack > 1 &&
           !this->bForceStreamPlayback)  // first: try loading a stream version of the music file
        {
            this->bForceStreamPlayback = true;
            this->unloadMusic();
            this->loadMusic(true);

            // we are waiting for an asynchronous start of the beatmap in the next update()
            this->bIsWaiting = true;
            this->fWaitTime = engine->getTimeReal();
        } else if(this->iResourceLoadUpdateDelayHack >
                  3)  // second: if that still doesn't work, stop and display an error message
        {
            osu->getNotificationOverlay()->addToast("Couldn't load music file :(", 0xffff0000);
            this->stop(true);
        }
    }

    // detect and handle music end
    if(!this->bIsWaiting && this->music->isReady()) {
        const bool isMusicFinished = this->music->isFinished();

        // trigger virtual audio time after music finishes
        if(!isMusicFinished)
            this->fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
        else if(this->fAfterMusicIsFinishedVirtualAudioTimeStart < 0.0f)
            this->fAfterMusicIsFinishedVirtualAudioTimeStart = engine->getTimeReal();

        if(isMusicFinished) {
            // continue with virtual audio time until the last hitobject is done (plus sanity offset given via
            // osu_end_delay_time) because some beatmaps have hitobjects going until >= the exact end of the music ffs
            // NOTE: this overwrites m_iCurMusicPos for the rest of the update loop
            this->iCurMusicPos =
                (long)this->music->getLengthMS() +
                (long)((engine->getTimeReal() - this->fAfterMusicIsFinishedVirtualAudioTimeStart) * 1000.0f);
        }

        const bool hasAnyHitObjects = (this->hitobjects.size() > 0);
        const bool isTimePastLastHitObjectPlusLenience =
            (this->iCurMusicPos >
             (this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->click_time +
              this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->duration +
              (long)cv::end_delay_time.getInt()));
        if(!hasAnyHitObjects || (cv::end_skip.getBool() && isTimePastLastHitObjectPlusLenience) ||
           (!cv::end_skip.getBool() && isMusicFinished)) {
            if(!this->bFailed) {
                this->stop(false);
                return;
            }
        }
    }

    // update timing (points)
    this->iCurMusicPosWithOffsets =
        this->iCurMusicPos + (long)(cv::universal_offset.getFloat() * this->getSpeedMultiplier()) +
        cv::universal_offset_hardcoded.getInt() - this->selectedDifficulty2->getLocalOffset() -
        this->selectedDifficulty2->getOnlineOffset() -
        (this->selectedDifficulty2->getVersion() < 5 ? cv::old_beatmap_offset.getInt() : 0);
    this->updateTimingPoints(this->iCurMusicPosWithOffsets);

    // Make sure we're not too far behind the liveplay
    if(bancho->spectating) {
        if(this->iCurMusicPos + (2 * cv::spec_buffer.getInt()) < this->last_frame_ms) {
            i32 target = this->last_frame_ms - cv::spec_buffer.getInt();
            f32 percent = (f32)target / (f32)this->getLength();
            debugLog("We're %dms behind, seeking to catch up to player...\n", this->last_frame_ms - this->iCurMusicPos);
            this->seekPercent(percent);
            return;
        }
    }

    if((this->is_watching || bancho->spectating) && this->spectated_replay.size() >= 2) {
        LegacyReplay::Frame current_frame = this->spectated_replay[this->current_frame_idx];
        LegacyReplay::Frame next_frame = this->spectated_replay[this->current_frame_idx + 1];

        while(next_frame.cur_music_pos <= this->iCurMusicPosWithOffsets) {
            if(this->current_frame_idx + 2 >= this->spectated_replay.size()) break;

            this->last_keys = this->current_keys;

            this->current_frame_idx++;
            current_frame = this->spectated_replay[this->current_frame_idx];
            next_frame = this->spectated_replay[this->current_frame_idx + 1];
            this->current_keys = current_frame.key_flags;

            Click click;
            click.click_time = current_frame.cur_music_pos;
            click.pos.x = current_frame.x;
            click.pos.y = current_frame.y;
            click.pos *= GameRules::getPlayfieldScaleFactor();
            click.pos += GameRules::getPlayfieldOffset();

            // Flag fix to simplify logic (stable sets both K1 and M1 when K1 is pressed)
            if(this->current_keys & LegacyReplay::K1) this->current_keys &= ~LegacyReplay::M1;
            if(this->current_keys & LegacyReplay::K2) this->current_keys &= ~LegacyReplay::M2;

            // Released key 1
            if(this->last_keys & LegacyReplay::K1 && !(this->current_keys & LegacyReplay::K1)) {
                osu->getHUD()->animateInputoverlay(1, false);
            }
            if(this->last_keys & LegacyReplay::M1 && !(this->current_keys & LegacyReplay::M1)) {
                osu->getHUD()->animateInputoverlay(3, false);
            }

            // Released key 2
            if(this->last_keys & LegacyReplay::K2 && !(this->current_keys & LegacyReplay::K2)) {
                osu->getHUD()->animateInputoverlay(2, false);
            }
            if(this->last_keys & LegacyReplay::M2 && !(this->current_keys & LegacyReplay::M2)) {
                osu->getHUD()->animateInputoverlay(4, false);
            }

            // Pressed key 1
            if(!(this->last_keys & LegacyReplay::K1) && this->current_keys & LegacyReplay::K1) {
                this->bPrevKeyWasKey1 = true;
                osu->getHUD()->animateInputoverlay(1, true);
                this->clicks.push_back(click);
                if(!this->bInBreak && !this->bIsInSkippableSection) osu->getScore()->addKeyCount(1);
            }
            if(!(this->last_keys & LegacyReplay::M1) && this->current_keys & LegacyReplay::M1) {
                this->bPrevKeyWasKey1 = true;
                osu->getHUD()->animateInputoverlay(3, true);
                this->clicks.push_back(click);
                if(!this->bInBreak && !this->bIsInSkippableSection) osu->getScore()->addKeyCount(3);
            }

            // Pressed key 2
            if(!(this->last_keys & LegacyReplay::K2) && this->current_keys & LegacyReplay::K2) {
                this->bPrevKeyWasKey1 = false;
                osu->getHUD()->animateInputoverlay(2, true);
                this->clicks.push_back(click);
                if(!this->bInBreak && !this->bIsInSkippableSection) osu->getScore()->addKeyCount(2);
            }
            if(!(this->last_keys & LegacyReplay::M2) && this->current_keys & LegacyReplay::M2) {
                this->bPrevKeyWasKey1 = false;
                osu->getHUD()->animateInputoverlay(4, true);
                this->clicks.push_back(click);
                if(!this->bInBreak && !this->bIsInSkippableSection) osu->getScore()->addKeyCount(4);
            }
        }

        f32 percent = 0.f;
        if(next_frame.milliseconds_since_last_frame > 0) {
            long ms_since_last_frame = this->iCurMusicPosWithOffsets - current_frame.cur_music_pos;
            percent = (f32)ms_since_last_frame / (f32)next_frame.milliseconds_since_last_frame;
        }

        this->interpolatedMousePos =
            Vector2{std::lerp(current_frame.x, next_frame.x, percent), std::lerp(current_frame.y, next_frame.y, percent)};

        if(cv::playfield_mirror_horizontal.getBool())
            this->interpolatedMousePos.y = GameRules::OSU_COORD_HEIGHT - this->interpolatedMousePos.y;
        if(cv::playfield_mirror_vertical.getBool())
            this->interpolatedMousePos.x = GameRules::OSU_COORD_WIDTH - this->interpolatedMousePos.x;

        if(cv::playfield_rotation.getFloat() != 0.0f) {
            this->interpolatedMousePos.x -= GameRules::OSU_COORD_WIDTH / 2;
            this->interpolatedMousePos.y -= GameRules::OSU_COORD_HEIGHT / 2;
            Vector3 coords3 = Vector3(this->interpolatedMousePos.x, this->interpolatedMousePos.y, 0);
            Matrix4 rot;
            rot.rotateZ(cv::playfield_rotation.getFloat());
            coords3 = coords3 * rot;
            coords3.x += GameRules::OSU_COORD_WIDTH / 2;
            coords3.y += GameRules::OSU_COORD_HEIGHT / 2;
            this->interpolatedMousePos.x = coords3.x;
            this->interpolatedMousePos.y = coords3.y;
        }

        this->interpolatedMousePos *= GameRules::getPlayfieldScaleFactor();
        this->interpolatedMousePos += GameRules::getPlayfieldOffset();
    }

    // for performance reasons, a lot of operations are crammed into 1 loop over all hitobjects:
    // update all hitobjects,
    // handle click events,
    // also get the time of the next/previous hitobject and their indices for later,
    // and get the current hitobject,
    // also handle miss hiterrorbar slots,
    // also calculate nps and nd,
    // also handle note blocking
    this->currentHitObject = NULL;
    this->iNextHitObjectTime = 0;
    this->iPreviousHitObjectTime = 0;
    this->iPreviousFollowPointObjectIndex = 0;
    this->iNPS = 0;
    this->iND = 0;
    this->iCurrentNumCircles = 0;
    this->iCurrentNumSliders = 0;
    this->iCurrentNumSpinners = 0;
    {
        bool blockNextNotes = false;

        const long pvs = !cv::mod_mafham.getBool()
                             ? this->getPVS()
                             : (this->hitobjects.size() > 0
                                    ? (this->hitobjects[std::clamp<int>(this->iCurrentHitObjectIndex +
                                                                       cv::mod_mafham_render_livesize.getInt() + 1,
                                                                   0, this->hitobjects.size() - 1)]
                                           ->click_time -
                                       this->iCurMusicPosWithOffsets + 1500)
                                    : this->getPVS());
        const bool usePVS = cv::pvs.getBool();

        const int notelockType = cv::notelock_type.getInt();
        const long tolerance2B = (long)cv::notelock_stable_tolerance2b.getInt();

        this->iCurrentHitObjectIndex = 0;  // reset below here, since it's needed for mafham pvs

        for(int i = 0; i < this->hitobjects.size(); i++) {
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
            const bool isCircle = this->hitobjects[i]->type == HitObjectType::CIRCLE;
            const bool isSlider = this->hitobjects[i]->type == HitObjectType::SLIDER;
            const bool isSpinner = this->hitobjects[i]->type == HitObjectType::SPINNER;
            // ************ live pp block end ************** //

            // determine previous & next object time, used for auto + followpoints + warning arrows + empty section
            // skipping
            if(this->iNextHitObjectTime == 0) {
                if(this->hitobjects[i]->click_time > this->iCurMusicPosWithOffsets)
                    this->iNextHitObjectTime = this->hitobjects[i]->click_time;
                else {
                    this->currentHitObject = this->hitobjects[i];
                    const long actualPrevHitObjectTime =
                        this->hitobjects[i]->click_time + this->hitobjects[i]->duration;
                    this->iPreviousHitObjectTime = actualPrevHitObjectTime;

                    if(this->iCurMusicPosWithOffsets >
                       actualPrevHitObjectTime + (long)cv::followpoints_prevfadetime.getFloat())
                        this->iPreviousFollowPointObjectIndex = i;
                }
            }

            // PVS optimization
            if(usePVS) {
                if(this->hitobjects[i]->isFinished() &&
                   (this->iCurMusicPosWithOffsets - pvs >
                    this->hitobjects[i]->click_time + this->hitobjects[i]->duration))  // past objects
                {
                    // ************ live pp block start ************ //
                    if(isCircle) this->iCurrentNumCircles++;
                    if(isSlider) this->iCurrentNumSliders++;
                    if(isSpinner) this->iCurrentNumSpinners++;

                    this->iCurrentHitObjectIndex = i;
                    // ************ live pp block end ************** //

                    continue;
                }
                if(this->hitobjects[i]->click_time > this->iCurMusicPosWithOffsets + pvs)  // future objects
                    break;
            }

            // ************ live pp block start ************ //
            if(this->iCurMusicPosWithOffsets >= this->hitobjects[i]->click_time + this->hitobjects[i]->duration)
                this->iCurrentHitObjectIndex = i;
            // ************ live pp block end ************** //

            // main hitobject update
            this->hitobjects[i]->update(this->iCurMusicPosWithOffsets, engine->getFrameTime());

            // note blocking / notelock (1)
            const Slider *currentSliderPointer = dynamic_cast<Slider *>(this->hitobjects[i]);
            if(notelockType > 0) {
                this->hitobjects[i]->setBlocked(blockNextNotes);

                if(notelockType == 1)  // neosu
                {
                    // (nothing, handled in (2) block)
                } else if(notelockType == 2)  // osu!stable
                {
                    if(!this->hitobjects[i]->isFinished()) {
                        blockNextNotes = true;

                        // Sliders are "finished" after they end
                        // Extra handling for simultaneous/2b hitobjects, as these would otherwise get blocked
                        // NOTE: this will still unlock some simultaneous/2b patterns too early
                        //       (slider slider circle [circle]), but nobody from that niche has complained so far
                        {
                            const bool isSlider = (currentSliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(isSlider || isSpinner) {
                                if((i + 1) < this->hitobjects.size()) {
                                    if((isSpinner || currentSliderPointer->isStartCircleFinished()) &&
                                       (this->hitobjects[i + 1]->click_time <=
                                        (this->hitobjects[i]->click_time + this->hitobjects[i]->duration +
                                         tolerance2B)))
                                        blockNextNotes = false;
                                }
                            }
                        }
                    }
                } else if(notelockType == 3)  // osu!lazer 2020
                {
                    if(!this->hitobjects[i]->isFinished()) {
                        const bool isSlider = (currentSliderPointer != NULL);
                        const bool isSpinner = (!isSlider && !isCircle);

                        if(!isSpinner)  // spinners are completely ignored (transparent)
                        {
                            blockNextNotes = (this->iCurMusicPosWithOffsets <= this->hitobjects[i]->click_time);

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
                this->hitobjects[i]->setBlocked(false);

            // click events (this also handles hitsounds!)
            const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents =
                (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());
            const bool isCurrentHitObjectFinishedBeforeClickEvents = this->hitobjects[i]->isFinished();
            {
                if(this->clicks.size() > 0) this->hitobjects[i]->onClickEvent(this->clicks);
            }
            const bool isCurrentHitObjectFinishedAfterClickEvents = this->hitobjects[i]->isFinished();
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
                    // m_hitobjects[i]->onClickEvent(this->clicks); so, we check if the currentSliderPointer got its
                    // isStartCircleFinished() within this m_hitobjects[i]->onClickEvent(this->clicks); and unlock
                    // blockNextNotes if that is the case note that we still only unlock within duration + tolerance2B
                    // (same as in (1))
                    if((i + 1) < this->hitobjects.size()) {
                        if((this->hitobjects[i + 1]->click_time <=
                            (this->hitobjects[i]->click_time + this->hitobjects[i]->duration + tolerance2B)))
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
                        if(!this->hitobjects[m]->isFinished()) {
                            const Slider *sliderPointer = dynamic_cast<Slider *>(this->hitobjects[m]);

                            const bool isSlider = (sliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(!isSpinner)  // spinners are completely ignored (transparent)
                            {
                                if(this->hitobjects[i]->click_time >
                                   (this->hitobjects[m]->click_time +
                                    this->hitobjects[m]->duration))  // NOTE: 2b exception. only force miss if objects
                                                                     // are not overlapping.
                                    this->hitobjects[m]->miss(this->iCurMusicPosWithOffsets);
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
                        if(!this->hitobjects[m]->isFinished()) {
                            const Slider *sliderPointer = dynamic_cast<Slider *>(this->hitobjects[m]);

                            const bool isSlider = (sliderPointer != NULL);
                            const bool isSpinner = (!isSlider && !isCircle);

                            if(!isSpinner)  // spinners are completely ignored (transparent)
                            {
                                if(this->iCurMusicPosWithOffsets > this->hitobjects[m]->click_time) {
                                    if(this->hitobjects[i]->click_time >
                                       (this->hitobjects[m]->click_time +
                                        this->hitobjects[m]->duration))  // NOTE: 2b exception. only force miss if
                                                                         // objects are not overlapping.
                                        this->hitobjects[m]->miss(this->iCurMusicPosWithOffsets);
                                }
                            }
                        } else
                            break;
                    }
                }
            }

            // ************ live pp block start ************ //
            if(isCircle && this->hitobjects[i]->isFinished()) this->iCurrentNumCircles++;
            if(isSlider && this->hitobjects[i]->isFinished()) this->iCurrentNumSliders++;
            if(isSpinner && this->hitobjects[i]->isFinished()) this->iCurrentNumSpinners++;

            if(this->hitobjects[i]->isFinished()) this->iCurrentHitObjectIndex = i;
            // ************ live pp block end ************** //

            // notes per second
            const long npsHalfGateSizeMS = (long)(500.0f * this->getSpeedMultiplier());
            if(this->hitobjects[i]->click_time > this->iCurMusicPosWithOffsets - npsHalfGateSizeMS &&
               this->hitobjects[i]->click_time < this->iCurMusicPosWithOffsets + npsHalfGateSizeMS)
                this->iNPS++;

            // note density
            if(this->hitobjects[i]->isVisible()) this->iND++;
        }

        // miss hiterrorbar slots
        // this gets the closest previous unfinished hitobject, as well as all following hitobjects which are in 50
        // range and could be clicked
        if(cv::hiterrorbar_misaims.getBool()) {
            this->misaimObjects.clear();
            HitObject *lastUnfinishedHitObject = NULL;
            const long hitWindow50 = (long)this->getHitWindow50();
            for(int i = 0; i < this->hitobjects.size(); i++)  // this shouldn't hurt performance too much, since no
                                                              // expensive operations are happening within the loop
            {
                if(!this->hitobjects[i]->isFinished()) {
                    if(this->iCurMusicPosWithOffsets >= this->hitobjects[i]->click_time)
                        lastUnfinishedHitObject = this->hitobjects[i];
                    else if(std::abs(this->hitobjects[i]->click_time - this->iCurMusicPosWithOffsets) < hitWindow50)
                        this->misaimObjects.push_back(this->hitobjects[i]);
                    else
                        break;
                }
            }
            if(lastUnfinishedHitObject != NULL &&
               std::abs(lastUnfinishedHitObject->click_time - this->iCurMusicPosWithOffsets) < hitWindow50)
                this->misaimObjects.insert(this->misaimObjects.begin(), lastUnfinishedHitObject);

            // now, go through the remaining clicks, and go through the unfinished hitobjects.
            // handle misaim clicks sequentially (setting the misaim flag on the hitobjects to only allow 1 entry in the
            // hiterrorbar for misses per object) clicks don't have to be consumed here, as they are deleted below
            // anyway
            for(int c = 0; c < this->clicks.size(); c++) {
                for(int i = 0; i < this->misaimObjects.size(); i++) {
                    if(this->misaimObjects[i]->hasMisAimed())  // only 1 slot per object!
                        continue;

                    this->misaimObjects[i]->misAimed();
                    const long delta = this->clicks[c].click_time - (long)this->misaimObjects[i]->click_time;
                    osu->getHUD()->addHitError(delta, false, true);

                    break;  // the current click has been dealt with (and the hitobject has been misaimed)
                }
            }
        }

        // all remaining clicks which have not been consumed by any hitobjects can safely be deleted
        if(this->clicks.size() > 0) {
            if(cv::play_hitsound_on_click_while_playing.getBool()) osu->getSkin()->playHitCircleSound(0, 0.f, 0);

            // nightmare mod: extra clicks = sliderbreak
            if((osu->getModNightmare() || cv::mod_jigsaw1.getBool()) && !this->bIsInSkippableSection &&
               !this->bInBreak && this->iCurrentHitObjectIndex > 0) {
                this->addSliderBreak();
                this->addHitResult(NULL, LiveScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true,
                                   false);  // only decrease health
            }

            this->clicks.clear();
        }
    }

    // empty section detection & skipping
    if(this->hitobjects.size() > 0) {
        const long legacyOffset = (this->iPreviousHitObjectTime < this->hitobjects[0]->click_time ? 0 : 1000);  // Mc
        const long nextHitObjectDelta = this->iNextHitObjectTime - (long)this->iCurMusicPosWithOffsets;
        if(nextHitObjectDelta > 0 && nextHitObjectDelta > (long)cv::skip_time.getInt() &&
           this->iCurMusicPosWithOffsets > (this->iPreviousHitObjectTime + legacyOffset))
            this->bIsInSkippableSection = true;
        else if(!cv::end_skip.getBool() && nextHitObjectDelta < 0)
            this->bIsInSkippableSection = true;
        else
            this->bIsInSkippableSection = false;

        osu->chat->updateVisibility();

        // While we want to allow the chat to pop up during breaks, we don't
        // want to be able to skip after the start in multiplayer rooms
        if(bancho->is_playing_a_multi_map() && this->iCurrentHitObjectIndex > 0) {
            this->bIsInSkippableSection = false;
        }
    }

    // warning arrow logic
    if(this->hitobjects.size() > 0) {
        const long legacyOffset = (this->iPreviousHitObjectTime < this->hitobjects[0]->click_time ? 0 : 1000);  // Mc
        const long minGapSize = 1000;
        const long lastVisibleMin = 400;
        const long blinkDelta = 100;

        const long gapSize = this->iNextHitObjectTime - (this->iPreviousHitObjectTime + legacyOffset);
        const long nextDelta = (this->iNextHitObjectTime - this->iCurMusicPosWithOffsets);
        const bool drawWarningArrows = gapSize > minGapSize && nextDelta > 0;
        if(drawWarningArrows &&
           ((nextDelta <= lastVisibleMin + blinkDelta * 13 && nextDelta > lastVisibleMin + blinkDelta * 12) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 11 && nextDelta > lastVisibleMin + blinkDelta * 10) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 9 && nextDelta > lastVisibleMin + blinkDelta * 8) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 7 && nextDelta > lastVisibleMin + blinkDelta * 6) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 5 && nextDelta > lastVisibleMin + blinkDelta * 4) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 3 && nextDelta > lastVisibleMin + blinkDelta * 2) ||
            (nextDelta <= lastVisibleMin + blinkDelta * 1 && nextDelta > lastVisibleMin)))
            this->bShouldFlashWarningArrows = true;
        else
            this->bShouldFlashWarningArrows = false;
    }

    // break time detection, and background fade during breaks
    const DatabaseBeatmap::BREAK breakEvent = this->getBreakForTimeRange(
        this->iPreviousHitObjectTime, this->iCurMusicPosWithOffsets, this->iNextHitObjectTime);
    const bool isInBreak = ((int)this->iCurMusicPosWithOffsets >= breakEvent.startTime &&
                            (int)this->iCurMusicPosWithOffsets <= breakEvent.endTime);
    if(isInBreak != this->bInBreak) {
        this->bInBreak = !this->bInBreak;

        if(!cv::background_dont_fade_during_breaks.getBool() || this->fBreakBackgroundFade != 0.0f) {
            if(this->bInBreak && !cv::background_dont_fade_during_breaks.getBool()) {
                const int breakDuration = breakEvent.endTime - breakEvent.startTime;
                if(breakDuration > (int)(cv::background_fade_min_duration.getFloat() * 1000.0f))
                    anim->moveLinear(&this->fBreakBackgroundFade, 1.0f, cv::background_fade_in_duration.getFloat(),
                                     true);
            } else
                anim->moveLinear(&this->fBreakBackgroundFade, 0.0f, cv::background_fade_out_duration.getFloat(), true);
        }
    }

    // section pass/fail logic
    if(this->hitobjects.size() > 0) {
        const long minGapSize = 2880;
        const long fadeStart = 1280;
        const long fadeEnd = 1480;

        const long gapSize = this->iNextHitObjectTime - this->iPreviousHitObjectTime;
        const long start = (gapSize / 2 > minGapSize ? this->iPreviousHitObjectTime + (gapSize / 2)
                                                     : this->iNextHitObjectTime - minGapSize);
        const long nextDelta = this->iCurMusicPosWithOffsets - start;
        const bool inSectionPassFail =
            (gapSize > minGapSize && nextDelta > 0) &&
            this->iCurMusicPosWithOffsets > this->hitobjects[0]->click_time &&
            this->iCurMusicPosWithOffsets <
                (this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->click_time +
                 this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]->duration) &&
            !this->bFailed && this->bInBreak && (breakEvent.endTime - breakEvent.startTime) > minGapSize;

        const bool passing = (this->fHealth >= 0.5);

        // draw logic
        if(passing) {
            if(inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280) ||
                                     (nextDelta <= 230 && nextDelta >= 160) || (nextDelta <= 100 && nextDelta >= 20))) {
                const f32 fadeAlpha = 1.0f - (f32)(nextDelta - fadeStart) / (f32)(fadeEnd - fadeStart);
                this->fShouldFlashSectionPass = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
            } else
                this->fShouldFlashSectionPass = 0.0f;
        } else {
            if(inSectionPassFail &&
               ((nextDelta <= fadeEnd && nextDelta >= 280) || (nextDelta <= 230 && nextDelta >= 130))) {
                const f32 fadeAlpha = 1.0f - (f32)(nextDelta - fadeStart) / (f32)(fadeEnd - fadeStart);
                this->fShouldFlashSectionFail = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
            } else
                this->fShouldFlashSectionFail = 0.0f;
        }

        // sound logic
        if(inSectionPassFail) {
            if(this->iPreviousSectionPassFailTime != start &&
               ((passing && nextDelta >= 20) || (!passing && nextDelta >= 130))) {
                this->iPreviousSectionPassFailTime = start;

                if(!wasSeekFrame) {
                    if(passing)
                        soundEngine->play(osu->getSkin()->getSectionPassSound());
                    else
                        soundEngine->play(osu->getSkin()->getSectionFailSound());
                }
            }
        }
    }

    // hp drain & failing
    // handle constant drain
    {
        if(this->fDrainRate > 0.0) {
            if(this->bIsPlaying                  // not paused
               && !this->bInBreak                // not in a break
               && !this->bIsInSkippableSection)  // not in a skippable section
            {
                // special case: break drain edge cases
                bool drainAfterLastHitobjectBeforeBreakStart = (this->selectedDifficulty2->getVersion() < 8);

                const bool isBetweenHitobjectsAndBreak = (int)this->iPreviousHitObjectTime <= breakEvent.startTime &&
                                                         (int)this->iNextHitObjectTime >= breakEvent.endTime &&
                                                         this->iCurMusicPosWithOffsets > this->iPreviousHitObjectTime;
                const bool isLastHitobjectBeforeBreakStart =
                    isBetweenHitobjectsAndBreak && (int)this->iCurMusicPosWithOffsets <= breakEvent.startTime;

                if(!isBetweenHitobjectsAndBreak ||
                   (drainAfterLastHitobjectBeforeBreakStart && isLastHitobjectBeforeBreakStart)) {
                    // special case: spinner nerf
                    f64 spinnerDrainNerf = this->isSpinnerActive() ? 0.25 : 1.0;
                    this->addHealth(
                        -this->fDrainRate * engine->getFrameTime() * (f64)this->getSpeedMultiplier() * spinnerDrainNerf,
                        false);
                }
            }
        }
    }

    // revive in mp
    if(this->fHealth > 0.999 && osu->getScore()->isDead()) osu->getScore()->setDead(false);

    // handle fail animation
    if(this->bFailed) {
        if(this->fFailAnim <= 0.0f) {
            if(this->music->isPlaying() || !osu->getPauseMenu()->isVisible()) {
                soundEngine->pause(this->music);
                this->bIsPaused = true;

                if(bancho->spectating) {
                    osu->songBrowser2->bHasSelectedAndIsPlaying = false;
                } else {
                    osu->getPauseMenu()->setVisible(true);
                    osu->updateConfineCursor();
                }
            }
        } else {
            this->music->setFrequency(this->fMusicFrequencyBackup * this->fFailAnim > 100
                                          ? this->fMusicFrequencyBackup * this->fFailAnim
                                          : 100);
        }
    }

    // spectator score correction
    if(bancho->spectating && this->spectated_replay.size() >= 2) {
        auto current_frame = this->spectated_replay[this->current_frame_idx];

        i32 score_frame_idx = -1;
        for(i32 i = 0; i < this->score_frames.size(); i++) {
            if(this->score_frames[i].time == current_frame.cur_music_pos) {
                score_frame_idx = i;
                break;
            }
        }

        if(score_frame_idx != -1) {
            auto fixed_score = this->score_frames[score_frame_idx];
            osu->score->iNum300s = fixed_score.num300;
            osu->score->iNum100s = fixed_score.num100;
            osu->score->iNum50s = fixed_score.num50;
            osu->score->iNum300gs = fixed_score.num_geki;
            osu->score->iNum100ks = fixed_score.num_katu;
            osu->score->iNumMisses = fixed_score.num_miss;
            osu->score->iScoreV1 = fixed_score.total_score;
            osu->score->iScoreV2 = fixed_score.total_score;
            osu->score->iComboMax = fixed_score.max_combo;
            osu->score->iCombo = fixed_score.current_combo;

            // XXX: instead naively of setting it, we should simulate time decay
            this->fHealth = ((f32)fixed_score.current_hp) / 255.f;

            // fixed_score.is_perfect is unused
            // fixed_score.tag is unused (XXX: i think scorev2 uses this?)
            // fixed_score.is_scorev2 is unused
        }
    }
}

void Beatmap::broadcast_spectator_frames() {
    if(bancho->spectators.empty()) return;

    Packet packet;
    packet.id = OUT_SPECTATE_FRAMES;
    proto::write<i32>(&packet, 0);
    proto::write<u16>(&packet, this->frame_batch.size());
    for(auto batch : this->frame_batch) {
        proto::write<LiveReplayFrame>(&packet, batch);
    }
    proto::write<u8>(&packet, LiveReplayBundle::Action::NONE);
    proto::write<ScoreFrame>(&packet, ScoreFrame::get());
    proto::write<u16>(&packet, this->spectator_sequence++);
    BANCHO::Net::send_packet(packet);

    this->frame_batch.clear();
    this->last_spectator_broadcast = engine->getTime();
}

void Beatmap::write_frame() {
    if(!this->bIsPlaying || this->bFailed || this->is_watching || bancho->spectating) return;

    long delta = this->iCurMusicPosWithOffsets - this->last_event_ms;
    if(delta < 0) return;
    if(delta == 0 && this->last_keys == this->current_keys) return;

    Vector2 pos = this->pixels2OsuCoords(this->getCursorPos());
    if(cv::playfield_mirror_horizontal.getBool()) pos.y = GameRules::OSU_COORD_HEIGHT - pos.y;
    if(cv::playfield_mirror_vertical.getBool()) pos.x = GameRules::OSU_COORD_WIDTH - pos.x;
    if(cv::playfield_rotation.getFloat() != 0.0f) {
        pos.x -= GameRules::OSU_COORD_WIDTH / 2;
        pos.y -= GameRules::OSU_COORD_HEIGHT / 2;
        Vector3 coords3 = Vector3(pos.x, pos.y, 0);
        Matrix4 rot;
        rot.rotateZ(-cv::playfield_rotation.getFloat());
        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;
        pos.x = coords3.x;
        pos.y = coords3.y;
    }

    this->live_replay.push_back(LegacyReplay::Frame{
        .cur_music_pos = this->iCurMusicPosWithOffsets,
        .milliseconds_since_last_frame = delta,
        .x = pos.x,
        .y = pos.y,
        .key_flags = this->current_keys,
    });

    this->frame_batch.push_back(LiveReplayFrame{
        .key_flags = this->current_keys,
        .padding = 0,
        .mouse_x = pos.x,
        .mouse_y = pos.y,
        .time = (i32)this->iCurMusicPos,  // NOTE: might be incorrect
    });

    this->last_event_time = this->fLastRealTimeForInterpolationDelta;
    this->last_event_ms = this->iCurMusicPosWithOffsets;
    this->last_keys = this->current_keys;

    if(!bancho->spectators.empty() && engine->getTime() > this->last_spectator_broadcast + 1.0) {
        this->broadcast_spectator_frames();
    }
}

void Beatmap::onModUpdate(bool rebuildSliderVertexBuffers, bool recomputeDrainRate) {
    if(cv::debug.getBool()) debugLog("Beatmap::onModUpdate() @ %f\n", engine->getTime());

    this->updatePlayfieldMetrics();
    this->updateHitobjectMetrics();

    if(recomputeDrainRate) this->computeDrainRate();

    if(this->music != NULL) {
        // Updates not just speed but also nightcore state
        this->music->setSpeed(this->getSpeedMultiplier());
    }

    // recalculate slider vertexbuffers
    if(osu->getModHR() != this->bWasHREnabled ||
       cv::playfield_mirror_horizontal.getBool() != this->bWasHorizontalMirrorEnabled ||
       cv::playfield_mirror_vertical.getBool() != this->bWasVerticalMirrorEnabled) {
        this->bWasHREnabled = osu->getModHR();
        this->bWasHorizontalMirrorEnabled = cv::playfield_mirror_horizontal.getBool();
        this->bWasVerticalMirrorEnabled = cv::playfield_mirror_vertical.getBool();

        this->calculateStacks();

        if(rebuildSliderVertexBuffers) this->updateSliderVertexBuffers();
    }
    if(osu->getModEZ() != this->bWasEZEnabled) {
        this->calculateStacks();

        this->bWasEZEnabled = osu->getModEZ();
        if(rebuildSliderVertexBuffers) this->updateSliderVertexBuffers();
    }
    if(this->fHitcircleDiameter != this->fPrevHitCircleDiameter && this->hitobjects.size() > 0) {
        this->calculateStacks();

        this->fPrevHitCircleDiameter = this->fHitcircleDiameter;
        if(rebuildSliderVertexBuffers) this->updateSliderVertexBuffers();
    }
    if(cv::playfield_rotation.getFloat() != this->fPrevPlayfieldRotationFromConVar) {
        this->fPrevPlayfieldRotationFromConVar = cv::playfield_rotation.getFloat();
        if(rebuildSliderVertexBuffers) this->updateSliderVertexBuffers();
    }
    if(cv::mod_mafham.getBool() != this->bWasMafhamEnabled) {
        this->bWasMafhamEnabled = cv::mod_mafham.getBool();
        for(int i = 0; i < this->hitobjects.size(); i++) {
            this->hitobjects[i]->update(this->iCurMusicPosWithOffsets, engine->getFrameTime());
        }
    }

    this->resetLiveStarsTasks();
}

void Beatmap::resetLiveStarsTasks() {
    if(cv::debug.getBool()) debugLog("Beatmap::resetLiveStarsTasks() called\n");

    osu->getHUD()->live_pp = 0.0;
    osu->getHUD()->live_stars = 0.0;

    this->last_calculated_hitobject = -1;
}

// HACK: Updates buffering state and pauses/unpauses the music!
bool Beatmap::isBuffering() {
    if(!bancho->spectating) return false;

    i32 leeway = this->last_frame_ms - this->iCurMusicPos;
    if(this->is_buffering) {
        // Make sure music is actually paused
        if(this->music->isPlaying()) {
            soundEngine->pause(this->music);
            this->bIsPlaying = false;
            this->bIsPaused = true;
        }

        if(leeway >= cv::spec_buffer.getInt()) {
            debugLog("UNPAUSING: leeway: %i, last_event: %d, last_frame: %d\n", leeway, this->iCurMusicPos,
                     this->last_frame_ms);
            soundEngine->play(this->music);
            this->bIsPlaying = true;
            this->bIsPaused = false;
            this->is_buffering = false;
        }
    } else {
        HitObject *lastHitObject = this->hitobjectsSortedByEndTime.size() > 0
                                       ? this->hitobjectsSortedByEndTime[this->hitobjectsSortedByEndTime.size() - 1]
                                       : NULL;
        bool is_finished = lastHitObject != NULL && lastHitObject->isFinished();

        if(leeway < 0 && !is_finished) {
            debugLog("PAUSING: leeway: %i, last_event: %d, last_frame: %d\n", leeway, this->iCurMusicPos,
                     this->last_frame_ms);
            soundEngine->pause(this->music);
            this->bIsPlaying = false;
            this->bIsPaused = true;
            this->is_buffering = true;
        }
    }

    return this->is_buffering;
}

bool Beatmap::isLoading() {
    return (this->isActuallyLoading() || this->isBuffering() ||
            (bancho->is_playing_a_multi_map() && !bancho->room.all_players_loaded));
}

bool Beatmap::isActuallyLoading() {
    return (!soundEngine->isReady() || !this->music->isAsyncReady() || this->bIsPreLoading);
}

Vector2 Beatmap::pixels2OsuCoords(Vector2 pixelCoords) const {
    // un-first-person
    if(cv::mod_fps.getBool()) {
        // HACKHACK: this is the worst hack possible (engine->isDrawing()), but it works
        // the problem is that this same function is called while draw()ing and update()ing
        if(!((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) ||
             !(osu->getModAuto() || osu->getModAutopilot())))
            pixelCoords += this->getFirstPersonCursorDelta();
    }

    // un-offset and un-scale, reverse order
    pixelCoords -= this->vPlayfieldOffset;
    pixelCoords /= this->fScaleFactor;

    return pixelCoords;
}

Vector2 Beatmap::osuCoords2Pixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv::playfield_mirror_horizontal.getBool()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv::playfield_mirror_vertical.getBool()) coords.x = GameRules::OSU_COORD_WIDTH - coords.x;

    // wobble
    if(cv::mod_wobble.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / this->getSpeedMultiplier();
        coords.x += std::sin((this->iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                             cv::mod_wobble_frequency.getFloat()) *
                    cv::mod_wobble_strength.getFloat();
        coords.y += std::sin((this->iCurMusicPos / 1000.0f) * 4 * speedMultiplierCompensation *
                             cv::mod_wobble_frequency.getFloat()) *
                    cv::mod_wobble_strength.getFloat();
    }

    // wobble2
    if(cv::mod_wobble2.getBool()) {
        const f32 speedMultiplierCompensation = 1.0f / this->getSpeedMultiplier();
        Vector2 centerDelta = coords - Vector2(GameRules::OSU_COORD_WIDTH, GameRules::OSU_COORD_HEIGHT) / 2;
        coords.x += centerDelta.x * 0.25f *
                    std::sin((this->iCurMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                             cv::mod_wobble_frequency.getFloat()) *
                    cv::mod_wobble_strength.getFloat();
        coords.y += centerDelta.y * 0.25f *
                    std::sin((this->iCurMusicPos / 1000.0f) * 3 * speedMultiplierCompensation *
                             cv::mod_wobble_frequency.getFloat()) *
                    cv::mod_wobble_strength.getFloat();
    }

    // rotation
    if(this->fPlayfieldRotation + cv::playfield_rotation.getFloat() != 0.0f) {
        coords.x -= GameRules::OSU_COORD_WIDTH / 2;
        coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

        Vector3 coords3 = Vector3(coords.x, coords.y, 0);
        Matrix4 rot;
        rot.rotateZ(this->fPlayfieldRotation + cv::playfield_rotation.getFloat());

        coords3 = coords3 * rot;
        coords3.x += GameRules::OSU_COORD_WIDTH / 2;
        coords3.y += GameRules::OSU_COORD_HEIGHT / 2;

        coords.x = coords3.x;
        coords.y = coords3.y;
    }

    // if wobble, clamp coordinates
    if(cv::mod_wobble.getBool() || cv::mod_wobble2.getBool()) {
        coords.x = std::clamp<f32>(coords.x, 0.0f, GameRules::OSU_COORD_WIDTH);
        coords.y = std::clamp<f32>(coords.y, 0.0f, GameRules::OSU_COORD_HEIGHT);
    }

    if(this->bFailed) {
        f32 failTimePercentInv = 1.0f - this->fFailAnim;  // goes from 0 to 1 over the duration of osu_fail_time
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
    coords *= this->fScaleFactor;
    coords += this->vPlayfieldOffset;  // the offset is already scaled, just add it

    // first person mod, centered cursor
    if(cv::mod_fps.getBool()) {
        // this is the worst hack possible (engine->isDrawing()), but it works
        // the problem is that this same function is called while draw()ing and update()ing
        if((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) ||
           !(osu->getModAuto() || osu->getModAutopilot()))
            coords += this->getFirstPersonCursorDelta();
    }

    return coords;
}

Vector2 Beatmap::osuCoords2RawPixels(Vector2 coords) const {
    // scale and offset
    coords *= this->fScaleFactor;
    coords += this->vPlayfieldOffset;  // the offset is already scaled, just add it

    return coords;
}

Vector2 Beatmap::osuCoords2LegacyPixels(Vector2 coords) const {
    if(osu->getModHR()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv::playfield_mirror_horizontal.getBool()) coords.y = GameRules::OSU_COORD_HEIGHT - coords.y;
    if(cv::playfield_mirror_vertical.getBool()) coords.x = GameRules::OSU_COORD_WIDTH - coords.x;

    // rotation
    if(this->fPlayfieldRotation + cv::playfield_rotation.getFloat() != 0.0f) {
        coords.x -= GameRules::OSU_COORD_WIDTH / 2;
        coords.y -= GameRules::OSU_COORD_HEIGHT / 2;

        Vector3 coords3 = Vector3(coords.x, coords.y, 0);
        Matrix4 rot;
        rot.rotateZ(this->fPlayfieldRotation + cv::playfield_rotation.getFloat());

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
    if((this->is_watching && !this->bIsPaused) || bancho->spectating) {
        return this->interpolatedMousePos;
    } else {
        return mouse->getPos();
    }
}

Vector2 Beatmap::getCursorPos() const {
    if(cv::mod_fps.getBool() && !this->bIsPaused) {
        if(osu->getModAuto() || osu->getModAutopilot()) {
            return this->vAutoCursorPos;
        } else {
            return this->vPlayfieldCenter;
        }
    } else if(osu->getModAuto() || osu->getModAutopilot()) {
        return this->vAutoCursorPos;
    } else {
        Vector2 pos = this->getMousePos();
        if(cv::mod_shirone.getBool() && osu->getScore()->getCombo() > 0) {
            return pos + Vector2(std::sin((this->iCurMusicPos / 20.0f) * 1.15f) *
                                     ((f32)osu->getScore()->getCombo() / cv::mod_shirone_combo.getFloat()),
                                 std::cos((this->iCurMusicPos / 20.0f) * 1.3f) *
                                     ((f32)osu->getScore()->getCombo() / cv::mod_shirone_combo.getFloat()));
        } else {
            return pos;
        }
    }
}

Vector2 Beatmap::getFirstPersonCursorDelta() const {
    return this->vPlayfieldCenter -
           (osu->getModAuto() || osu->getModAutopilot() ? this->vAutoCursorPos : this->getMousePos());
}

FinishedScore Beatmap::saveAndSubmitScore(bool quit) {
    // calculate stars
    f64 aim = 0.0;
    f64 aimSliderFactor = 0.0;
    f64 speed = 0.0;
    f64 speedNotes = 0.0;
    f64 difficultAimStrains = 0.0;
    f64 difficultSpeedStrains = 0.0;
    const std::string &osuFilePath = this->selectedDifficulty2->getFilePath();
    const f32 AR = this->getAR();
    const f32 CS = this->getCS();
    const f32 OD = this->getOD();
    const f32 speedMultiplier = this->getSpeedMultiplier();  // NOTE: not this->getSpeedMultiplier()!
    const bool relax = osu->getModRelax();
    const bool touchDevice = osu->getModTD();

    auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, AR, CS, speedMultiplier);

    DifficultyCalculator::StarCalcParams params;
    params.sortedHitObjects.swap(diffres.diffobjects);
    params.CS = CS;
    params.OD = OD;
    params.speedMultiplier = speedMultiplier;
    params.relax = relax;
    params.touchDevice = touchDevice;
    params.aim = &aim;
    params.aimSliderFactor = &aimSliderFactor;
    params.difficultAimStrains = &difficultAimStrains;
    params.speed = &speed;
    params.speedNotes = &speedNotes;
    params.difficultSpeedStrains = &difficultSpeedStrains;
    params.upToObjectIndex = -1;
    params.outAimStrains = &this->aimStrains;
    params.outSpeedStrains = &this->speedStrains;
    const f64 totalStars = DifficultyCalculator::calculateStarDiffForHitObjects(params);

    this->fAimStars = (f32)aim;
    this->fSpeedStars = (f32)speed;

    // calculate final pp
    const int numHitObjects = this->hitobjects.size();
    const int numCircles = this->selectedDifficulty2->getNumCircles();
    const int numSliders = this->selectedDifficulty2->getNumSliders();
    const int numSpinners = this->selectedDifficulty2->getNumSpinners();
    const int highestCombo = osu->getScore()->getComboMax();
    const int numMisses = osu->getScore()->getNumMisses();
    const int num300s = osu->getScore()->getNum300s();
    const int num100s = osu->getScore()->getNum100s();
    const int num50s = osu->getScore()->getNum50s();
    const f32 pp = DifficultyCalculator::calculatePPv2(
        osu->getScore()->getModsLegacy(), speedMultiplier, AR, OD, aim, aimSliderFactor, difficultAimStrains, speed,
        speedNotes, difficultSpeedStrains, numCircles, numSliders, numSpinners, this->iMaxPossibleCombo, highestCombo,
        numMisses, num300s, num100s, num50s);
    osu->getScore()->setStarsTomTotal(totalStars);
    osu->getScore()->setStarsTomAim(this->fAimStars);
    osu->getScore()->setStarsTomSpeed(this->fSpeedStars);
    osu->getScore()->setPPv2(pp);

    // save local score, but only under certain conditions
    bool isComplete = (num300s + num100s + num50s + numMisses >= numHitObjects);
    bool isZero = (osu->getScore()->getScore() < 1);
    bool isCheated = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax())) ||
                     osu->getScore()->isUnranked() || this->is_watching || bancho->spectating;

    FinishedScore score;
    UString client_ver = "neosu-" OS_NAME "-" NEOSU_STREAM "-";
    client_ver.append(UString::format("%.2f", cv::version.getFloat()));
    score.client = client_ver.toUtf8();

    score.unixTimestamp =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // @neonet: check if we're connected to neonet
    if(bancho->is_online()) {
        score.player_id = bancho->user_id;
        score.playerName = bancho->username.toUtf8();
        score.server = bancho->endpoint.toUtf8();
    } else {
        score.playerName = cv::name.getString();
    }
    score.passed = isComplete && !isZero && !osu->getScore()->hasDied();
    score.grade = score.passed ? osu->getScore()->getGrade() : FinishedScore::Grade::F;
    score.diff2 = this->selectedDifficulty2;
    score.ragequit = quit;
    score.play_time_ms = this->iCurMusicPos / this->getSpeedMultiplier();

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
    score.perfect = (this->iMaxPossibleCombo > 0 && score.comboMax > 0 && score.comboMax >= this->iMaxPossibleCombo);
    score.numSliderBreaks = osu->getScore()->getNumSliderBreaks();
    score.unstableRate = osu->getScore()->getUnstableRate();
    score.hitErrorAvgMin = osu->getScore()->getHitErrorAvgMin();
    score.hitErrorAvgMax = osu->getScore()->getHitErrorAvgMax();
    score.maxPossibleCombo = this->iMaxPossibleCombo;
    score.numHitObjects = numHitObjects;
    score.numCircles = numCircles;
    score.mods = osu->getScore()->mods;
    score.beatmap_hash = this->selectedDifficulty2->getMD5Hash();  // NOTE: necessary for "Use Mods"
    score.replay = this->live_replay;

    // @PPV3: store ppv3 data if not already done. also double check replay is marked correctly
    score.ppv2_version = DifficultyCalculator::PP_ALGORITHM_VERSION;
    score.ppv2_score = pp;
    score.ppv2_total_stars = totalStars;
    score.ppv2_aim_stars = aim;
    score.ppv2_speed_stars = speed;

    if(!isCheated) {
        RichPresence::onPlayEnd(quit);

        if(bancho->submit_scores() && !isZero && this->vanilla) {
            score.server = bancho->endpoint.toUtf8();
            BANCHO::Net::submit_score(score);
            // XXX: Save bancho_score_id after getting submission result
        }

        if(score.passed) {
            int scoreIndex = db->addScore(score);
            if(scoreIndex == -1) {
                osu->getNotificationOverlay()->addToast("Failed saving score!", 0xffff0000);
            }
        }
    }

    if(!bancho->spectators.empty()) {
        this->broadcast_spectator_frames();

        Packet packet;
        packet.id = OUT_SPECTATE_FRAMES;
        proto::write<i32>(&packet, 0);
        proto::write<u16>(&packet, 0);
        proto::write<u8>(&packet, isComplete ? LiveReplayBundle::Action::COMPLETION : LiveReplayBundle::Action::FAIL);
        proto::write<ScoreFrame>(&packet, ScoreFrame::get());
        proto::write<u16>(&packet, this->spectator_sequence++);
        BANCHO::Net::send_packet(packet);
    }

    // special case: incomplete scores should NEVER show pp, even if auto
    if(!isComplete) {
        osu->getScore()->setPPv2(0.0f);
    }

    return score;
}

void Beatmap::updateAutoCursorPos() {
    this->vAutoCursorPos = this->vPlayfieldCenter;
    this->vAutoCursorPos.y *= 2.5f;  // start moving in offscreen from bottom

    if(!this->bIsPlaying && !this->bIsPaused) {
        this->vAutoCursorPos = this->vPlayfieldCenter;
        return;
    }
    if(this->hitobjects.size() < 1) {
        this->vAutoCursorPos = this->vPlayfieldCenter;
        return;
    }

    const long curMusicPos = this->iCurMusicPosWithOffsets;

    // general
    long prevTime = 0;
    long nextTime = this->hitobjects[0]->click_time;
    Vector2 prevPos = this->vAutoCursorPos;
    Vector2 curPos = this->vAutoCursorPos;
    Vector2 nextPos = this->vAutoCursorPos;
    bool haveCurPos = false;

    // dance
    int nextPosIndex = 0;

    if(this->hitobjects[0]->click_time < (long)cv::early_note_time.getInt())
        prevTime = -(long)cv::early_note_time.getInt() * this->getSpeedMultiplier();

    if(osu->getModAuto()) {
        bool autoDanceOverride = false;
        for(int i = 0; i < this->hitobjects.size(); i++) {
            HitObject *o = this->hitobjects[i];

            // get previous object
            if(o->click_time <= curMusicPos) {
                prevTime = o->click_time + o->duration;
                prevPos = o->getAutoCursorPos(curMusicPos);
                if(o->duration > 0 && curMusicPos - o->click_time <= o->duration) {
                    if(cv::auto_cursordance.getBool()) {
                        Slider *sliderPointer = dynamic_cast<Slider *>(o);
                        if(sliderPointer != NULL) {
                            const std::vector<Slider::SLIDERCLICK> &clicks = sliderPointer->getClicks();

                            // start
                            prevTime = o->click_time;
                            prevPos = this->osuCoords2Pixels(o->getRawPosAt(prevTime));

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
                                    prevPos = this->osuCoords2Pixels(o->getRawPosAt(prevTime));
                                }

                                // get next click
                                if(clicks[c].time > curMusicPos && clicks[c].time < smallestNext) {
                                    smallestNext = clicks[c].time;
                                    nextTime = clicks[c].time;
                                    nextPos = this->osuCoords2Pixels(o->getRawPosAt(nextTime));
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
                                if(endTime == 0) endTime = o->click_time;

                                prevTime = endTime;
                                prevPos = this->osuCoords2Pixels(o->getRawPosAt(prevTime));
                                nextTime = o->click_time + o->duration;
                                nextPos = this->osuCoords2Pixels(o->getRawPosAt(nextTime));
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
            if(o->click_time > curMusicPos) {
                nextPosIndex = i;
                if(!autoDanceOverride) {
                    nextPos = o->getAutoCursorPos(curMusicPos);
                    nextTime = o->click_time;
                }
                break;
            }
        }
    } else if(osu->getModAutopilot()) {
        for(int i = 0; i < this->hitobjects.size(); i++) {
            HitObject *o = this->hitobjects[i];

            // get previous object
            if(o->isFinished() ||
               (curMusicPos >
                o->click_time + o->duration + (long)(this->getHitWindow50() * cv::autopilot_lenience.getFloat()))) {
                prevTime = o->click_time + o->duration + o->getAutopilotDelta();
                prevPos = o->getAutoCursorPos(curMusicPos);
            } else if(!o->isFinished())  // get next object
            {
                nextPosIndex = i;
                nextPos = o->getAutoCursorPos(curMusicPos);
                nextTime = o->click_time;

                // wait for the user to click
                if(curMusicPos >= nextTime + o->duration) {
                    haveCurPos = true;
                    curPos = nextPos;

                    // long delta = curMusicPos - (nextTime + o->duration);
                    o->setAutopilotDelta(curMusicPos - (nextTime + o->duration));
                } else if(o->duration > 0 && curMusicPos >= nextTime)  // handle objects with duration
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
        this->vAutoCursorPos = curPos;
    else {
        // interpolation
        f32 percent = 1.0f;
        if((nextTime == 0 && prevTime == 0) || (nextTime - prevTime) == 0)
            percent = 1.0f;
        else
            percent = (f32)((long)curMusicPos - prevTime) / (f32)(nextTime - prevTime);

        percent = std::clamp<f32>(percent, 0.0f, 1.0f);

        // scaled distance (not osucoords)
        f32 distance = (nextPos - prevPos).length();
        if(distance > this->fHitcircleDiameter * 1.05f)  // snap only if not in a stream (heuristic)
        {
            int numIterations = std::clamp<int>(
                osu->getModAutopilot() ? cv::autopilot_snapping_strength.getInt() : cv::auto_snapping_strength.getInt(),
                0, 42);
            for(int i = 0; i < numIterations; i++) {
                percent = (-percent) * (percent - 2.0f);
            }
        } else  // in a stream
        {
            this->iAutoCursorDanceIndex = nextPosIndex;
        }

        this->vAutoCursorPos = prevPos + (nextPos - prevPos) * percent;

        if(cv::auto_cursordance.getBool() && !osu->getModAutopilot()) {
            Vector3 dir = Vector3(nextPos.x, nextPos.y, 0) - Vector3(prevPos.x, prevPos.y, 0);
            Vector3 center = dir * 0.5f;
            Matrix4 worldMatrix;
            worldMatrix.translate(center);
            worldMatrix.rotate((1.0f - percent) * 180.0f * (this->iAutoCursorDanceIndex % 2 == 0 ? 1 : -1), 0, 0, 1);
            Vector3 fancyAutoCursorPos = worldMatrix * center;
            this->vAutoCursorPos =
                prevPos + (nextPos - prevPos) * 0.5f + Vector2(fancyAutoCursorPos.x, fancyAutoCursorPos.y);
        }
    }
}

void Beatmap::updatePlayfieldMetrics() {
    this->fScaleFactor = GameRules::getPlayfieldScaleFactor();
    this->vPlayfieldSize = GameRules::getPlayfieldSize();
    this->vPlayfieldOffset = GameRules::getPlayfieldOffset();
    this->vPlayfieldCenter = GameRules::getPlayfieldCenter();
}

void Beatmap::updateHitobjectMetrics() {
    Skin *skin = osu->getSkin();

    this->fRawHitcircleDiameter = GameRules::getRawHitCircleDiameter(this->getCS());
    this->fXMultiplier = GameRules::getHitCircleXMultiplier();
    this->fHitcircleDiameter = GameRules::getRawHitCircleDiameter(this->getCS()) * GameRules::getHitCircleXMultiplier();

    const f32 osuCoordScaleMultiplier = (this->fHitcircleDiameter / this->fRawHitcircleDiameter);
    this->fNumberScale = (this->fRawHitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) *
                         osuCoordScaleMultiplier * cv::number_scale_multiplier.getFloat();
    this->fHitcircleOverlapScale =
        (this->fRawHitcircleDiameter / (160.0f)) * osuCoordScaleMultiplier * cv::number_scale_multiplier.getFloat();

    const f32 followcircle_size_multiplier = 2.4f;
    const f32 sliderFollowCircleDiameterMultiplier =
        (osu->getModNightmare() || cv::mod_jigsaw2.getBool()
             ? (1.0f * (1.0f - cv::mod_jigsaw_followcircle_radius_factor.getFloat()) +
                cv::mod_jigsaw_followcircle_radius_factor.getFloat() * followcircle_size_multiplier)
             : followcircle_size_multiplier);
    this->fSliderFollowCircleDiameter = this->fHitcircleDiameter * sliderFollowCircleDiameterMultiplier;
}

void Beatmap::updateSliderVertexBuffers() {
    this->updatePlayfieldMetrics();
    this->updateHitobjectMetrics();

    this->bWasEZEnabled = osu->getModEZ();                    // to avoid useless double updates in onModUpdate()
    this->fPrevHitCircleDiameter = this->fHitcircleDiameter;  // same here
    this->fPrevPlayfieldRotationFromConVar = cv::playfield_rotation.getFloat();  // same here

    debugLog("Beatmap::updateSliderVertexBuffers() for %i hitobjects ...\n", this->hitobjects.size());

    for(int i = 0; i < this->hitobjects.size(); i++) {
        Slider *sliderPointer = dynamic_cast<Slider *>(this->hitobjects[i]);
        if(sliderPointer != NULL) sliderPointer->rebuildVertexBuffer();
    }
}

void Beatmap::calculateStacks() {
    this->updateHitobjectMetrics();

    debugLog("Beatmap: Calculating stacks ...\n");

    // reset
    for(int i = 0; i < this->hitobjects.size(); i++) {
        this->hitobjects[i]->setStack(0);
    }

    const f32 STACK_LENIENCE = 3.0f;
    const f32 STACK_OFFSET = 0.05f;

    const f32 approachTime =
        GameRules::mapDifficultyRange(this->getAR(), GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                      GameRules::getMaxApproachTime());
    const f32 stackLeniency = this->selectedDifficulty2->getStackLeniency();

    if(this->getSelectedDifficulty2()->getVersion() > 5) {
        // peppy's algorithm
        // https://gist.github.com/peppy/1167470

        for(int i = this->hitobjects.size() - 1; i >= 0; i--) {
            int n = i;

            HitObject *objectI = this->hitobjects[i];

            bool isSpinner = dynamic_cast<Spinner *>(objectI) != NULL;

            if(objectI->getStack() != 0 || isSpinner) continue;

            bool isHitCircle = dynamic_cast<Circle *>(objectI) != NULL;
            bool isSlider = dynamic_cast<Slider *>(objectI) != NULL;

            if(isHitCircle) {
                while(--n >= 0) {
                    HitObject *objectN = this->hitobjects[n];

                    bool isSpinnerN = dynamic_cast<Spinner *>(objectN);

                    if(isSpinnerN) continue;

                    if(objectI->click_time - (approachTime * stackLeniency) > (objectN->click_time + objectN->duration))
                        break;

                    Vector2 objectNEndPosition = objectN->getOriginalRawPosAt(objectN->click_time + objectN->duration);
                    if(objectN->duration != 0 &&
                       (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->click_time)).length() <
                           STACK_LENIENCE) {
                        int offset = objectI->getStack() - objectN->getStack() + 1;
                        for(int j = n + 1; j <= i; j++) {
                            if((objectNEndPosition -
                                this->hitobjects[j]->getOriginalRawPosAt(this->hitobjects[j]->click_time))
                                   .length() < STACK_LENIENCE)
                                this->hitobjects[j]->setStack(this->hitobjects[j]->getStack() - offset);
                        }

                        break;
                    }

                    if((objectN->getOriginalRawPosAt(objectN->click_time) -
                        objectI->getOriginalRawPosAt(objectI->click_time))
                           .length() < STACK_LENIENCE) {
                        objectN->setStack(objectI->getStack() + 1);
                        objectI = objectN;
                    }
                }
            } else if(isSlider) {
                while(--n >= 0) {
                    HitObject *objectN = this->hitobjects[n];

                    bool isSpinner = dynamic_cast<Spinner *>(objectN) != NULL;

                    if(isSpinner) continue;

                    if(objectI->click_time - (approachTime * stackLeniency) > objectN->click_time) break;

                    if(((objectN->duration != 0 ? objectN->getOriginalRawPosAt(objectN->click_time + objectN->duration)
                                                : objectN->getOriginalRawPosAt(objectN->click_time)) -
                        objectI->getOriginalRawPosAt(objectI->click_time))
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

        for(int i = 0; i < this->hitobjects.size(); i++) {
            HitObject *currHitObject = this->hitobjects[i];
            Slider *sliderPointer = dynamic_cast<Slider *>(currHitObject);

            const bool isSlider = (sliderPointer != NULL);

            if(currHitObject->getStack() != 0 && !isSlider) continue;

            long startTime = currHitObject->click_time + currHitObject->duration;
            int sliderStack = 0;

            for(int j = i + 1; j < this->hitobjects.size(); j++) {
                HitObject *objectJ = this->hitobjects[j];

                if(objectJ->click_time - (approachTime * stackLeniency) > startTime) break;

                // "The start position of the hitobject, or the position at the end of the path if the hitobject is a
                // slider"
                Vector2 position2 =
                    isSlider ? sliderPointer->getOriginalRawPosAt(sliderPointer->click_time + sliderPointer->duration)
                             : currHitObject->getOriginalRawPosAt(currHitObject->click_time);

                if((objectJ->getOriginalRawPosAt(objectJ->click_time) -
                    currHitObject->getOriginalRawPosAt(currHitObject->click_time))
                       .length() < 3) {
                    currHitObject->setStack(currHitObject->getStack() + 1);
                    startTime = objectJ->click_time + objectJ->duration;
                } else if((objectJ->getOriginalRawPosAt(objectJ->click_time) - position2).length() < 3) {
                    // "Case for sliders - bump notes down and right, rather than up and left."
                    sliderStack++;
                    objectJ->setStack(objectJ->getStack() - sliderStack);
                    startTime = objectJ->click_time + objectJ->duration;
                }
            }
        }
    }

    // update hitobject positions
    f32 stackOffset = this->fRawHitcircleDiameter * STACK_OFFSET;
    for(int i = 0; i < this->hitobjects.size(); i++) {
        if(this->hitobjects[i]->getStack() != 0) this->hitobjects[i]->updateStackPosition(stackOffset);
    }
}

void Beatmap::computeDrainRate() {
    this->fDrainRate = 0.0;
    this->fHpMultiplierNormal = 1.0;
    this->fHpMultiplierComboEnd = 1.0;

    if(this->hitobjects.size() < 1 || this->selectedDifficulty2 == NULL) return;

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

                this->hpMultiplierNormal = 1.0;
                this->hpMultiplierComboEnd = 1.0;

                this->resetHealth();
            }

            void resetHealth() {
                this->health = this->hpBarMaximum;
                this->healthUncapped = this->hpBarMaximum;
            }

            void increaseHealth(f64 amount) {
                this->healthUncapped += amount;
                this->health += amount;

                if(this->health > this->hpBarMaximum) this->health = this->hpBarMaximum;

                if(this->health < 0.0) this->health = 0.0;

                if(this->healthUncapped < 0.0) this->healthUncapped = 0.0;
            }

            void decreaseHealth(f64 amount) {
                this->health -= amount;

                if(this->health < 0.0) this->health = 0.0;

                if(this->health > this->hpBarMaximum) this->health = this->hpBarMaximum;

                this->healthUncapped -= amount;

                if(this->healthUncapped < 0.0) this->healthUncapped = 0.0;
            }

            f64 hpBarMaximum;

            f64 health;
            f64 healthUncapped;

            f64 hpMultiplierNormal;
            f64 hpMultiplierComboEnd;
        };
        TestPlayer testPlayer(200.0);

        const f64 HP = this->getHP();
        const int version = this->selectedDifficulty2->getVersion();

        f64 testDrop = 0.05;

        const f64 lowestHpEver = GameRules::mapDifficultyRangeDouble(HP, 195.0, 160.0, 60.0);
        const f64 lowestHpComboEnd = GameRules::mapDifficultyRangeDouble(HP, 198.0, 170.0, 80.0);
        const f64 lowestHpEnd = GameRules::mapDifficultyRangeDouble(HP, 198.0, 180.0, 80.0);
        const f64 HpRecoveryAvailable = GameRules::mapDifficultyRangeDouble(HP, 8.0, 4.0, 0.0);

        bool fail = false;

        do {
            testPlayer.resetHealth();

            f64 lowestHp = testPlayer.health;
            int lastTime = (int)(this->hitobjects[0]->click_time - (long)this->getApproachTime());
            fail = false;

            const int breakCount = this->breaks.size();
            int breakNumber = 0;

            int comboTooLowCount = 0;

            for(int i = 0; i < this->hitobjects.size(); i++) {
                const HitObject *h = this->hitobjects[i];
                const Slider *sliderPointer = dynamic_cast<const Slider *>(h);
                const Spinner *spinnerPointer = dynamic_cast<const Spinner *>(h);

                const int localLastTime = lastTime;

                int breakTime = 0;
                if(breakCount > 0 && breakNumber < breakCount) {
                    const DatabaseBeatmap::BREAK &e = this->breaks[breakNumber];
                    if(e.startTime >= localLastTime && e.endTime <= h->click_time) {
                        // consider break start equal to object end time for version 8+ since drain stops during this
                        // time
                        breakTime = (version < 8) ? (e.endTime - e.startTime) : (e.endTime - localLastTime);
                        breakNumber++;
                    }
                }

                testPlayer.decreaseHealth(testDrop * (h->click_time - lastTime - breakTime));

                lastTime = (int)(h->click_time + h->duration);

                if(testPlayer.health < lowestHp) lowestHp = testPlayer.health;

                if(testPlayer.health > lowestHpEver) {
                    const f64 longObjectDrop = testDrop * (f64)h->duration;
                    const f64 maxLongObjectDrop = std::max(0.0, longObjectDrop - testPlayer.health);

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
                        const int rotationsNeeded =
                            (int)((f32)spinnerPointer->duration / 1000.0f * GameRules::getSpinnerSpinsPerSecond(this));
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
                        if((i == this->hitobjects.size() - 1) || this->hitobjects[i]->is_end_of_combo) {
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

            const f64 recovery = (testPlayer.healthUncapped - testPlayer.hpBarMaximum) / (f64)this->hitobjects.size();
            if(!fail && recovery < HpRecoveryAvailable) {
                fail = true;
                testDrop *= 0.96;
                testPlayer.hpMultiplierComboEnd *= 1.02;
                testPlayer.hpMultiplierNormal *= 1.01;
            }
        } while(fail);

        this->fDrainRate =
            (testDrop / testPlayer.hpBarMaximum) * 1000.0;  // from [0, 200] to [0, 1], and from ms to seconds
        this->fHpMultiplierComboEnd = testPlayer.hpMultiplierComboEnd;
        this->fHpMultiplierNormal = testPlayer.hpMultiplierNormal;
    }
}

f32 Beatmap::getApproachTime() const {
    return cv::mod_mafham.getBool()
               ? this->getLength() * 2
               : GameRules::mapDifficultyRange(this->getAR(), GameRules::getMinApproachTime(),
                                               GameRules::getMidApproachTime(), GameRules::getMaxApproachTime());
}

f32 Beatmap::getRawApproachTime() const {
    return cv::mod_mafham.getBool()
               ? this->getLength() * 2
               : GameRules::mapDifficultyRange(this->getRawAR(), GameRules::getMinApproachTime(),
                                               GameRules::getMidApproachTime(), GameRules::getMaxApproachTime());
}

Replay::Mods Beatmap::getMods() const { return osu->getScore()->mods; }

i32 Beatmap::getModsLegacy() const { return osu->getScore()->getModsLegacy(); }
