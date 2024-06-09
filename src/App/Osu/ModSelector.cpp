#include "ModSelector.h"

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "CBaseUICheckbox.h"
#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "CBaseUISlider.h"
#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Environment.h"
#include "GameRules.h"
#include "HUD.h"
#include "Icons.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "RoomScreen.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"
#include "UIButton.h"
#include "UICheckbox.h"
#include "UIModSelectorModButton.h"
#include "UISlider.h"
#include "VertexArrayObject.h"

class ModSelectorOverrideSliderDescButton : public CBaseUIButton {
   public:
    ModSelectorOverrideSliderDescButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {}

    virtual void mouse_update(bool *propagate_clicks) {
        if(!m_bVisible) return;
        CBaseUIButton::mouse_update(propagate_clicks);

        if(isMouseInside() && m_sTooltipText.length() > 0) {
            osu->getTooltipOverlay()->begin();
            { osu->getTooltipOverlay()->addLine(m_sTooltipText); }
            osu->getTooltipOverlay()->end();
        }
    }

    void setTooltipText(UString tooltipText) { m_sTooltipText = tooltipText; }

   private:
    virtual void drawText(Graphics *g) {
        if(m_font != NULL && m_sText.length() > 0) {
            float xPosAdd = m_vSize.x / 2.0f - m_fStringWidth / 2.0f;

            // g->pushClipRect(McRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y));
            {
                g->setColor(m_textColor);
                g->pushTransform();
                {
                    g->translate((int)(m_vPos.x + (xPosAdd)),
                                 (int)(m_vPos.y + m_vSize.y / 2.0f + m_fStringHeight / 2.0f));
                    g->drawString(m_font, m_sText);
                }
                g->popTransform();
            }
            // g->popClipRect();
        }
    }

    UString m_sTooltipText;
};

class ModSelectorOverrideSliderLockButton : public CBaseUICheckbox {
   public:
    ModSelectorOverrideSliderLockButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUICheckbox(xPos, yPos, xSize, ySize, name, text) {
        m_fAnim = 1.0f;
    }

    virtual void draw(Graphics *g) {
        if(!m_bVisible) return;

        const wchar_t icon = (m_bChecked ? Icons::LOCK : Icons::UNLOCK);
        UString iconString;
        iconString.insert(0, icon);

        McFont *iconFont = osu->getFontIcons();
        const float scale = (m_vSize.y / iconFont->getHeight()) * m_fAnim;
        g->setColor(m_bChecked ? 0xffffffff : 0xff1166ff);

        g->pushTransform();
        {
            g->scale(scale, scale);
            g->translate(m_vPos.x + m_vSize.x / 2.0f - iconFont->getStringWidth(iconString) * scale / 2.0f,
                         m_vPos.y + m_vSize.y / 2.0f + (iconFont->getHeight() * scale / 2.0f) * 0.8f);
            g->drawString(iconFont, iconString);
        }
        g->popTransform();
    }

   private:
    virtual void onPressed() {
        CBaseUICheckbox::onPressed();
        engine->getSound()->play(isChecked() ? osu->getSkin()->getCheckOn() : osu->getSkin()->getCheckOff());

        if(isChecked()) {
            // anim->moveQuadOut(&m_fAnim, 1.5f, 0.060f, true);
            // anim->moveQuadIn(&m_fAnim, 1.0f, 0.250f, 0.150f, false);
        } else {
            anim->deleteExistingAnimation(&m_fAnim);
            m_fAnim = 1.0f;
        }
    }

    float m_fAnim;
};

ModSelector::ModSelector() : OsuScreen() {
    m_fAnimation = 0.0f;
    m_fExperimentalAnimation = 0.0f;
    m_bScheduledHide = false;
    m_bExperimentalVisible = false;
    setSize(osu->getScreenWidth(), osu->getScreenHeight());
    m_overrideSliderContainer = new CBaseUIContainer(0, 0, osu->getScreenWidth(), osu->getScreenHeight(), "");
    m_experimentalContainer = new CBaseUIScrollView(-1, 0, osu->getScreenWidth(), osu->getScreenHeight(), "");
    m_experimentalContainer->setHorizontalScrolling(false);
    m_experimentalContainer->setVerticalScrolling(true);
    m_experimentalContainer->setDrawFrame(false);
    m_experimentalContainer->setDrawBackground(false);

    m_bWaitForF1KeyUp = false;

    m_bWaitForCSChangeFinished = false;
    m_bWaitForSpeedChangeFinished = false;
    m_bWaitForHPChangeFinished = false;

    m_speedSlider = NULL;
    m_bShowOverrideSliderALTHint = true;

    // convar refs
    m_osu_drain_type_ref = convar->getConVarByName("osu_drain_type");
    m_osu_mod_touchdevice_ref = convar->getConVarByName("osu_mod_touchdevice");

    // build mod grid buttons
    m_iGridWidth = 6;
    m_iGridHeight = 3;

    for(int x = 0; x < m_iGridWidth; x++) {
        for(int y = 0; y < m_iGridHeight; y++) {
            UIModSelectorModButton *imageButton = new UIModSelectorModButton(this, 50, 50, 100, 100, "");
            imageButton->setDrawBackground(false);
            imageButton->setVisible(false);

            addBaseUIElement(imageButton);
            m_modButtons.push_back(imageButton);
        }
    }

    // build override sliders
    OVERRIDE_SLIDER overrideCS = addOverrideSlider("CS Override", "CS:", convar->getConVarByName("osu_cs_override"),
                                                   0.0f, 12.5f, "Circle Size (higher number = smaller circles).");
    OVERRIDE_SLIDER overrideAR = addOverrideSlider("AR Override", "AR:", convar->getConVarByName("osu_ar_override"),
                                                   0.0f, 12.5f, "Approach Rate (higher number = faster circles).",
                                                   convar->getConVarByName("osu_ar_override_lock"));
    OVERRIDE_SLIDER overrideOD = addOverrideSlider("OD Override", "OD:", convar->getConVarByName("osu_od_override"),
                                                   0.0f, 12.5f, "Overall Difficulty (higher number = harder accuracy).",
                                                   convar->getConVarByName("osu_od_override_lock"));
    OVERRIDE_SLIDER overrideHP = addOverrideSlider("HP Override", "HP:", convar->getConVarByName("osu_hp_override"),
                                                   0.0f, 12.5f, "Hit/Health Points (higher number = harder survival).");

    overrideCS.slider->setAnimated(false);  // quick fix for otherwise possible inconsistencies due to slider vertex
                                            // buffers and animated CS changes
    overrideCS.slider->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderChange));
    overrideAR.slider->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderChange));
    overrideOD.slider->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderChange));
    overrideHP.slider->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderChange));

    overrideAR.desc->setClickCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideARSliderDescClicked));
    overrideOD.desc->setClickCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideODSliderDescClicked));

    m_CSSlider = overrideCS.slider;
    m_ARSlider = overrideAR.slider;
    m_ODSlider = overrideOD.slider;
    m_HPSlider = overrideHP.slider;
    m_ARLock = overrideAR.lock;
    m_ODLock = overrideOD.lock;

    OVERRIDE_SLIDER overrideSpeed =
        addOverrideSlider("Speed/BPM Multiplier", "x", convar->getConVarByName("osu_speed_override"), 0.0f, 2.5f);

    overrideSpeed.slider->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderChange));
    // overrideSpeed.slider->setValue(-1.0f, false);
    overrideSpeed.slider->setAnimated(false);  // same quick fix as above
    overrideSpeed.slider->setLiveUpdate(false);

    m_speedSlider = overrideSpeed.slider;

    // build experimental buttons
    addExperimentalLabel(" Experimental Mods (!)");
    addExperimentalCheckbox("FPoSu: Strafing",
                            "Playfield moves in 3D space (see fposu_mod_strafing_...).\nOnly works in FPoSu mode!",
                            convar->getConVarByName("fposu_mod_strafing"));
    addExperimentalCheckbox("FPoSu 4D: Z Wobble",
                            "Each hitobject individually moves in 3D space (see fposu_mod_3d_depthwobble_...)\nOnly "
                            "works in FPoSu \"4D Mode\"!",
                            convar->getConVarByName("fposu_mod_3d_depthwobble"));
    addExperimentalCheckbox("Wobble", "Playfield rotates and moves.", convar->getConVarByName("osu_mod_wobble"));
    addExperimentalCheckbox("AR Wobble", "Approach rate oscillates between -1 and +1.",
                            convar->getConVarByName("osu_mod_arwobble"));
    addExperimentalCheckbox("Approach Different",
                            "Customize the approach circle animation.\nSee osu_mod_approach_different_style.\nSee "
                            "osu_mod_approach_different_initial_size.",
                            convar->getConVarByName("osu_mod_approach_different"));
    addExperimentalCheckbox("Timewarp", "Speed increases from 100% to 150% over the course of the beatmap.",
                            convar->getConVarByName("osu_mod_timewarp"));
    addExperimentalCheckbox("AR Timewarp", "Approach rate decreases from 100% to 50% over the course of the beatmap.",
                            convar->getConVarByName("osu_mod_artimewarp"));
    addExperimentalCheckbox("Minimize", "Circle size decreases from 100% to 50% over the course of the beatmap.",
                            convar->getConVarByName("osu_mod_minimize"));
    addExperimentalCheckbox("Fading Cursor", "The cursor fades the higher the combo, becoming invisible at 50.",
                            convar->getConVarByName("osu_mod_fadingcursor"));
    addExperimentalCheckbox("First Person", "Centered cursor.", convar->getConVarByName("osu_mod_fps"));
    addExperimentalCheckbox("Full Alternate", "You can never use the same key twice in a row.",
                            convar->getConVarByName("osu_mod_fullalternate"));
    addExperimentalCheckbox("Jigsaw 1", "Unnecessary clicks count as misses.",
                            convar->getConVarByName("osu_mod_jigsaw1"));
    addExperimentalCheckbox("Jigsaw 2", "Massively reduced slider follow circle radius.",
                            convar->getConVarByName("osu_mod_jigsaw2"));
    m_experimentalModRandomCheckbox = addExperimentalCheckbox(
        "Random",
        "Randomizes hitobject positions. (VERY experimental!)\nUse osu_mod_random_seed to set a fixed rng seed.",
        convar->getConVarByName("osu_mod_random"));
    addExperimentalCheckbox("Reverse Sliders", "Reverses the direction of all sliders. (Reload beatmap to apply!)",
                            convar->getConVarByName("osu_mod_reverse_sliders"));
    addExperimentalCheckbox("No 50s", "Only 300s or 100s. Try harder.", convar->getConVarByName("osu_mod_no50s"));
    addExperimentalCheckbox("No 100s no 50s", "300 or miss. PF \"lite\"", convar->getConVarByName("osu_mod_no100s"));
    addExperimentalCheckbox("MinG3012", "No 100s. Only 300s or 50s. Git gud.",
                            convar->getConVarByName("osu_mod_ming3012"));
    addExperimentalCheckbox("Half Timing Window", "The hit timing window is cut in half. Hit early or perfect (300).",
                            convar->getConVarByName("osu_mod_halfwindow"));
    addExperimentalCheckbox("MillhioreF", "Go below AR 0. Doubled approach time.",
                            convar->getConVarByName("osu_mod_millhioref"));
    addExperimentalCheckbox("Mafham",
                            "Approach rate is set to negative infinity. See the entire beatmap at once.\nUses very "
                            "aggressive optimizations to keep the framerate high, you have been warned!",
                            convar->getConVarByName("osu_mod_mafham"));
    addExperimentalCheckbox("Strict Tracking",
                            "Leaving sliders in any way counts as a miss and combo break. (Reload beatmap to apply!)",
                            convar->getConVarByName("osu_mod_strict_tracking"));
    addExperimentalCheckbox("Flip Up/Down", "Playfield is flipped upside down (mirrored at horizontal axis).",
                            convar->getConVarByName("osu_playfield_mirror_horizontal"));
    addExperimentalCheckbox("Flip Left/Right", "Playfield is flipped left/right (mirrored at vertical axis).",
                            convar->getConVarByName("osu_playfield_mirror_vertical"));

    m_nonVanillaWarning = new CBaseUILabel();
    m_nonVanillaWarning->setDrawFrame(false);
    m_nonVanillaWarning->setDrawBackground(false);
    m_nonVanillaWarning->setText("WARNING: Score submission will be disabled due to non-vanilla mod selection.");
    m_nonVanillaWarning->setTextColor(0xffff0000);
    m_nonVanillaWarning->setCenterText(true);
    m_nonVanillaWarning->setVisible(false);
    addBaseUIElement(m_nonVanillaWarning);

    // build score multiplier label
    m_scoreMultiplierLabel = new CBaseUILabel();
    m_scoreMultiplierLabel->setDrawFrame(false);
    m_scoreMultiplierLabel->setDrawBackground(false);
    m_scoreMultiplierLabel->setCenterText(true);
    addBaseUIElement(m_scoreMultiplierLabel);

    // build action buttons
    m_resetModsButton = addActionButton("1. Reset All Mods");
    m_resetModsButton->setClickCallback(fastdelegate::MakeDelegate(this, &ModSelector::resetModsUserInitiated));
    m_resetModsButton->setColor(0xffff3800);
    m_closeButton = addActionButton("2. Close");
    m_closeButton->setClickCallback(fastdelegate::MakeDelegate(this, &ModSelector::close));
    m_closeButton->setColor(0xff8f8f8f);

    updateButtons(true);
    updateLayout();
}

