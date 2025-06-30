#include "Skin.h"

#include <string.h>

#include "Archival.h"
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

using namespace std;

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

Image *Skin::m_missingTexture = NULL;

void Skin::unpack(const char *filepath) {
    auto skin_name = env->getFileNameFromFilePath(filepath);
    debugLog("Extracting %s...\n", skin_name.c_str());
    skin_name.erase(skin_name.size() - 4);  // remove .osk extension

    auto skin_root = std::string(MCENGINE_DATA_DIR "skins/");
    skin_root.append(skin_name);
    skin_root.append("/");

    File file(filepath);
    if(!file.canRead()) {
        debugLog("Failed to read skin file %s\n", filepath);
        return;
    }

    Archive archive(reinterpret_cast<const u8 *>(file.readFile()), file.getFileSize());
    if(!archive.isValid()) {
        debugLog("Failed to open .osk file\n");
        return;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog(".osk file is empty!\n");
        return;
    }

    if(!env->directoryExists(skin_root)) {
        env->createDirectory(skin_root);
    }

    for(const auto &entry : entries) {
        if(entry.isDirectory()) continue;

        std::string filename = entry.getFilename();
        auto folders = UString(filename).split("/");
        std::string file_path = skin_root;

        for(const auto &folder : folders) {
            if(!env->directoryExists(file_path)) {
                env->createDirectory(file_path);
            }

            if(folder == UString("..")) {
                // security check: skip files with path traversal attempts
                goto skip_file;
            } else {
                file_path.append("/");
                file_path.append(folder.toUtf8());
            }
        }

        if(!entry.extractToFile(file_path)) {
            debugLog("Failed to extract skin file %s\n", filename.c_str());
        }

    skip_file:;
        // when a file can't be extracted we just ignore it (as long as the archive is valid)
    }
}

Skin::Skin(UString name, std::string filepath, bool isDefaultSkin) {
    this->sName = name.utf8View();
    this->sFilePath = filepath;
    this->bIsDefaultSkin = isDefaultSkin;

    this->bReady = false;

    if(m_missingTexture == NULL) m_missingTexture = resourceManager->getImage("MISSING_TEXTURE");

    // vars
    this->hitCircle = m_missingTexture;
    this->approachCircle = m_missingTexture;
    this->reverseArrow = m_missingTexture;

    this->default0 = m_missingTexture;
    this->default1 = m_missingTexture;
    this->default2 = m_missingTexture;
    this->default3 = m_missingTexture;
    this->default4 = m_missingTexture;
    this->default5 = m_missingTexture;
    this->default6 = m_missingTexture;
    this->default7 = m_missingTexture;
    this->default8 = m_missingTexture;
    this->default9 = m_missingTexture;

    this->score0 = m_missingTexture;
    this->score1 = m_missingTexture;
    this->score2 = m_missingTexture;
    this->score3 = m_missingTexture;
    this->score4 = m_missingTexture;
    this->score5 = m_missingTexture;
    this->score6 = m_missingTexture;
    this->score7 = m_missingTexture;
    this->score8 = m_missingTexture;
    this->score9 = m_missingTexture;
    this->scoreX = m_missingTexture;
    this->scorePercent = m_missingTexture;
    this->scoreDot = m_missingTexture;

    this->combo0 = m_missingTexture;
    this->combo1 = m_missingTexture;
    this->combo2 = m_missingTexture;
    this->combo3 = m_missingTexture;
    this->combo4 = m_missingTexture;
    this->combo5 = m_missingTexture;
    this->combo6 = m_missingTexture;
    this->combo7 = m_missingTexture;
    this->combo8 = m_missingTexture;
    this->combo9 = m_missingTexture;
    this->comboX = m_missingTexture;

    this->playWarningArrow = m_missingTexture;
    this->circularmetre = m_missingTexture;

    this->particle50 = m_missingTexture;
    this->particle100 = m_missingTexture;
    this->particle300 = m_missingTexture;

    this->sliderGradient = m_missingTexture;
    this->sliderScorePoint = m_missingTexture;
    this->sliderStartCircle = m_missingTexture;
    this->sliderStartCircleOverlay = m_missingTexture;
    this->sliderEndCircle = m_missingTexture;
    this->sliderEndCircleOverlay = m_missingTexture;

    this->spinnerBackground = m_missingTexture;
    this->spinnerCircle = m_missingTexture;
    this->spinnerApproachCircle = m_missingTexture;
    this->spinnerBottom = m_missingTexture;
    this->spinnerMiddle = m_missingTexture;
    this->spinnerMiddle2 = m_missingTexture;
    this->spinnerTop = m_missingTexture;
    this->spinnerSpin = m_missingTexture;
    this->spinnerClear = m_missingTexture;

    this->defaultCursor = m_missingTexture;
    this->cursor = m_missingTexture;
    this->cursorMiddle = m_missingTexture;
    this->cursorTrail = m_missingTexture;
    this->cursorRipple = m_missingTexture;

    this->pauseContinue = m_missingTexture;
    this->pauseReplay = m_missingTexture;
    this->pauseRetry = m_missingTexture;
    this->pauseBack = m_missingTexture;
    this->pauseOverlay = m_missingTexture;
    this->failBackground = m_missingTexture;
    this->unpause = m_missingTexture;

    this->buttonLeft = m_missingTexture;
    this->buttonMiddle = m_missingTexture;
    this->buttonRight = m_missingTexture;
    this->defaultButtonLeft = m_missingTexture;
    this->defaultButtonMiddle = m_missingTexture;
    this->defaultButtonRight = m_missingTexture;

    this->songSelectTop = m_missingTexture;
    this->songSelectBottom = m_missingTexture;
    this->menuButtonBackground = m_missingTexture;
    this->star = m_missingTexture;
    this->rankingPanel = m_missingTexture;
    this->rankingGraph = m_missingTexture;
    this->rankingTitle = m_missingTexture;
    this->rankingMaxCombo = m_missingTexture;
    this->rankingAccuracy = m_missingTexture;
    this->rankingA = m_missingTexture;
    this->rankingB = m_missingTexture;
    this->rankingC = m_missingTexture;
    this->rankingD = m_missingTexture;
    this->rankingS = m_missingTexture;
    this->rankingSH = m_missingTexture;
    this->rankingX = m_missingTexture;
    this->rankingXH = m_missingTexture;

    this->beatmapImportSpinner = m_missingTexture;
    this->loadingSpinner = m_missingTexture;
    this->circleEmpty = m_missingTexture;
    this->circleFull = m_missingTexture;
    this->seekTriangle = m_missingTexture;
    this->userIcon = m_missingTexture;
    this->backgroundCube = m_missingTexture;
    this->menuBackground = m_missingTexture;
    this->skybox = m_missingTexture;

    this->normalHitNormal = NULL;
    this->normalHitWhistle = NULL;
    this->normalHitFinish = NULL;
    this->normalHitClap = NULL;

    this->normalSliderTick = NULL;
    this->normalSliderSlide = NULL;
    this->normalSliderWhistle = NULL;

    this->softHitNormal = NULL;
    this->softHitWhistle = NULL;
    this->softHitFinish = NULL;
    this->softHitClap = NULL;

    this->softSliderTick = NULL;
    this->softSliderSlide = NULL;
    this->softSliderWhistle = NULL;

    this->drumHitNormal = NULL;
    this->drumHitWhistle = NULL;
    this->drumHitFinish = NULL;
    this->drumHitClap = NULL;

    this->drumSliderTick = NULL;
    this->drumSliderSlide = NULL;
    this->drumSliderWhistle = NULL;

    this->spinnerBonus = NULL;
    this->spinnerSpinSound = NULL;

    this->combobreak = NULL;
    this->failsound = NULL;
    this->applause = NULL;
    this->menuHit = NULL;
    this->menuHover = NULL;
    this->checkOn = NULL;
    this->checkOff = NULL;
    this->shutter = NULL;
    this->sectionPassSound = NULL;
    this->sectionFailSound = NULL;

    this->spinnerApproachCircleColor = 0xffffffff;
    this->sliderBorderColor = 0xffffffff;
    this->sliderBallColor = 0xffffffff;  // NOTE: 0xff02aaff is a hardcoded special case for osu!'s default skin, but it
                                         // does not apply to user skins

    this->songSelectActiveText = 0xff000000;
    this->songSelectInactiveText = 0xffffffff;
    this->inputOverlayText = 0xff000000;

    // scaling
    this->bCursor2x = false;
    this->bCursorTrail2x = false;
    this->bCursorRipple2x = false;
    this->bApproachCircle2x = false;
    this->bReverseArrow2x = false;
    this->bHitCircle2x = false;
    this->bIsDefault02x = false;
    this->bIsDefault12x = false;
    this->bIsScore02x = false;
    this->bIsCombo02x = false;
    this->bSpinnerApproachCircle2x = false;
    this->bSpinnerBottom2x = false;
    this->bSpinnerCircle2x = false;
    this->bSpinnerTop2x = false;
    this->bSpinnerMiddle2x = false;
    this->bSpinnerMiddle22x = false;
    this->bSliderScorePoint2x = false;
    this->bSliderStartCircle2x = false;
    this->bSliderEndCircle2x = false;

    this->bCircularmetre2x = false;

    this->bPauseContinue2x = false;

    this->bMenuButtonBackground2x = false;
    this->bStar2x = false;
    this->bRankingPanel2x = false;
    this->bRankingMaxCombo2x = false;
    this->bRankingAccuracy2x = false;
    this->bRankingA2x = false;
    this->bRankingB2x = false;
    this->bRankingC2x = false;
    this->bRankingD2x = false;
    this->bRankingS2x = false;
    this->bRankingSH2x = false;
    this->bRankingX2x = false;
    this->bRankingXH2x = false;

    // skin.ini
    this->fVersion = 1;
    this->fAnimationFramerate = 0.0f;
    this->bCursorCenter = true;
    this->bCursorRotate = true;
    this->bCursorExpand = true;

    this->bSliderBallFlip = true;
    this->bAllowSliderBallTint = false;
    this->iSliderStyle = 2;
    this->bHitCircleOverlayAboveNumber = true;
    this->bSliderTrackOverride = false;

    this->iHitCircleOverlap = 0;
    this->iScoreOverlap = 0;
    this->iComboOverlap = 0;

    // custom
    this->iSampleSet = 1;
    this->iSampleVolume = (int)(cv_volume_effects.getFloat() * 100.0f);

    this->bIsRandom = cv_skin_random.getBool();
    this->bIsRandomElements = cv_skin_random_elements.getBool();

    // load all files
    this->load();

    // convar callbacks
    cv_ignore_beatmap_sample_volume.setCallback(
        fastdelegate::MakeDelegate(this, &Skin::onIgnoreBeatmapSampleVolumeChange));
}

