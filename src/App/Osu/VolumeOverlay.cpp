#include "VolumeOverlay.h"

#include "AnimationHandler.h"
#include "Beatmap.h"
#include "CBaseUIContainer.h"
#include "Changelog.h"
#include "Chat.h"
#include "ConVar.h"
#include "Engine.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "PauseMenu.h"
#include "RankingScreen.h"
#include "Skin.h"
#include "SongBrowser/SongBrowser.h"
#include "Sound.h"
#include "SoundEngine.h"
#include "UIContextMenu.h"
#include "UIVolumeSlider.h"

VolumeOverlay::VolumeOverlay() : OsuScreen() {
    cv_volume_master.setCallback(fastdelegate::MakeDelegate(this, &VolumeOverlay::onMasterVolumeChange));
    cv_volume_effects.setCallback(fastdelegate::MakeDelegate(this, &VolumeOverlay::onEffectVolumeChange));
    cv_volume_music.setCallback(fastdelegate::MakeDelegate(this, &VolumeOverlay::onMusicVolumeChange));
    cv_hud_volume_size_multiplier.setCallback(fastdelegate::MakeDelegate(this, &VolumeOverlay::updateLayout));

    m_fVolumeChangeTime = 0.0f;
    m_fVolumeChangeFade = 1.0f;
    m_fLastVolume = cv_volume_master.getFloat();
    m_volumeSliderOverlayContainer = new CBaseUIContainer();

    m_volumeMaster = new UIVolumeSlider(0, 0, 450, 75, "");
    m_volumeMaster->setType(UIVolumeSlider::TYPE::MASTER);
    m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7, m_volumeMaster->getSize().y);
    m_volumeMaster->setAllowMouseWheel(false);
    m_volumeMaster->setAnimated(false);
    m_volumeMaster->setSelected(true);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMaster);
    m_volumeEffects = new UIVolumeSlider(0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f, "");
    m_volumeEffects->setType(UIVolumeSlider::TYPE::EFFECTS);
    m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7) / 1.5f, m_volumeMaster->getSize().y / 1.5f);
    m_volumeEffects->setAllowMouseWheel(false);
    m_volumeEffects->setKeyDelta(cv_volume_change_interval.getFloat());
    m_volumeEffects->setAnimated(false);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeEffects);
    m_volumeMusic = new UIVolumeSlider(0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f, "");
    m_volumeMusic->setType(UIVolumeSlider::TYPE::MUSIC);
    m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7) / 1.5f, m_volumeMaster->getSize().y / 1.5f);
    m_volumeMusic->setAllowMouseWheel(false);
    m_volumeMusic->setKeyDelta(cv_volume_change_interval.getFloat());
    m_volumeMusic->setAnimated(false);
    m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMusic);

    engine->getSound()->setVolume(cv_volume_master.getFloat());
    updateLayout();
}

void VolumeOverlay::animate() {
    const bool active = m_fVolumeChangeTime > engine->getTime();
    m_fVolumeChangeTime = engine->getTime() + cv_hud_volume_duration.getFloat() + 0.2f;

    if(!active) {
        m_fVolumeChangeFade = 0.0f;
        anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.15f, true);
    } else
        anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.1f * (1.0f - m_fVolumeChangeFade), true);

    anim->moveQuadOut(&m_fVolumeChangeFade, 0.0f, 0.20f, cv_hud_volume_duration.getFloat(), false);
    anim->moveQuadOut(&m_fLastVolume, cv_volume_master.getFloat(), 0.15f, 0.0f, true);
}

void VolumeOverlay::draw(Graphics *g) {
    if(!isVisible()) return;

    const float dpiScale = Osu::getUIScale();
    const float sizeMultiplier = cv_hud_volume_size_multiplier.getFloat() * dpiScale;

    if(m_fVolumeChangeFade != 1.0f) {
        g->push3DScene(McRect(m_volumeMaster->getPos().x, m_volumeMaster->getPos().y, m_volumeMaster->getSize().x,
                              (osu->getScreenHeight() - m_volumeMaster->getPos().y) * 2.05f));
        g->rotate3DScene(-(1.0f - m_fVolumeChangeFade) * 90, 0, 0);
        g->translate3DScene(0, (m_fVolumeChangeFade * 60 - 60) * sizeMultiplier / 1.5f,
                            ((m_fVolumeChangeFade) * 500 - 500) * sizeMultiplier / 1.5f);
    }

    m_volumeMaster->setPos(osu->getScreenSize() - m_volumeMaster->getSize() -
                           Vector2(m_volumeMaster->getMinimumExtraTextWidth(), m_volumeMaster->getSize().y));
    m_volumeEffects->setPos(m_volumeMaster->getPos() - Vector2(0, m_volumeEffects->getSize().y + 20 * sizeMultiplier));
    m_volumeMusic->setPos(m_volumeEffects->getPos() - Vector2(0, m_volumeMusic->getSize().y + 20 * sizeMultiplier));

    m_volumeSliderOverlayContainer->draw(g);

    if(m_fVolumeChangeFade != 1.0f) g->pop3DScene();
}

