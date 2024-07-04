#include "Skin.h"

#include <string.h>

#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "File.h"
#include "GameRules.h"
#include "LinuxEnvironment.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "SkinImage.h"
#include "SoundEngine.h"
#include "VolumeOverlay.h"
#include "miniz.h"

using namespace std;

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

ConVar osu2_sound_source_id("osu2_sound_source_id", 1, FCVAR_DEFAULT,
                            "which instance/player/client should play hitsounds (e.g. master top left is always 1)");

ConVar osu_skin_async("osu_skin_async", true, FCVAR_DEFAULT, "load in background without blocking");
ConVar osu_skin_hd("osu_skin_hd", true, FCVAR_DEFAULT, "load and use @2x versions of skin images, if available");
ConVar osu_skin_mipmaps(
    "osu_skin_mipmaps", false, FCVAR_DEFAULT,
    "generate mipmaps for every skin image (only useful on lower game resolutions, requires more vram)");
ConVar osu_skin_color_index_add("osu_skin_color_index_add", 0, FCVAR_DEFAULT);
ConVar osu_skin_animation_force("osu_skin_animation_force", false, FCVAR_DEFAULT);
ConVar osu_skin_use_skin_hitsounds(
    "osu_skin_use_skin_hitsounds", true, FCVAR_DEFAULT,
    "If enabled: Use skin's sound samples. If disabled: Use default skin's sound samples. For hitsounds only.");
ConVar osu_skin_force_hitsound_sample_set("osu_skin_force_hitsound_sample_set", 0, FCVAR_DEFAULT,
                                          "force a specific hitsound sample set to always be used regardless of what "
                                          "the beatmap says. 0 = disabled, 1 = normal, 2 = soft, 3 = drum.");
ConVar osu_skin_random("osu_skin_random", false, FCVAR_DEFAULT,
                       "select random skin from list on every skin load/reload");
ConVar osu_skin_random_elements("osu_skin_random_elements", false, FCVAR_DEFAULT,
                                "sElECt RanDOM sKIn eLemENTs FRoM ranDom SkINs");
ConVar osu_mod_fposu_sound_panning("osu_mod_fposu_sound_panning", false, FCVAR_DEFAULT, "see osu_sound_panning");
ConVar osu_mod_fps_sound_panning("osu_mod_fps_sound_panning", false, FCVAR_DEFAULT, "see osu_sound_panning");
ConVar osu_sound_panning("osu_sound_panning", true, FCVAR_DEFAULT,
                         "positional hitsound audio depending on the playfield position");
ConVar osu_sound_panning_multiplier("osu_sound_panning_multiplier", 1.0f, FCVAR_DEFAULT,
                                    "the final panning value is multiplied with this, e.g. if you want to reduce or "
                                    "increase the effect strength by a percentage");

ConVar osu_ignore_beatmap_combo_colors("osu_ignore_beatmap_combo_colors", false, FCVAR_DEFAULT);
ConVar osu_ignore_beatmap_sample_volume("osu_ignore_beatmap_sample_volume", false, FCVAR_DEFAULT);

Image *Skin::m_missingTexture = NULL;

ConVar *Skin::m_osu_skin_async = &osu_skin_async;
ConVar *Skin::m_osu_skin_hd = &osu_skin_hd;

ConVar *Skin::m_osu_skin_ref = NULL;
ConVar *Skin::m_osu_mod_fposu_ref = NULL;

void Skin::unpack(const char *filepath) {
    auto skin_name = env->getFileNameFromFilePath(filepath);
    debugLog("Extracting %s...\n", skin_name.c_str());
    skin_name.erase(skin_name.size() - 4);  // remove .osk extension

    auto skin_root = std::string(MCENGINE_DATA_DIR "skins/");
    skin_root.append(skin_name);
    skin_root.append("/");

    File file(filepath);

    mz_zip_archive zip = {0};
    mz_zip_archive_file_stat file_stat;
    mz_uint num_files = 0;

    if(!mz_zip_reader_init_mem(&zip, file.readFile(), file.getFileSize(), 0)) {
        debugLog("Failed to open .osk file\n");
        return;
    }

    num_files = mz_zip_reader_get_num_files(&zip);
    if(num_files <= 0) {
        debugLog(".osk file is empty!\n");
        mz_zip_reader_end(&zip);
        return;
    }

    if(!env->directoryExists(skin_root)) {
        env->createDirectory(skin_root);
    }

    for(mz_uint i = 0; i < num_files; i++) {
        if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
        if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

        auto folders = UString(file_stat.m_filename).split("/");
        std::string file_path = skin_root;
        for(auto folder : folders) {
            if(!env->directoryExists(file_path)) {
                env->createDirectory(file_path);
            }

            if(folder == UString("..")) {
                // Bro...
                goto skip_file;
            } else {
                file_path.append("/");
                file_path.append(folder.toUtf8());
            }
        }

        mz_zip_reader_extract_to_file(&zip, i, file_path.c_str(), 0);

    skip_file:;
        // When a file can't be extracted we just ignore it (as long as the archive is valid).
    }

    // Success
    mz_zip_reader_end(&zip);
}

Skin::Skin(UString name, std::string filepath, bool isDefaultSkin) {
    m_sName = name;
    m_sFilePath = filepath;
    m_bIsDefaultSkin = isDefaultSkin;

    m_bReady = false;

    // convar refs
    if(m_osu_skin_ref == NULL) m_osu_skin_ref = convar->getConVarByName("osu_skin");
    if(m_osu_mod_fposu_ref == NULL) m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");

    if(m_missingTexture == NULL) m_missingTexture = engine->getResourceManager()->getImage("MISSING_TEXTURE");

    // vars
    m_hitCircle = m_missingTexture;
    m_approachCircle = m_missingTexture;
    m_reverseArrow = m_missingTexture;

    m_default0 = m_missingTexture;
    m_default1 = m_missingTexture;
    m_default2 = m_missingTexture;
    m_default3 = m_missingTexture;
    m_default4 = m_missingTexture;
    m_default5 = m_missingTexture;
    m_default6 = m_missingTexture;
    m_default7 = m_missingTexture;
    m_default8 = m_missingTexture;
    m_default9 = m_missingTexture;

    m_score0 = m_missingTexture;
    m_score1 = m_missingTexture;
    m_score2 = m_missingTexture;
    m_score3 = m_missingTexture;
    m_score4 = m_missingTexture;
    m_score5 = m_missingTexture;
    m_score6 = m_missingTexture;
    m_score7 = m_missingTexture;
    m_score8 = m_missingTexture;
    m_score9 = m_missingTexture;
    m_scoreX = m_missingTexture;
    m_scorePercent = m_missingTexture;
    m_scoreDot = m_missingTexture;

    m_combo0 = m_missingTexture;
    m_combo1 = m_missingTexture;
    m_combo2 = m_missingTexture;
    m_combo3 = m_missingTexture;
    m_combo4 = m_missingTexture;
    m_combo5 = m_missingTexture;
    m_combo6 = m_missingTexture;
    m_combo7 = m_missingTexture;
    m_combo8 = m_missingTexture;
    m_combo9 = m_missingTexture;
    m_comboX = m_missingTexture;

    m_playWarningArrow = m_missingTexture;
    m_circularmetre = m_missingTexture;

    m_particle50 = m_missingTexture;
    m_particle100 = m_missingTexture;
    m_particle300 = m_missingTexture;

    m_sliderGradient = m_missingTexture;
    m_sliderScorePoint = m_missingTexture;
    m_sliderStartCircle = m_missingTexture;
    m_sliderStartCircleOverlay = m_missingTexture;
    m_sliderEndCircle = m_missingTexture;
    m_sliderEndCircleOverlay = m_missingTexture;

    m_spinnerBackground = m_missingTexture;
    m_spinnerCircle = m_missingTexture;
    m_spinnerApproachCircle = m_missingTexture;
    m_spinnerBottom = m_missingTexture;
    m_spinnerMiddle = m_missingTexture;
    m_spinnerMiddle2 = m_missingTexture;
    m_spinnerTop = m_missingTexture;
    m_spinnerSpin = m_missingTexture;
    m_spinnerClear = m_missingTexture;

    m_defaultCursor = m_missingTexture;
    m_cursor = m_missingTexture;
    m_cursorMiddle = m_missingTexture;
    m_cursorTrail = m_missingTexture;
    m_cursorRipple = m_missingTexture;

    m_pauseContinue = m_missingTexture;
    m_pauseReplay = m_missingTexture;
    m_pauseRetry = m_missingTexture;
    m_pauseBack = m_missingTexture;
    m_pauseOverlay = m_missingTexture;
    m_failBackground = m_missingTexture;
    m_unpause = m_missingTexture;

    m_buttonLeft = m_missingTexture;
    m_buttonMiddle = m_missingTexture;
    m_buttonRight = m_missingTexture;
    m_defaultButtonLeft = m_missingTexture;
    m_defaultButtonMiddle = m_missingTexture;
    m_defaultButtonRight = m_missingTexture;

    m_songSelectTop = m_missingTexture;
    m_songSelectBottom = m_missingTexture;
    m_menuButtonBackground = m_missingTexture;
    m_star = m_missingTexture;
    m_rankingPanel = m_missingTexture;
    m_rankingGraph = m_missingTexture;
    m_rankingTitle = m_missingTexture;
    m_rankingMaxCombo = m_missingTexture;
    m_rankingAccuracy = m_missingTexture;
    m_rankingA = m_missingTexture;
    m_rankingB = m_missingTexture;
    m_rankingC = m_missingTexture;
    m_rankingD = m_missingTexture;
    m_rankingS = m_missingTexture;
    m_rankingSH = m_missingTexture;
    m_rankingX = m_missingTexture;
    m_rankingXH = m_missingTexture;

    m_beatmapImportSpinner = m_missingTexture;
    m_loadingSpinner = m_missingTexture;
    m_circleEmpty = m_missingTexture;
    m_circleFull = m_missingTexture;
    m_seekTriangle = m_missingTexture;
    m_userIcon = m_missingTexture;
    m_backgroundCube = m_missingTexture;
    m_menuBackground = m_missingTexture;
    m_skybox = m_missingTexture;

    m_normalHitNormal = NULL;
    m_normalHitWhistle = NULL;
    m_normalHitFinish = NULL;
    m_normalHitClap = NULL;

    m_normalSliderTick = NULL;
    m_normalSliderSlide = NULL;
    m_normalSliderWhistle = NULL;

    m_softHitNormal = NULL;
    m_softHitWhistle = NULL;
    m_softHitFinish = NULL;
    m_softHitClap = NULL;

    m_softSliderTick = NULL;
    m_softSliderSlide = NULL;
    m_softSliderWhistle = NULL;

    m_drumHitNormal = NULL;
    m_drumHitWhistle = NULL;
    m_drumHitFinish = NULL;
    m_drumHitClap = NULL;

    m_drumSliderTick = NULL;
    m_drumSliderSlide = NULL;
    m_drumSliderWhistle = NULL;

    m_spinnerBonus = NULL;
    m_spinnerSpinSound = NULL;

    m_tooearly = NULL;
    m_toolate = NULL;

    m_combobreak = NULL;
    m_failsound = NULL;
    m_applause = NULL;
    m_menuHit = NULL;
    m_menuHover = NULL;
    m_checkOn = NULL;
    m_checkOff = NULL;
    m_shutter = NULL;
    m_sectionPassSound = NULL;
    m_sectionFailSound = NULL;

    m_spinnerApproachCircleColor = 0xffffffff;
    m_sliderBorderColor = 0xffffffff;
    m_sliderBallColor = 0xffffffff;  // NOTE: 0xff02aaff is a hardcoded special case for osu!'s default skin, but it
                                     // does not apply to user skins

    m_songSelectActiveText = 0xff000000;
    m_songSelectInactiveText = 0xffffffff;
    m_inputOverlayText = 0xff000000;

    // scaling
    m_bCursor2x = false;
    m_bCursorTrail2x = false;
    m_bCursorRipple2x = false;
    m_bApproachCircle2x = false;
    m_bReverseArrow2x = false;
    m_bHitCircle2x = false;
    m_bIsDefault02x = false;
    m_bIsDefault12x = false;
    m_bIsScore02x = false;
    m_bIsCombo02x = false;
    m_bSpinnerApproachCircle2x = false;
    m_bSpinnerBottom2x = false;
    m_bSpinnerCircle2x = false;
    m_bSpinnerTop2x = false;
    m_bSpinnerMiddle2x = false;
    m_bSpinnerMiddle22x = false;
    m_bSliderScorePoint2x = false;
    m_bSliderStartCircle2x = false;
    m_bSliderEndCircle2x = false;

    m_bCircularmetre2x = false;

    m_bPauseContinue2x = false;

    m_bMenuButtonBackground2x = false;
    m_bStar2x = false;
    m_bRankingPanel2x = false;
    m_bRankingMaxCombo2x = false;
    m_bRankingAccuracy2x = false;
    m_bRankingA2x = false;
    m_bRankingB2x = false;
    m_bRankingC2x = false;
    m_bRankingD2x = false;
    m_bRankingS2x = false;
    m_bRankingSH2x = false;
    m_bRankingX2x = false;
    m_bRankingXH2x = false;

    // skin.ini
    m_fVersion = 1;
    m_fAnimationFramerate = 0.0f;
    m_bCursorCenter = true;
    m_bCursorRotate = true;
    m_bCursorExpand = true;

    m_bSliderBallFlip = true;
    m_bAllowSliderBallTint = false;
    m_iSliderStyle = 2;
    m_bHitCircleOverlayAboveNumber = true;
    m_bSliderTrackOverride = false;

    m_iHitCircleOverlap = 0;
    m_iScoreOverlap = 0;
    m_iComboOverlap = 0;

    // custom
    m_iSampleSet = 1;
    m_iSampleVolume = (int)(convar->getConVarByName("osu_volume_effects")->getFloat() * 100.0f);

    m_bIsRandom = osu_skin_random.getBool();
    m_bIsRandomElements = osu_skin_random_elements.getBool();

    // load all files
    load();

    // convar callbacks
    osu_ignore_beatmap_sample_volume.setCallback(
        fastdelegate::MakeDelegate(this, &Skin::onIgnoreBeatmapSampleVolumeChange));
}

