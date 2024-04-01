#include "OsuVolumeOverlay.h"

#include "AnimationHandler.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Engine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuChangelog.h"
#include "OsuChat.h"
#include "OsuKeyBindings.h"
#include "OsuModSelector.h"
#include "OsuOptionsMenu.h"
#include "OsuPauseMenu.h"
#include "OsuRankingScreen.h"
#include "OsuSkin.h"
#include "OsuSongBrowser2.h"
#include "OsuUIVolumeSlider.h"
#include "OsuUserStatsScreen.h"
#include "Sound.h"
#include "SoundEngine.h"

OsuVolumeOverlay::OsuVolumeOverlay(Osu *osu) : OsuScreen(osu) {
    osu_volume_master = convar->getConVarByName("osu_volume_master");
    osu_volume_master->setCallback(fastdelegate::MakeDelegate(this, &OsuVolumeOverlay::onMasterVolumeChange));
    osu_volume_effects = convar->getConVarByName("osu_volume_effects");
    osu_volume_effects->setCallback(fastdelegate::MakeDelegate(this, &OsuVolumeOverlay::onEffectVolumeChange));
    osu_volume_music = convar->getConVarByName("osu_volume_music");
    osu_volume_music->setCallback(fastdelegate::MakeDelegate(this, &OsuVolumeOverlay::onMusicVolumeChange));
    osu_hud_volume_size_multiplier = convar->getConVarByName("osu_hud_volume_size_multiplier");
    osu_hud_volume_size_multiplier->setCallback(fastdelegate::MakeDelegate(this, &OsuVolumeOverlay::updateLayout));
    osu_volume_master_inactive = convar->getConVarByName("osu_volume_master_inactive");
    osu_volume_change_interval = convar->getConVarByName("osu_volume_change_interval");

    if(env->getOS() == Environment::OS::OS_HORIZON) {
        osu_volume_music->setValue(0.3f);
    }

    m_fVolumeChangeTime = 0.0f;
    m_fVolumeChangeFade = 1.0f;
    m_fLastVolume = osu_volume_master->getFloat();
    m_volumeSliderOverlayContainer = new CBaseUIContainer();

    m_volumeMaster = new OsuUIVolumeSlider(m_osu, 0, 0, 450, 75, "");
    m_volumeMaster->setType(OsuUIVolumeSlider::TYPE::MASTER);
    m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7, m_volumeMaster->getSize().y);
    m_volumeMaster->setAllowMouseWheel(false);
    m_volumeMaster->setAnimated(false);
    m_volumeMaster->setSelected(true);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMaster);
    m_volumeEffects =
        new OsuUIVolumeSlider(m_osu, 0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f, "");
    m_volumeEffects->setType(OsuUIVolumeSlider::TYPE::EFFECTS);
    m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7) / 1.5f, m_volumeMaster->getSize().y / 1.5f);
    m_volumeEffects->setAllowMouseWheel(false);
    m_volumeEffects->setKeyDelta(osu_volume_change_interval->getFloat());
    m_volumeEffects->setAnimated(false);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeEffects);
    m_volumeMusic =
        new OsuUIVolumeSlider(m_osu, 0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f, "");
    m_volumeMusic->setType(OsuUIVolumeSlider::TYPE::MUSIC);
    m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7) / 1.5f, m_volumeMaster->getSize().y / 1.5f);
    m_volumeMusic->setAllowMouseWheel(false);
    m_volumeMusic->setKeyDelta(osu_volume_change_interval->getFloat());
    m_volumeMusic->setAnimated(false);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMusic);

    engine->getSound()->setVolume(osu_volume_master->getFloat());
    updateLayout();
}