void ModSelector::updateButtons(bool initial) {
    m_modButtonEasy = setModButtonOnGrid(
        0, 0, 0, initial && osu->getModEZ(), "ez",
        "Reduces overall difficulty - larger circles, more forgiving HP drain, less accuracy required.",
        []() -> SkinImage * { return osu->getSkin()->getSelectionModEasy(); });
    m_modButtonNofail =
        setModButtonOnGrid(1, 0, 0, initial && osu->getModNF(), "nf",
                           "You can't fail. No matter what.\nNOTE: To disable drain completely:\nOptions > Gameplay > "
                           "Mechanics > \"Select HP Drain\" > \"None\".",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModNoFail(); });
    m_modButtonNofail->setAvailable(m_osu_drain_type_ref->getInt() > 1);
    setModButtonOnGrid(4, 0, 0, initial && osu->getModNightmare(), "nightmare",
                       "Unnecessary clicks count as misses.\nMassively reduced slider follow circle radius.",
                       []() -> SkinImage * { return osu->getSkin()->getSelectionModNightmare(); });

    m_modButtonHardrock =
        setModButtonOnGrid(0, 1, 0, initial && osu->getModHR(), "hr", "Everything just got a bit harder...",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModHardRock(); });
    m_modButtonSuddendeath =
        setModButtonOnGrid(1, 1, 0, initial && osu->getModSD(), "sd", "Miss a note and fail.",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModSuddenDeath(); });
    setModButtonOnGrid(1, 1, 1, initial && osu->getModSS(), "ss", "SS or quit.",
                       []() -> SkinImage * { return osu->getSkin()->getSelectionModPerfect(); });

    if(convar->getConVarByName("nightcore_enjoyer")->getBool()) {
        m_modButtonHalftime =
            setModButtonOnGrid(2, 0, 0, initial && osu->getModDC(), "dc", "A E S T H E T I C",
                               []() -> SkinImage * { return osu->getSkin()->getSelectionModDayCore(); });
        setModButtonOnGrid(2, 0, 1, initial && osu->getModHT(), "ht", "Less zoom.",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModHalfTime(); });

        m_modButtonDoubletime =
            setModButtonOnGrid(2, 1, 0, initial && osu->getModNC(), "nc", "uguuuuuuuu",
                               []() -> SkinImage * { return osu->getSkin()->getSelectionModNightCore(); });
        setModButtonOnGrid(2, 1, 1, initial && osu->getModDT(), "dt", "Zoooooooooom.",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModDoubleTime(); });
    } else {
        m_modButtonHalftime =
            setModButtonOnGrid(2, 0, 0, initial && osu->getModHT(), "ht", "Less zoom.",
                               []() -> SkinImage * { return osu->getSkin()->getSelectionModHalfTime(); });
        setModButtonOnGrid(2, 0, 1, initial && osu->getModDC(), "dc", "A E S T H E T I C",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModDayCore(); });

        m_modButtonDoubletime =
            setModButtonOnGrid(2, 1, 0, initial && osu->getModDT(), "dt", "Zoooooooooom.",
                               []() -> SkinImage * { return osu->getSkin()->getSelectionModDoubleTime(); });
        setModButtonOnGrid(2, 1, 1, initial && osu->getModNC(), "nc", "uguuuuuuuu",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModNightCore(); });
    }

    m_modButtonHidden =
        setModButtonOnGrid(3, 1, 0, initial && osu->getModHD(), "hd",
                           "Play with no approach circles and fading notes for a slight score advantage.",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModHidden(); });
    m_modButtonFlashlight = setModButtonOnGrid(4, 1, 0, false, "fl", "Restricted view area.", []() -> SkinImage * {
        return osu->getSkin()->getSelectionModFlashlight();
    });
    m_modButtonTD = setModButtonOnGrid(5, 1, 0, initial && (osu->getModTD() || m_osu_mod_touchdevice_ref->getBool()),
                                       "nerftd", "Simulate pp nerf for touch devices.\nOnly affects pp calculation.",
                                       []() -> SkinImage * { return osu->getSkin()->getSelectionModTD(); });
    getModButtonOnGrid(5, 1)->setAvailable(!m_osu_mod_touchdevice_ref->getBool());

    m_modButtonRelax = setModButtonOnGrid(
        0, 2, 0, initial && osu->getModRelax(), "relax",
        "You don't need to click.\nGive your clicking/tapping fingers a break from the heat of things.\n** UNRANKED **",
        []() -> SkinImage * { return osu->getSkin()->getSelectionModRelax(); });
    m_modButtonAutopilot =
        setModButtonOnGrid(1, 2, 0, initial && osu->getModAutopilot(), "autopilot",
                           "Automatic cursor movement - just follow the rhythm.\n** UNRANKED **",
                           []() -> SkinImage * { return osu->getSkin()->getSelectionModAutopilot(); });
    m_modButtonSpunout = setModButtonOnGrid(2, 2, 0, initial && osu->getModSpunout(), "spunout",
                                            "Spinners will be automatically completed.",
                                            []() -> SkinImage * { return osu->getSkin()->getSelectionModSpunOut(); });
    m_modButtonAuto = setModButtonOnGrid(3, 2, 0, initial && osu->getModAuto(), "auto",
                                         "Watch a perfect automated play through the song.",
                                         []() -> SkinImage * { return osu->getSkin()->getSelectionModAutoplay(); });
    setModButtonOnGrid(4, 2, 0, initial && osu->getModTarget(), "practicetarget",
                       "Accuracy is based on the distance to the center of all hitobjects.\n300s still require at "
                       "least being in the hit window of a 100 in addition to the rule above.",
                       []() -> SkinImage * { return osu->getSkin()->getSelectionModTarget(); });
    m_modButtonScoreV2 = setModButtonOnGrid(5, 2, 0, initial && osu->getModScorev2(), "v2",
                                            "Try the future scoring system.\n** UNRANKED **",
                                            []() -> SkinImage * { return osu->getSkin()->getSelectionModScorev2(); });

    // Enable all mods that we disable conditionally below
    getModButtonOnGrid(2, 0)->setAvailable(true);
    getModButtonOnGrid(2, 1)->setAvailable(true);
    getModButtonOnGrid(3, 2)->setAvailable(true);
    getModButtonOnGrid(4, 2)->setAvailable(true);
    getModButtonOnGrid(4, 0)->setAvailable(true);
    getModButtonOnGrid(5, 2)->setAvailable(true);

    if(bancho.is_in_a_multi_room()) {
        if(bancho.room.freemods && !bancho.room.is_host()) {
            getModButtonOnGrid(2, 0)->setAvailable(false);  // Disable DC/HT
            getModButtonOnGrid(2, 1)->setAvailable(false);  // Disable DT/NC
            getModButtonOnGrid(4, 2)->setAvailable(false);  // Disable Target
        }

        getModButtonOnGrid(5, 2)->setAvailable(false);  // Disable ScoreV2 (we use win condition instead)
        getModButtonOnGrid(4, 0)->setAvailable(false);  // Disable nightmare mod
        getModButtonOnGrid(3, 2)->setAvailable(false);  // Disable auto mod
    }
}