Skin::~Skin() {
    for(int i = 0; i < m_resources.size(); i++) {
        if(m_resources[i] != (Resource *)m_missingTexture)
            engine->getResourceManager()->destroyResource(m_resources[i]);
    }
    m_resources.clear();

    for(int i = 0; i < m_images.size(); i++) {
        delete m_images[i];
    }
    m_images.clear();

    m_sounds.clear();

    m_filepathsForExport.clear();
}

void Skin::update() {
    // tasks which have to be run after async loading finishes
    if(!m_bReady && isReady()) {
        m_bReady = true;

        // force effect volume update
        osu->m_volumeOverlay->onEffectVolumeChange();
    }

    // shitty check to not animate while paused with hitobjects in background
    if(osu->isInPlayMode() && !osu->getSelectedBeatmap()->isPlaying() && !osu_skin_animation_force.getBool()) return;

    const bool useEngineTimeForAnimations = !osu->isInPlayMode();
    const long curMusicPos = osu->getSelectedBeatmap()->getCurMusicPosWithOffsets();
    for(int i = 0; i < m_images.size(); i++) {
        m_images[i]->update(m_animationSpeedMultiplier, useEngineTimeForAnimations, curMusicPos);
    }
}

bool Skin::isReady() {
    if(m_bReady) return true;

    for(int i = 0; i < m_resources.size(); i++) {
        if(engine->getResourceManager()->isLoadingResource(m_resources[i])) return false;
    }

    for(int i = 0; i < m_images.size(); i++) {
        if(!m_images[i]->isReady()) return false;
    }

    // (ready is set in update())

    return true;
}