void OsuVolumeOverlay::animate() {
    const bool active = m_fVolumeChangeTime > engine->getTime();
    auto osu_hud_volume_duration = convar->getConVarByName("osu_hud_volume_duration");

    m_fVolumeChangeTime = engine->getTime() + osu_hud_volume_duration->getFloat() + 0.2f;

    if(!active) {
        m_fVolumeChangeFade = 0.0f;
        anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.15f, true);
    } else
        anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.1f * (1.0f - m_fVolumeChangeFade), true);

    anim->moveQuadOut(&m_fVolumeChangeFade, 0.0f, 0.20f, osu_hud_volume_duration->getFloat(), false);
    anim->moveQuadOut(&m_fLastVolume, osu_volume_master->getFloat(), 0.15f, 0.0f, true);
}

void OsuVolumeOverlay::draw(Graphics *g) {
    if(!isVisible()) return;

    const float dpiScale = Osu::getUIScale(m_osu);
    const float sizeMultiplier = osu_hud_volume_size_multiplier->getFloat() * dpiScale;

    if(m_fVolumeChangeFade != 1.0f) {
        g->push3DScene(McRect(m_volumeMaster->getPos().x, m_volumeMaster->getPos().y, m_volumeMaster->getSize().x,
                              (m_osu->getScreenHeight() - m_volumeMaster->getPos().y) * 2.05f));
        g->rotate3DScene(-(1.0f - m_fVolumeChangeFade) * 90, 0, 0);
        g->translate3DScene(0, (m_fVolumeChangeFade * 60 - 60) * sizeMultiplier / 1.5f,
                            ((m_fVolumeChangeFade) * 500 - 500) * sizeMultiplier / 1.5f);
    }

    m_volumeMaster->setPos(m_osu->getScreenSize() - m_volumeMaster->getSize() -
                           Vector2(m_volumeMaster->getMinimumExtraTextWidth(), m_volumeMaster->getSize().y));
    m_volumeEffects->setPos(m_volumeMaster->getPos() - Vector2(0, m_volumeEffects->getSize().y + 20 * sizeMultiplier));
    m_volumeMusic->setPos(m_volumeEffects->getPos() - Vector2(0, m_volumeMusic->getSize().y + 20 * sizeMultiplier));

    m_volumeSliderOverlayContainer->draw(g);

    if(m_fVolumeChangeFade != 1.0f) g->pop3DScene();
}

void OsuVolumeOverlay::mouse_update(bool *propagate_clicks) {
    m_volumeMaster->setEnabled(m_fVolumeChangeTime > engine->getTime());
    m_volumeEffects->setEnabled(m_volumeMaster->isEnabled());
    m_volumeMusic->setEnabled(m_volumeMaster->isEnabled());
    m_volumeSliderOverlayContainer->setSize(m_osu->getScreenSize());
    m_volumeSliderOverlayContainer->mouse_update(propagate_clicks);

    if(!m_volumeMaster->isBusy())
        m_volumeMaster->setValue(osu_volume_master->getFloat(), false);
    else {
        osu_volume_master->setValue(m_volumeMaster->getFloat());
        m_fLastVolume = m_volumeMaster->getFloat();
    }

    if(!m_volumeMusic->isBusy())
        m_volumeMusic->setValue(osu_volume_music->getFloat(), false);
    else
        osu_volume_music->setValue(m_volumeMusic->getFloat());

    if(!m_volumeEffects->isBusy())
        m_volumeEffects->setValue(osu_volume_effects->getFloat(), false);
    else
        osu_volume_effects->setValue(m_volumeEffects->getFloat());

    // force focus back to master if no longer active
    if(engine->getTime() > m_fVolumeChangeTime && !m_volumeMaster->isSelected()) {
        m_volumeMusic->setSelected(false);
        m_volumeEffects->setSelected(false);
        m_volumeMaster->setSelected(true);
    }

    // keep overlay alive as long as the user is doing something
    // switch selection if cursor moves inside one of the sliders
    if(m_volumeSliderOverlayContainer->isBusy() ||
       (m_volumeMaster->isEnabled() &&
        (m_volumeMaster->isMouseInside() || m_volumeEffects->isMouseInside() || m_volumeMusic->isMouseInside()))) {
        animate();

        const std::vector<CBaseUIElement *> &elements = m_volumeSliderOverlayContainer->getElements();
        for(int i = 0; i < elements.size(); i++) {
            if(((OsuUIVolumeSlider *)elements[i])->checkWentMouseInside()) {
                for(int c = 0; c < elements.size(); c++) {
                    if(c != i) ((OsuUIVolumeSlider *)elements[c])->setSelected(false);
                }
                ((OsuUIVolumeSlider *)elements[i])->setSelected(true);
            }
        }
    }

    // volume inactive to active animation
    if(m_bVolumeInactiveToActiveScheduled && m_fVolumeInactiveToActiveAnim > 0.0f) {
        engine->getSound()->setVolume(
            lerp<float>(osu_volume_master_inactive->getFloat() * osu_volume_master->getFloat(),
                        osu_volume_master->getFloat(), m_fVolumeInactiveToActiveAnim));

        // check if we're done
        if(m_fVolumeInactiveToActiveAnim == 1.0f) m_bVolumeInactiveToActiveScheduled = false;
    }

    // scroll wheel events (should be separate from mouse_update events, but... oh well...)
    const int wheelDelta = engine->getMouse()->getWheelDeltaVertical() / 120;
    if(canChangeVolume() && wheelDelta != 0) {
        if(wheelDelta > 0) {
            volumeUp(wheelDelta);
        } else {
            volumeDown(-wheelDelta);
        }
    }
}