void ModSelector::updateScoreMultiplierLabelText() {
    const float scoreMultiplier = osu->getScoreMultiplier();

    const int alpha = 200;
    if(scoreMultiplier > 1.0f)
        m_scoreMultiplierLabel->setTextColor(COLOR(alpha, 173, 255, 47));
    else if(scoreMultiplier == 1.0f)
        m_scoreMultiplierLabel->setTextColor(COLOR(alpha, 255, 255, 255));
    else
        m_scoreMultiplierLabel->setTextColor(COLOR(alpha, 255, 69, 00));

    m_scoreMultiplierLabel->setText(UString::format("Score Multiplier: %.2fX", scoreMultiplier));
}

void ModSelector::updateExperimentalButtons(bool initial) {
    if(initial) {
        for(int i = 0; i < m_experimentalMods.size(); i++) {
            ConVar *cvar = m_experimentalMods[i].cvar;
            if(cvar != NULL) {
                CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox *>(m_experimentalMods[i].element);
                if(checkboxPointer != NULL) {
                    if(cvar->getBool() != checkboxPointer->isChecked())
                        checkboxPointer->setChecked(cvar->getBool(), false);
                }
            }
        }
    }
}

ModSelector::~ModSelector() {
    SAFE_DELETE(m_overrideSliderContainer);
    SAFE_DELETE(m_experimentalContainer);
}

void ModSelector::draw(Graphics *g) {
    if(!m_bVisible && !m_bScheduledHide) return;

    // for compact mode (and experimental mods)
    const int margin = 10;
    const Color backgroundColor = 0x88000000;

    const float experimentalModsAnimationTranslation =
        -(m_experimentalContainer->getSize().x + 2.0f) * (1.0f - m_fExperimentalAnimation);

    if(bancho.is_in_a_multi_room()) {
        // get mod button element bounds
        Vector2 modGridButtonsStart = Vector2(osu->getScreenWidth(), osu->getScreenHeight());
        Vector2 modGridButtonsSize = Vector2(0, osu->getScreenHeight());
        for(int i = 0; i < m_modButtons.size(); i++) {
            CBaseUIButton *button = m_modButtons[i];

            if(button->getPos().x < modGridButtonsStart.x) modGridButtonsStart.x = button->getPos().x;
            if(button->getPos().y < modGridButtonsStart.y) modGridButtonsStart.y = button->getPos().y;

            if(button->getPos().x + button->getSize().x > modGridButtonsSize.x)
                modGridButtonsSize.x = button->getPos().x + button->getSize().x;
            if(button->getPos().y < modGridButtonsSize.y) modGridButtonsSize.y = button->getPos().y;
        }
        modGridButtonsSize.x -= modGridButtonsStart.x;

        // draw mod grid buttons
        g->pushTransform();
        {
            g->translate(0, (1.0f - m_fAnimation) * modGridButtonsSize.y);
            g->setColor(backgroundColor);
            g->fillRect(modGridButtonsStart.x - margin, modGridButtonsStart.y - margin,
                        modGridButtonsSize.x + 2 * margin, modGridButtonsSize.y + 2 * margin);
            OsuScreen::draw(g);
        }
        g->popTransform();
    } else if(isInCompactMode()) {
        // if we are in compact mode, draw some backgrounds under the override sliders & mod grid buttons

        // get override slider element bounds
        Vector2 overrideSlidersStart = Vector2(osu->getScreenWidth(), 0);
        Vector2 overrideSlidersSize;
        for(int i = 0; i < m_overrideSliders.size(); i++) {
            CBaseUIButton *desc = m_overrideSliders[i].desc;
            CBaseUILabel *label = m_overrideSliders[i].label;

            if(desc->getPos().x < overrideSlidersStart.x) overrideSlidersStart.x = desc->getPos().x;

            if((label->getPos().x + label->getSize().x) > overrideSlidersSize.x)
                overrideSlidersSize.x = (label->getPos().x + label->getSize().x);
            if(desc->getPos().y + desc->getSize().y > overrideSlidersSize.y)
                overrideSlidersSize.y = desc->getPos().y + desc->getSize().y;
        }
        overrideSlidersSize.x -= overrideSlidersStart.x;

        // get mod button element bounds
        Vector2 modGridButtonsStart = Vector2(osu->getScreenWidth(), osu->getScreenHeight());
        Vector2 modGridButtonsSize = Vector2(0, osu->getScreenHeight());
        for(int i = 0; i < m_modButtons.size(); i++) {
            CBaseUIButton *button = m_modButtons[i];

            if(button->getPos().x < modGridButtonsStart.x) modGridButtonsStart.x = button->getPos().x;
            if(button->getPos().y < modGridButtonsStart.y) modGridButtonsStart.y = button->getPos().y;

            if(button->getPos().x + button->getSize().x > modGridButtonsSize.x)
                modGridButtonsSize.x = button->getPos().x + button->getSize().x;
            if(button->getPos().y < modGridButtonsSize.y) modGridButtonsSize.y = button->getPos().y;
        }
        modGridButtonsSize.x -= modGridButtonsStart.x;

        // draw mod grid buttons
        g->pushTransform();
        {
            g->translate(0, (1.0f - m_fAnimation) * modGridButtonsSize.y);
            g->setColor(backgroundColor);
            g->fillRect(modGridButtonsStart.x - margin, modGridButtonsStart.y - margin,
                        modGridButtonsSize.x + 2 * margin, modGridButtonsSize.y + 2 * margin);
            OsuScreen::draw(g);
        }
        g->popTransform();

        // draw override sliders
        g->pushTransform();
        {
            g->translate(0, -(1.0f - m_fAnimation) * overrideSlidersSize.y);
            g->setColor(backgroundColor);
            g->fillRect(overrideSlidersStart.x - margin, 0, overrideSlidersSize.x + 2 * margin,
                        overrideSlidersSize.y + margin);
            m_overrideSliderContainer->draw(g);
        }
        g->popTransform();
    } else  // normal mode, just draw everything
    {
        // draw hint text on left edge of screen
        {
            const float dpiScale = Osu::getUIScale();

            UString experimentalText = "Experimental Mods";
            McFont *experimentalFont = osu->getSubTitleFont();

            const float experimentalTextWidth = experimentalFont->getStringWidth(experimentalText);
            const float experimentalTextHeight = experimentalFont->getHeight();

            const float rectMargin = 5 * dpiScale;
            const float rectWidth = experimentalTextWidth + 2 * rectMargin;
            const float rectHeight = experimentalTextHeight + 2 * rectMargin;

            g->pushTransform();
            {
                g->rotate(90);
                g->translate(
                    (int)(experimentalTextHeight / 3.0f +
                          max(0.0f, experimentalModsAnimationTranslation + m_experimentalContainer->getSize().x)),
                    (int)(osu->getScreenHeight() / 2 - experimentalTextWidth / 2));
                g->setColor(0xff777777);
                g->setAlpha(1.0f - m_fExperimentalAnimation * m_fExperimentalAnimation);
                g->drawString(experimentalFont, experimentalText);
            }
            g->popTransform();

            g->pushTransform();
            {
                g->rotate(90);
                g->translate((int)(rectHeight + max(0.0f, experimentalModsAnimationTranslation +
                                                                   m_experimentalContainer->getSize().x)),
                             (int)(osu->getScreenHeight() / 2 - rectWidth / 2));
                g->drawRect(0, 0, rectWidth, rectHeight);
            }
            g->popTransform();
        }

        OsuScreen::draw(g);
        m_overrideSliderContainer->draw(g);
    }

    // draw experimental mods
    if(!bancho.is_in_a_multi_room()) {
        g->pushTransform();
        {
            g->translate(experimentalModsAnimationTranslation, 0);
            g->setColor(backgroundColor);
            g->fillRect(m_experimentalContainer->getPos().x - margin, m_experimentalContainer->getPos().y - margin,
                        m_experimentalContainer->getSize().x + 2 * margin * m_fExperimentalAnimation,
                        m_experimentalContainer->getSize().y + 2 * margin);
            m_experimentalContainer->draw(g);
        }
        g->popTransform();
    }
}