void Skin::load() {
    // random skins
    {
        filepathsForRandomSkin.clear();
        if(m_bIsRandom || m_bIsRandomElements) {
            std::vector<std::string> skinNames;

            // regular skins
            {
                auto osu_folder = convar->getConVarByName("osu_folder")->getString();
                auto osu_folder_sub_skins = convar->getConVarByName("osu_folder_sub_skins")->getString();
                std::string skinFolder = osu_folder.toUtf8();
                skinFolder.append(osu_folder_sub_skins.toUtf8());
                std::vector<std::string> skinFolders = env->getFoldersInFolder(skinFolder);

                for(int i = 0; i < skinFolders.size(); i++) {
                    if(skinFolders[i].compare(".") == 0 ||
                       skinFolders[i].compare("..") == 0)  // is this universal in every file system? too lazy to check.
                                                           // should probably fix this in the engine and not here
                        continue;

                    std::string randomSkinFolder = skinFolder;
                    randomSkinFolder.append(skinFolders[i]);
                    randomSkinFolder.append("/");

                    filepathsForRandomSkin.push_back(randomSkinFolder);
                    skinNames.push_back(skinFolders[i]);
                }
            }

            if(m_bIsRandom && filepathsForRandomSkin.size() > 0) {
                const int randomIndex = std::rand() % min(filepathsForRandomSkin.size(), skinNames.size());

                m_sName = skinNames[randomIndex];
                m_sFilePath = filepathsForRandomSkin[randomIndex];
            }
        }
    }

    // spinner loading has top priority in async
    randomizeFilePath();
    { checkLoadImage(&m_loadingSpinner, "loading-spinner", "OSU_SKIN_LOADING_SPINNER"); }

    // and the cursor comes right after that
    randomizeFilePath();
    {
        checkLoadImage(&m_cursor, "cursor", "OSU_SKIN_CURSOR");
        checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE", true);
        checkLoadImage(&m_cursorTrail, "cursortrail", "OSU_SKIN_CURSORTRAIL");
        checkLoadImage(&m_cursorRipple, "cursor-ripple", "OSU_SKIN_CURSORRIPPLE");

        // special case: if fallback to default cursor, do load cursorMiddle
        if(m_cursor == engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT"))
            checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE");
    }

    // skin ini
    randomizeFilePath();
    m_sSkinIniFilePath = m_sFilePath;
    m_sSkinIniFilePath.append("skin.ini");
    bool parseSkinIni1Status = true;
    bool parseSkinIni2Status = true;
    if(!parseSkinINI(m_sSkinIniFilePath)) {
        parseSkinIni1Status = false;
        m_sSkinIniFilePath = MCENGINE_DATA_DIR "materials/default/skin.ini";
        parseSkinIni2Status = parseSkinINI(m_sSkinIniFilePath);
    }

    // default values, if none were loaded
    if(m_comboColors.size() == 0) {
        m_comboColors.push_back(COLOR(255, 255, 192, 0));
        m_comboColors.push_back(COLOR(255, 0, 202, 0));
        m_comboColors.push_back(COLOR(255, 18, 124, 255));
        m_comboColors.push_back(COLOR(255, 242, 24, 57));
    }

    // images
    randomizeFilePath();
    checkLoadImage(&m_hitCircle, "hitcircle", "OSU_SKIN_HITCIRCLE");
    m_hitCircleOverlay2 = createSkinImage("hitcircleoverlay", Vector2(128, 128), 64);
    m_hitCircleOverlay2->setAnimationFramerate(2);

    randomizeFilePath();
    checkLoadImage(&m_approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
    randomizeFilePath();
    checkLoadImage(&m_reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

    randomizeFilePath();
    m_followPoint2 = createSkinImage("followpoint", Vector2(16, 22), 64);

    randomizeFilePath();
    {
        std::string hitCirclePrefix = m_sHitCirclePrefix.length() > 0 ? m_sHitCirclePrefix : "default";
        std::string hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-0");
        checkLoadImage(&m_default0, hitCircleStringFinal, "OSU_SKIN_DEFAULT0");
        if(m_default0 == m_missingTexture)
            checkLoadImage(&m_default0, "default-0",
                           "OSU_SKIN_DEFAULT0");  // special cases: fallback to default skin hitcircle numbers if the
                                                  // defined prefix doesn't point to any valid files
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-1");
        checkLoadImage(&m_default1, hitCircleStringFinal, "OSU_SKIN_DEFAULT1");
        if(m_default1 == m_missingTexture) checkLoadImage(&m_default1, "default-1", "OSU_SKIN_DEFAULT1");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-2");
        checkLoadImage(&m_default2, hitCircleStringFinal, "OSU_SKIN_DEFAULT2");
        if(m_default2 == m_missingTexture) checkLoadImage(&m_default2, "default-2", "OSU_SKIN_DEFAULT2");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-3");
        checkLoadImage(&m_default3, hitCircleStringFinal, "OSU_SKIN_DEFAULT3");
        if(m_default3 == m_missingTexture) checkLoadImage(&m_default3, "default-3", "OSU_SKIN_DEFAULT3");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-4");
        checkLoadImage(&m_default4, hitCircleStringFinal, "OSU_SKIN_DEFAULT4");
        if(m_default4 == m_missingTexture) checkLoadImage(&m_default4, "default-4", "OSU_SKIN_DEFAULT4");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-5");
        checkLoadImage(&m_default5, hitCircleStringFinal, "OSU_SKIN_DEFAULT5");
        if(m_default5 == m_missingTexture) checkLoadImage(&m_default5, "default-5", "OSU_SKIN_DEFAULT5");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-6");
        checkLoadImage(&m_default6, hitCircleStringFinal, "OSU_SKIN_DEFAULT6");
        if(m_default6 == m_missingTexture) checkLoadImage(&m_default6, "default-6", "OSU_SKIN_DEFAULT6");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-7");
        checkLoadImage(&m_default7, hitCircleStringFinal, "OSU_SKIN_DEFAULT7");
        if(m_default7 == m_missingTexture) checkLoadImage(&m_default7, "default-7", "OSU_SKIN_DEFAULT7");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-8");
        checkLoadImage(&m_default8, hitCircleStringFinal, "OSU_SKIN_DEFAULT8");
        if(m_default8 == m_missingTexture) checkLoadImage(&m_default8, "default-8", "OSU_SKIN_DEFAULT8");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-9");
        checkLoadImage(&m_default9, hitCircleStringFinal, "OSU_SKIN_DEFAULT9");
        if(m_default9 == m_missingTexture) checkLoadImage(&m_default9, "default-9", "OSU_SKIN_DEFAULT9");
    }

    randomizeFilePath();
    {
        std::string scorePrefix = m_sScorePrefix.length() > 0 ? m_sScorePrefix : "score";
        std::string scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-0");
        checkLoadImage(&m_score0, scoreStringFinal, "OSU_SKIN_SCORE0");
        if(m_score0 == m_missingTexture)
            checkLoadImage(&m_score0, "score-0",
                           "OSU_SKIN_SCORE0");  // special cases: fallback to default skin score numbers if the defined
                                                // prefix doesn't point to any valid files
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-1");
        checkLoadImage(&m_score1, scoreStringFinal, "OSU_SKIN_SCORE1");
        if(m_score1 == m_missingTexture) checkLoadImage(&m_score1, "score-1", "OSU_SKIN_SCORE1");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-2");
        checkLoadImage(&m_score2, scoreStringFinal, "OSU_SKIN_SCORE2");
        if(m_score2 == m_missingTexture) checkLoadImage(&m_score2, "score-2", "OSU_SKIN_SCORE2");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-3");
        checkLoadImage(&m_score3, scoreStringFinal, "OSU_SKIN_SCORE3");
        if(m_score3 == m_missingTexture) checkLoadImage(&m_score3, "score-3", "OSU_SKIN_SCORE3");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-4");
        checkLoadImage(&m_score4, scoreStringFinal, "OSU_SKIN_SCORE4");
        if(m_score4 == m_missingTexture) checkLoadImage(&m_score4, "score-4", "OSU_SKIN_SCORE4");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-5");
        checkLoadImage(&m_score5, scoreStringFinal, "OSU_SKIN_SCORE5");
        if(m_score5 == m_missingTexture) checkLoadImage(&m_score5, "score-5", "OSU_SKIN_SCORE5");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-6");
        checkLoadImage(&m_score6, scoreStringFinal, "OSU_SKIN_SCORE6");
        if(m_score6 == m_missingTexture) checkLoadImage(&m_score6, "score-6", "OSU_SKIN_SCORE6");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-7");
        checkLoadImage(&m_score7, scoreStringFinal, "OSU_SKIN_SCORE7");
        if(m_score7 == m_missingTexture) checkLoadImage(&m_score7, "score-7", "OSU_SKIN_SCORE7");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-8");
        checkLoadImage(&m_score8, scoreStringFinal, "OSU_SKIN_SCORE8");
        if(m_score8 == m_missingTexture) checkLoadImage(&m_score8, "score-8", "OSU_SKIN_SCORE8");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-9");
        checkLoadImage(&m_score9, scoreStringFinal, "OSU_SKIN_SCORE9");
        if(m_score9 == m_missingTexture) checkLoadImage(&m_score9, "score-9", "OSU_SKIN_SCORE9");

        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-x");
        checkLoadImage(&m_scoreX, scoreStringFinal, "OSU_SKIN_SCOREX");
        // if (m_scoreX == m_missingTexture) checkLoadImage(&m_scoreX, "score-x", "OSU_SKIN_SCOREX"); // special case:
        // ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are simply not
        // drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-percent");
        checkLoadImage(&m_scorePercent, scoreStringFinal, "OSU_SKIN_SCOREPERCENT");
        // if (m_scorePercent == m_missingTexture) checkLoadImage(&m_scorePercent, "score-percent",
        // "OSU_SKIN_SCOREPERCENT"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing
        // extraneous things like the X are simply not drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-dot");
        checkLoadImage(&m_scoreDot, scoreStringFinal, "OSU_SKIN_SCOREDOT");
        // if (m_scoreDot == m_missingTexture) checkLoadImage(&m_scoreDot, "score-dot", "OSU_SKIN_SCOREDOT"); // special
        // case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are
        // simply not drawn
    }

    randomizeFilePath();
    {
        std::string comboPrefix = m_sComboPrefix.length() > 0
                                      ? m_sComboPrefix
                                      : "score";  // yes, "score" is the default value for the combo prefix
        std::string comboStringFinal = comboPrefix;
        comboStringFinal.append("-0");
        checkLoadImage(&m_combo0, comboStringFinal, "OSU_SKIN_COMBO0");
        if(m_combo0 == m_missingTexture)
            checkLoadImage(&m_combo0, "score-0",
                           "OSU_SKIN_COMBO0");  // special cases: fallback to default skin combo numbers if the defined
                                                // prefix doesn't point to any valid files
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-1");
        checkLoadImage(&m_combo1, comboStringFinal, "OSU_SKIN_COMBO1");
        if(m_combo1 == m_missingTexture) checkLoadImage(&m_combo1, "score-1", "OSU_SKIN_COMBO1");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-2");
        checkLoadImage(&m_combo2, comboStringFinal, "OSU_SKIN_COMBO2");
        if(m_combo2 == m_missingTexture) checkLoadImage(&m_combo2, "score-2", "OSU_SKIN_COMBO2");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-3");
        checkLoadImage(&m_combo3, comboStringFinal, "OSU_SKIN_COMBO3");
        if(m_combo3 == m_missingTexture) checkLoadImage(&m_combo3, "score-3", "OSU_SKIN_COMBO3");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-4");
        checkLoadImage(&m_combo4, comboStringFinal, "OSU_SKIN_COMBO4");
        if(m_combo4 == m_missingTexture) checkLoadImage(&m_combo4, "score-4", "OSU_SKIN_COMBO4");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-5");
        checkLoadImage(&m_combo5, comboStringFinal, "OSU_SKIN_COMBO5");
        if(m_combo5 == m_missingTexture) checkLoadImage(&m_combo5, "score-5", "OSU_SKIN_COMBO5");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-6");
        checkLoadImage(&m_combo6, comboStringFinal, "OSU_SKIN_COMBO6");
        if(m_combo6 == m_missingTexture) checkLoadImage(&m_combo6, "score-6", "OSU_SKIN_COMBO6");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-7");
        checkLoadImage(&m_combo7, comboStringFinal, "OSU_SKIN_COMBO7");
        if(m_combo7 == m_missingTexture) checkLoadImage(&m_combo7, "score-7", "OSU_SKIN_COMBO7");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-8");
        checkLoadImage(&m_combo8, comboStringFinal, "OSU_SKIN_COMBO8");
        if(m_combo8 == m_missingTexture) checkLoadImage(&m_combo8, "score-8", "OSU_SKIN_COMBO8");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-9");
        checkLoadImage(&m_combo9, comboStringFinal, "OSU_SKIN_COMBO9");
        if(m_combo9 == m_missingTexture) checkLoadImage(&m_combo9, "score-9", "OSU_SKIN_COMBO9");

        comboStringFinal = comboPrefix;
        comboStringFinal.append("-x");
        checkLoadImage(&m_comboX, comboStringFinal, "OSU_SKIN_COMBOX");
        // if (m_comboX == m_missingTexture) m_comboX = m_scoreX; // special case: ComboPrefix'd skins don't get default
        // fallbacks, instead missing extraneous things like the X are simply not drawn
    }

    randomizeFilePath();
    m_playSkip = createSkinImage("play-skip", Vector2(193, 147), 94);
    randomizeFilePath();
    checkLoadImage(&m_playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
    m_playWarningArrow2 = createSkinImage("play-warningarrow", Vector2(167, 129), 128);
    randomizeFilePath();
    checkLoadImage(&m_circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");
    randomizeFilePath();
    m_scorebarBg = createSkinImage("scorebar-bg", Vector2(695, 44), 27.5f);
    m_scorebarColour = createSkinImage("scorebar-colour", Vector2(645, 10), 6.25f);
    m_scorebarMarker = createSkinImage("scorebar-marker", Vector2(24, 24), 15.0f);
    m_scorebarKi = createSkinImage("scorebar-ki", Vector2(116, 116), 72.0f);
    m_scorebarKiDanger = createSkinImage("scorebar-kidanger", Vector2(116, 116), 72.0f);
    m_scorebarKiDanger2 = createSkinImage("scorebar-kidanger2", Vector2(116, 116), 72.0f);
    randomizeFilePath();
    m_sectionPassImage = createSkinImage("section-pass", Vector2(650, 650), 400.0f);
    randomizeFilePath();
    m_sectionFailImage = createSkinImage("section-fail", Vector2(650, 650), 400.0f);
    randomizeFilePath();
    m_inputoverlayBackground = createSkinImage("inputoverlay-background", Vector2(193, 55), 34.25f);
    m_inputoverlayKey = createSkinImage("inputoverlay-key", Vector2(43, 46), 26.75f);

    randomizeFilePath();
    m_hit0 = createSkinImage("hit0", Vector2(128, 128), 42);
    m_hit0->setAnimationFramerate(60);
    m_hit50 = createSkinImage("hit50", Vector2(128, 128), 42);
    m_hit50->setAnimationFramerate(60);
    m_hit50g = createSkinImage("hit50g", Vector2(128, 128), 42);
    m_hit50g->setAnimationFramerate(60);
    m_hit50k = createSkinImage("hit50k", Vector2(128, 128), 42);
    m_hit50k->setAnimationFramerate(60);
    m_hit100 = createSkinImage("hit100", Vector2(128, 128), 42);
    m_hit100->setAnimationFramerate(60);
    m_hit100g = createSkinImage("hit100g", Vector2(128, 128), 42);
    m_hit100g->setAnimationFramerate(60);
    m_hit100k = createSkinImage("hit100k", Vector2(128, 128), 42);
    m_hit100k->setAnimationFramerate(60);
    m_hit300 = createSkinImage("hit300", Vector2(128, 128), 42);
    m_hit300->setAnimationFramerate(60);
    m_hit300g = createSkinImage("hit300g", Vector2(128, 128), 42);
    m_hit300g->setAnimationFramerate(60);
    m_hit300k = createSkinImage("hit300k", Vector2(128, 128), 42);
    m_hit300k->setAnimationFramerate(60);

    randomizeFilePath();
    checkLoadImage(&m_particle50, "particle50", "OSU_SKIN_PARTICLE50", true);
    checkLoadImage(&m_particle100, "particle100", "OSU_SKIN_PARTICLE100", true);
    checkLoadImage(&m_particle300, "particle300", "OSU_SKIN_PARTICLE300", true);

    randomizeFilePath();
    checkLoadImage(&m_sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
    randomizeFilePath();
    m_sliderb = createSkinImage("sliderb", Vector2(128, 128), 64, false, "");
    m_sliderb->setAnimationFramerate(/*45.0f*/ 50.0f);
    randomizeFilePath();
    checkLoadImage(&m_sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
    randomizeFilePath();
    m_sliderFollowCircle2 = createSkinImage("sliderfollowcircle", Vector2(259, 259), 64);
    randomizeFilePath();
    checkLoadImage(
        &m_sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE",
        !m_bIsDefaultSkin);  // !m_bIsDefaultSkin ensures that default doesn't override user, in these special cases
    m_sliderStartCircle2 = createSkinImage("sliderstartcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
    checkLoadImage(&m_sliderStartCircleOverlay, "sliderstartcircleoverlay", "OSU_SKIN_SLIDERSTARTCIRCLEOVERLAY",
                   !m_bIsDefaultSkin);
    m_sliderStartCircleOverlay2 = createSkinImage("sliderstartcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
    m_sliderStartCircleOverlay2->setAnimationFramerate(2);
    randomizeFilePath();
    checkLoadImage(&m_sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE", !m_bIsDefaultSkin);
    m_sliderEndCircle2 = createSkinImage("sliderendcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
    checkLoadImage(&m_sliderEndCircleOverlay, "sliderendcircleoverlay", "OSU_SKIN_SLIDERENDCIRCLEOVERLAY",
                   !m_bIsDefaultSkin);
    m_sliderEndCircleOverlay2 = createSkinImage("sliderendcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
    m_sliderEndCircleOverlay2->setAnimationFramerate(2);

    randomizeFilePath();
    checkLoadImage(&m_spinnerBackground, "spinner-background", "OSU_SKIN_SPINNERBACKGROUND");
    checkLoadImage(&m_spinnerCircle, "spinner-circle", "OSU_SKIN_SPINNERCIRCLE");
    checkLoadImage(&m_spinnerApproachCircle, "spinner-approachcircle", "OSU_SKIN_SPINNERAPPROACHCIRCLE");
    checkLoadImage(&m_spinnerBottom, "spinner-bottom", "OSU_SKIN_SPINNERBOTTOM");
    checkLoadImage(&m_spinnerMiddle, "spinner-middle", "OSU_SKIN_SPINNERMIDDLE");
    checkLoadImage(&m_spinnerMiddle2, "spinner-middle2", "OSU_SKIN_SPINNERMIDDLE2");
    checkLoadImage(&m_spinnerTop, "spinner-top", "OSU_SKIN_SPINNERTOP");
    checkLoadImage(&m_spinnerSpin, "spinner-spin", "OSU_SKIN_SPINNERSPIN");
    checkLoadImage(&m_spinnerClear, "spinner-clear", "OSU_SKIN_SPINNERCLEAR");

    {
        // cursor loading was here, moved up to improve async usability
    }

    randomizeFilePath();
    m_selectionModEasy = createSkinImage("selection-mod-easy", Vector2(68, 66), 38);
    m_selectionModNoFail = createSkinImage("selection-mod-nofail", Vector2(68, 66), 38);
    m_selectionModHalfTime = createSkinImage("selection-mod-halftime", Vector2(68, 66), 38);
    m_selectionModHardRock = createSkinImage("selection-mod-hardrock", Vector2(68, 66), 38);
    m_selectionModSuddenDeath = createSkinImage("selection-mod-suddendeath", Vector2(68, 66), 38);
    m_selectionModPerfect = createSkinImage("selection-mod-perfect", Vector2(68, 66), 38);
    m_selectionModDoubleTime = createSkinImage("selection-mod-doubletime", Vector2(68, 66), 38);
    m_selectionModNightCore = createSkinImage("selection-mod-nightcore", Vector2(68, 66), 38);
    m_selectionModDayCore = createSkinImage("selection-mod-daycore", Vector2(68, 66), 38);
    m_selectionModHidden = createSkinImage("selection-mod-hidden", Vector2(68, 66), 38);
    m_selectionModFlashlight = createSkinImage("selection-mod-flashlight", Vector2(68, 66), 38);
    m_selectionModRelax = createSkinImage("selection-mod-relax", Vector2(68, 66), 38);
    m_selectionModAutopilot = createSkinImage("selection-mod-relax2", Vector2(68, 66), 38);
    m_selectionModSpunOut = createSkinImage("selection-mod-spunout", Vector2(68, 66), 38);
    m_selectionModAutoplay = createSkinImage("selection-mod-autoplay", Vector2(68, 66), 38);
    m_selectionModNightmare = createSkinImage("selection-mod-nightmare", Vector2(68, 66), 38);
    m_selectionModTarget = createSkinImage("selection-mod-target", Vector2(68, 66), 38);
    m_selectionModScorev2 = createSkinImage("selection-mod-scorev2", Vector2(68, 66), 38);
    m_selectionModTD = createSkinImage("selection-mod-touchdevice", Vector2(68, 66), 38);
    m_selectionModCinema = createSkinImage("selection-mod-cinema", Vector2(68, 66), 38);

    randomizeFilePath();
    checkLoadImage(&m_pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
    checkLoadImage(&m_pauseReplay, "pause-replay", "OSU_SKIN_PAUSE_REPLAY");
    checkLoadImage(&m_pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
    checkLoadImage(&m_pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
    checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY");
    if(m_pauseOverlay == m_missingTexture)
        checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY", true, "jpg");
    checkLoadImage(&m_failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND");
    if(m_failBackground == m_missingTexture)
        checkLoadImage(&m_failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND", true, "jpg");
    checkLoadImage(&m_unpause, "unpause", "OSU_SKIN_UNPAUSE");

    randomizeFilePath();
    checkLoadImage(&m_buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
    checkLoadImage(&m_buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
    checkLoadImage(&m_buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
    randomizeFilePath();
    m_menuBackImg = createSkinImage("menu-back", Vector2(225, 87), 54);
    randomizeFilePath();
    m_selectionMode = createSkinImage("selection-mode", Vector2(90, 90),
                                      38);  // NOTE: should actually be Vector2(88, 90), but slightly overscale to
                                            // make most skins fit better on the bottombar blue line
    m_selectionModeOver = createSkinImage("selection-mode-over", Vector2(88, 90), 38);
    m_selectionMods = createSkinImage("selection-mods", Vector2(74, 90), 38);
    m_selectionModsOver = createSkinImage("selection-mods-over", Vector2(74, 90), 38);
    m_selectionRandom = createSkinImage("selection-random", Vector2(74, 90), 38);
    m_selectionRandomOver = createSkinImage("selection-random-over", Vector2(74, 90), 38);
    m_selectionOptions = createSkinImage("selection-options", Vector2(74, 90), 38);
    m_selectionOptionsOver = createSkinImage("selection-options-over", Vector2(74, 90), 38);

    randomizeFilePath();
    checkLoadImage(&m_songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
    checkLoadImage(&m_songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
    randomizeFilePath();
    checkLoadImage(&m_menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");
    m_menuButtonBackground2 = createSkinImage("menu-button-background", Vector2(699, 103), 64.0f);
    randomizeFilePath();
    checkLoadImage(&m_star, "star", "OSU_SKIN_STAR");

    randomizeFilePath();
    checkLoadImage(&m_rankingPanel, "ranking-panel", "OSU_SKIN_RANKING_PANEL");
    checkLoadImage(&m_rankingGraph, "ranking-graph", "OSU_SKIN_RANKING_GRAPH");
    checkLoadImage(&m_rankingTitle, "ranking-title", "OSU_SKIN_RANKING_TITLE");
    checkLoadImage(&m_rankingMaxCombo, "ranking-maxcombo", "OSU_SKIN_RANKING_MAXCOMBO");
    checkLoadImage(&m_rankingAccuracy, "ranking-accuracy", "OSU_SKIN_RANKING_ACCURACY");

    checkLoadImage(&m_rankingA, "ranking-A", "OSU_SKIN_RANKING_A");
    checkLoadImage(&m_rankingB, "ranking-B", "OSU_SKIN_RANKING_B");
    checkLoadImage(&m_rankingC, "ranking-C", "OSU_SKIN_RANKING_C");
    checkLoadImage(&m_rankingD, "ranking-D", "OSU_SKIN_RANKING_D");
    checkLoadImage(&m_rankingS, "ranking-S", "OSU_SKIN_RANKING_S");
    checkLoadImage(&m_rankingSH, "ranking-SH", "OSU_SKIN_RANKING_SH");
    checkLoadImage(&m_rankingX, "ranking-X", "OSU_SKIN_RANKING_X");
    checkLoadImage(&m_rankingXH, "ranking-XH", "OSU_SKIN_RANKING_XH");

    m_rankingAsmall = createSkinImage("ranking-A-small", Vector2(34, 38), 128);
    m_rankingBsmall = createSkinImage("ranking-B-small", Vector2(33, 38), 128);
    m_rankingCsmall = createSkinImage("ranking-C-small", Vector2(30, 38), 128);
    m_rankingDsmall = createSkinImage("ranking-D-small", Vector2(33, 38), 128);
    m_rankingSsmall = createSkinImage("ranking-S-small", Vector2(31, 38), 128);
    m_rankingSHsmall = createSkinImage("ranking-SH-small", Vector2(31, 38), 128);
    m_rankingXsmall = createSkinImage("ranking-X-small", Vector2(34, 40), 128);
    m_rankingXHsmall = createSkinImage("ranking-XH-small", Vector2(34, 41), 128);

    m_rankingPerfect = createSkinImage("ranking-perfect", Vector2(478, 150), 128);

    randomizeFilePath();
    checkLoadImage(&m_beatmapImportSpinner, "beatmapimport-spinner", "OSU_SKIN_BEATMAP_IMPORT_SPINNER");
    {
        // loading spinner load was here, moved up to improve async usability
    }
    checkLoadImage(&m_circleEmpty, "circle-empty", "OSU_SKIN_CIRCLE_EMPTY");
    checkLoadImage(&m_circleFull, "circle-full", "OSU_SKIN_CIRCLE_FULL");
    randomizeFilePath();
    checkLoadImage(&m_seekTriangle, "seektriangle", "OSU_SKIN_SEEKTRIANGLE");
    randomizeFilePath();
    checkLoadImage(&m_userIcon, "user-icon", "OSU_SKIN_USER_ICON");
    randomizeFilePath();
    checkLoadImage(&m_backgroundCube, "backgroundcube", "OSU_SKIN_FPOSU_BACKGROUNDCUBE", false, "png",
                   true);  // force mipmaps
    randomizeFilePath();
    checkLoadImage(&m_menuBackground, "menu-background", "OSU_SKIN_MENU_BACKGROUND", false, "jpg");
    randomizeFilePath();
    checkLoadImage(&m_skybox, "skybox", "OSU_SKIN_FPOSU_3D_SKYBOX");

    // sounds

    // samples
    checkLoadSound(&m_normalHitNormal, "normal-hitnormal", "OSU_SKIN_NORMALHITNORMAL_SND", true, true, false, true,
                   0.8f);
    checkLoadSound(&m_normalHitWhistle, "normal-hitwhistle", "OSU_SKIN_NORMALHITWHISTLE_SND", true, true, false, true,
                   0.85f);
    checkLoadSound(&m_normalHitFinish, "normal-hitfinish", "OSU_SKIN_NORMALHITFINISH_SND", true, true);
    checkLoadSound(&m_normalHitClap, "normal-hitclap", "OSU_SKIN_NORMALHITCLAP_SND", true, true, false, true, 0.85f);

    checkLoadSound(&m_normalSliderTick, "normal-slidertick", "OSU_SKIN_NORMALSLIDERTICK_SND", true, true);
    checkLoadSound(&m_normalSliderSlide, "normal-sliderslide", "OSU_SKIN_NORMALSLIDERSLIDE_SND", false, true, true);
    checkLoadSound(&m_normalSliderWhistle, "normal-sliderwhistle", "OSU_SKIN_NORMALSLIDERWHISTLE_SND", true, true);

    checkLoadSound(&m_softHitNormal, "soft-hitnormal", "OSU_SKIN_SOFTHITNORMAL_SND", true, true, false, true, 0.8f);
    checkLoadSound(&m_softHitWhistle, "soft-hitwhistle", "OSU_SKIN_SOFTHITWHISTLE_SND", true, true, false, true, 0.85f);
    checkLoadSound(&m_softHitFinish, "soft-hitfinish", "OSU_SKIN_SOFTHITFINISH_SND", true, true);
    checkLoadSound(&m_softHitClap, "soft-hitclap", "OSU_SKIN_SOFTHITCLAP_SND", true, true, false, true, 0.85f);

    checkLoadSound(&m_softSliderTick, "soft-slidertick", "OSU_SKIN_SOFTSLIDERTICK_SND", true, true);
    checkLoadSound(&m_softSliderSlide, "soft-sliderslide", "OSU_SKIN_SOFTSLIDERSLIDE_SND", false, true, true);
    checkLoadSound(&m_softSliderWhistle, "soft-sliderwhistle", "OSU_SKIN_SOFTSLIDERWHISTLE_SND", true, true);

    checkLoadSound(&m_drumHitNormal, "drum-hitnormal", "OSU_SKIN_DRUMHITNORMAL_SND", true, true, false, true, 0.8f);
    checkLoadSound(&m_drumHitWhistle, "drum-hitwhistle", "OSU_SKIN_DRUMHITWHISTLE_SND", true, true, false, true, 0.85f);
    checkLoadSound(&m_drumHitFinish, "drum-hitfinish", "OSU_SKIN_DRUMHITFINISH_SND", true, true);
    checkLoadSound(&m_drumHitClap, "drum-hitclap", "OSU_SKIN_DRUMHITCLAP_SND", true, true, false, true, 0.85f);

    checkLoadSound(&m_drumSliderTick, "drum-slidertick", "OSU_SKIN_DRUMSLIDERTICK_SND", true, true);
    checkLoadSound(&m_drumSliderSlide, "drum-sliderslide", "OSU_SKIN_DRUMSLIDERSLIDE_SND", false, true, true);
    checkLoadSound(&m_drumSliderWhistle, "drum-sliderwhistle", "OSU_SKIN_DRUMSLIDERWHISTLE_SND", true, true);

    checkLoadSound(&m_spinnerBonus, "spinnerbonus", "OSU_SKIN_SPINNERBONUS_SND", true, true);
    checkLoadSound(&m_spinnerSpinSound, "spinnerspin", "OSU_SKIN_SPINNERSPIN_SND", false, true, true);

    checkLoadSound(&m_tooearly, "tooearly", "OSU_SKIN_TOOEARLY_SND", true, true, false, false, 0.8f);
    checkLoadSound(&m_toolate, "toolate", "OSU_SKIN_TOOLATE_SND", true, true, false, false, 0.85f);

    // others
    checkLoadSound(&m_combobreak, "combobreak", "OSU_SKIN_COMBOBREAK_SND", true, true);
    checkLoadSound(&m_failsound, "failsound", "OSU_SKIN_FAILSOUND_SND");
    checkLoadSound(&m_applause, "applause", "OSU_SKIN_APPLAUSE_SND");
    checkLoadSound(&m_menuHit, "menuhit", "OSU_SKIN_MENUHIT_SND", true, true);
    checkLoadSound(&m_menuHover, "menuclick", "OSU_SKIN_MENUCLICK_SND", true, true);
    checkLoadSound(&m_checkOn, "check-on", "OSU_SKIN_CHECKON_SND", true, true);
    checkLoadSound(&m_checkOff, "check-off", "OSU_SKIN_CHECKOFF_SND", true, true);
    checkLoadSound(&m_shutter, "shutter", "OSU_SKIN_SHUTTER_SND", true, true);
    checkLoadSound(&m_sectionPassSound, "sectionpass", "OSU_SKIN_SECTIONPASS_SND");
    checkLoadSound(&m_sectionFailSound, "sectionfail", "OSU_SKIN_SECTIONFAIL_SND");

    // UI feedback
    checkLoadSound(&m_messageSent, "key-confirm", "OSU_SKIN_MESSAGE_SENT_SND", true, true, false);
    checkLoadSound(&m_deletingText, "key-delete", "OSU_SKIN_DELETING_TEXT_SND", true, true, false);
    checkLoadSound(&m_movingTextCursor, "key-movement", "OSU_MOVING_TEXT_CURSOR_SND", true, true, false);
    checkLoadSound(&m_typing1, "key-press-1", "OSU_TYPING_1_SND", true, true, false);
    checkLoadSound(&m_typing2, "key-press-2", "OSU_TYPING_2_SND", true, true, false, false);
    checkLoadSound(&m_typing3, "key-press-3", "OSU_TYPING_3_SND", true, true, false, false);
    checkLoadSound(&m_typing4, "key-press-4", "OSU_TYPING_4_SND", true, true, false, false);
    checkLoadSound(&m_menuBack, "menuback", "OSU_MENU_BACK_SND", true, true, false, false);
    checkLoadSound(&m_closeChatTab, "click-close", "OSU_CLOSE_CHAT_TAB_SND", true, true, false, false);
    checkLoadSound(&m_clickButton, "click-short-confirm", "OSU_CLICK_BUTTON_SND", true, true, false, false);
    checkLoadSound(&m_hoverButton, "click-short", "OSU_HOVER_BUTTON_SND", true, true, false, false);
    checkLoadSound(&m_backButtonClick, "back-button-click", "OSU_BACK_BUTTON_CLICK_SND", true, true, false, false);
    checkLoadSound(&m_backButtonHover, "back-button-hover", "OSU_BACK_BUTTON_HOVER_SND", true, true, false, false);
    checkLoadSound(&m_clickMainMenuCube, "menu-play-click", "OSU_CLICK_MAIN_MENU_CUBE_SND", true, true, false, false);
    checkLoadSound(&m_hoverMainMenuCube, "menu-play-hover", "OSU_HOVER_MAIN_MENU_CUBE_SND", true, true, false, false);
    checkLoadSound(&m_clickSingleplayer, "menu-freeplay-click", "OSU_CLICK_SINGLEPLAYER_SND", true, true, false, false);
    checkLoadSound(&m_hoverSingleplayer, "menu-freeplay-hover", "OSU_HOVER_SINGLEPLAYER_SND", true, true, false, false);
    checkLoadSound(&m_clickMultiplayer, "menu-multiplayer-click", "OSU_CLICK_MULTIPLAYER_SND", true, true, false,
                   false);
    checkLoadSound(&m_hoverMultiplayer, "menu-multiplayer-hover", "OSU_HOVER_MULTIPLAYER_SND", true, true, false,
                   false);
    checkLoadSound(&m_clickOptions, "menu-options-click", "OSU_CLICK_OPTIONS_SND", true, true, false, false);
    checkLoadSound(&m_hoverOptions, "menu-options-hover", "OSU_HOVER_OPTIONS_SND", true, true, false, false);
    checkLoadSound(&m_clickExit, "menu-exit-click", "OSU_CLICK_EXIT_SND", true, true, false, false);
    checkLoadSound(&m_hoverExit, "menu-exit-hover", "OSU_HOVER_EXIT_SND", true, true, false, false);
    checkLoadSound(&m_expand, "select-expand", "OSU_EXPAND_SND", true, true, false);
    checkLoadSound(&m_selectDifficulty, "select-difficulty", "OSU_SELECT_DIFFICULTY_SND", true, true, false, false);
    checkLoadSound(&m_sliderbar, "sliderbar", "OSU_DRAG_SLIDER_SND", true, true, false);
    checkLoadSound(&m_matchConfirm, "match-confirm", "OSU_ALL_PLAYERS_READY_SND", true, true, false);
    checkLoadSound(&m_roomJoined, "match-join", "OSU_ROOM_JOINED_SND", true, true, false);
    checkLoadSound(&m_roomQuit, "match-leave", "OSU_ROOM_QUIT_SND", true, true, false);
    checkLoadSound(&m_roomNotReady, "match-notready", "OSU_ROOM_NOT_READY_SND", true, true, false);
    checkLoadSound(&m_roomReady, "match-ready", "OSU_ROOM_READY_SND", true, true, false);
    checkLoadSound(&m_matchStart, "match-start", "OSU_MATCH_START_SND", true, true, false);

    checkLoadSound(&m_pauseLoop, "pause-loop", "OSU_PAUSE_LOOP_SND", false, false, true, true);
    checkLoadSound(&m_pauseHover, "pause-hover", "OSU_PAUSE_HOVER_SND", true, true, false, false);
    checkLoadSound(&m_clickPauseBack, "pause-back-click", "OSU_CLICK_QUIT_SONG_SND", true, true, false, false);
    checkLoadSound(&m_hoverPauseBack, "pause-back-hover", "OSU_HOVER_QUIT_SONG_SND", true, true, false, false);
    checkLoadSound(&m_clickPauseContinue, "pause-continue-click", "OSU_CLICK_RESUME_SONG_SND", true, true, false,
                   false);
    checkLoadSound(&m_hoverPauseContinue, "pause-continue-hover", "OSU_HOVER_RESUME_SONG_SND", true, true, false,
                   false);
    checkLoadSound(&m_clickPauseRetry, "pause-retry-click", "OSU_CLICK_RETRY_SONG_SND", true, true, false, false);
    checkLoadSound(&m_hoverPauseRetry, "pause-retry-hover", "OSU_HOVER_RETRY_SONG_SND", true, true, false, false);

    if(m_clickButton == NULL) m_clickButton = m_menuHit;
    if(m_hoverButton == NULL) m_hoverButton = m_menuHover;
    if(m_pauseHover == NULL) m_pauseHover = m_hoverButton;
    if(m_selectDifficulty == NULL) m_selectDifficulty = m_clickButton;
    if(m_typing2 == NULL) m_typing2 = m_typing1;
    if(m_typing3 == NULL) m_typing3 = m_typing2;
    if(m_typing4 == NULL) m_typing4 = m_typing3;
    if(m_backButtonClick == NULL) m_backButtonClick = m_clickButton;
    if(m_backButtonHover == NULL) m_backButtonHover = m_hoverButton;
    if(m_menuBack == NULL) m_menuBack = m_clickButton;
    if(m_closeChatTab == NULL) m_closeChatTab = m_clickButton;
    if(m_clickMainMenuCube == NULL) m_clickMainMenuCube = m_clickButton;
    if(m_hoverMainMenuCube == NULL) m_hoverMainMenuCube = m_menuHover;
    if(m_clickSingleplayer == NULL) m_clickSingleplayer = m_clickButton;
    if(m_hoverSingleplayer == NULL) m_hoverSingleplayer = m_menuHover;
    if(m_clickMultiplayer == NULL) m_clickMultiplayer = m_clickButton;
    if(m_hoverMultiplayer == NULL) m_hoverMultiplayer = m_menuHover;
    if(m_clickOptions == NULL) m_clickOptions = m_clickButton;
    if(m_hoverOptions == NULL) m_hoverOptions = m_menuHover;
    if(m_clickExit == NULL) m_clickExit = m_clickButton;
    if(m_hoverExit == NULL) m_hoverExit = m_menuHover;
    if(m_clickPauseBack == NULL) m_clickPauseBack = m_clickButton;
    if(m_hoverPauseBack == NULL) m_hoverPauseBack = m_pauseHover;
    if(m_clickPauseContinue == NULL) m_clickPauseContinue = m_clickButton;
    if(m_hoverPauseContinue == NULL) m_hoverPauseContinue = m_pauseHover;
    if(m_clickPauseRetry == NULL) m_clickPauseRetry = m_clickButton;
    if(m_hoverPauseRetry == NULL) m_hoverPauseRetry = m_pauseHover;

    // scaling
    // HACKHACK: this is pure cancer
    if(m_cursor != NULL && m_cursor->getFilePath().find("@2x") != -1) m_bCursor2x = true;
    if(m_cursorTrail != NULL && m_cursorTrail->getFilePath().find("@2x") != -1) m_bCursorTrail2x = true;
    if(m_cursorRipple != NULL && m_cursorRipple->getFilePath().find("@2x") != -1) m_bCursorRipple2x = true;
    if(m_approachCircle != NULL && m_approachCircle->getFilePath().find("@2x") != -1) m_bApproachCircle2x = true;
    if(m_reverseArrow != NULL && m_reverseArrow->getFilePath().find("@2x") != -1) m_bReverseArrow2x = true;
    if(m_hitCircle != NULL && m_hitCircle->getFilePath().find("@2x") != -1) m_bHitCircle2x = true;
    if(m_default0 != NULL && m_default0->getFilePath().find("@2x") != -1) m_bIsDefault02x = true;
    if(m_default1 != NULL && m_default1->getFilePath().find("@2x") != -1) m_bIsDefault12x = true;
    if(m_score0 != NULL && m_score0->getFilePath().find("@2x") != -1) m_bIsScore02x = true;
    if(m_combo0 != NULL && m_combo0->getFilePath().find("@2x") != -1) m_bIsCombo02x = true;
    if(m_spinnerApproachCircle != NULL && m_spinnerApproachCircle->getFilePath().find("@2x") != -1)
        m_bSpinnerApproachCircle2x = true;
    if(m_spinnerBottom != NULL && m_spinnerBottom->getFilePath().find("@2x") != -1) m_bSpinnerBottom2x = true;
    if(m_spinnerCircle != NULL && m_spinnerCircle->getFilePath().find("@2x") != -1) m_bSpinnerCircle2x = true;
    if(m_spinnerTop != NULL && m_spinnerTop->getFilePath().find("@2x") != -1) m_bSpinnerTop2x = true;
    if(m_spinnerMiddle != NULL && m_spinnerMiddle->getFilePath().find("@2x") != -1) m_bSpinnerMiddle2x = true;
    if(m_spinnerMiddle2 != NULL && m_spinnerMiddle2->getFilePath().find("@2x") != -1) m_bSpinnerMiddle22x = true;
    if(m_sliderScorePoint != NULL && m_sliderScorePoint->getFilePath().find("@2x") != -1) m_bSliderScorePoint2x = true;
    if(m_sliderStartCircle != NULL && m_sliderStartCircle->getFilePath().find("@2x") != -1)
        m_bSliderStartCircle2x = true;
    if(m_sliderEndCircle != NULL && m_sliderEndCircle->getFilePath().find("@2x") != -1) m_bSliderEndCircle2x = true;

    if(m_circularmetre != NULL && m_circularmetre->getFilePath().find("@2x") != -1) m_bCircularmetre2x = true;

    if(m_pauseContinue != NULL && m_pauseContinue->getFilePath().find("@2x") != -1) m_bPauseContinue2x = true;

    if(m_menuButtonBackground != NULL && m_menuButtonBackground->getFilePath().find("@2x") != -1)
        m_bMenuButtonBackground2x = true;
    if(m_star != NULL && m_star->getFilePath().find("@2x") != -1) m_bStar2x = true;
    if(m_rankingPanel != NULL && m_rankingPanel->getFilePath().find("@2x") != -1) m_bRankingPanel2x = true;
    if(m_rankingMaxCombo != NULL && m_rankingMaxCombo->getFilePath().find("@2x") != -1) m_bRankingMaxCombo2x = true;
    if(m_rankingAccuracy != NULL && m_rankingAccuracy->getFilePath().find("@2x") != -1) m_bRankingAccuracy2x = true;
    if(m_rankingA != NULL && m_rankingA->getFilePath().find("@2x") != -1) m_bRankingA2x = true;
    if(m_rankingB != NULL && m_rankingB->getFilePath().find("@2x") != -1) m_bRankingB2x = true;
    if(m_rankingC != NULL && m_rankingC->getFilePath().find("@2x") != -1) m_bRankingC2x = true;
    if(m_rankingD != NULL && m_rankingD->getFilePath().find("@2x") != -1) m_bRankingD2x = true;
    if(m_rankingS != NULL && m_rankingS->getFilePath().find("@2x") != -1) m_bRankingS2x = true;
    if(m_rankingSH != NULL && m_rankingSH->getFilePath().find("@2x") != -1) m_bRankingSH2x = true;
    if(m_rankingX != NULL && m_rankingX->getFilePath().find("@2x") != -1) m_bRankingX2x = true;
    if(m_rankingXH != NULL && m_rankingXH->getFilePath().find("@2x") != -1) m_bRankingXH2x = true;

    // HACKHACK: all of the <>2 loads are temporary fixes until I fix the checkLoadImage() function logic

    // custom
    Image *defaultCursor = engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT");
    Image *defaultCursor2 = m_cursor;
    if(defaultCursor != NULL)
        m_defaultCursor = defaultCursor;
    else if(defaultCursor2 != NULL)
        m_defaultCursor = defaultCursor2;

    Image *defaultButtonLeft = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
    Image *defaultButtonLeft2 = m_buttonLeft;
    if(defaultButtonLeft != NULL)
        m_defaultButtonLeft = defaultButtonLeft;
    else if(defaultButtonLeft2 != NULL)
        m_defaultButtonLeft = defaultButtonLeft2;

    Image *defaultButtonMiddle = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
    Image *defaultButtonMiddle2 = m_buttonMiddle;
    if(defaultButtonMiddle != NULL)
        m_defaultButtonMiddle = defaultButtonMiddle;
    else if(defaultButtonMiddle2 != NULL)
        m_defaultButtonMiddle = defaultButtonMiddle2;

    Image *defaultButtonRight = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
    Image *defaultButtonRight2 = m_buttonRight;
    if(defaultButtonRight != NULL)
        m_defaultButtonRight = defaultButtonRight;
    else if(defaultButtonRight2 != NULL)
        m_defaultButtonRight = defaultButtonRight2;

    // print some debug info
    debugLog("Skin: Version %f\n", m_fVersion);
    debugLog("Skin: HitCircleOverlap = %i\n", m_iHitCircleOverlap);

    // delayed error notifications due to resource loading potentially blocking engine time
    if(!parseSkinIni1Status && parseSkinIni2Status && m_osu_skin_ref->getString() != UString("default"))
        osu->getNotificationOverlay()->addNotification("Error: Couldn't load skin.ini!", 0xffff0000);
    else if(!parseSkinIni2Status)
        osu->getNotificationOverlay()->addNotification("Error: Couldn't load DEFAULT skin.ini!!!", 0xffff0000);

    // TODO: is this crashing some users?
    // HACKHACK: speed up initial game startup time by async loading the skin (if osu_skin_async 1 in underride)
    if(osu->getSkin() == NULL && osu_skin_async.getBool()) {
        while(engine->getResourceManager()->isLoading()) {
            engine->getResourceManager()->update();
            env->sleep(0);
        }
    }
}

void Skin::loadBeatmapOverride(std::string filepath) {
    // debugLog("Skin::loadBeatmapOverride( %s )\n", filepath.c_str());
    //  TODO: beatmap skin support
}

void Skin::reloadSounds() {
    for(int i = 0; i < m_sounds.size(); i++) {
        m_sounds[i]->reload();
    }
}

bool Skin::parseSkinINI(std::string filepath) {
    File file(filepath);
    if(!file.canRead()) {
        debugLog("Skin Error: Couldn't load %s\n", filepath.c_str());
        return false;
    }

    int curBlock = 0;  // NOTE: was -1, but osu incorrectly defaults to [General] and loads properties even before the
                       // actual section start (just for this first section though)
    while(file.canRead()) {
        std::string curLine = file.readLine();
        if(curLine.find("//") > 2)  // ignore comments // TODO: this is incorrect, but it works well enough
        {
            if(curLine.find("[General]") != std::string::npos)
                curBlock = 0;
            else if(curLine.find("[Colours]") != std::string::npos || curLine.find("[Colors]") != std::string::npos)
                curBlock = 1;
            else if(curLine.find("[Fonts]") != std::string::npos)
                curBlock = 2;

            switch(curBlock) {
                case 0:  // General
                {
                    int val;
                    float floatVal;
                    char stringBuffer[1024];

                    memset(stringBuffer, '\0', 1024);
                    if(sscanf(curLine.c_str(), " Version : %1023[^\n]", stringBuffer) == 1) {
                        UString versionString = UString(stringBuffer);
                        if(versionString.find("latest") != -1 || versionString.find("User") != -1)
                            m_fVersion = 2.5f;  // default to latest version available
                        else
                            m_fVersion = versionString.toFloat();
                    }

                    if(sscanf(curLine.c_str(), " CursorRotate : %i \n", &val) == 1)
                        m_bCursorRotate = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " CursorCentre : %i \n", &val) == 1)
                        m_bCursorCenter = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " CursorExpand : %i \n", &val) == 1)
                        m_bCursorExpand = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " SliderBallFlip : %i \n", &val) == 1)
                        m_bSliderBallFlip = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " AllowSliderBallTint : %i \n", &val) == 1)
                        m_bAllowSliderBallTint = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " HitCircleOverlayAboveNumber : %i \n", &val) == 1)
                        m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " HitCircleOverlayAboveNumer : %i \n", &val) == 1)
                        m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " SliderStyle : %i \n", &val) == 1) {
                        m_iSliderStyle = val;
                        if(m_iSliderStyle != 1 && m_iSliderStyle != 2) m_iSliderStyle = 2;
                    }
                    if(sscanf(curLine.c_str(), " AnimationFramerate : %f \n", &floatVal) == 1)
                        m_fAnimationFramerate = floatVal < 0 ? 0.0f : floatVal;
                } break;
                case 1:  // Colors
                {
                    int comboNum;
                    int r, g, b;

                    if(sscanf(curLine.c_str(), " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
                        m_comboColors.push_back(COLOR(255, r, g, b));
                    if(sscanf(curLine.c_str(), " SpinnerApproachCircle : %i , %i , %i \n", &r, &g, &b) == 3)
                        m_spinnerApproachCircleColor = COLOR(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SliderBorder: %i , %i , %i \n", &r, &g, &b) == 3)
                        m_sliderBorderColor = COLOR(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SliderTrackOverride : %i , %i , %i \n", &r, &g, &b) == 3) {
                        m_sliderTrackOverride = COLOR(255, r, g, b);
                        m_bSliderTrackOverride = true;
                    }
                    if(sscanf(curLine.c_str(), " SliderBall : %i , %i , %i \n", &r, &g, &b) == 3)
                        m_sliderBallColor = COLOR(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SongSelectActiveText : %i , %i , %i \n", &r, &g, &b) == 3)
                        m_songSelectActiveText = COLOR(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SongSelectInactiveText : %i , %i , %i \n", &r, &g, &b) == 3)
                        m_songSelectInactiveText = COLOR(255, r, g, b);
                    if(sscanf(curLine.c_str(), " InputOverlayText : %i , %i , %i \n", &r, &g, &b) == 3)
                        m_inputOverlayText = COLOR(255, r, g, b);
                } break;
                case 2:  // Fonts
                {
                    int val;
                    char stringBuffer[1024];

                    memset(stringBuffer, '\0', 1024);
                    if(sscanf(curLine.c_str(), " ComboPrefix : %1023[^\n]", stringBuffer) == 1) {
                        m_sComboPrefix = UString(stringBuffer);

                        for(int i = 0; i < m_sComboPrefix.length(); i++) {
                            if(m_sComboPrefix[i] == '\\') {
                                m_sComboPrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " ComboOverlap : %i \n", &val) == 1) m_iComboOverlap = val;

                    if(sscanf(curLine.c_str(), " ScorePrefix : %1023[^\n]", stringBuffer) == 1) {
                        m_sScorePrefix = UString(stringBuffer);

                        for(int i = 0; i < m_sScorePrefix.length(); i++) {
                            if(m_sScorePrefix[i] == '\\') {
                                m_sScorePrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " ScoreOverlap : %i \n", &val) == 1) m_iScoreOverlap = val;

                    if(sscanf(curLine.c_str(), " HitCirclePrefix : %1023[^\n]", stringBuffer) == 1) {
                        m_sHitCirclePrefix = UString(stringBuffer);

                        for(int i = 0; i < m_sHitCirclePrefix.length(); i++) {
                            if(m_sHitCirclePrefix[i] == '\\') {
                                m_sHitCirclePrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " HitCircleOverlap : %i \n", &val) == 1) m_iHitCircleOverlap = val;
                } break;
            }
        }
    }

    return true;
}

void Skin::onIgnoreBeatmapSampleVolumeChange(UString oldValue, UString newValue) { resetSampleVolume(); }

void Skin::setSampleSet(int sampleSet) {
    if(m_iSampleSet == sampleSet) return;

    /// debugLog("sample set = %i\n", sampleSet);
    m_iSampleSet = sampleSet;
}

void Skin::resetSampleVolume() { setSampleVolume(clamp<float>((float)m_iSampleVolume / 100.0f, 0.0f, 1.0f), true); }

void Skin::setSampleVolume(float volume, bool force) {
    auto osu_volume_effects = convar->getConVarByName("osu_volume_effects");
    if(osu_ignore_beatmap_sample_volume.getBool() && (int)(osu_volume_effects->getFloat() * 100.0f) == m_iSampleVolume)
        return;

    const float newSampleVolume =
        (!osu_ignore_beatmap_sample_volume.getBool() ? volume : 1.0f) * osu_volume_effects->getFloat();

    if(!force && m_iSampleVolume == (int)(newSampleVolume * 100.0f)) return;

    m_iSampleVolume = (int)(newSampleVolume * 100.0f);
    /// debugLog("sample volume = %f\n", sampleVolume);
    for(int i = 0; i < m_soundSamples.size(); i++) {
        m_soundSamples[i].sound->setVolume(newSampleVolume * m_soundSamples[i].hardcodedVolumeMultiplier);
    }
}

Color Skin::getComboColorForCounter(int i, int offset) {
    i += osu_skin_color_index_add.getInt();
    i = max(i, 0);

    if(m_beatmapComboColors.size() > 0 && !osu_ignore_beatmap_combo_colors.getBool())
        return m_beatmapComboColors[(i + offset) % m_beatmapComboColors.size()];
    else if(m_comboColors.size() > 0)
        return m_comboColors[i % m_comboColors.size()];
    else
        return COLOR(255, 0, 255, 0);
}

void Skin::setBeatmapComboColors(std::vector<Color> colors) { m_beatmapComboColors = colors; }

void Skin::playHitCircleSound(int sampleType, float pan, long delta) {
    if(m_iSampleVolume <= 0) {
        return;
    }

    if(!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) ||
       (GameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= osu_sound_panning_multiplier.getFloat();
    }

    debugLog("delta: %d\n", delta);
    if(delta < 0 && m_tooearly != NULL) {
        debugLog("Too early!\n");
        engine->getSound()->play(m_tooearly, pan);
        return;
    }
    if(delta > 0 && m_toolate != NULL) {
        debugLog("Too late!\n");
        engine->getSound()->play(m_toolate, pan);
        return;
    }

    int actualSampleSet = m_iSampleSet;
    if(osu_skin_force_hitsound_sample_set.getInt() > 0) actualSampleSet = osu_skin_force_hitsound_sample_set.getInt();

    switch(actualSampleSet) {
        case 3:
            engine->getSound()->play(m_drumHitNormal, pan);

            if(sampleType & OSU_BITMASK_HITWHISTLE) engine->getSound()->play(m_drumHitWhistle, pan);
            if(sampleType & OSU_BITMASK_HITFINISH) engine->getSound()->play(m_drumHitFinish, pan);
            if(sampleType & OSU_BITMASK_HITCLAP) engine->getSound()->play(m_drumHitClap, pan);
            break;
        case 2:
            engine->getSound()->play(m_softHitNormal, pan);

            if(sampleType & OSU_BITMASK_HITWHISTLE) engine->getSound()->play(m_softHitWhistle, pan);
            if(sampleType & OSU_BITMASK_HITFINISH) engine->getSound()->play(m_softHitFinish, pan);
            if(sampleType & OSU_BITMASK_HITCLAP) engine->getSound()->play(m_softHitClap, pan);
            break;
        default:
            engine->getSound()->play(m_normalHitNormal, pan);

            if(sampleType & OSU_BITMASK_HITWHISTLE) engine->getSound()->play(m_normalHitWhistle, pan);
            if(sampleType & OSU_BITMASK_HITFINISH) engine->getSound()->play(m_normalHitFinish, pan);
            if(sampleType & OSU_BITMASK_HITCLAP) engine->getSound()->play(m_normalHitClap, pan);
            break;
    }
}

void Skin::playSliderTickSound(float pan) {
    if(m_iSampleVolume <= 0) return;

    if(!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) ||
       (GameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= osu_sound_panning_multiplier.getFloat();
    }

    switch(m_iSampleSet) {
        case 3:
            engine->getSound()->play(m_drumSliderTick, pan);
            break;
        case 2:
            engine->getSound()->play(m_softSliderTick, pan);
            break;
        default:
            engine->getSound()->play(m_normalSliderTick, pan);
            break;
    }
}

void Skin::playSliderSlideSound(float pan) {
    if(!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) ||
       (GameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= osu_sound_panning_multiplier.getFloat();
    }

    switch(m_iSampleSet) {
        case 3:
            if(m_softSliderSlide->isPlaying()) engine->getSound()->stop(m_softSliderSlide);
            if(m_normalSliderSlide->isPlaying()) engine->getSound()->stop(m_normalSliderSlide);

            if(!m_drumSliderSlide->isPlaying())
                engine->getSound()->play(m_drumSliderSlide, pan);
            else
                m_drumSliderSlide->setPan(pan);
            break;
        case 2:
            if(m_drumSliderSlide->isPlaying()) engine->getSound()->stop(m_drumSliderSlide);
            if(m_normalSliderSlide->isPlaying()) engine->getSound()->stop(m_normalSliderSlide);

            if(!m_softSliderSlide->isPlaying())
                engine->getSound()->play(m_softSliderSlide, pan);
            else
                m_softSliderSlide->setPan(pan);
            break;
        default:
            if(m_softSliderSlide->isPlaying()) engine->getSound()->stop(m_softSliderSlide);
            if(m_drumSliderSlide->isPlaying()) engine->getSound()->stop(m_drumSliderSlide);

            if(!m_normalSliderSlide->isPlaying())
                engine->getSound()->play(m_normalSliderSlide, pan);
            else
                m_normalSliderSlide->setPan(pan);
            break;
    }
}

void Skin::playSpinnerSpinSound() {
    if(!m_spinnerSpinSound->isPlaying()) {
        engine->getSound()->play(m_spinnerSpinSound);
    }
}

void Skin::playSpinnerBonusSound() {
    if(m_iSampleVolume > 0) {
        engine->getSound()->play(m_spinnerBonus);
    }
}

void Skin::stopSliderSlideSound(int sampleSet) {
    if((sampleSet == -2 || sampleSet == 3) && m_drumSliderSlide->isPlaying())
        engine->getSound()->stop(m_drumSliderSlide);
    if((sampleSet == -2 || sampleSet == 2) && m_softSliderSlide->isPlaying())
        engine->getSound()->stop(m_softSliderSlide);
    if((sampleSet == -2 || sampleSet == 1 || sampleSet == 0) && m_normalSliderSlide->isPlaying())
        engine->getSound()->stop(m_normalSliderSlide);
}

void Skin::stopSpinnerSpinSound() {
    if(m_spinnerSpinSound->isPlaying()) engine->getSound()->stop(m_spinnerSpinSound);
}

void Skin::randomizeFilePath() {
    if(m_bIsRandomElements && filepathsForRandomSkin.size() > 0)
        m_sFilePath = filepathsForRandomSkin[rand() % filepathsForRandomSkin.size()];
}

SkinImage *Skin::createSkinImage(std::string skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
                                 bool ignoreDefaultSkin, std::string animationSeparator) {
    SkinImage *skinImage =
        new SkinImage(this, skinElementName, baseSizeForScaling2x, osuSize, animationSeparator, ignoreDefaultSkin);
    m_images.push_back(skinImage);

    const std::vector<std::string> &filepathsForExport = skinImage->getFilepathsForExport();
    m_filepathsForExport.insert(m_filepathsForExport.end(), filepathsForExport.begin(), filepathsForExport.end());

    return skinImage;
}

void Skin::checkLoadImage(Image **addressOfPointer, std::string skinElementName, std::string resourceName,
                          bool ignoreDefaultSkin, std::string fileExtension, bool forceLoadMipmaps) {
    if(*addressOfPointer != m_missingTexture) return;  // we are already loaded

    // NOTE: only the default skin is loaded with a resource name (it must never be unloaded by other instances), and it
    // is NOT added to the resources vector

    std::string defaultFilePath1 = MCENGINE_DATA_DIR "materials/default/";
    defaultFilePath1.append(skinElementName);
    defaultFilePath1.append("@2x.");
    defaultFilePath1.append(fileExtension);

    std::string defaultFilePath2 = MCENGINE_DATA_DIR "materials/default/";
    defaultFilePath2.append(skinElementName);
    defaultFilePath2.append(".");
    defaultFilePath2.append(fileExtension);

    std::string filepath1 = m_sFilePath;
    filepath1.append(skinElementName);
    filepath1.append("@2x.");
    filepath1.append(fileExtension);

    std::string filepath2 = m_sFilePath;
    filepath2.append(skinElementName);
    filepath2.append(".");
    filepath2.append(fileExtension);

    const bool existsDefaultFilePath1 = env->fileExists(defaultFilePath1);
    const bool existsDefaultFilePath2 = env->fileExists(defaultFilePath2);
    const bool existsFilepath1 = env->fileExists(filepath1);
    const bool existsFilepath2 = env->fileExists(filepath2);

    // check if an @2x version of this image exists
    if(osu_skin_hd.getBool()) {
        // load default skin

        if(!ignoreDefaultSkin) {
            if(existsDefaultFilePath1) {
                std::string defaultResourceName = resourceName;
                defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                if(osu_skin_async.getBool()) engine->getResourceManager()->requestNextLoadAsync();

                *addressOfPointer = engine->getResourceManager()->loadImageAbs(
                    defaultFilePath1, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
                /// m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
            } else  // fallback to @1x
            {
                if(existsDefaultFilePath2) {
                    std::string defaultResourceName = resourceName;
                    defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                    if(osu_skin_async.getBool()) engine->getResourceManager()->requestNextLoadAsync();

                    *addressOfPointer = engine->getResourceManager()->loadImageAbs(
                        defaultFilePath2, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
                    /// m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
                }
            }
        }

        // load user skin

        if(existsFilepath1) {
            if(osu_skin_async.getBool()) engine->getResourceManager()->requestNextLoadAsync();

            *addressOfPointer = engine->getResourceManager()->loadImageAbs(
                filepath1, "", osu_skin_mipmaps.getBool() || forceLoadMipmaps);
            m_resources.push_back(*addressOfPointer);

            // export
            {
                if(existsFilepath1) m_filepathsForExport.push_back(filepath1);

                if(existsFilepath2) m_filepathsForExport.push_back(filepath2);

                if(!existsFilepath1 && !existsFilepath2) {
                    if(existsDefaultFilePath1) m_filepathsForExport.push_back(defaultFilePath1);

                    if(existsDefaultFilePath2) m_filepathsForExport.push_back(defaultFilePath2);
                }
            }

            return;  // nothing more to do here
        }
    }

    // else load normal @1x version

    // load default skin

    if(!ignoreDefaultSkin) {
        if(existsDefaultFilePath2) {
            std::string defaultResourceName = resourceName;
            defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

            if(osu_skin_async.getBool()) engine->getResourceManager()->requestNextLoadAsync();

            *addressOfPointer = engine->getResourceManager()->loadImageAbs(
                defaultFilePath2, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
            /// m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
        }
    }

    // load user skin

    if(existsFilepath2) {
        if(osu_skin_async.getBool()) engine->getResourceManager()->requestNextLoadAsync();

        *addressOfPointer =
            engine->getResourceManager()->loadImageAbs(filepath2, "", osu_skin_mipmaps.getBool() || forceLoadMipmaps);
        m_resources.push_back(*addressOfPointer);
    }

    // export
    {
        if(existsFilepath1) m_filepathsForExport.push_back(filepath1);

        if(existsFilepath2) m_filepathsForExport.push_back(filepath2);

        if(!existsFilepath1 && !existsFilepath2) {
            if(existsDefaultFilePath1) m_filepathsForExport.push_back(defaultFilePath1);

            if(existsDefaultFilePath2) m_filepathsForExport.push_back(defaultFilePath2);
        }
    }
}

void Skin::checkLoadSound(Sound **addressOfPointer, std::string skinElementName, std::string resourceName,
                          bool isOverlayable, bool isSample, bool loop, bool fallback_to_default,
                          float hardcodedVolumeMultiplier) {
    if(*addressOfPointer != NULL) return;  // we are already loaded

    // NOTE: only the default skin is loaded with a resource name (it must never be unloaded by other instances), and it
    // is NOT added to the resources vector

    // random skin support
    randomizeFilePath();

    auto try_load_sound = [=](std::string base_path, std::string filename, std::string resource_name, bool loop) {
        const char *extensions[] = {".wav", ".mp3", ".ogg"};
        for(int i = 0; i < 3; i++) {
            std::string fn = filename;
            fn.append(extensions[i]);
            fn = fix_filename_casing(base_path, fn);

            std::string path = base_path;
            path.append(fn);
            debugLog("Loading %s\n", path.c_str());

            if(env->fileExists(path)) {
                if(osu_skin_async.getBool()) {
                    engine->getResourceManager()->requestNextLoadAsync();
                }
                return engine->getResourceManager()->loadSoundAbs(path, resource_name, !isSample, isOverlayable, loop);
            }
        }

        debugLog("Failed to load %s\n", filename.c_str());

        return (Sound *)NULL;
    };

    // load default skin
    if(fallback_to_default) {
        std::string defaultpath = MCENGINE_DATA_DIR "materials/default/";
        std::string defaultResourceName = resourceName;
        defaultResourceName.append("_DEFAULT");
        *addressOfPointer = try_load_sound(defaultpath, skinElementName, defaultResourceName, loop);
    }

    // load user skin
    Sound *skin_sound = NULL;
    if(osu_skin_use_skin_hitsounds.getBool() || !isSample) {
        skin_sound = try_load_sound(m_sFilePath, skinElementName, "", loop);
        if(skin_sound != NULL) {
            *addressOfPointer = skin_sound;
        }
    }

    Sound *sound = *addressOfPointer;
    if(sound == NULL) {
        debugLog("Skin Warning: NULL sound %s!\n", skinElementName.c_str());
        return;
    }

    // force reload default skin sound anyway if the custom skin does not include it (e.g. audio device change)
    if(skin_sound == NULL) {
        sound->reload();
    } else {
        m_resources.push_back(sound);
    }

    if(isSample) {
        SOUND_SAMPLE sample;
        sample.sound = sound;
        sample.hardcodedVolumeMultiplier = (hardcodedVolumeMultiplier >= 0.0f ? hardcodedVolumeMultiplier : 1.0f);
        m_soundSamples.push_back(sample);
    }

    m_sounds.push_back(sound);

    // export
    m_filepathsForExport.push_back(sound->getFilePath());
}

bool Skin::compareFilenameWithSkinElementName(std::string filename, std::string skinElementName) {
    if(filename.length() == 0 || skinElementName.length() == 0) return false;
    return filename.substr(0, filename.find_last_of('.')) == skinElementName;
}