void VolumeOverlay::mouse_update(bool *propagate_clicks) {
    m_volumeMaster->setEnabled(m_fVolumeChangeTime > engine->getTime());
    m_volumeEffects->setEnabled(m_volumeMaster->isEnabled());
    m_volumeMusic->setEnabled(m_volumeMaster->isEnabled());
    m_volumeSliderOverlayContainer->setSize(osu->getScreenSize());
    m_volumeSliderOverlayContainer->mouse_update(propagate_clicks);

    if(!m_volumeMaster->isBusy())
        m_volumeMaster->setValue(cv_volume_master.getFloat(), false);
    else {
        cv_volume_master.setValue(m_volumeMaster->getFloat());
        m_fLastVolume = m_volumeMaster->getFloat();
    }

    if(!m_volumeMusic->isBusy())
        m_volumeMusic->setValue(cv_volume_music.getFloat(), false);
    else
        cv_volume_music.setValue(m_volumeMusic->getFloat());

    if(!m_volumeEffects->isBusy())
        m_volumeEffects->setValue(cv_volume_effects.getFloat(), false);
    else
        cv_volume_effects.setValue(m_volumeEffects->getFloat());

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
            if(((UIVolumeSlider *)elements[i])->checkWentMouseInside()) {
                for(int c = 0; c < elements.size(); c++) {
                    if(c != i) ((UIVolumeSlider *)elements[c])->setSelected(false);
                }
                ((UIVolumeSlider *)elements[i])->setSelected(true);
            }
        }
    }

    // volume inactive to active animation
    if(m_bVolumeInactiveToActiveScheduled && m_fVolumeInactiveToActiveAnim > 0.0f) {
        engine->getSound()->setVolume(lerp<float>(cv_volume_master_inactive.getFloat() * cv_volume_master.getFloat(),
                                                  cv_volume_master.getFloat(), m_fVolumeInactiveToActiveAnim));

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

void VolumeOverlay::updateLayout() {
    const float dpiScale = Osu::getUIScale();
    const float sizeMultiplier = cv_hud_volume_size_multiplier.getFloat() * dpiScale;

    m_volumeMaster->setSize(300 * sizeMultiplier, 50 * sizeMultiplier);
    m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7 * dpiScale, m_volumeMaster->getSize().y);

    m_volumeEffects->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f);
    m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale) / 1.5f,
                                  m_volumeMaster->getSize().y / 1.5f);

    m_volumeMusic->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y / 1.5f);
    m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale) / 1.5f,
                                m_volumeMaster->getSize().y / 1.5f);
}

void VolumeOverlay::onResolutionChange(Vector2 newResolution) { updateLayout(); }

void VolumeOverlay::onKeyDown(KeyboardEvent &key) {
    if(!canChangeVolume()) return;

    if(key == (KEYCODE)cv_INCREASE_VOLUME.getInt()) {
        volumeUp();
        key.consume();
    } else if(key == (KEYCODE)cv_DECREASE_VOLUME.getInt()) {
        volumeDown();
        key.consume();
    } else if(isVisible()) {
        if(key == KEY_LEFT) {
            const std::vector<CBaseUIElement *> &elements = m_volumeSliderOverlayContainer->getElements();
            for(int i = 0; i < elements.size(); i++) {
                if(((UIVolumeSlider *)elements[i])->isSelected()) {
                    const int nextIndex = (i == elements.size() - 1 ? 0 : i + 1);
                    ((UIVolumeSlider *)elements[i])->setSelected(false);
                    ((UIVolumeSlider *)elements[nextIndex])->setSelected(true);
                    break;
                }
            }
            animate();
            key.consume();
        } else if(key == KEY_RIGHT) {
            const std::vector<CBaseUIElement *> &elements = m_volumeSliderOverlayContainer->getElements();
            for(int i = 0; i < elements.size(); i++) {
                if(((UIVolumeSlider *)elements[i])->isSelected()) {
                    const int prevIndex = (i == 0 ? elements.size() - 1 : i - 1);
                    ((UIVolumeSlider *)elements[i])->setSelected(false);
                    ((UIVolumeSlider *)elements[prevIndex])->setSelected(true);
                    break;
                }
            }
            animate();
            key.consume();
        }
    }
}