void ModSelector::mouse_update(bool *propagate_clicks) {
    // HACKHACK: updating while invisible is stupid, but the only quick solution for still animating otherwise stuck
    // sliders while closed
    if(!m_bVisible) {
        for(int i = 0; i < m_overrideSliders.size(); i++) {
            if(m_overrideSliders[i].slider->hasChanged()) m_overrideSliders[i].slider->mouse_update(propagate_clicks);
        }
        if(m_bScheduledHide) {
            if(m_fAnimation == 0.0f) {
                m_bScheduledHide = false;
            }
        }
        return;
    }

    // update experimental mods, they take focus precedence over everything else
    if(m_bExperimentalVisible) {
        m_experimentalContainer->mouse_update(propagate_clicks);
    }

    // update
    OsuScreen::mouse_update(propagate_clicks);

    if(!bancho.is_in_a_multi_room()) {
        m_overrideSliderContainer->mouse_update(propagate_clicks);

        // override slider tooltips (ALT)
        if(m_bShowOverrideSliderALTHint) {
            for(int i = 0; i < m_overrideSliders.size(); i++) {
                if(m_overrideSliders[i].slider->isBusy()) {
                    osu->getTooltipOverlay()->begin();
                    { osu->getTooltipOverlay()->addLine("Hold [ALT] to slide in 0.01 increments."); }
                    osu->getTooltipOverlay()->end();

                    if(engine->getKeyboard()->isAltDown()) m_bShowOverrideSliderALTHint = false;
                }
            }
        }

        // some experimental mod tooltip overrides
        if(m_experimentalModRandomCheckbox->isChecked())
            m_experimentalModRandomCheckbox->setTooltipText(
                UString::format("Seed = %i", osu->getSelectedBeatmap()->getRandomSeed()));

        // handle experimental mods visibility
        bool experimentalModEnabled = false;
        for(int i = 0; i < m_experimentalMods.size(); i++) {
            CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox *>(m_experimentalMods[i].element);
            if(checkboxPointer != NULL && checkboxPointer->isChecked()) {
                experimentalModEnabled = true;
                break;
            }
        }

        McRect experimentalTrigger =
            McRect(0, 0, m_bExperimentalVisible ? m_experimentalContainer->getSize().x : osu->getScreenWidth() * 0.05f,
                   osu->getScreenHeight());
        if(experimentalTrigger.contains(engine->getMouse()->getPos())) {
            if(!m_bExperimentalVisible) {
                m_bExperimentalVisible = true;
                anim->moveQuadOut(&m_fExperimentalAnimation, 1.0f, (1.0f - m_fExperimentalAnimation) * 0.11f, 0.0f,
                                  true);
            }
        } else if(m_bExperimentalVisible && !m_experimentalContainer->isMouseInside() &&
                  !m_experimentalContainer->isActive() && !experimentalModEnabled) {
            m_bExperimentalVisible = false;
            anim->moveQuadIn(&m_fExperimentalAnimation, 0.0f, m_fExperimentalAnimation * 0.11f, 0.0f, true);
        }
    }

    // delayed onModUpdate() triggers when changing some values
    {
        // handle dynamic CS and slider vertex buffer updates
        if(m_CSSlider != NULL && (m_CSSlider->isActive() || m_CSSlider->hasChanged())) {
            m_bWaitForCSChangeFinished = true;
        } else if(m_bWaitForCSChangeFinished) {
            m_bWaitForCSChangeFinished = false;
            m_bWaitForSpeedChangeFinished = false;
            m_bWaitForHPChangeFinished = false;

            {
                if(osu->isInPlayMode()) osu->getSelectedBeatmap()->onModUpdate();
                osu->getSongBrowser()->recalculateStarsForSelectedBeatmap(true);
            }
        }

        // handle dynamic live pp calculation updates (when CS or Speed/BPM changes)
        if(m_speedSlider != NULL && (m_speedSlider->isActive() || m_speedSlider->hasChanged())) {
            m_bWaitForSpeedChangeFinished = true;
        } else if(m_bWaitForSpeedChangeFinished) {
            m_bWaitForCSChangeFinished = false;
            m_bWaitForSpeedChangeFinished = false;
            m_bWaitForHPChangeFinished = false;

            {
                if(osu->isInPlayMode()) osu->getSelectedBeatmap()->onModUpdate();
                osu->getSongBrowser()->recalculateStarsForSelectedBeatmap(true);
            }
        }

        // handle dynamic HP drain updates
        if(m_HPSlider != NULL && (m_HPSlider->isActive() || m_HPSlider->hasChanged())) {
            m_bWaitForHPChangeFinished = true;
        } else if(m_bWaitForHPChangeFinished) {
            m_bWaitForCSChangeFinished = false;
            m_bWaitForSpeedChangeFinished = false;
            m_bWaitForHPChangeFinished = false;
            if(osu->isInPlayMode()) osu->getSelectedBeatmap()->onModUpdate();
        }
    }
}

void ModSelector::onKeyDown(KeyboardEvent &key) {
    OsuScreen::onKeyDown(key);  // only used for options menu
    if(!m_bVisible || key.isConsumed()) return;

    m_overrideSliderContainer->onKeyDown(key);

    if(key == KEY_1) resetModsUserInitiated();

    if(((key == KEY_F1 || key == (KEYCODE)KeyBindings::TOGGLE_MODSELECT.getInt()) && !m_bWaitForF1KeyUp) ||
       key == KEY_2 || key == (KEYCODE)KeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE || key == KEY_ENTER)
        close();

    // mod hotkeys
    if(key == (KEYCODE)KeyBindings::MOD_EASY.getInt()) m_modButtonEasy->click();
    if(key == (KEYCODE)KeyBindings::MOD_NOFAIL.getInt()) m_modButtonNofail->click();
    if(key == (KEYCODE)KeyBindings::MOD_HALFTIME.getInt()) m_modButtonHalftime->click();
    if(key == (KEYCODE)KeyBindings::MOD_HARDROCK.getInt()) m_modButtonHardrock->click();
    if(key == (KEYCODE)KeyBindings::MOD_SUDDENDEATH.getInt()) m_modButtonSuddendeath->click();
    if(key == (KEYCODE)KeyBindings::MOD_DOUBLETIME.getInt()) m_modButtonDoubletime->click();
    if(key == (KEYCODE)KeyBindings::MOD_HIDDEN.getInt()) m_modButtonHidden->click();
    if(key == (KEYCODE)KeyBindings::MOD_FLASHLIGHT.getInt()) m_modButtonFlashlight->click();
    if(key == (KEYCODE)KeyBindings::MOD_RELAX.getInt()) m_modButtonRelax->click();
    if(key == (KEYCODE)KeyBindings::MOD_AUTOPILOT.getInt()) m_modButtonAutopilot->click();
    if(key == (KEYCODE)KeyBindings::MOD_SPUNOUT.getInt()) m_modButtonSpunout->click();
    if(key == (KEYCODE)KeyBindings::MOD_AUTO.getInt()) m_modButtonAuto->click();
    if(key == (KEYCODE)KeyBindings::MOD_SCOREV2.getInt()) m_modButtonScoreV2->click();

    key.consume();
}

void ModSelector::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible) return;

    if(key == KEY_F1 || key == (KEYCODE)KeyBindings::TOGGLE_MODSELECT.getInt()) m_bWaitForF1KeyUp = false;
}