void OsuVolumeOverlay::updateLayout() {
    const float dpiScale = Osu::getUIScale(m_osu);
    const float sizeMultiplier = osu_hud_volume_size_multiplier->getFloat() * dpiScale;

    m_volumeMaster->setSize(300 * sizeMultiplier, 50 * sizeMultiplier);
    m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7 * dpiScale, m_volumeMaster->getSize().y);

    m_volumeEffects->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f);
    m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale) / 1.5f,
                                  m_volumeMaster->getSize().y / 1.5f);

    m_volumeMusic->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f);
    m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale) / 1.5f,
                                m_volumeMaster->getSize().y / 1.5f);
}

void OsuVolumeOverlay::onResolutionChange(Vector2 newResolution) { updateLayout(); }

void OsuVolumeOverlay::onKeyDown(KeyboardEvent &key) {
    if(!canChangeVolume()) return;

    if(key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt()) {
        volumeUp();
        key.consume();
    } else if(key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt()) {
        volumeDown();
        key.consume();
    } else if(isVisible()) {
        if(key == KEY_LEFT) {
            const std::vector<CBaseUIElement *> &elements = m_volumeSliderOverlayContainer->getElements();
            for(int i = 0; i < elements.size(); i++) {
                if(((OsuUIVolumeSlider *)elements[i])->isSelected()) {
                    const int nextIndex = (i == elements.size() - 1 ? 0 : i + 1);
                    ((OsuUIVolumeSlider *)elements[i])->setSelected(false);
                    ((OsuUIVolumeSlider *)elements[nextIndex])->setSelected(true);
                    break;
                }
            }
            animate();
            key.consume();
        } else if(key == KEY_RIGHT) {
            const std::vector<CBaseUIElement *> &elements = m_volumeSliderOverlayContainer->getElements();
            for(int i = 0; i < elements.size(); i++) {
                if(((OsuUIVolumeSlider *)elements[i])->isSelected()) {
                    const int prevIndex = (i == 0 ? elements.size() - 1 : i - 1);
                    ((OsuUIVolumeSlider *)elements[i])->setSelected(false);
                    ((OsuUIVolumeSlider *)elements[prevIndex])->setSelected(true);
                    break;
                }
            }
            animate();
            key.consume();
        }
    }
}

bool OsuVolumeOverlay::isBusy() {
    return (m_volumeMaster->isEnabled() && (m_volumeMaster->isBusy() || m_volumeMaster->isMouseInside())) ||
           (m_volumeEffects->isEnabled() && (m_volumeEffects->isBusy() || m_volumeEffects->isMouseInside())) ||
           (m_volumeMusic->isEnabled() && (m_volumeMusic->isBusy() || m_volumeMusic->isMouseInside()));
}