bool VolumeOverlay::isBusy() {
    return (m_volumeMaster->isEnabled() && (m_volumeMaster->isBusy() || m_volumeMaster->isMouseInside())) ||
           (m_volumeEffects->isEnabled() && (m_volumeEffects->isBusy() || m_volumeEffects->isMouseInside())) ||
           (m_volumeMusic->isEnabled() && (m_volumeMusic->isBusy() || m_volumeMusic->isMouseInside()));
}

bool VolumeOverlay::isVisible() { return engine->getTime() < m_fVolumeChangeTime; }

bool VolumeOverlay::canChangeVolume() {
    bool can_scroll = true;
    if(osu->m_songBrowser2->isVisible() && osu->m_songBrowser2->getDatabase()->isFinished() == 1.f) {
        can_scroll = false;
    }
    if(osu->m_optionsMenu->isVisible()) can_scroll = false;
    if(osu->m_optionsMenu->m_contextMenu->isVisible()) can_scroll = false;
    if(osu->m_changelog->isVisible()) can_scroll = false;
    if(osu->m_rankingScreen->isVisible()) can_scroll = false;
    if(osu->m_modSelector->isMouseInScrollView()) can_scroll = false;
    if(osu->m_chat->isMouseInChat()) can_scroll = false;
    if(osu->isInPlayMode() && cv_disable_mousewheel.getBool() && !osu->m_pauseMenu->isVisible()) can_scroll = false;

    if(isBusy()) can_scroll = true;
    if(engine->getKeyboard()->isAltDown()) can_scroll = true;

    return can_scroll;
}

void VolumeOverlay::gainFocus() {
    if(engine->getSound()->hasExclusiveOutput()) return;

    m_fVolumeInactiveToActiveAnim = 0.0f;
    anim->moveLinear(&m_fVolumeInactiveToActiveAnim, 1.0f, 0.3f, 0.1f, true);
}

void VolumeOverlay::loseFocus() {
    if(engine->getSound()->hasExclusiveOutput()) return;

    m_bVolumeInactiveToActiveScheduled = true;
    anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
    m_fVolumeInactiveToActiveAnim = 0.0f;
    engine->getSound()->setVolume(cv_volume_master_inactive.getFloat() * cv_volume_master.getFloat());
}

void VolumeOverlay::onVolumeChange(int multiplier) {
    // sanity reset
    m_bVolumeInactiveToActiveScheduled = false;
    anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
    m_fVolumeInactiveToActiveAnim = 0.0f;

    // chose which volume to change, depending on the volume overlay, default is master
    ConVar *volumeConVar = &cv_volume_master;
    if(m_volumeMusic->isSelected())
        volumeConVar = &cv_volume_music;
    else if(m_volumeEffects->isSelected())
        volumeConVar = &cv_volume_effects;

    // change the volume
    float newVolume =
        clamp<float>(volumeConVar->getFloat() + cv_volume_change_interval.getFloat() * multiplier, 0.0f, 1.0f);
    volumeConVar->setValue(newVolume);
    animate();
}

void VolumeOverlay::onMasterVolumeChange(UString oldValue, UString newValue) {
    if(m_bVolumeInactiveToActiveScheduled) return;  // not very clean, but w/e

    float newVolume = newValue.toFloat();
    engine->getSound()->setVolume(newVolume);
}

void VolumeOverlay::onEffectVolumeChange() {
    float volume = cv_volume_effects.getFloat();

    auto skin = osu->getSkin();
    for(int i = 0; i < skin->m_sounds.size(); i++) {
        skin->m_sounds[i]->setVolume(volume);
    }

    skin->resetSampleVolume();
}

void VolumeOverlay::onMusicVolumeChange(UString oldValue, UString newValue) {
    auto music = osu->getSelectedBeatmap()->getMusic();
    if(music != NULL) {
        music->setVolume(osu->getSelectedBeatmap()->getIdealVolume());
    }
}