CBaseUIContainer *ModSelector::setVisible(bool visible) {
    if(visible && !m_bVisible) {
        m_bScheduledHide = false;

        updateButtons(true);              // force state update without firing callbacks
        updateExperimentalButtons(true);  // force state update without firing callbacks
        updateLayout();
        updateScoreMultiplierLabelText();
        updateOverrideSliderLabels();

        m_fAnimation = 0.0f;
        anim->moveQuadOut(&m_fAnimation, 1.0f, 0.1f, 0.0f, true);

        bool experimentalModEnabled = false;
        for(int i = 0; i < m_experimentalMods.size(); i++) {
            CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox *>(m_experimentalMods[i].element);
            if(checkboxPointer != NULL && checkboxPointer->isChecked()) {
                experimentalModEnabled = true;
                break;
            }
        }
        if(experimentalModEnabled) {
            m_bExperimentalVisible = true;
            if(isInCompactMode())
                anim->moveQuadOut(&m_fExperimentalAnimation, 1.0f, (1.0f - m_fExperimentalAnimation) * 0.06f, 0.0f,
                                  true);
            else
                m_fExperimentalAnimation = 1.0f;
        } else {
            m_bExperimentalVisible = false;
            m_fExperimentalAnimation = 0.0f;
            anim->deleteExistingAnimation(&m_fExperimentalAnimation);
        }
    } else if(!visible && m_bVisible) {
        m_bScheduledHide = isInCompactMode();

        m_fAnimation = 1.0f;
        anim->moveQuadIn(&m_fAnimation, 0.0f, 0.06f, 0.0f, true);
        updateModConVar();

        m_bExperimentalVisible = false;
        anim->moveQuadIn(&m_fExperimentalAnimation, 0.0f, 0.06f, 0.0f, true);
    }

    m_bVisible = visible;
    return this;
}

bool ModSelector::isInCompactMode() { return osu->isInPlayMode(); }

bool ModSelector::isCSOverrideSliderActive() { return m_CSSlider->isActive(); }

bool ModSelector::isMouseInScrollView() { return m_experimentalContainer->isMouseInside() && isVisible(); }

bool ModSelector::isMouseInside() {
    bool isMouseInsideAnyModSelectorModButton = false;
    for(size_t i = 0; i < m_modButtons.size(); i++) {
        if(m_modButtons[i]->isMouseInside()) {
            isMouseInsideAnyModSelectorModButton = true;
            break;
        }
    }

    bool isMouseInsideAnyOverrideSliders = false;
    for(size_t i = 0; i < m_overrideSliders.size(); i++) {
        if((m_overrideSliders[i].lock != NULL && m_overrideSliders[i].lock->isMouseInside()) ||
           m_overrideSliders[i].desc->isMouseInside() || m_overrideSliders[i].slider->isMouseInside() ||
           m_overrideSliders[i].label->isMouseInside()) {
            isMouseInsideAnyOverrideSliders = true;
            break;
        }
    }

    return isVisible() && (m_experimentalContainer->isMouseInside() || isMouseInsideAnyModSelectorModButton ||
                           isMouseInsideAnyOverrideSliders);
}

void ModSelector::updateLayout() {
    if(m_modButtons.size() < 1 || m_overrideSliders.size() < 1) return;

    const float dpiScale = Osu::getUIScale();
    const float uiScale = Osu::ui_scale->getFloat();

    if(!isInCompactMode())  // normal layout
    {
        // mod grid buttons
        Vector2 center = osu->getScreenSize() / 2.0f;
        Vector2 size = osu->getSkin()->getSelectionModEasy()->getSizeBase() * uiScale;
        Vector2 offset = Vector2(size.x * 1.0f, size.y * 0.25f);
        Vector2 start = Vector2(center.x - (size.x * m_iGridWidth) / 2.0f - (offset.x * (m_iGridWidth - 1)) / 2.0f,
                                center.y - (size.y * m_iGridHeight) / 2.0f - (offset.y * (m_iGridHeight - 1)) / 2.0f);

        for(int x = 0; x < m_iGridWidth; x++) {
            for(int y = 0; y < m_iGridHeight; y++) {
                UIModSelectorModButton *button = getModButtonOnGrid(x, y);

                if(button != NULL) {
                    button->setPos(start + Vector2(size.x * x + offset.x * x, size.y * y + offset.y * y));
                    button->setBaseScale(1.0f * uiScale, 1.0f * uiScale);
                    button->setSize(size);
                }
            }
        }

        // override sliders (down here because they depend on the mod grid button alignment)
        const int margin = 10 * dpiScale;
        const int overrideSliderWidth = osu->getUIScale(250.0f);
        const int overrideSliderHeight = 25 * dpiScale;
        const int overrideSliderOffsetY =
            ((start.y - m_overrideSliders.size() * overrideSliderHeight) / (m_overrideSliders.size() - 1)) * 0.35f;
        const Vector2 overrideSliderStart =
            Vector2(osu->getScreenWidth() / 2 - overrideSliderWidth / 2,
                    start.y / 2 - (m_overrideSliders.size() * overrideSliderHeight +
                                   (m_overrideSliders.size() - 1) * overrideSliderOffsetY) /
                                      1.75f);
        for(int i = 0; i < m_overrideSliders.size(); i++) {
            m_overrideSliders[i].desc->setSizeToContent(5 * dpiScale, 0);
            m_overrideSliders[i].desc->setSizeY(overrideSliderHeight);

            m_overrideSliders[i].slider->setBlockSize(20 * dpiScale, 20 * dpiScale);
            m_overrideSliders[i].slider->setPos(
                overrideSliderStart.x, overrideSliderStart.y + i * overrideSliderHeight + i * overrideSliderOffsetY);
            m_overrideSliders[i].slider->setSize(overrideSliderWidth, overrideSliderHeight);

            m_overrideSliders[i].desc->setPos(
                m_overrideSliders[i].slider->getPos().x - m_overrideSliders[i].desc->getSize().x - margin,
                m_overrideSliders[i].slider->getPos().y);

            if(m_overrideSliders[i].lock != NULL && m_overrideSliders.size() > 1) {
                m_overrideSliders[i].lock->setPos(m_overrideSliders[1].desc->getPos().x -
                                                      m_overrideSliders[i].lock->getSize().x - margin - 3 * dpiScale,
                                                  m_overrideSliders[i].desc->getPos().y);
                m_overrideSliders[i].lock->setSizeY(overrideSliderHeight);
            }

            m_overrideSliders[i].label->setPos(
                m_overrideSliders[i].slider->getPos().x + m_overrideSliders[i].slider->getSize().x + margin,
                m_overrideSliders[i].slider->getPos().y);
            m_overrideSliders[i].label->setSizeToContent(0, 0);
            m_overrideSliders[i].label->setSizeY(overrideSliderHeight);
        }

        // action buttons
        float actionMinY =
            start.y + size.y * m_iGridHeight + offset.y * (m_iGridHeight - 1);  // exact bottom of the mod buttons
        Vector2 actionSize = Vector2(osu->getUIScale(448.0f) * uiScale, size.y * 0.75f);
        float actionOffsetY = actionSize.y * 0.5f;
        Vector2 actionStart =
            Vector2(osu->getScreenWidth() / 2.0f - actionSize.x / 2.0f,
                    actionMinY + (osu->getScreenHeight() - actionMinY) / 2.0f -
                        (actionSize.y * m_actionButtons.size() + actionOffsetY * (m_actionButtons.size() - 1)) / 2.0f);
        for(int i = 0; i < m_actionButtons.size(); i++) {
            m_actionButtons[i]->setVisible(true);
            m_actionButtons[i]->setPos(actionStart.x, actionStart.y + actionSize.y * i + actionOffsetY * i);
            m_actionButtons[i]->onResized();  // HACKHACK: framework, setSize*() does not update string metrics
            m_actionButtons[i]->setSize(actionSize);
        }

        // score multiplier info label
        const float modGridMaxY =
            start.y + size.y * m_iGridHeight + offset.y * (m_iGridHeight - 1);  // exact bottom of the mod buttons

        m_nonVanillaWarning->setVisible(!convar->isVanilla() && bancho.submit_scores());
        m_nonVanillaWarning->setSizeToContent();
        m_nonVanillaWarning->setSize(Vector2(osu->getScreenWidth(), 20 * uiScale));
        m_nonVanillaWarning->setPos(
            0, modGridMaxY + std::abs(actionStart.y - modGridMaxY) / 2 - m_nonVanillaWarning->getSize().y);

        m_scoreMultiplierLabel->setVisible(true);
        m_scoreMultiplierLabel->setSizeToContent();
        m_scoreMultiplierLabel->setSize(Vector2(osu->getScreenWidth(), 30 * uiScale));
        m_scoreMultiplierLabel->setPos(0, m_nonVanillaWarning->getPos().y + 20 * uiScale);
    } else  // compact in-beatmap mode
    {
        // mod grid buttons
        Vector2 center = osu->getScreenSize() / 2.0f;
        Vector2 blockSize = osu->getSkin()->getSelectionModEasy()->getSizeBase() * uiScale;
        Vector2 offset = Vector2(blockSize.x * 0.15f, blockSize.y * 0.05f);
        Vector2 size = Vector2((blockSize.x * m_iGridWidth) + (offset.x * (m_iGridWidth - 1)),
                               (blockSize.y * m_iGridHeight) + (offset.y * (m_iGridHeight - 1)));
        center.y = osu->getScreenHeight() - size.y / 2 - offset.y * 3.0f;
        Vector2 start = Vector2(center.x - size.x / 2.0f, center.y - size.y / 2.0f);

        for(int x = 0; x < m_iGridWidth; x++) {
            for(int y = 0; y < m_iGridHeight; y++) {
                UIModSelectorModButton *button = getModButtonOnGrid(x, y);

                if(button != NULL) {
                    button->setPos(start + Vector2(blockSize.x * x + offset.x * x, blockSize.y * y + offset.y * y));
                    button->setBaseScale(1 * uiScale, 1 * uiScale);
                    button->setSize(blockSize);
                }
            }
        }

        // override sliders (down here because they depend on the mod grid button alignment)
        const int margin = 10 * dpiScale;
        int overrideSliderWidth = osu->getUIScale(250.0f);
        int overrideSliderHeight = 25 * dpiScale;
        int overrideSliderOffsetY = 5 * dpiScale;
        Vector2 overrideSliderStart = Vector2(osu->getScreenWidth() / 2 - overrideSliderWidth / 2, 5 * dpiScale);
        for(int i = 0; i < m_overrideSliders.size(); i++) {
            m_overrideSliders[i].desc->setSizeToContent(5 * dpiScale, 0);
            m_overrideSliders[i].desc->setSizeY(overrideSliderHeight);

            m_overrideSliders[i].slider->setPos(
                overrideSliderStart.x, overrideSliderStart.y + i * overrideSliderHeight + i * overrideSliderOffsetY);
            m_overrideSliders[i].slider->setSizeX(overrideSliderWidth);

            m_overrideSliders[i].desc->setPos(
                m_overrideSliders[i].slider->getPos().x - m_overrideSliders[i].desc->getSize().x - margin,
                m_overrideSliders[i].slider->getPos().y);

            if(m_overrideSliders[i].lock != NULL && m_overrideSliders.size() > 1) {
                m_overrideSliders[i].lock->setPos(m_overrideSliders[1].desc->getPos().x -
                                                      m_overrideSliders[i].lock->getSize().x - margin - 3 * dpiScale,
                                                  m_overrideSliders[i].desc->getPos().y);
                m_overrideSliders[i].lock->setSizeY(overrideSliderHeight);
            }

            m_overrideSliders[i].label->setPos(
                m_overrideSliders[i].slider->getPos().x + m_overrideSliders[i].slider->getSize().x + margin,
                m_overrideSliders[i].slider->getPos().y);
            m_overrideSliders[i].label->setSizeToContent(0, 0);
            m_overrideSliders[i].label->setSizeY(overrideSliderHeight);
        }

        // action buttons
        for(int i = 0; i < m_actionButtons.size(); i++) {
            m_actionButtons[i]->setVisible(false);
        }

        // score multiplier info label
        m_scoreMultiplierLabel->setVisible(false);
    }

    updateExperimentalLayout();
}