Skin::~Skin() {
    for(int i = 0; i < this->resources.size(); i++) {
        if(this->resources[i] != (Resource *)m_missingTexture)
            resourceManager->destroyResource(this->resources[i]);
    }
    this->resources.clear();

    for(int i = 0; i < this->images.size(); i++) {
        delete this->images[i];
    }
    this->images.clear();

    this->sounds.clear();

    this->filepathsForExport.clear();
}

void Skin::update() {
    // tasks which have to be run after async loading finishes
    if(!this->bReady && this->isReady()) {
        this->bReady = true;

        // force effect volume update
        osu->volumeOverlay->onEffectVolumeChange();
    }

    // shitty check to not animate while paused with hitobjects in background
    if(osu->isInPlayMode() && !osu->getSelectedBeatmap()->isPlaying() && !cv_skin_animation_force.getBool()) return;

    const bool useEngineTimeForAnimations = !osu->isInPlayMode();
    const long curMusicPos = osu->getSelectedBeatmap()->getCurMusicPosWithOffsets();
    for(int i = 0; i < this->images.size(); i++) {
        this->images[i]->update(this->animationSpeedMultiplier, useEngineTimeForAnimations, curMusicPos);
    }
}

bool Skin::isReady() {
    if(this->bReady) return true;

    for(int i = 0; i < this->resources.size(); i++) {
        if(resourceManager->isLoadingResource(this->resources[i])) return false;
    }

    for(int i = 0; i < this->images.size(); i++) {
        if(!this->images[i]->isReady()) return false;
    }

    // (ready is set in update())

    return true;
}