bool OsuVolumeOverlay::isVisible() { return engine->getTime() < m_fVolumeChangeTime; }

bool OsuVolumeOverlay::canChangeVolume() {
    bool can_scroll = true;
    if(m_osu->m_songBrowser2->isVisible()) can_scroll = false;
    if(m_osu->m_optionsMenu->isVisible()) can_scroll = false;
    if(m_osu->m_userStatsScreen->isVisible()) can_scroll = false;
    if(m_osu->m_changelog->isVisible()) can_scroll = false;
    if(m_osu->m_rankingScreen->isVisible()) can_scroll = false;
    if(m_osu->m_modSelector->isMouseInScrollView()) can_scroll = false;
    if(m_osu->m_chat->isMouseInChat()) can_scroll = false;
    if(m_osu->isInPlayMode() && convar->getConVarByName("osu_disable_mousewheel")->getBool() &&
       !m_osu->m_pauseMenu->isVisible())
        can_scroll = false;

    if(isBusy()) can_scroll = true;
    if(engine->getKeyboard()->isAltDown()) can_scroll = true;

    return can_scroll;
}

void OsuVolumeOverlay::gainFocus() {
    if(engine->getSound()->isWASAPI() && convar->getConVarByName("win_snd_wasapi_exclusive")->getBool()) {
        // NOTE: wasapi exclusive mode controls the system volume, so don't bother
        return;
    }

    m_fVolumeInactiveToActiveAnim = 0.0f;
    anim->moveLinear(&m_fVolumeInactiveToActiveAnim, 1.0f, 0.3f, 0.1f, true);
}

void OsuVolumeOverlay::loseFocus() {
    if(engine->getSound()->isWASAPI() && convar->getConVarByName("win_snd_wasapi_exclusive")->getBool()) {
        // NOTE: wasapi exclusive mode controls the system volume, so don't bother
        return;
    }

    m_bVolumeInactiveToActiveScheduled = true;
    anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
    m_fVolumeInactiveToActiveAnim = 0.0f;
    engine->getSound()->setVolume(osu_volume_master_inactive->getFloat() * osu_volume_master->getFloat());
}

void OsuVolumeOverlay::onVolumeChange(int multiplier) {
    // sanity reset
    m_bVolumeInactiveToActiveScheduled = false;
    anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
    m_fVolumeInactiveToActiveAnim = 0.0f;

    // chose which volume to change, depending on the volume overlay, default is master
    ConVar *volumeConVar = osu_volume_master;
    if(m_volumeMusic->isSelected())
        volumeConVar = osu_volume_music;
    else if(m_volumeEffects->isSelected())
        volumeConVar = osu_volume_effects;

    // change the volume
    float newVolume =
        clamp<float>(volumeConVar->getFloat() + osu_volume_change_interval->getFloat() * multiplier, 0.0f, 1.0f);
    volumeConVar->setValue(newVolume);
    animate();
}

void OsuVolumeOverlay::onMasterVolumeChange(UString oldValue, UString newValue) {
    if(m_bVolumeInactiveToActiveScheduled) return;  // not very clean, but w/e

    float newVolume = newValue.toFloat();
    engine->getSound()->setVolume(newVolume);
}

void OsuVolumeOverlay::onEffectVolumeChange() {
    float volume = osu_volume_effects->getFloat();

    auto skin = m_osu->getSkin();
    for(int i = 0; i < skin->m_sounds.size(); i++) {
        skin->m_sounds[i]->setVolume(volume);
    }

    skin->resetSampleVolume();
}

void OsuVolumeOverlay::onMusicVolumeChange(UString oldValue, UString newValue) {
    if(m_osu->getSelectedBeatmap() != NULL) m_osu->getSelectedBeatmap()->setVolume(newValue.toFloat());
}