void ModSelector::updateExperimentalLayout() {
    const float dpiScale = Osu::getUIScale();

    // experimental mods
    int yCounter = 5 * dpiScale;
    int experimentalMaxWidth = 0;
    int experimentalOffsetY = 6 * dpiScale;
    for(int i = 0; i < m_experimentalMods.size(); i++) {
        CBaseUIElement *e = m_experimentalMods[i].element;
        e->setRelPosY(yCounter);
        e->setSizeY(e->getRelSize().y * dpiScale);

        // custom
        {
            CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox *>(e);
            if(checkboxPointer != NULL) {
                checkboxPointer->onResized();
                checkboxPointer->setWidthToContent(0);
            }

            CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel *>(e);
            if(labelPointer != NULL) {
                labelPointer->onResized();
                labelPointer->setWidthToContent(0);
            }
        }

        if(e->getSize().x > experimentalMaxWidth) experimentalMaxWidth = e->getSize().x;

        yCounter += e->getSize().y + experimentalOffsetY;

        if(i == 0) yCounter += 8 * dpiScale;
    }

    // laziness
    if(osu->getScreenHeight() > yCounter)
        yCounter = 5 * dpiScale + osu->getScreenHeight() / 2.0f - yCounter / 2.0f;
    else
        yCounter = 5 * dpiScale;

    for(int i = 0; i < m_experimentalMods.size(); i++) {
        CBaseUIElement *e = m_experimentalMods[i].element;
        e->setRelPosY(yCounter);

        if(e->getSize().x > experimentalMaxWidth) experimentalMaxWidth = e->getSize().x;

        yCounter += e->getSize().y + experimentalOffsetY;

        if(i == 0) yCounter += 8 * dpiScale;
    }

    m_experimentalContainer->setSizeX(experimentalMaxWidth + 25 * dpiScale /*, yCounter*/);
    m_experimentalContainer->setPosY(-1);
    m_experimentalContainer->setScrollSizeToContent(1 * dpiScale);
    m_experimentalContainer->getContainer()->update_pos();
    m_experimentalContainer->setVisible(!bancho.is_in_a_multi_room());
}

void ModSelector::updateModConVar() {
    ConVar *osu_mods_ref = convar->getConVarByName("osu_mods");

    UString modString = "";
    for(int i = 0; i < m_modButtons.size(); i++) {
        UIModSelectorModButton *button = m_modButtons[i];
        if(button->isOn()) modString.append(button->getActiveModName());
    }

    osu_mods_ref->setValue(modString);

    updateScoreMultiplierLabelText();
    updateOverrideSliderLabels();
    osu->updateMods();
}

UIModSelectorModButton *ModSelector::setModButtonOnGrid(int x, int y, int state, bool initialState, UString modName,
                                                        UString tooltipText,
                                                        std::function<SkinImage *()> getImageFunc) {
    UIModSelectorModButton *modButton = getModButtonOnGrid(x, y);

    if(modButton != NULL) {
        modButton->setState(state, initialState, modName, tooltipText, getImageFunc);
        modButton->setVisible(true);
    }

    return modButton;
}

UIModSelectorModButton *ModSelector::getModButtonOnGrid(int x, int y) {
    const int index = x * m_iGridHeight + y;

    if(index < m_modButtons.size())
        return m_modButtons[index];
    else
        return NULL;
}

ModSelector::OVERRIDE_SLIDER ModSelector::addOverrideSlider(UString text, UString labelText, ConVar *cvar, float min,
                                                            float max, UString tooltipText, ConVar *lockCvar) {
    int height = 25;

    OVERRIDE_SLIDER os;
    if(lockCvar != NULL) {
        os.lock = new ModSelectorOverrideSliderLockButton(0, 0, height, height, "", "");
        os.lock->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onOverrideSliderLockChange));
    }
    os.desc = new ModSelectorOverrideSliderDescButton(0, 0, 100, height, "", text);
    os.desc->setTooltipText(tooltipText);
    os.slider = new UISlider(0, 0, 100, height, "");
    os.label = new CBaseUILabel(0, 0, 100, height, labelText, labelText);
    os.cvar = cvar;
    os.lockCvar = lockCvar;

    bool debugDrawFrame = false;

    os.slider->setDrawFrame(debugDrawFrame);
    os.slider->setDrawBackground(false);
    os.slider->setFrameColor(0xff777777);
    // os.desc->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION::TEXT_JUSTIFICATION_CENTERED);
    os.desc->setEnabled(lockCvar != NULL);
    os.desc->setDrawFrame(debugDrawFrame);
    os.desc->setDrawBackground(false);
    os.desc->setTextColor(0xff777777);
    os.label->setDrawFrame(debugDrawFrame);
    os.label->setDrawBackground(false);
    os.label->setTextColor(0xff777777);

    os.slider->setBounds(min, max + 1.0f);
    os.slider->setKeyDelta(0.1f);
    os.slider->setLiveUpdate(true);
    os.slider->setAllowMouseWheel(false);

    if(os.cvar != NULL) os.slider->setValue(os.cvar->getFloat() + 1.0f, false);

    if(os.lock != NULL) m_overrideSliderContainer->addBaseUIElement(os.lock);

    m_overrideSliderContainer->addBaseUIElement(os.desc);
    m_overrideSliderContainer->addBaseUIElement(os.slider);
    m_overrideSliderContainer->addBaseUIElement(os.label);
    m_overrideSliders.push_back(os);

    return os;
}

UIButton *ModSelector::addActionButton(UString text) {
    UIButton *actionButton = new UIButton(50, 50, 100, 100, text, text);
    m_actionButtons.push_back(actionButton);
    addBaseUIElement(actionButton);

    return actionButton;
}

CBaseUILabel *ModSelector::addExperimentalLabel(UString text) {
    CBaseUILabel *label = new CBaseUILabel(0, 0, 0, 25, text, text);
    label->setFont(osu->getSubTitleFont());
    label->setWidthToContent(0);
    label->setDrawBackground(false);
    label->setDrawFrame(false);
    m_experimentalContainer->getContainer()->addBaseUIElement(label);

    EXPERIMENTAL_MOD em;
    em.element = label;
    em.cvar = NULL;
    m_experimentalMods.push_back(em);

    return label;
}