void Skin::load() {
    // random skins
    {
        this->filepathsForRandomSkin.clear();
        if(this->bIsRandom || this->bIsRandomElements) {
            std::vector<std::string> skinNames;

            // regular skins
            {
                auto osu_folder = cv_osu_folder.getString();
                auto osu_folder_sub_skins = cv_osu_folder_sub_skins.getString();
                std::string skinFolder = osu_folder.toUtf8();
                skinFolder.append("/");
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

                    this->filepathsForRandomSkin.push_back(randomSkinFolder);
                    skinNames.push_back(skinFolders[i]);
                }
            }

            if(this->bIsRandom && this->filepathsForRandomSkin.size() > 0) {
                const int randomIndex = std::rand() % std::min(this->filepathsForRandomSkin.size(), skinNames.size());

                this->sName = skinNames[randomIndex];
                this->sFilePath = this->filepathsForRandomSkin[randomIndex];
            }
        }
    }

    // spinner loading has top priority in async
    this->randomizeFilePath();
    {
        this->checkLoadImage(&this->loadingSpinner, "loading-spinner", "OSU_SKIN_LOADING_SPINNER");
    }

    // and the cursor comes right after that
    this->randomizeFilePath();
    {
        this->checkLoadImage(&this->cursor, "cursor", "OSU_SKIN_CURSOR");
        this->checkLoadImage(&this->cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE", true);
        this->checkLoadImage(&this->cursorTrail, "cursortrail", "OSU_SKIN_CURSORTRAIL");
        this->checkLoadImage(&this->cursorRipple, "cursor-ripple", "OSU_SKIN_CURSORRIPPLE");

        // special case: if fallback to default cursor, do load cursorMiddle
        if(this->cursor == resourceManager->getImage("OSU_SKIN_CURSOR_DEFAULT"))
            this->checkLoadImage(&this->cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE");
    }

    // skin ini
    this->randomizeFilePath();
    this->sSkinIniFilePath = this->sFilePath;
    this->sSkinIniFilePath.append(fix_filename_casing(this->sFilePath, "skin.ini"));

    bool parseSkinIni1Status = true;
    bool parseSkinIni2Status = true;
    if(!this->parseSkinINI(this->sSkinIniFilePath)) {
        parseSkinIni1Status = false;
        this->sSkinIniFilePath = MCENGINE_DATA_DIR "materials/default/skin.ini";
        parseSkinIni2Status = this->parseSkinINI(this->sSkinIniFilePath);
    }

    // default values, if none were loaded
    if(this->comboColors.size() == 0) {
        this->comboColors.push_back(argb(255, 255, 192, 0));
        this->comboColors.push_back(argb(255, 0, 202, 0));
        this->comboColors.push_back(argb(255, 18, 124, 255));
        this->comboColors.push_back(argb(255, 242, 24, 57));
    }

    // images
    this->randomizeFilePath();
    this->checkLoadImage(&this->hitCircle, "hitcircle", "OSU_SKIN_HITCIRCLE");
    this->hitCircleOverlay2 = this->createSkinImage("hitcircleoverlay", Vector2(128, 128), 64);
    this->hitCircleOverlay2->setAnimationFramerate(2);

    this->randomizeFilePath();
    this->checkLoadImage(&this->approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
    this->randomizeFilePath();
    this->checkLoadImage(&this->reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

    this->randomizeFilePath();
    this->followPoint2 = this->createSkinImage("followpoint", Vector2(16, 22), 64);

    this->randomizeFilePath();
    {
        std::string hitCirclePrefix = this->sHitCirclePrefix.length() > 0 ? this->sHitCirclePrefix : "default";
        std::string hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-0");
        this->checkLoadImage(&this->default0, hitCircleStringFinal, "OSU_SKIN_DEFAULT0");
        if(this->default0 == m_missingTexture)
            this->checkLoadImage(&this->default0, "default-0",
                                 "OSU_SKIN_DEFAULT0");  // special cases: fallback to default skin hitcircle numbers if
                                                        // the defined prefix doesn't point to any valid files
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-1");
        this->checkLoadImage(&this->default1, hitCircleStringFinal, "OSU_SKIN_DEFAULT1");
        if(this->default1 == m_missingTexture) this->checkLoadImage(&this->default1, "default-1", "OSU_SKIN_DEFAULT1");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-2");
        this->checkLoadImage(&this->default2, hitCircleStringFinal, "OSU_SKIN_DEFAULT2");
        if(this->default2 == m_missingTexture) this->checkLoadImage(&this->default2, "default-2", "OSU_SKIN_DEFAULT2");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-3");
        this->checkLoadImage(&this->default3, hitCircleStringFinal, "OSU_SKIN_DEFAULT3");
        if(this->default3 == m_missingTexture) this->checkLoadImage(&this->default3, "default-3", "OSU_SKIN_DEFAULT3");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-4");
        this->checkLoadImage(&this->default4, hitCircleStringFinal, "OSU_SKIN_DEFAULT4");
        if(this->default4 == m_missingTexture) this->checkLoadImage(&this->default4, "default-4", "OSU_SKIN_DEFAULT4");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-5");
        this->checkLoadImage(&this->default5, hitCircleStringFinal, "OSU_SKIN_DEFAULT5");
        if(this->default5 == m_missingTexture) this->checkLoadImage(&this->default5, "default-5", "OSU_SKIN_DEFAULT5");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-6");
        this->checkLoadImage(&this->default6, hitCircleStringFinal, "OSU_SKIN_DEFAULT6");
        if(this->default6 == m_missingTexture) this->checkLoadImage(&this->default6, "default-6", "OSU_SKIN_DEFAULT6");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-7");
        this->checkLoadImage(&this->default7, hitCircleStringFinal, "OSU_SKIN_DEFAULT7");
        if(this->default7 == m_missingTexture) this->checkLoadImage(&this->default7, "default-7", "OSU_SKIN_DEFAULT7");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-8");
        this->checkLoadImage(&this->default8, hitCircleStringFinal, "OSU_SKIN_DEFAULT8");
        if(this->default8 == m_missingTexture) this->checkLoadImage(&this->default8, "default-8", "OSU_SKIN_DEFAULT8");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-9");
        this->checkLoadImage(&this->default9, hitCircleStringFinal, "OSU_SKIN_DEFAULT9");
        if(this->default9 == m_missingTexture) this->checkLoadImage(&this->default9, "default-9", "OSU_SKIN_DEFAULT9");
    }

    this->randomizeFilePath();
    {
        std::string scorePrefix = this->sScorePrefix.length() > 0 ? this->sScorePrefix : "score";
        std::string scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-0");
        this->checkLoadImage(&this->score0, scoreStringFinal, "OSU_SKIN_SCORE0");
        if(this->score0 == m_missingTexture)
            this->checkLoadImage(&this->score0, "score-0",
                                 "OSU_SKIN_SCORE0");  // special cases: fallback to default skin score numbers if the
                                                      // defined prefix doesn't point to any valid files
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-1");
        this->checkLoadImage(&this->score1, scoreStringFinal, "OSU_SKIN_SCORE1");
        if(this->score1 == m_missingTexture) this->checkLoadImage(&this->score1, "score-1", "OSU_SKIN_SCORE1");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-2");
        this->checkLoadImage(&this->score2, scoreStringFinal, "OSU_SKIN_SCORE2");
        if(this->score2 == m_missingTexture) this->checkLoadImage(&this->score2, "score-2", "OSU_SKIN_SCORE2");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-3");
        this->checkLoadImage(&this->score3, scoreStringFinal, "OSU_SKIN_SCORE3");
        if(this->score3 == m_missingTexture) this->checkLoadImage(&this->score3, "score-3", "OSU_SKIN_SCORE3");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-4");
        this->checkLoadImage(&this->score4, scoreStringFinal, "OSU_SKIN_SCORE4");
        if(this->score4 == m_missingTexture) this->checkLoadImage(&this->score4, "score-4", "OSU_SKIN_SCORE4");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-5");
        this->checkLoadImage(&this->score5, scoreStringFinal, "OSU_SKIN_SCORE5");
        if(this->score5 == m_missingTexture) this->checkLoadImage(&this->score5, "score-5", "OSU_SKIN_SCORE5");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-6");
        this->checkLoadImage(&this->score6, scoreStringFinal, "OSU_SKIN_SCORE6");
        if(this->score6 == m_missingTexture) this->checkLoadImage(&this->score6, "score-6", "OSU_SKIN_SCORE6");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-7");
        this->checkLoadImage(&this->score7, scoreStringFinal, "OSU_SKIN_SCORE7");
        if(this->score7 == m_missingTexture) this->checkLoadImage(&this->score7, "score-7", "OSU_SKIN_SCORE7");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-8");
        this->checkLoadImage(&this->score8, scoreStringFinal, "OSU_SKIN_SCORE8");
        if(this->score8 == m_missingTexture) this->checkLoadImage(&this->score8, "score-8", "OSU_SKIN_SCORE8");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-9");
        this->checkLoadImage(&this->score9, scoreStringFinal, "OSU_SKIN_SCORE9");
        if(this->score9 == m_missingTexture) this->checkLoadImage(&this->score9, "score-9", "OSU_SKIN_SCORE9");

        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-x");
        this->checkLoadImage(&this->scoreX, scoreStringFinal, "OSU_SKIN_SCOREX");
        // if (this->scoreX == m_missingTexture) checkLoadImage(&m_scoreX, "score-x", "OSU_SKIN_SCOREX"); // special
        // case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are
        // simply not drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-percent");
        this->checkLoadImage(&this->scorePercent, scoreStringFinal, "OSU_SKIN_SCOREPERCENT");
        // if (this->scorePercent == m_missingTexture) checkLoadImage(&m_scorePercent, "score-percent",
        // "OSU_SKIN_SCOREPERCENT"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing
        // extraneous things like the X are simply not drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-dot");
        this->checkLoadImage(&this->scoreDot, scoreStringFinal, "OSU_SKIN_SCOREDOT");
        // if (this->scoreDot == m_missingTexture) checkLoadImage(&m_scoreDot, "score-dot", "OSU_SKIN_SCOREDOT"); //
        // special case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X
        // are simply not drawn
    }

    this->randomizeFilePath();
    {
        std::string comboPrefix = this->sComboPrefix.length() > 0
                                      ? this->sComboPrefix
                                      : "score";  // yes, "score" is the default value for the combo prefix
        std::string comboStringFinal = comboPrefix;
        comboStringFinal.append("-0");
        this->checkLoadImage(&this->combo0, comboStringFinal, "OSU_SKIN_COMBO0");
        if(this->combo0 == m_missingTexture)
            this->checkLoadImage(&this->combo0, "score-0",
                                 "OSU_SKIN_COMBO0");  // special cases: fallback to default skin combo numbers if the
                                                      // defined prefix doesn't point to any valid files
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-1");
        this->checkLoadImage(&this->combo1, comboStringFinal, "OSU_SKIN_COMBO1");
        if(this->combo1 == m_missingTexture) this->checkLoadImage(&this->combo1, "score-1", "OSU_SKIN_COMBO1");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-2");
        this->checkLoadImage(&this->combo2, comboStringFinal, "OSU_SKIN_COMBO2");
        if(this->combo2 == m_missingTexture) this->checkLoadImage(&this->combo2, "score-2", "OSU_SKIN_COMBO2");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-3");
        this->checkLoadImage(&this->combo3, comboStringFinal, "OSU_SKIN_COMBO3");
        if(this->combo3 == m_missingTexture) this->checkLoadImage(&this->combo3, "score-3", "OSU_SKIN_COMBO3");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-4");
        this->checkLoadImage(&this->combo4, comboStringFinal, "OSU_SKIN_COMBO4");
        if(this->combo4 == m_missingTexture) this->checkLoadImage(&this->combo4, "score-4", "OSU_SKIN_COMBO4");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-5");
        this->checkLoadImage(&this->combo5, comboStringFinal, "OSU_SKIN_COMBO5");
        if(this->combo5 == m_missingTexture) this->checkLoadImage(&this->combo5, "score-5", "OSU_SKIN_COMBO5");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-6");
        this->checkLoadImage(&this->combo6, comboStringFinal, "OSU_SKIN_COMBO6");
        if(this->combo6 == m_missingTexture) this->checkLoadImage(&this->combo6, "score-6", "OSU_SKIN_COMBO6");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-7");
        this->checkLoadImage(&this->combo7, comboStringFinal, "OSU_SKIN_COMBO7");
        if(this->combo7 == m_missingTexture) this->checkLoadImage(&this->combo7, "score-7", "OSU_SKIN_COMBO7");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-8");
        this->checkLoadImage(&this->combo8, comboStringFinal, "OSU_SKIN_COMBO8");
        if(this->combo8 == m_missingTexture) this->checkLoadImage(&this->combo8, "score-8", "OSU_SKIN_COMBO8");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-9");
        this->checkLoadImage(&this->combo9, comboStringFinal, "OSU_SKIN_COMBO9");
        if(this->combo9 == m_missingTexture) this->checkLoadImage(&this->combo9, "score-9", "OSU_SKIN_COMBO9");

        comboStringFinal = comboPrefix;
        comboStringFinal.append("-x");
        this->checkLoadImage(&this->comboX, comboStringFinal, "OSU_SKIN_COMBOX");
        // if (this->comboX == m_missingTexture) m_comboX = m_scoreX; // special case: ComboPrefix'd skins don't get
        // default fallbacks, instead missing extraneous things like the X are simply not drawn
    }

    this->randomizeFilePath();
    this->playSkip = this->createSkinImage("play-skip", Vector2(193, 147), 94);
    this->randomizeFilePath();
    this->checkLoadImage(&this->playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
    this->playWarningArrow2 = this->createSkinImage("play-warningarrow", Vector2(167, 129), 128);
    this->randomizeFilePath();
    this->checkLoadImage(&this->circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");
    this->randomizeFilePath();
    this->scorebarBg = this->createSkinImage("scorebar-bg", Vector2(695, 44), 27.5f);
    this->scorebarColour = this->createSkinImage("scorebar-colour", Vector2(645, 10), 6.25f);
    this->scorebarMarker = this->createSkinImage("scorebar-marker", Vector2(24, 24), 15.0f);
    this->scorebarKi = this->createSkinImage("scorebar-ki", Vector2(116, 116), 72.0f);
    this->scorebarKiDanger = this->createSkinImage("scorebar-kidanger", Vector2(116, 116), 72.0f);
    this->scorebarKiDanger2 = this->createSkinImage("scorebar-kidanger2", Vector2(116, 116), 72.0f);
    this->randomizeFilePath();
    this->sectionPassImage = this->createSkinImage("section-pass", Vector2(650, 650), 400.0f);
    this->randomizeFilePath();
    this->sectionFailImage = this->createSkinImage("section-fail", Vector2(650, 650), 400.0f);
    this->randomizeFilePath();
    this->inputoverlayBackground = this->createSkinImage("inputoverlay-background", Vector2(193, 55), 34.25f);
    this->inputoverlayKey = this->createSkinImage("inputoverlay-key", Vector2(43, 46), 26.75f);

    this->randomizeFilePath();
    this->hit0 = this->createSkinImage("hit0", Vector2(128, 128), 42);
    this->hit0->setAnimationFramerate(60);
    this->hit50 = this->createSkinImage("hit50", Vector2(128, 128), 42);
    this->hit50->setAnimationFramerate(60);
    this->hit50g = this->createSkinImage("hit50g", Vector2(128, 128), 42);
    this->hit50g->setAnimationFramerate(60);
    this->hit50k = this->createSkinImage("hit50k", Vector2(128, 128), 42);
    this->hit50k->setAnimationFramerate(60);
    this->hit100 = this->createSkinImage("hit100", Vector2(128, 128), 42);
    this->hit100->setAnimationFramerate(60);
    this->hit100g = this->createSkinImage("hit100g", Vector2(128, 128), 42);
    this->hit100g->setAnimationFramerate(60);
    this->hit100k = this->createSkinImage("hit100k", Vector2(128, 128), 42);
    this->hit100k->setAnimationFramerate(60);
    this->hit300 = this->createSkinImage("hit300", Vector2(128, 128), 42);
    this->hit300->setAnimationFramerate(60);
    this->hit300g = this->createSkinImage("hit300g", Vector2(128, 128), 42);
    this->hit300g->setAnimationFramerate(60);
    this->hit300k = this->createSkinImage("hit300k", Vector2(128, 128), 42);
    this->hit300k->setAnimationFramerate(60);

    this->randomizeFilePath();
    this->checkLoadImage(&this->particle50, "particle50", "OSU_SKIN_PARTICLE50", true);
    this->checkLoadImage(&this->particle100, "particle100", "OSU_SKIN_PARTICLE100", true);
    this->checkLoadImage(&this->particle300, "particle300", "OSU_SKIN_PARTICLE300", true);

    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
    this->randomizeFilePath();
    this->sliderb = this->createSkinImage("sliderb", Vector2(128, 128), 64, false, "");
    this->sliderb->setAnimationFramerate(/*45.0f*/ 50.0f);
    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
    this->randomizeFilePath();
    this->sliderFollowCircle2 = this->createSkinImage("sliderfollowcircle", Vector2(259, 259), 64);
    this->randomizeFilePath();
    this->checkLoadImage(
        &this->sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE",
        !this->bIsDefaultSkin);  // !m_bIsDefaultSkin ensures that default doesn't override user, in these special cases
    this->sliderStartCircle2 = this->createSkinImage("sliderstartcircle", Vector2(128, 128), 64, !this->bIsDefaultSkin);
    this->checkLoadImage(&this->sliderStartCircleOverlay, "sliderstartcircleoverlay",
                         "OSU_SKIN_SLIDERSTARTCIRCLEOVERLAY", !this->bIsDefaultSkin);
    this->sliderStartCircleOverlay2 =
        this->createSkinImage("sliderstartcircleoverlay", Vector2(128, 128), 64, !this->bIsDefaultSkin);
    this->sliderStartCircleOverlay2->setAnimationFramerate(2);
    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE", !this->bIsDefaultSkin);
    this->sliderEndCircle2 = this->createSkinImage("sliderendcircle", Vector2(128, 128), 64, !this->bIsDefaultSkin);
    this->checkLoadImage(&this->sliderEndCircleOverlay, "sliderendcircleoverlay", "OSU_SKIN_SLIDERENDCIRCLEOVERLAY",
                         !this->bIsDefaultSkin);
    this->sliderEndCircleOverlay2 =
        this->createSkinImage("sliderendcircleoverlay", Vector2(128, 128), 64, !this->bIsDefaultSkin);
    this->sliderEndCircleOverlay2->setAnimationFramerate(2);

    this->randomizeFilePath();
    this->checkLoadImage(&this->spinnerBackground, "spinner-background", "OSU_SKIN_SPINNERBACKGROUND");
    this->checkLoadImage(&this->spinnerCircle, "spinner-circle", "OSU_SKIN_SPINNERCIRCLE");
    this->checkLoadImage(&this->spinnerApproachCircle, "spinner-approachcircle", "OSU_SKIN_SPINNERAPPROACHCIRCLE");
    this->checkLoadImage(&this->spinnerBottom, "spinner-bottom", "OSU_SKIN_SPINNERBOTTOM");
    this->checkLoadImage(&this->spinnerMiddle, "spinner-middle", "OSU_SKIN_SPINNERMIDDLE");
    this->checkLoadImage(&this->spinnerMiddle2, "spinner-middle2", "OSU_SKIN_SPINNERMIDDLE2");
    this->checkLoadImage(&this->spinnerTop, "spinner-top", "OSU_SKIN_SPINNERTOP");
    this->checkLoadImage(&this->spinnerSpin, "spinner-spin", "OSU_SKIN_SPINNERSPIN");
    this->checkLoadImage(&this->spinnerClear, "spinner-clear", "OSU_SKIN_SPINNERCLEAR");

    {
        // cursor loading was here, moved up to improve async usability
    }

    this->randomizeFilePath();
    this->selectionModEasy = this->createSkinImage("selection-mod-easy", Vector2(68, 66), 38);
    this->selectionModNoFail = this->createSkinImage("selection-mod-nofail", Vector2(68, 66), 38);
    this->selectionModHalfTime = this->createSkinImage("selection-mod-halftime", Vector2(68, 66), 38);
    this->selectionModHardRock = this->createSkinImage("selection-mod-hardrock", Vector2(68, 66), 38);
    this->selectionModSuddenDeath = this->createSkinImage("selection-mod-suddendeath", Vector2(68, 66), 38);
    this->selectionModPerfect = this->createSkinImage("selection-mod-perfect", Vector2(68, 66), 38);
    this->selectionModDoubleTime = this->createSkinImage("selection-mod-doubletime", Vector2(68, 66), 38);
    this->selectionModNightCore = this->createSkinImage("selection-mod-nightcore", Vector2(68, 66), 38);
    this->selectionModDayCore = this->createSkinImage("selection-mod-daycore", Vector2(68, 66), 38);
    this->selectionModHidden = this->createSkinImage("selection-mod-hidden", Vector2(68, 66), 38);
    this->selectionModFlashlight = this->createSkinImage("selection-mod-flashlight", Vector2(68, 66), 38);
    this->selectionModRelax = this->createSkinImage("selection-mod-relax", Vector2(68, 66), 38);
    this->selectionModAutopilot = this->createSkinImage("selection-mod-relax2", Vector2(68, 66), 38);
    this->selectionModSpunOut = this->createSkinImage("selection-mod-spunout", Vector2(68, 66), 38);
    this->selectionModAutoplay = this->createSkinImage("selection-mod-autoplay", Vector2(68, 66), 38);
    this->selectionModNightmare = this->createSkinImage("selection-mod-nightmare", Vector2(68, 66), 38);
    this->selectionModTarget = this->createSkinImage("selection-mod-target", Vector2(68, 66), 38);
    this->selectionModScorev2 = this->createSkinImage("selection-mod-scorev2", Vector2(68, 66), 38);
    this->selectionModTD = this->createSkinImage("selection-mod-touchdevice", Vector2(68, 66), 38);
    this->selectionModCinema = this->createSkinImage("selection-mod-cinema", Vector2(68, 66), 38);

    this->mode_osu = this->createSkinImage("mode-osu", Vector2(32, 32), 32);
    this->mode_osu_small = this->createSkinImage("mode-osu-small", Vector2(32, 32), 32);

    this->randomizeFilePath();
    this->checkLoadImage(&this->pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
    this->checkLoadImage(&this->pauseReplay, "pause-replay", "OSU_SKIN_PAUSE_REPLAY");
    this->checkLoadImage(&this->pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
    this->checkLoadImage(&this->pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
    this->checkLoadImage(&this->pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY");
    if(this->pauseOverlay == m_missingTexture)
        this->checkLoadImage(&this->pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY", true, "jpg");
    this->checkLoadImage(&this->failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND");
    if(this->failBackground == m_missingTexture)
        this->checkLoadImage(&this->failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND", true, "jpg");
    this->checkLoadImage(&this->unpause, "unpause", "OSU_SKIN_UNPAUSE");

    this->randomizeFilePath();
    this->checkLoadImage(&this->buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
    this->checkLoadImage(&this->buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
    this->checkLoadImage(&this->buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
    this->randomizeFilePath();
    this->menuBackImg = this->createSkinImage("menu-back", Vector2(225, 87), 54);
    this->randomizeFilePath();

    // NOTE: scaling is ignored when drawing this specific element
    this->selectionMode = this->createSkinImage("selection-mode", Vector2(90, 90), 38);

    this->selectionModeOver = this->createSkinImage("selection-mode-over", Vector2(88, 90), 38);
    this->selectionMods = this->createSkinImage("selection-mods", Vector2(74, 90), 38);
    this->selectionModsOver = this->createSkinImage("selection-mods-over", Vector2(74, 90), 38);
    this->selectionRandom = this->createSkinImage("selection-random", Vector2(74, 90), 38);
    this->selectionRandomOver = this->createSkinImage("selection-random-over", Vector2(74, 90), 38);
    this->selectionOptions = this->createSkinImage("selection-options", Vector2(74, 90), 38);
    this->selectionOptionsOver = this->createSkinImage("selection-options-over", Vector2(74, 90), 38);

    this->randomizeFilePath();
    this->checkLoadImage(&this->songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
    this->checkLoadImage(&this->songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
    this->randomizeFilePath();
    this->checkLoadImage(&this->menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");
    this->menuButtonBackground2 = this->createSkinImage("menu-button-background", Vector2(699, 103), 64.0f);
    this->randomizeFilePath();
    this->checkLoadImage(&this->star, "star", "OSU_SKIN_STAR");

    this->randomizeFilePath();
    this->checkLoadImage(&this->rankingPanel, "ranking-panel", "OSU_SKIN_RANKING_PANEL");
    this->checkLoadImage(&this->rankingGraph, "ranking-graph", "OSU_SKIN_RANKING_GRAPH");
    this->checkLoadImage(&this->rankingTitle, "ranking-title", "OSU_SKIN_RANKING_TITLE");
    this->checkLoadImage(&this->rankingMaxCombo, "ranking-maxcombo", "OSU_SKIN_RANKING_MAXCOMBO");
    this->checkLoadImage(&this->rankingAccuracy, "ranking-accuracy", "OSU_SKIN_RANKING_ACCURACY");

    this->checkLoadImage(&this->rankingA, "ranking-A", "OSU_SKIN_RANKING_A");
    this->checkLoadImage(&this->rankingB, "ranking-B", "OSU_SKIN_RANKING_B");
    this->checkLoadImage(&this->rankingC, "ranking-C", "OSU_SKIN_RANKING_C");
    this->checkLoadImage(&this->rankingD, "ranking-D", "OSU_SKIN_RANKING_D");
    this->checkLoadImage(&this->rankingS, "ranking-S", "OSU_SKIN_RANKING_S");
    this->checkLoadImage(&this->rankingSH, "ranking-SH", "OSU_SKIN_RANKING_SH");
    this->checkLoadImage(&this->rankingX, "ranking-X", "OSU_SKIN_RANKING_X");
    this->checkLoadImage(&this->rankingXH, "ranking-XH", "OSU_SKIN_RANKING_XH");

    this->rankingAsmall = this->createSkinImage("ranking-A-small", Vector2(34, 40), 128);
    this->rankingBsmall = this->createSkinImage("ranking-B-small", Vector2(34, 40), 128);
    this->rankingCsmall = this->createSkinImage("ranking-C-small", Vector2(34, 40), 128);
    this->rankingDsmall = this->createSkinImage("ranking-D-small", Vector2(34, 40), 128);
    this->rankingSsmall = this->createSkinImage("ranking-S-small", Vector2(34, 40), 128);
    this->rankingSHsmall = this->createSkinImage("ranking-SH-small", Vector2(34, 40), 128);
    this->rankingXsmall = this->createSkinImage("ranking-X-small", Vector2(34, 40), 128);
    this->rankingXHsmall = this->createSkinImage("ranking-XH-small", Vector2(34, 40), 128);

    this->rankingPerfect = this->createSkinImage("ranking-perfect", Vector2(478, 150), 128);

    this->randomizeFilePath();
    this->checkLoadImage(&this->beatmapImportSpinner, "beatmapimport-spinner", "OSU_SKIN_BEATMAP_IMPORT_SPINNER");
    {
        // loading spinner load was here, moved up to improve async usability
    }
    this->checkLoadImage(&this->circleEmpty, "circle-empty", "OSU_SKIN_CIRCLE_EMPTY");
    this->checkLoadImage(&this->circleFull, "circle-full", "OSU_SKIN_CIRCLE_FULL");
    this->randomizeFilePath();
    this->checkLoadImage(&this->seekTriangle, "seektriangle", "OSU_SKIN_SEEKTRIANGLE");
    this->randomizeFilePath();
    this->checkLoadImage(&this->userIcon, "user-icon", "OSU_SKIN_USER_ICON");
    this->randomizeFilePath();
    this->checkLoadImage(&this->backgroundCube, "backgroundcube", "OSU_SKIN_FPOSU_BACKGROUNDCUBE", false, "png",
                         true);  // force mipmaps
    this->randomizeFilePath();
    this->checkLoadImage(&this->menuBackground, "menu-background", "OSU_SKIN_MENU_BACKGROUND", false, "jpg");
    this->randomizeFilePath();
    this->checkLoadImage(&this->skybox, "skybox", "OSU_SKIN_FPOSU_3D_SKYBOX");

    // sounds

    // samples
    this->checkLoadSound(&this->normalHitNormal, "normal-hitnormal", "OSU_SKIN_NORMALHITNORMAL_SND", true, true, false,
                         true, 0.8f);
    this->checkLoadSound(&this->normalHitWhistle, "normal-hitwhistle", "OSU_SKIN_NORMALHITWHISTLE_SND", true, true,
                         false, true, 0.85f);
    this->checkLoadSound(&this->normalHitFinish, "normal-hitfinish", "OSU_SKIN_NORMALHITFINISH_SND", true, true);
    this->checkLoadSound(&this->normalHitClap, "normal-hitclap", "OSU_SKIN_NORMALHITCLAP_SND", true, true, false, true,
                         0.85f);

    this->checkLoadSound(&this->normalSliderTick, "normal-slidertick", "OSU_SKIN_NORMALSLIDERTICK_SND", true, true);
    this->checkLoadSound(&this->normalSliderSlide, "normal-sliderslide", "OSU_SKIN_NORMALSLIDERSLIDE_SND", false, true,
                         true);
    this->checkLoadSound(&this->normalSliderWhistle, "normal-sliderwhistle", "OSU_SKIN_NORMALSLIDERWHISTLE_SND", true,
                         true);

    this->checkLoadSound(&this->softHitNormal, "soft-hitnormal", "OSU_SKIN_SOFTHITNORMAL_SND", true, true, false, true,
                         0.8f);
    this->checkLoadSound(&this->softHitWhistle, "soft-hitwhistle", "OSU_SKIN_SOFTHITWHISTLE_SND", true, true, false,
                         true, 0.85f);
    this->checkLoadSound(&this->softHitFinish, "soft-hitfinish", "OSU_SKIN_SOFTHITFINISH_SND", true, true);
    this->checkLoadSound(&this->softHitClap, "soft-hitclap", "OSU_SKIN_SOFTHITCLAP_SND", true, true, false, true,
                         0.85f);

    this->checkLoadSound(&this->softSliderTick, "soft-slidertick", "OSU_SKIN_SOFTSLIDERTICK_SND", true, true);
    this->checkLoadSound(&this->softSliderSlide, "soft-sliderslide", "OSU_SKIN_SOFTSLIDERSLIDE_SND", false, true, true);
    this->checkLoadSound(&this->softSliderWhistle, "soft-sliderwhistle", "OSU_SKIN_SOFTSLIDERWHISTLE_SND", true, true);

    this->checkLoadSound(&this->drumHitNormal, "drum-hitnormal", "OSU_SKIN_DRUMHITNORMAL_SND", true, true, false, true,
                         0.8f);
    this->checkLoadSound(&this->drumHitWhistle, "drum-hitwhistle", "OSU_SKIN_DRUMHITWHISTLE_SND", true, true, false,
                         true, 0.85f);
    this->checkLoadSound(&this->drumHitFinish, "drum-hitfinish", "OSU_SKIN_DRUMHITFINISH_SND", true, true);
    this->checkLoadSound(&this->drumHitClap, "drum-hitclap", "OSU_SKIN_DRUMHITCLAP_SND", true, true, false, true,
                         0.85f);

    this->checkLoadSound(&this->drumSliderTick, "drum-slidertick", "OSU_SKIN_DRUMSLIDERTICK_SND", true, true);
    this->checkLoadSound(&this->drumSliderSlide, "drum-sliderslide", "OSU_SKIN_DRUMSLIDERSLIDE_SND", false, true, true);
    this->checkLoadSound(&this->drumSliderWhistle, "drum-sliderwhistle", "OSU_SKIN_DRUMSLIDERWHISTLE_SND", true, true);

    this->checkLoadSound(&this->spinnerBonus, "spinnerbonus", "OSU_SKIN_SPINNERBONUS_SND", true, true);
    this->checkLoadSound(&this->spinnerSpinSound, "spinnerspin", "OSU_SKIN_SPINNERSPIN_SND", false, true, true);

    // others
    this->checkLoadSound(&this->combobreak, "combobreak", "OSU_SKIN_COMBOBREAK_SND", true, true);
    this->checkLoadSound(&this->failsound, "failsound", "OSU_SKIN_FAILSOUND_SND");
    this->checkLoadSound(&this->applause, "applause", "OSU_SKIN_APPLAUSE_SND");
    this->checkLoadSound(&this->menuHit, "menuhit", "OSU_SKIN_MENUHIT_SND", true, true);
    this->checkLoadSound(&this->menuHover, "menuclick", "OSU_SKIN_MENUCLICK_SND", true, true);
    this->checkLoadSound(&this->checkOn, "check-on", "OSU_SKIN_CHECKON_SND", true, true);
    this->checkLoadSound(&this->checkOff, "check-off", "OSU_SKIN_CHECKOFF_SND", true, true);
    this->checkLoadSound(&this->shutter, "shutter", "OSU_SKIN_SHUTTER_SND", true, true);
    this->checkLoadSound(&this->sectionPassSound, "sectionpass", "OSU_SKIN_SECTIONPASS_SND");
    this->checkLoadSound(&this->sectionFailSound, "sectionfail", "OSU_SKIN_SECTIONFAIL_SND");

    // UI feedback
    this->checkLoadSound(&this->messageSent, "key-confirm", "OSU_SKIN_MESSAGE_SENT_SND", true, true, false);
    this->checkLoadSound(&this->deletingText, "key-delete", "OSU_SKIN_DELETING_TEXT_SND", true, true, false);
    this->checkLoadSound(&this->movingTextCursor, "key-movement", "OSU_MOVING_TEXT_CURSOR_SND", true, true, false);
    this->checkLoadSound(&this->typing1, "key-press-1", "OSU_TYPING_1_SND", true, true, false);
    this->checkLoadSound(&this->typing2, "key-press-2", "OSU_TYPING_2_SND", true, true, false, false);
    this->checkLoadSound(&this->typing3, "key-press-3", "OSU_TYPING_3_SND", true, true, false, false);
    this->checkLoadSound(&this->typing4, "key-press-4", "OSU_TYPING_4_SND", true, true, false, false);
    this->checkLoadSound(&this->menuBack, "menuback", "OSU_MENU_BACK_SND", true, true, false, false);
    this->checkLoadSound(&this->closeChatTab, "click-close", "OSU_CLOSE_CHAT_TAB_SND", true, true, false, false);
    this->checkLoadSound(&this->clickButton, "click-short-confirm", "OSU_CLICK_BUTTON_SND", true, true, false, false);
    this->checkLoadSound(&this->hoverButton, "click-short", "OSU_HOVER_BUTTON_SND", true, true, false, false);
    this->checkLoadSound(&this->backButtonClick, "back-button-click", "OSU_BACK_BUTTON_CLICK_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->backButtonHover, "back-button-hover", "OSU_BACK_BUTTON_HOVER_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->clickMainMenuCube, "menu-play-click", "OSU_CLICK_MAIN_MENU_CUBE_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->hoverMainMenuCube, "menu-play-hover", "OSU_HOVER_MAIN_MENU_CUBE_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->clickSingleplayer, "menu-freeplay-click", "OSU_CLICK_SINGLEPLAYER_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->hoverSingleplayer, "menu-freeplay-hover", "OSU_HOVER_SINGLEPLAYER_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->clickMultiplayer, "menu-multiplayer-click", "OSU_CLICK_MULTIPLAYER_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->hoverMultiplayer, "menu-multiplayer-hover", "OSU_HOVER_MULTIPLAYER_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->clickOptions, "menu-options-click", "OSU_CLICK_OPTIONS_SND", true, true, false, false);
    this->checkLoadSound(&this->hoverOptions, "menu-options-hover", "OSU_HOVER_OPTIONS_SND", true, true, false, false);
    this->checkLoadSound(&this->clickExit, "menu-exit-click", "OSU_CLICK_EXIT_SND", true, true, false, false);
    this->checkLoadSound(&this->hoverExit, "menu-exit-hover", "OSU_HOVER_EXIT_SND", true, true, false, false);
    this->checkLoadSound(&this->expand, "select-expand", "OSU_EXPAND_SND", true, true, false);
    this->checkLoadSound(&this->selectDifficulty, "select-difficulty", "OSU_SELECT_DIFFICULTY_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->sliderbar, "sliderbar", "OSU_DRAG_SLIDER_SND", true, true, false);
    this->checkLoadSound(&this->matchConfirm, "match-confirm", "OSU_ALL_PLAYERS_READY_SND", true, true, false);
    this->checkLoadSound(&this->roomJoined, "match-join", "OSU_ROOM_JOINED_SND", true, true, false);
    this->checkLoadSound(&this->roomQuit, "match-leave", "OSU_ROOM_QUIT_SND", true, true, false);
    this->checkLoadSound(&this->roomNotReady, "match-notready", "OSU_ROOM_NOT_READY_SND", true, true, false);
    this->checkLoadSound(&this->roomReady, "match-ready", "OSU_ROOM_READY_SND", true, true, false);
    this->checkLoadSound(&this->matchStart, "match-start", "OSU_MATCH_START_SND", true, true, false);

    this->checkLoadSound(&this->pauseLoop, "pause-loop", "OSU_PAUSE_LOOP_SND", false, false, true, true);
    this->checkLoadSound(&this->pauseHover, "pause-hover", "OSU_PAUSE_HOVER_SND", true, true, false, false);
    this->checkLoadSound(&this->clickPauseBack, "pause-back-click", "OSU_CLICK_QUIT_SONG_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->hoverPauseBack, "pause-back-hover", "OSU_HOVER_QUIT_SONG_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->clickPauseContinue, "pause-continue-click", "OSU_CLICK_RESUME_SONG_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->hoverPauseContinue, "pause-continue-hover", "OSU_HOVER_RESUME_SONG_SND", true, true,
                         false, false);
    this->checkLoadSound(&this->clickPauseRetry, "pause-retry-click", "OSU_CLICK_RETRY_SONG_SND", true, true, false,
                         false);
    this->checkLoadSound(&this->hoverPauseRetry, "pause-retry-hover", "OSU_HOVER_RETRY_SONG_SND", true, true, false,
                         false);

    if(this->clickButton == NULL) this->clickButton = this->menuHit;
    if(this->hoverButton == NULL) this->hoverButton = this->menuHover;
    if(this->pauseHover == NULL) this->pauseHover = this->hoverButton;
    if(this->selectDifficulty == NULL) this->selectDifficulty = this->clickButton;
    if(this->typing2 == NULL) this->typing2 = this->typing1;
    if(this->typing3 == NULL) this->typing3 = this->typing2;
    if(this->typing4 == NULL) this->typing4 = this->typing3;
    if(this->backButtonClick == NULL) this->backButtonClick = this->clickButton;
    if(this->backButtonHover == NULL) this->backButtonHover = this->hoverButton;
    if(this->menuBack == NULL) this->menuBack = this->clickButton;
    if(this->closeChatTab == NULL) this->closeChatTab = this->clickButton;
    if(this->clickMainMenuCube == NULL) this->clickMainMenuCube = this->clickButton;
    if(this->hoverMainMenuCube == NULL) this->hoverMainMenuCube = this->menuHover;
    if(this->clickSingleplayer == NULL) this->clickSingleplayer = this->clickButton;
    if(this->hoverSingleplayer == NULL) this->hoverSingleplayer = this->menuHover;
    if(this->clickMultiplayer == NULL) this->clickMultiplayer = this->clickButton;
    if(this->hoverMultiplayer == NULL) this->hoverMultiplayer = this->menuHover;
    if(this->clickOptions == NULL) this->clickOptions = this->clickButton;
    if(this->hoverOptions == NULL) this->hoverOptions = this->menuHover;
    if(this->clickExit == NULL) this->clickExit = this->clickButton;
    if(this->hoverExit == NULL) this->hoverExit = this->menuHover;
    if(this->clickPauseBack == NULL) this->clickPauseBack = this->clickButton;
    if(this->hoverPauseBack == NULL) this->hoverPauseBack = this->pauseHover;
    if(this->clickPauseContinue == NULL) this->clickPauseContinue = this->clickButton;
    if(this->hoverPauseContinue == NULL) this->hoverPauseContinue = this->pauseHover;
    if(this->clickPauseRetry == NULL) this->clickPauseRetry = this->clickButton;
    if(this->hoverPauseRetry == NULL) this->hoverPauseRetry = this->pauseHover;

    // scaling
    // HACKHACK: this is pure cancer
    if(this->cursor != NULL && this->cursor->getFilePath().find("@2x") != -1) this->bCursor2x = true;
    if(this->cursorTrail != NULL && this->cursorTrail->getFilePath().find("@2x") != -1) this->bCursorTrail2x = true;
    if(this->cursorRipple != NULL && this->cursorRipple->getFilePath().find("@2x") != -1) this->bCursorRipple2x = true;
    if(this->approachCircle != NULL && this->approachCircle->getFilePath().find("@2x") != -1)
        this->bApproachCircle2x = true;
    if(this->reverseArrow != NULL && this->reverseArrow->getFilePath().find("@2x") != -1) this->bReverseArrow2x = true;
    if(this->hitCircle != NULL && this->hitCircle->getFilePath().find("@2x") != -1) this->bHitCircle2x = true;
    if(this->default0 != NULL && this->default0->getFilePath().find("@2x") != -1) this->bIsDefault02x = true;
    if(this->default1 != NULL && this->default1->getFilePath().find("@2x") != -1) this->bIsDefault12x = true;
    if(this->score0 != NULL && this->score0->getFilePath().find("@2x") != -1) this->bIsScore02x = true;
    if(this->combo0 != NULL && this->combo0->getFilePath().find("@2x") != -1) this->bIsCombo02x = true;
    if(this->spinnerApproachCircle != NULL && this->spinnerApproachCircle->getFilePath().find("@2x") != -1)
        this->bSpinnerApproachCircle2x = true;
    if(this->spinnerBottom != NULL && this->spinnerBottom->getFilePath().find("@2x") != -1)
        this->bSpinnerBottom2x = true;
    if(this->spinnerCircle != NULL && this->spinnerCircle->getFilePath().find("@2x") != -1)
        this->bSpinnerCircle2x = true;
    if(this->spinnerTop != NULL && this->spinnerTop->getFilePath().find("@2x") != -1) this->bSpinnerTop2x = true;
    if(this->spinnerMiddle != NULL && this->spinnerMiddle->getFilePath().find("@2x") != -1)
        this->bSpinnerMiddle2x = true;
    if(this->spinnerMiddle2 != NULL && this->spinnerMiddle2->getFilePath().find("@2x") != -1)
        this->bSpinnerMiddle22x = true;
    if(this->sliderScorePoint != NULL && this->sliderScorePoint->getFilePath().find("@2x") != -1)
        this->bSliderScorePoint2x = true;
    if(this->sliderStartCircle != NULL && this->sliderStartCircle->getFilePath().find("@2x") != -1)
        this->bSliderStartCircle2x = true;
    if(this->sliderEndCircle != NULL && this->sliderEndCircle->getFilePath().find("@2x") != -1)
        this->bSliderEndCircle2x = true;

    if(this->circularmetre != NULL && this->circularmetre->getFilePath().find("@2x") != -1)
        this->bCircularmetre2x = true;

    if(this->pauseContinue != NULL && this->pauseContinue->getFilePath().find("@2x") != -1)
        this->bPauseContinue2x = true;

    if(this->menuButtonBackground != NULL && this->menuButtonBackground->getFilePath().find("@2x") != -1)
        this->bMenuButtonBackground2x = true;
    if(this->star != NULL && this->star->getFilePath().find("@2x") != -1) this->bStar2x = true;
    if(this->rankingPanel != NULL && this->rankingPanel->getFilePath().find("@2x") != -1) this->bRankingPanel2x = true;
    if(this->rankingMaxCombo != NULL && this->rankingMaxCombo->getFilePath().find("@2x") != -1)
        this->bRankingMaxCombo2x = true;
    if(this->rankingAccuracy != NULL && this->rankingAccuracy->getFilePath().find("@2x") != -1)
        this->bRankingAccuracy2x = true;
    if(this->rankingA != NULL && this->rankingA->getFilePath().find("@2x") != -1) this->bRankingA2x = true;
    if(this->rankingB != NULL && this->rankingB->getFilePath().find("@2x") != -1) this->bRankingB2x = true;
    if(this->rankingC != NULL && this->rankingC->getFilePath().find("@2x") != -1) this->bRankingC2x = true;
    if(this->rankingD != NULL && this->rankingD->getFilePath().find("@2x") != -1) this->bRankingD2x = true;
    if(this->rankingS != NULL && this->rankingS->getFilePath().find("@2x") != -1) this->bRankingS2x = true;
    if(this->rankingSH != NULL && this->rankingSH->getFilePath().find("@2x") != -1) this->bRankingSH2x = true;
    if(this->rankingX != NULL && this->rankingX->getFilePath().find("@2x") != -1) this->bRankingX2x = true;
    if(this->rankingXH != NULL && this->rankingXH->getFilePath().find("@2x") != -1) this->bRankingXH2x = true;

    // HACKHACK: all of the <>2 loads are temporary fixes until I fix the checkLoadImage() function logic

    // custom
    Image *defaultCursor = resourceManager->getImage("OSU_SKIN_CURSOR_DEFAULT");
    Image *defaultCursor2 = this->cursor;
    if(defaultCursor != NULL)
        this->defaultCursor = defaultCursor;
    else if(defaultCursor2 != NULL)
        this->defaultCursor = defaultCursor2;

    Image *defaultButtonLeft = resourceManager->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
    Image *defaultButtonLeft2 = this->buttonLeft;
    if(defaultButtonLeft != NULL)
        this->defaultButtonLeft = defaultButtonLeft;
    else if(defaultButtonLeft2 != NULL)
        this->defaultButtonLeft = defaultButtonLeft2;

    Image *defaultButtonMiddle = resourceManager->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
    Image *defaultButtonMiddle2 = this->buttonMiddle;
    if(defaultButtonMiddle != NULL)
        this->defaultButtonMiddle = defaultButtonMiddle;
    else if(defaultButtonMiddle2 != NULL)
        this->defaultButtonMiddle = defaultButtonMiddle2;

    Image *defaultButtonRight = resourceManager->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
    Image *defaultButtonRight2 = this->buttonRight;
    if(defaultButtonRight != NULL)
        this->defaultButtonRight = defaultButtonRight;
    else if(defaultButtonRight2 != NULL)
        this->defaultButtonRight = defaultButtonRight2;

    // print some debug info
    debugLog("Skin: Version %f\n", this->fVersion);
    debugLog("Skin: HitCircleOverlap = %i\n", this->iHitCircleOverlap);

    // delayed error notifications due to resource loading potentially blocking engine time
    if(!parseSkinIni1Status && parseSkinIni2Status && cv_skin.getString() != UString("default"))
        osu->getNotificationOverlay()->addNotification("Error: Couldn't load skin.ini!", 0xffff0000);
    else if(!parseSkinIni2Status)
        osu->getNotificationOverlay()->addNotification("Error: Couldn't load DEFAULT skin.ini!!!", 0xffff0000);
}

void Skin::loadBeatmapOverride(std::string filepath) {
    // debugLog("Skin::loadBeatmapOverride( %s )\n", filepath.c_str());
    //  TODO: beatmap skin support
}

void Skin::reloadSounds() {
	std::vector<Resource*> soundResources;
	soundResources.reserve(this->sounds.size());

	for (auto & sound : this->sounds)
	{
		soundResources.push_back(sound);
	}

	resourceManager->reloadResources(soundResources, cv_skin_async.getBool());
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
                        UString versionString = stringBuffer;
                        if(versionString.find("latest") != -1 || versionString.find("User") != -1)
                            this->fVersion = 2.5f;  // default to latest version available
                        else
                            this->fVersion = versionString.toFloat();
                    }

                    if(sscanf(curLine.c_str(), " CursorRotate : %i \n", &val) == 1)
                        this->bCursorRotate = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " CursorCentre : %i \n", &val) == 1)
                        this->bCursorCenter = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " CursorExpand : %i \n", &val) == 1)
                        this->bCursorExpand = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " SliderBallFlip : %i \n", &val) == 1)
                        this->bSliderBallFlip = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " AllowSliderBallTint : %i \n", &val) == 1)
                        this->bAllowSliderBallTint = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " HitCircleOverlayAboveNumber : %i \n", &val) == 1)
                        this->bHitCircleOverlayAboveNumber = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " HitCircleOverlayAboveNumer : %i \n", &val) == 1)
                        this->bHitCircleOverlayAboveNumber = val > 0 ? true : false;
                    if(sscanf(curLine.c_str(), " SliderStyle : %i \n", &val) == 1) {
                        this->iSliderStyle = val;
                        if(this->iSliderStyle != 1 && this->iSliderStyle != 2) this->iSliderStyle = 2;
                    }
                    if(sscanf(curLine.c_str(), " AnimationFramerate : %f \n", &floatVal) == 1)
                        this->fAnimationFramerate = floatVal < 0 ? 0.0f : floatVal;
                } break;
                case 1:  // Colors
                {
                    int comboNum;
                    int r, g, b;

                    if(sscanf(curLine.c_str(), " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
                        this->comboColors.push_back(argb(255, r, g, b));
                    if(sscanf(curLine.c_str(), " SpinnerApproachCircle : %i , %i , %i \n", &r, &g, &b) == 3)
                        this->spinnerApproachCircleColor = argb(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SliderBorder: %i , %i , %i \n", &r, &g, &b) == 3)
                        this->sliderBorderColor = argb(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SliderTrackOverride : %i , %i , %i \n", &r, &g, &b) == 3) {
                        this->sliderTrackOverride = argb(255, r, g, b);
                        this->bSliderTrackOverride = true;
                    }
                    if(sscanf(curLine.c_str(), " SliderBall : %i , %i , %i \n", &r, &g, &b) == 3)
                        this->sliderBallColor = argb(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SongSelectActiveText : %i , %i , %i \n", &r, &g, &b) == 3)
                        this->songSelectActiveText = argb(255, r, g, b);
                    if(sscanf(curLine.c_str(), " SongSelectInactiveText : %i , %i , %i \n", &r, &g, &b) == 3)
                        this->songSelectInactiveText = argb(255, r, g, b);
                    if(sscanf(curLine.c_str(), " InputOverlayText : %i , %i , %i \n", &r, &g, &b) == 3)
                        this->inputOverlayText = argb(255, r, g, b);
                } break;
                case 2:  // Fonts
                {
                    int val;
                    char stringBuffer[1024];

                    memset(stringBuffer, '\0', 1024);
                    if(sscanf(curLine.c_str(), " ComboPrefix : %1023[^\n]", stringBuffer) == 1) {
                        this->sComboPrefix = stringBuffer;

                        for(int i = 0; i < this->sComboPrefix.length(); i++) {
                            if(this->sComboPrefix[i] == '\\') {
                                this->sComboPrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " ComboOverlap : %i \n", &val) == 1) this->iComboOverlap = val;

                    if(sscanf(curLine.c_str(), " ScorePrefix : %1023[^\n]", stringBuffer) == 1) {
                        this->sScorePrefix = stringBuffer;

                        for(int i = 0; i < this->sScorePrefix.length(); i++) {
                            if(this->sScorePrefix[i] == '\\') {
                                this->sScorePrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " ScoreOverlap : %i \n", &val) == 1) this->iScoreOverlap = val;

                    if(sscanf(curLine.c_str(), " HitCirclePrefix : %1023[^\n]", stringBuffer) == 1) {
                        this->sHitCirclePrefix = stringBuffer;

                        for(int i = 0; i < this->sHitCirclePrefix.length(); i++) {
                            if(this->sHitCirclePrefix[i] == '\\') {
                                this->sHitCirclePrefix[i] = '/';
                            }
                        }
                    }
                    if(sscanf(curLine.c_str(), " HitCircleOverlap : %i \n", &val) == 1) this->iHitCircleOverlap = val;
                } break;
            }
        }
    }

    return true;
}

void Skin::onIgnoreBeatmapSampleVolumeChange(UString oldValue, UString newValue) { this->resetSampleVolume(); }

void Skin::setSampleSet(int sampleSet) {
    if(this->iSampleSet == sampleSet) return;

    /// debugLog("sample set = %i\n", sampleSet);
    this->iSampleSet = sampleSet;
}

void Skin::resetSampleVolume() {
    this->setSampleVolume(std::clamp<float>((float)this->iSampleVolume / 100.0f, 0.0f, 1.0f), true);
}

void Skin::setSampleVolume(float volume, bool force) {
    if(cv_ignore_beatmap_sample_volume.getBool() && (int)(cv_volume_effects.getFloat() * 100.0f) == this->iSampleVolume)
        return;

    const float newSampleVolume =
        (!cv_ignore_beatmap_sample_volume.getBool() ? volume : 1.0f) * cv_volume_effects.getFloat();

    if(!force && this->iSampleVolume == (int)(newSampleVolume * 100.0f)) return;

    this->iSampleVolume = (int)(newSampleVolume * 100.0f);
    /// debugLog("sample volume = %f\n", sampleVolume);
    for(int i = 0; i < this->soundSamples.size(); i++) {
        this->soundSamples[i].sound->setVolume(newSampleVolume * this->soundSamples[i].hardcodedVolumeMultiplier);
    }
}

Color Skin::getComboColorForCounter(int i, int offset) {
    i += cv_skin_color_index_add.getInt();
    i = std::max(i, 0);

    if(this->beatmapComboColors.size() > 0 && !cv_ignore_beatmap_combo_colors.getBool())
        return this->beatmapComboColors[(i + offset) % this->beatmapComboColors.size()];
    else if(this->comboColors.size() > 0)
        return this->comboColors[i % this->comboColors.size()];
    else
        return argb(255, 0, 255, 0);
}

void Skin::setBeatmapComboColors(std::vector<Color> colors) { this->beatmapComboColors = colors; }

void Skin::playHitCircleSound(int sampleType, float pan, long delta) {
    if(this->iSampleVolume <= 0) {
        return;
    }

    if(!cv_sound_panning.getBool() || (cv_mod_fposu.getBool() && !cv_mod_fposu_sound_panning.getBool()) ||
       (cv_mod_fps.getBool() && !cv_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv_sound_panning_multiplier.getFloat();
    }

    f32 pitch = 0.f;
    if(cv_snd_pitch_hitsounds.getBool()) {
        auto bm = osu->getSelectedBeatmap();
        if(bm) {
            f32 range = bm->getHitWindow100();
            pitch = (f32)delta / range * cv_snd_pitch_hitsounds_factor.getFloat();
        }
    }

    int actualSampleSet = this->iSampleSet;
    if(cv_skin_force_hitsound_sample_set.getInt() > 0) actualSampleSet = cv_skin_force_hitsound_sample_set.getInt();

    switch(actualSampleSet) {
        case 3:
            soundEngine->play(this->drumHitNormal, pan, pitch);

            if(sampleType & OSU_BITMASK_HITWHISTLE) soundEngine->play(this->drumHitWhistle, pan, pitch);
            if(sampleType & OSU_BITMASK_HITFINISH) soundEngine->play(this->drumHitFinish, pan, pitch);
            if(sampleType & OSU_BITMASK_HITCLAP) soundEngine->play(this->drumHitClap, pan, pitch);
            break;
        case 2:
            soundEngine->play(this->softHitNormal, pan, pitch);

            if(sampleType & OSU_BITMASK_HITWHISTLE) soundEngine->play(this->softHitWhistle, pan, pitch);
            if(sampleType & OSU_BITMASK_HITFINISH) soundEngine->play(this->softHitFinish, pan, pitch);
            if(sampleType & OSU_BITMASK_HITCLAP) soundEngine->play(this->softHitClap, pan, pitch);
            break;
        default:
            soundEngine->play(this->normalHitNormal, pan, pitch);

            if(sampleType & OSU_BITMASK_HITWHISTLE) soundEngine->play(this->normalHitWhistle, pan, pitch);
            if(sampleType & OSU_BITMASK_HITFINISH) soundEngine->play(this->normalHitFinish, pan, pitch);
            if(sampleType & OSU_BITMASK_HITCLAP) soundEngine->play(this->normalHitClap, pan, pitch);
            break;
    }
}

void Skin::playSliderTickSound(float pan) {
    if(this->iSampleVolume <= 0) return;

    if(!cv_sound_panning.getBool() || (cv_mod_fposu.getBool() && !cv_mod_fposu_sound_panning.getBool()) ||
       (cv_mod_fps.getBool() && !cv_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv_sound_panning_multiplier.getFloat();
    }

    switch(this->iSampleSet) {
        case 3:
            soundEngine->play(this->drumSliderTick, pan);
            break;
        case 2:
            soundEngine->play(this->softSliderTick, pan);
            break;
        default:
            soundEngine->play(this->normalSliderTick, pan);
            break;
    }
}

void Skin::playSliderSlideSound(float pan) {
    if(!cv_sound_panning.getBool() || (cv_mod_fposu.getBool() && !cv_mod_fposu_sound_panning.getBool()) ||
       (cv_mod_fps.getBool() && !cv_mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv_sound_panning_multiplier.getFloat();
    }

    switch(this->iSampleSet) {
        case 3:
            if(this->softSliderSlide->isPlaying()) soundEngine->stop(this->softSliderSlide);
            if(this->normalSliderSlide->isPlaying()) soundEngine->stop(this->normalSliderSlide);

            if(!this->drumSliderSlide->isPlaying())
                soundEngine->play(this->drumSliderSlide, pan);
            else
                this->drumSliderSlide->setPan(pan);
            break;
        case 2:
            if(this->drumSliderSlide->isPlaying()) soundEngine->stop(this->drumSliderSlide);
            if(this->normalSliderSlide->isPlaying()) soundEngine->stop(this->normalSliderSlide);

            if(!this->softSliderSlide->isPlaying())
                soundEngine->play(this->softSliderSlide, pan);
            else
                this->softSliderSlide->setPan(pan);
            break;
        default:
            if(this->softSliderSlide->isPlaying()) soundEngine->stop(this->softSliderSlide);
            if(this->drumSliderSlide->isPlaying()) soundEngine->stop(this->drumSliderSlide);

            if(!this->normalSliderSlide->isPlaying())
                soundEngine->play(this->normalSliderSlide, pan);
            else
                this->normalSliderSlide->setPan(pan);
            break;
    }
}

void Skin::playSpinnerSpinSound() {
    if(!this->spinnerSpinSound->isPlaying()) {
        soundEngine->play(this->spinnerSpinSound);
    }
}

void Skin::playSpinnerBonusSound() {
    if(this->iSampleVolume > 0) {
        soundEngine->play(this->spinnerBonus);
    }
}

void Skin::stopSliderSlideSound(int sampleSet) {
    if((sampleSet == -2 || sampleSet == 3) && this->drumSliderSlide->isPlaying())
        soundEngine->stop(this->drumSliderSlide);
    if((sampleSet == -2 || sampleSet == 2) && this->softSliderSlide->isPlaying())
        soundEngine->stop(this->softSliderSlide);
    if((sampleSet == -2 || sampleSet == 1 || sampleSet == 0) && this->normalSliderSlide->isPlaying())
        soundEngine->stop(this->normalSliderSlide);
}

void Skin::stopSpinnerSpinSound() {
    if(this->spinnerSpinSound->isPlaying()) soundEngine->stop(this->spinnerSpinSound);
}

void Skin::randomizeFilePath() {
    if(this->bIsRandomElements && this->filepathsForRandomSkin.size() > 0)
        this->sFilePath = this->filepathsForRandomSkin[rand() % this->filepathsForRandomSkin.size()];
}

SkinImage *Skin::createSkinImage(std::string skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
                                 bool ignoreDefaultSkin, std::string animationSeparator) {
    SkinImage *skinImage =
        new SkinImage(this, skinElementName, baseSizeForScaling2x, osuSize, animationSeparator, ignoreDefaultSkin);
    this->images.push_back(skinImage);

    const std::vector<std::string> &filepathsForExport = skinImage->getFilepathsForExport();
    this->filepathsForExport.insert(this->filepathsForExport.end(), filepathsForExport.begin(),
                                    filepathsForExport.end());

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

    std::string filepath1 = this->sFilePath;
    filepath1.append(skinElementName);
    filepath1.append("@2x.");
    filepath1.append(fileExtension);

    std::string filepath2 = this->sFilePath;
    filepath2.append(skinElementName);
    filepath2.append(".");
    filepath2.append(fileExtension);

    const bool existsDefaultFilePath1 = env->fileExists(defaultFilePath1);
    const bool existsDefaultFilePath2 = env->fileExists(defaultFilePath2);
    const bool existsFilepath1 = env->fileExists(filepath1);
    const bool existsFilepath2 = env->fileExists(filepath2);

    // check if an @2x version of this image exists
    if(cv_skin_hd.getBool()) {
        // load default skin
        if(!ignoreDefaultSkin) {
            if(existsDefaultFilePath1) {
                std::string defaultResourceName = resourceName;
                defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                if(cv_skin_async.getBool()) resourceManager->requestNextLoadAsync();

                *addressOfPointer = resourceManager->loadImageAbs(
                    defaultFilePath1, defaultResourceName, cv_skin_mipmaps.getBool() || forceLoadMipmaps);
                (*addressOfPointer)->is_2x = true;
            } else {
                // fallback to @1x
                if(existsDefaultFilePath2) {
                    std::string defaultResourceName = resourceName;
                    defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                    if(cv_skin_async.getBool()) resourceManager->requestNextLoadAsync();

                    *addressOfPointer = resourceManager->loadImageAbs(
                        defaultFilePath2, defaultResourceName, cv_skin_mipmaps.getBool() || forceLoadMipmaps);
                    (*addressOfPointer)->is_2x = false;
                }
            }
        }

        // load user skin
        if(existsFilepath1) {
            if(cv_skin_async.getBool()) resourceManager->requestNextLoadAsync();

            *addressOfPointer = resourceManager->loadImageAbs(
                filepath1, "", cv_skin_mipmaps.getBool() || forceLoadMipmaps);
            (*addressOfPointer)->is_2x = true;
            this->resources.push_back(*addressOfPointer);

            // export
            if(existsFilepath1) this->filepathsForExport.push_back(filepath1);
            if(existsFilepath2) this->filepathsForExport.push_back(filepath2);

            if(!existsFilepath1 && !existsFilepath2) {
                if(existsDefaultFilePath1) this->filepathsForExport.push_back(defaultFilePath1);
                if(existsDefaultFilePath2) this->filepathsForExport.push_back(defaultFilePath2);
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

            if(cv_skin_async.getBool()) resourceManager->requestNextLoadAsync();

            *addressOfPointer = resourceManager->loadImageAbs(
                defaultFilePath2, defaultResourceName, cv_skin_mipmaps.getBool() || forceLoadMipmaps);
            (*addressOfPointer)->is_2x = false;
        }
    }

    // load user skin
    if(existsFilepath2) {
        if(cv_skin_async.getBool()) resourceManager->requestNextLoadAsync();

        *addressOfPointer =
            resourceManager->loadImageAbs(filepath2, "", cv_skin_mipmaps.getBool() || forceLoadMipmaps);
        (*addressOfPointer)->is_2x = false;
        this->resources.push_back(*addressOfPointer);
    }

    // export
    if(existsFilepath1) this->filepathsForExport.push_back(filepath1);
    if(existsFilepath2) this->filepathsForExport.push_back(filepath2);

    if(!existsFilepath1 && !existsFilepath2) {
        if(existsDefaultFilePath1) this->filepathsForExport.push_back(defaultFilePath1);
        if(existsDefaultFilePath2) this->filepathsForExport.push_back(defaultFilePath2);
    }
}

void Skin::checkLoadSound(Sound **addressOfPointer, std::string skinElementName, std::string resourceName,
                          bool isOverlayable, bool isSample, bool loop, bool fallback_to_default,
                          float hardcodedVolumeMultiplier) {
    if(*addressOfPointer != NULL) return;  // we are already loaded

    // NOTE: only the default skin is loaded with a resource name (it must never be unloaded by other instances), and it
    // is NOT added to the resources vector

    // random skin support
    this->randomizeFilePath();

    auto try_load_sound = [=](std::string base_path, std::string filename, std::string resource_name, bool loop) {
        const char *extensions[] = {".wav", ".mp3", ".ogg"};
        for(int i = 0; i < 3; i++) {
            std::string fn = filename;
            fn.append(extensions[i]);
            fn = fix_filename_casing(base_path, fn);

            std::string path = base_path;
            path.append(fn);

            if(env->fileExists(path)) {
                if(cv_skin_async.getBool()) {
                    resourceManager->requestNextLoadAsync();
                }
                return resourceManager->loadSoundAbs(path, resource_name, !isSample, isOverlayable, loop);
            }
        }

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
    if(cv_skin_use_skin_hitsounds.getBool() || !isSample) {
        skin_sound = try_load_sound(this->sFilePath, skinElementName, "", loop);
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
        resourceManager->reloadResource(sound, cv_skin_async.getBool());
    } else {
        this->resources.push_back(sound);
    }

    if(isSample) {
        SOUND_SAMPLE sample;
        sample.sound = sound;
        sample.hardcodedVolumeMultiplier = (hardcodedVolumeMultiplier >= 0.0f ? hardcodedVolumeMultiplier : 1.0f);
        this->soundSamples.push_back(sample);
    }

    this->sounds.push_back(sound);

    // export
    this->filepathsForExport.push_back(sound->getFilePath());
}

bool Skin::compareFilenameWithSkinElementName(std::string filename, std::string skinElementName) {
    if(filename.length() == 0 || skinElementName.length() == 0) return false;
    return filename.substr(0, filename.find_last_of('.')) == skinElementName;
}