UICheckbox *ModSelector::addExperimentalCheckbox(UString text, UString tooltipText, ConVar *cvar) {
    UICheckbox *checkbox = new UICheckbox(0, 0, 0, 35, text, text);
    checkbox->setTooltipText(tooltipText);
    checkbox->setWidthToContent(0);
    if(cvar != NULL) {
        checkbox->setChecked(cvar->getBool());
        checkbox->setChangeCallback(fastdelegate::MakeDelegate(this, &ModSelector::onCheckboxChange));
    }
    m_experimentalContainer->getContainer()->addBaseUIElement(checkbox);

    EXPERIMENTAL_MOD em;
    em.element = checkbox;
    em.cvar = cvar;
    m_experimentalMods.push_back(em);

    return checkbox;
}

void ModSelector::resetModsUserInitiated() {
    bancho.prefer_daycore = false;
    resetMods();

    engine->getSound()->play(osu->getSkin()->getCheckOff());
    m_resetModsButton->animateClickColor();

    if(bancho.is_online()) {
        RichPresence::updateBanchoMods();
    }

    if(bancho.is_in_a_multi_room()) {
        for(int i = 0; i < 16; i++) {
            if(bancho.room.slots[i].player_id != bancho.user_id) continue;

            if(bancho.room.is_host()) {
                bancho.room.mods = getModFlags();
            } else {
                enableModsFromFlags(bancho.room.mods);
            }

            bancho.room.slots[i].mods = bancho.room.mods;

            Packet packet;
            packet.id = MATCH_CHANGE_MODS;
            write<u32>(&packet, bancho.room.slots[i].mods);
            send_packet(packet);

            osu->m_room->updateLayout(osu->getScreenSize());
            break;
        }
    }
}

void ModSelector::resetMods() {
    convar->getConVarByName("osu_mod_fposu")->setValue(false);

    for(int i = 0; i < m_overrideSliders.size(); i++) {
        if(m_overrideSliders[i].lock != NULL) m_overrideSliders[i].lock->setChecked(false);
    }

    for(int i = 0; i < m_overrideSliders.size(); i++) {
        // HACKHACK: force small delta to force an update (otherwise values could get stuck, e.g. for "Use Mods" context
        // menu) HACKHACK: only animate while visible to workaround "Use mods" bug (if custom speed multiplier already
        // set and then "Use mods" with different custom speed multiplier would reset to 1.0x because of anim)
        m_overrideSliders[i].slider->setValue(m_overrideSliders[i].slider->getMin() + 0.0001f, m_bVisible);
        m_overrideSliders[i].slider->setValue(m_overrideSliders[i].slider->getMin(), m_bVisible);
    }

    for(int i = 0; i < m_modButtons.size(); i++) {
        m_modButtons[i]->resetState();
    }

    for(int i = 0; i < m_experimentalMods.size(); i++) {
        ConVar *cvar = m_experimentalMods[i].cvar;
        CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox *>(m_experimentalMods[i].element);
        if(checkboxPointer != NULL) {
            // HACKHACK: we update both just in case because if the mod selector was not yet visible after a convar
            // change (e.g. because of "Use mods") then the checkbox has not yet updated its internal state
            checkboxPointer->setChecked(false);
            if(cvar != NULL) cvar->setValue(0.0f);
        }
    }
}

u32 ModSelector::getModFlags() {
    return osu->getScore()->getModsLegacy();
}

void ModSelector::enableModsFromFlags(u32 flags) {
    if(flags & ModFlags::DoubleTime) {
        m_modButtonDoubletime->setOn(true, true);
        if(flags & ModFlags::Nightcore) {
            m_modButtonDoubletime->setState(1, true);
        }
    }

    if(flags & ModFlags::Perfect) {
        m_modButtonSuddendeath->setOn(true, true);
        m_modButtonSuddendeath->setState(1, true);
    } else if(flags & ModFlags::SuddenDeath) {
        m_modButtonSuddendeath->setOn(true, true);
        m_modButtonSuddendeath->setState(0, true);
    }

    if(flags & ModFlags::HalfTime) {
        m_modButtonHalftime->setOn(true, true);
        m_modButtonHalftime->setState(bancho.prefer_daycore ? 1 : 0, true);
    }

    m_modButtonNofail->setOn(flags & ModFlags::NoFail, true);
    m_modButtonEasy->setOn(flags & ModFlags::Easy, true);
    m_modButtonTD->setOn(flags & ModFlags::TouchDevice, true);
    m_modButtonHidden->setOn(flags & ModFlags::Hidden, true);
    m_modButtonHardrock->setOn(flags & ModFlags::HardRock, true);
    m_modButtonRelax->setOn(flags & ModFlags::Relax, true);
    m_modButtonSpunout->setOn(flags & ModFlags::SpunOut, true);
    m_modButtonAutopilot->setOn(flags & ModFlags::Autopilot, true);
    getModButtonOnGrid(4, 2)->setOn(flags & ModFlags::Target, true);
    m_modButtonFlashlight->setOn(flags & ModFlags::Flashlight, true);
    m_modButtonScoreV2->setOn(flags & ModFlags::ScoreV2, true);
}

void ModSelector::close() {
    m_closeButton->animateClickColor();
    osu->toggleModSelection();
}

void ModSelector::onOverrideSliderChange(CBaseUISlider *slider) {
    for(int i = 0; i < m_overrideSliders.size(); i++) {
        if(m_overrideSliders[i].slider == slider) {
            float sliderValue = slider->getFloat() - 1.0f;
            const float rawSliderValue = slider->getFloat();

            // alt key allows rounding to only 1 decimal digit
            if(!engine->getKeyboard()->isAltDown())
                sliderValue = std::round(sliderValue * 10.0f) / 10.0f;
            else
                sliderValue = std::round(sliderValue * 100.0f) / 100.0f;

            if(sliderValue < 0.0f) {
                sliderValue = -1.0f;
                m_overrideSliders[i].label->setWidthToContent(0);

                // HACKHACK: dirty
                if(osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL) {
                    if(m_overrideSliders[i].label->getName().find("BPM") != -1) {
                        // reset AR and OD override sliders if the bpm slider was reset
                        if(!m_ARLock->isChecked()) m_ARSlider->setValue(0.0f, false);
                        if(!m_ODLock->isChecked()) m_ODSlider->setValue(0.0f, false);
                    }
                }

                // usability: auto disable lock if override slider is fully set to -1.0f (disabled)
                if(rawSliderValue == 0.0f) {
                    if(m_overrideSliders[i].lock != NULL && m_overrideSliders[i].lock->isChecked())
                        m_overrideSliders[i].lock->setChecked(false);
                }
            } else {
                // AR/OD lock may not be used in conjunction with BPM
                if(m_overrideSliders[i].label->getName().find("BPM") != -1) {
                    m_ARLock->setChecked(false);
                    m_ODLock->setChecked(false);
                }

                // HACKHACK: dirty
                if(osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL) {
                    if(m_overrideSliders[i].label->getName().find("BPM") != -1) {
                        // HACKHACK: force BPM slider to have a min value of 0.05 instead of 0 (because that's the
                        // minimum for BASS) note that the BPM slider is just a 'fake' slider, it directly controls the
                        // speed slider to do its thing (thus it needs the same limits)
                        sliderValue = max(sliderValue, 0.05f);

                        // speed slider may not be used in conjunction
                        m_speedSlider->setValue(0.0f, false);

                        // force early update
                        m_overrideSliders[i].cvar->setValue(sliderValue);

                        // force change all other depending sliders
                        const float newAR =
                            GameRules::getConstantApproachRateForSpeedMultiplier(osu->getSelectedBeatmap());
                        const float newOD =
                            GameRules::getConstantOverallDifficultyForSpeedMultiplier(osu->getSelectedBeatmap());

                        m_ARSlider->setValue(newAR + 1.0f,
                                             false);  // '+1' to compensate for turn-off area of the override sliders
                        m_ODSlider->setValue(newOD + 1.0f, false);
                    }
                }

                // HACKHACK: force speed slider to have a min value of 0.05 instead of 0 (because that's the minimum for
                // BASS)
                if(m_overrideSliders[i].desc->getText().find("Speed") != -1) sliderValue = max(sliderValue, 0.05f);
            }

            // update convar with final value (e.g. osu_ar_override, osu_speed_override, etc.)
            m_overrideSliders[i].cvar->setValue(sliderValue);

            updateOverrideSliderLabels();

            break;
        }
    }
}

void ModSelector::onOverrideSliderLockChange(CBaseUICheckbox *checkbox) {
    for(int i = 0; i < m_overrideSliders.size(); i++) {
        if(m_overrideSliders[i].lock == checkbox) {
            const bool locked = m_overrideSliders[i].lock->isChecked();
            const bool wasLocked = m_overrideSliders[i].lockCvar->getBool();

            // update convar with final value (e.g. osu_ar_override_lock, osu_od_override_lock)
            m_overrideSliders[i].lockCvar->setValue(locked ? 1.0f : 0.0f);

            // usability: if we just got locked, and the override slider value is < 0.0f (disabled), then set override
            // to current value
            if(locked && !wasLocked) {
                if(checkbox == m_ARLock) {
                    if(m_ARSlider->getFloat() < 1.0f)
                        m_ARSlider->setValue(osu->getSelectedBeatmap()->getRawAR() + 1.0f,
                                             false);  // '+1' to compensate for turn-off area of the override sliders
                } else if(checkbox == m_ODLock) {
                    if(m_ODSlider->getFloat() < 1.0f)
                        m_ODSlider->setValue(osu->getSelectedBeatmap()->getRawOD() + 1.0f,
                                             false);  // '+1' to compensate for turn-off area of the override sliders
                }
            }

            updateOverrideSliderLabels();

            break;
        }
    }
}

void ModSelector::onOverrideARSliderDescClicked(CBaseUIButton *button) { m_ARLock->click(); }

void ModSelector::onOverrideODSliderDescClicked(CBaseUIButton *button) { m_ODLock->click(); }

void ModSelector::updateOverrideSliderLabels() {
    const Color inactiveColor = 0xff777777;
    const Color activeColor = 0xffffffff;
    const Color inactiveLabelColor = 0xff1166ff;

    for(int i = 0; i < m_overrideSliders.size(); i++) {
        const float convarValue = m_overrideSliders[i].cvar->getFloat();
        const bool isLocked = (m_overrideSliders[i].lock != NULL && m_overrideSliders[i].lock->isChecked());

        // update colors
        if(convarValue < 0.0f && !isLocked) {
            m_overrideSliders[i].label->setTextColor(inactiveLabelColor);
            m_overrideSliders[i].desc->setTextColor(inactiveColor);
            m_overrideSliders[i].slider->setFrameColor(inactiveColor);
        } else {
            m_overrideSliders[i].label->setTextColor(activeColor);
            m_overrideSliders[i].desc->setTextColor(activeColor);
            m_overrideSliders[i].slider->setFrameColor(activeColor);
        }

        m_overrideSliders[i].desc->setDrawFrame(isLocked);

        // update label text
        m_overrideSliders[i].label->setText(getOverrideSliderLabelText(m_overrideSliders[i], convarValue >= 0.0f));
        m_overrideSliders[i].label->setWidthToContent(0);

        // update lock checkbox
        if(m_overrideSliders[i].lock != NULL && m_overrideSliders[i].lockCvar != NULL &&
           m_overrideSliders[i].lock->isChecked() != m_overrideSliders[i].lockCvar->getBool())
            m_overrideSliders[i].lock->setChecked(m_overrideSliders[i].lockCvar->getBool());
    }
}

UString ModSelector::getOverrideSliderLabelText(ModSelector::OVERRIDE_SLIDER s, bool active) {
    float convarValue = s.cvar->getFloat();

    UString newLabelText = s.label->getName();
    if(osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL) {
        // used for compensating speed changing experimental mods (which cause osu->getSpeedMultiplier() !=
        // beatmap->getSpeedMultiplier()) to keep the AR/OD display correct
        const float speedMultiplierLive =
            osu->isInPlayMode() ? osu->getSelectedBeatmap()->getSpeedMultiplier() : osu->getSpeedMultiplier();

        // for relevant values (AR/OD), any non-1.0x speed multiplier should show the fractional parts caused by such a
        // speed multiplier (same for non-1.0x difficulty multiplier)
        const bool forceDisplayTwoDecimalDigits =
            (speedMultiplierLive != 1.0f || osu->getDifficultyMultiplier() != 1.0f ||
             osu->getCSDifficultyMultiplier() != 1.0f);

        // HACKHACK: dirty
        bool wasSpeedSlider = false;
        float beatmapValue = 1.0f;
        if(s.label->getName().find("CS") != -1) {
            beatmapValue = clamp<float>(
                osu->getSelectedBeatmap()->getSelectedDifficulty2()->getCS() * osu->getCSDifficultyMultiplier(), 0.0f,
                10.0f);
            convarValue = osu->getSelectedBeatmap()->getCS();
        } else if(s.label->getName().find("AR") != -1) {
            beatmapValue = active ? GameRules::getRawApproachRateForSpeedMultiplier(osu->getSelectedBeatmap())
                                  : GameRules::getApproachRateForSpeedMultiplier(osu->getSelectedBeatmap());

            // compensate and round
            convarValue = GameRules::getApproachRateForSpeedMultiplier(osu->getSelectedBeatmap(), speedMultiplierLive);
            if(!engine->getKeyboard()->isAltDown() && !forceDisplayTwoDecimalDigits)
                convarValue = std::round(convarValue * 10.0f) / 10.0f;
            else
                convarValue = std::round(convarValue * 100.0f) / 100.0f;
        } else if(s.label->getName().find("OD") != -1) {
            beatmapValue = active ? GameRules::getRawOverallDifficultyForSpeedMultiplier(osu->getSelectedBeatmap())
                                  : GameRules::getOverallDifficultyForSpeedMultiplier(osu->getSelectedBeatmap());

            // compensate and round
            convarValue =
                GameRules::getOverallDifficultyForSpeedMultiplier(osu->getSelectedBeatmap(), speedMultiplierLive);
            if(!engine->getKeyboard()->isAltDown() && !forceDisplayTwoDecimalDigits)
                convarValue = std::round(convarValue * 10.0f) / 10.0f;
            else
                convarValue = std::round(convarValue * 100.0f) / 100.0f;
        } else if(s.label->getName().find("HP") != -1) {
            beatmapValue = clamp<float>(
                osu->getSelectedBeatmap()->getSelectedDifficulty2()->getHP() * osu->getDifficultyMultiplier(), 0.0f,
                10.0f);
            convarValue = osu->getSelectedBeatmap()->getHP();
        } else if(s.desc->getText().find("Speed") != -1) {
            beatmapValue = active ? osu->getRawSpeedMultiplier() : osu->getSpeedMultiplier();

            wasSpeedSlider = true;
            {
                {
                    if(!active)
                        newLabelText.append(UString::format(" %.4g", beatmapValue));
                    else
                        newLabelText.append(UString::format(" %.4g -> %.4g", beatmapValue, convarValue));
                }

                newLabelText.append("  (BPM: ");

                int minBPM = osu->getSelectedBeatmap()->getSelectedDifficulty2()->getMinBPM();
                int maxBPM = osu->getSelectedBeatmap()->getSelectedDifficulty2()->getMaxBPM();
                int mostCommonBPM = osu->getSelectedBeatmap()->getSelectedDifficulty2()->getMostCommonBPM();
                int newMinBPM = minBPM * osu->getSpeedMultiplier();
                int newMaxBPM = maxBPM * osu->getSpeedMultiplier();
                int newMostCommonBPM = mostCommonBPM * osu->getSpeedMultiplier();
                if(!active || osu->getSpeedMultiplier() == 1.0f) {
                    if(minBPM == maxBPM)
                        newLabelText.append(UString::format("%i", newMaxBPM));
                    else
                        newLabelText.append(UString::format("%i-%i (%i)", newMinBPM, newMaxBPM, newMostCommonBPM));
                } else {
                    if(minBPM == maxBPM)
                        newLabelText.append(UString::format("%i -> %i", maxBPM, newMaxBPM));
                    else
                        newLabelText.append(UString::format("%i-%i (%i) -> %i-%i (%i)", minBPM, maxBPM, mostCommonBPM,
                                                            newMinBPM, newMaxBPM, newMostCommonBPM));
                }

                newLabelText.append(")");
            }
        }

        // always round beatmapValue to 1 decimal digit, except for the speed slider, and except for non-1.0x speed
        // multipliers, and except for non-1.0x difficulty multipliers HACKHACK: dirty
        if(s.desc->getText().find("Speed") == -1) {
            if(forceDisplayTwoDecimalDigits)
                beatmapValue = std::round(beatmapValue * 100.0f) / 100.0f;
            else
                beatmapValue = std::round(beatmapValue * 10.0f) / 10.0f;
        }

        // update label
        if(!wasSpeedSlider) {
            if(!active)
                newLabelText.append(UString::format(" %.4g", beatmapValue));
            else
                newLabelText.append(UString::format(" %.4g -> %.4g", beatmapValue, convarValue));
        }
    }

    return newLabelText;
}

void ModSelector::enableAuto() {
    if(!m_modButtonAuto->isOn()) m_modButtonAuto->click();
}

void ModSelector::toggleAuto() { m_modButtonAuto->click(); }

void ModSelector::onCheckboxChange(CBaseUICheckbox *checkbox) {
    for(int i = 0; i < m_experimentalMods.size(); i++) {
        if(m_experimentalMods[i].element == checkbox) {
            if(m_experimentalMods[i].cvar != NULL) m_experimentalMods[i].cvar->setValue(checkbox->isChecked());

            // force mod update
            {
                if(osu->isInPlayMode()) osu->getSelectedBeatmap()->onModUpdate();
                osu->getSongBrowser()->recalculateStarsForSelectedBeatmap(true);
            }

            break;
        }
    }
}

void ModSelector::onResolutionChange(Vector2 newResolution) {
    setSize(newResolution);
    m_overrideSliderContainer->setSize(newResolution);
    m_experimentalContainer->setSizeY(newResolution.y + 1);

    updateLayout();
}
