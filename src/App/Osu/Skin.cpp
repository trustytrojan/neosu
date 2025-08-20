// Copyright (c) 2015, PG, All rights reserved.
#include "Skin.h"

#include <cstring>
#include <utility>

#include "Archival.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "File.h"
#include "Database.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "Parsing.h"
#include "ResourceManager.h"
#include "SString.h"
#include "SkinImage.h"
#include "SoundEngine.h"
#include "VolumeOverlay.h"

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

void Skin::unpack(const char *filepath) {
    auto skin_name = env->getFileNameFromFilePath(filepath);
    debugLog("Extracting {:s}...\n", skin_name.c_str());
    skin_name.erase(skin_name.size() - 4);  // remove .osk extension

    auto skin_root = std::string(MCENGINE_DATA_DIR "skins/");
    skin_root.append(skin_name);
    skin_root.append("/");

    std::vector<u8> fileBuffer;
    size_t fileSize{0};
    {
        File file(filepath);
        if(!file.canRead() || !(fileSize = file.getFileSize())) {
            debugLog("Failed to read skin file {:s}\n", filepath);
            return;
        }
        fileBuffer = file.takeFileBuffer();
        // close the file here
    }

    Archive archive(fileBuffer.data(), fileSize);
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
        const auto folders = SString::split(filename, "/");
        std::string file_path = skin_root;

        for(const auto &folder : folders) {
            if(!env->directoryExists(file_path)) {
                env->createDirectory(file_path);
            }

            if(folder == "..") {
                // security check: skip files with path traversal attempts
                goto skip_file;
            } else {
                file_path.append("/");
                file_path.append(folder.c_str());
            }
        }

        if(!entry.extractToFile(file_path)) {
            debugLog("Failed to extract skin file {:s}\n", filename.c_str());
        }

    skip_file:;
        // when a file can't be extracted we just ignore it (as long as the archive is valid)
    }
}

Skin::Skin(const UString &name, std::string filepath, bool isDefaultSkin) {
    this->sName = name.utf8View();
    this->sFilePath = std::move(filepath);
    this->bIsDefaultSkin = isDefaultSkin;

    // vars
    this->spinnerApproachCircleColor = 0xffffffff;
    this->sliderBorderColor = 0xffffffff;
    this->sliderBallColor = 0xffffffff;  // NOTE: 0xff02aaff is a hardcoded special case for osu!'s default skin, but it
                                         // does not apply to user skins

    this->songSelectActiveText = 0xff000000;
    this->songSelectInactiveText = 0xffffffff;
    this->inputOverlayText = 0xff000000;

    // custom
    this->iSampleVolume = (int)(cv::volume_effects.getFloat() * 100.0f);

    this->bIsRandom = cv::skin_random.getBool();
    this->bIsRandomElements = cv::skin_random_elements.getBool();

    // load all files
    this->load();

    // convar callbacks
    cv::ignore_beatmap_sample_volume.setCallback(SA::MakeDelegate<&Skin::onIgnoreBeatmapSampleVolumeChange>(this));
}

Skin::~Skin() {
    for(auto &resource : this->resources) {
        if(resource && resource != (Resource *)MISSING_TEXTURE) resourceManager->destroyResource(resource);
    }
    this->resources.clear();

    for(auto &image : this->images) {
        delete image;
    }
    this->images.clear();

    this->filepathsForExport.clear();
    // sounds are managed by resourcemanager, not unloaded here
}

void Skin::update() {
    // tasks which have to be run after async loading finishes
    if(!this->bReady && this->isReady()) {
        this->bReady = true;

        // force effect volume update
        osu->volumeOverlay->updateEffectVolume(this);
    }

    // shitty check to not animate while paused with hitobjects in background
    if(osu->isInPlayMode() && !osu->getSelectedBeatmap()->isPlaying() && !cv::skin_animation_force.getBool()) return;

    const bool useEngineTimeForAnimations = !osu->isInPlayMode();
    const long curMusicPos = osu->getSelectedBeatmap()->getCurMusicPosWithOffsets();
    for(auto &image : this->images) {
        image->update(this->animationSpeedMultiplier, useEngineTimeForAnimations, curMusicPos);
    }
}

bool Skin::isReady() {
    if(this->bReady) return true;

    // default skin sounds aren't added to the resources vector... so check explicitly for that
    for(auto &sound : this->sounds) {
        if(resourceManager->isLoadingResource(sound)) return false;
    }

    for(auto &resource : this->resources) {
        if(resourceManager->isLoadingResource(resource)) return false;
    }

    for(auto &image : this->images) {
        if(!image->isReady()) return false;
    }

    // (ready is set in update())
    return true;
}

void Skin::load() {
    resourceManager->setSyncLoadMaxBatchSize(512);
    // random skins
    {
        this->filepathsForRandomSkin.clear();
        if(this->bIsRandom || this->bIsRandomElements) {
            std::vector<std::string> skinNames;

            // regular skins
            {
                std::string skinFolder = cv::osu_folder.getString();
                skinFolder.append(cv::osu_folder_sub_skins.getString());
                std::vector<std::string> skinFolders = env->getFoldersInFolder(skinFolder);

                for(const auto &i : skinFolders) {
                    if(i.compare(".") == 0 ||
                       i.compare("..") == 0)  // is this universal in every file system? too lazy to check.
                                              // should probably fix this in the engine and not here
                        continue;

                    std::string randomSkinFolder = skinFolder;
                    randomSkinFolder.append(i);
                    randomSkinFolder.append("/");

                    this->filepathsForRandomSkin.push_back(randomSkinFolder);
                    skinNames.push_back(i);
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
        this->checkLoadImage(&this->cursorSmoke, "cursor-smoke", "OSU_SKIN_CURSORSMOKE");

        // special case: if fallback to default cursor, do load cursorMiddle
        if(this->cursor == resourceManager->getImage("OSU_SKIN_CURSOR_DEFAULT"))
            this->checkLoadImage(&this->cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE");
    }

    // skin ini
    this->randomizeFilePath();
    this->sSkinIniFilePath = this->sFilePath + "skin.ini";

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
    this->hitCircleOverlay2 = this->createSkinImage("hitcircleoverlay", vec2(128, 128), 64);
    this->hitCircleOverlay2->setAnimationFramerate(2);

    this->randomizeFilePath();
    this->checkLoadImage(&this->approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
    this->randomizeFilePath();
    this->checkLoadImage(&this->reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

    this->randomizeFilePath();
    this->followPoint2 = this->createSkinImage("followpoint", vec2(16, 22), 64);

    this->randomizeFilePath();
    {
        std::string hitCirclePrefix = this->sHitCirclePrefix.length() > 0 ? this->sHitCirclePrefix : "default";
        std::string hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-0");
        this->checkLoadImage(&this->default0, hitCircleStringFinal, "OSU_SKIN_DEFAULT0");
        if(this->default0 == MISSING_TEXTURE)
            this->checkLoadImage(&this->default0, "default-0",
                                 "OSU_SKIN_DEFAULT0");  // special cases: fallback to default skin hitcircle numbers if
                                                        // the defined prefix doesn't point to any valid files
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-1");
        this->checkLoadImage(&this->default1, hitCircleStringFinal, "OSU_SKIN_DEFAULT1");
        if(this->default1 == MISSING_TEXTURE) this->checkLoadImage(&this->default1, "default-1", "OSU_SKIN_DEFAULT1");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-2");
        this->checkLoadImage(&this->default2, hitCircleStringFinal, "OSU_SKIN_DEFAULT2");
        if(this->default2 == MISSING_TEXTURE) this->checkLoadImage(&this->default2, "default-2", "OSU_SKIN_DEFAULT2");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-3");
        this->checkLoadImage(&this->default3, hitCircleStringFinal, "OSU_SKIN_DEFAULT3");
        if(this->default3 == MISSING_TEXTURE) this->checkLoadImage(&this->default3, "default-3", "OSU_SKIN_DEFAULT3");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-4");
        this->checkLoadImage(&this->default4, hitCircleStringFinal, "OSU_SKIN_DEFAULT4");
        if(this->default4 == MISSING_TEXTURE) this->checkLoadImage(&this->default4, "default-4", "OSU_SKIN_DEFAULT4");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-5");
        this->checkLoadImage(&this->default5, hitCircleStringFinal, "OSU_SKIN_DEFAULT5");
        if(this->default5 == MISSING_TEXTURE) this->checkLoadImage(&this->default5, "default-5", "OSU_SKIN_DEFAULT5");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-6");
        this->checkLoadImage(&this->default6, hitCircleStringFinal, "OSU_SKIN_DEFAULT6");
        if(this->default6 == MISSING_TEXTURE) this->checkLoadImage(&this->default6, "default-6", "OSU_SKIN_DEFAULT6");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-7");
        this->checkLoadImage(&this->default7, hitCircleStringFinal, "OSU_SKIN_DEFAULT7");
        if(this->default7 == MISSING_TEXTURE) this->checkLoadImage(&this->default7, "default-7", "OSU_SKIN_DEFAULT7");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-8");
        this->checkLoadImage(&this->default8, hitCircleStringFinal, "OSU_SKIN_DEFAULT8");
        if(this->default8 == MISSING_TEXTURE) this->checkLoadImage(&this->default8, "default-8", "OSU_SKIN_DEFAULT8");
        hitCircleStringFinal = hitCirclePrefix;
        hitCircleStringFinal.append("-9");
        this->checkLoadImage(&this->default9, hitCircleStringFinal, "OSU_SKIN_DEFAULT9");
        if(this->default9 == MISSING_TEXTURE) this->checkLoadImage(&this->default9, "default-9", "OSU_SKIN_DEFAULT9");
    }

    this->randomizeFilePath();
    {
        std::string scorePrefix = this->sScorePrefix.length() > 0 ? this->sScorePrefix : "score";
        std::string scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-0");
        this->checkLoadImage(&this->score0, scoreStringFinal, "OSU_SKIN_SCORE0");
        if(this->score0 == MISSING_TEXTURE)
            this->checkLoadImage(&this->score0, "score-0",
                                 "OSU_SKIN_SCORE0");  // special cases: fallback to default skin score numbers if the
                                                      // defined prefix doesn't point to any valid files
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-1");
        this->checkLoadImage(&this->score1, scoreStringFinal, "OSU_SKIN_SCORE1");
        if(this->score1 == MISSING_TEXTURE) this->checkLoadImage(&this->score1, "score-1", "OSU_SKIN_SCORE1");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-2");
        this->checkLoadImage(&this->score2, scoreStringFinal, "OSU_SKIN_SCORE2");
        if(this->score2 == MISSING_TEXTURE) this->checkLoadImage(&this->score2, "score-2", "OSU_SKIN_SCORE2");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-3");
        this->checkLoadImage(&this->score3, scoreStringFinal, "OSU_SKIN_SCORE3");
        if(this->score3 == MISSING_TEXTURE) this->checkLoadImage(&this->score3, "score-3", "OSU_SKIN_SCORE3");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-4");
        this->checkLoadImage(&this->score4, scoreStringFinal, "OSU_SKIN_SCORE4");
        if(this->score4 == MISSING_TEXTURE) this->checkLoadImage(&this->score4, "score-4", "OSU_SKIN_SCORE4");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-5");
        this->checkLoadImage(&this->score5, scoreStringFinal, "OSU_SKIN_SCORE5");
        if(this->score5 == MISSING_TEXTURE) this->checkLoadImage(&this->score5, "score-5", "OSU_SKIN_SCORE5");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-6");
        this->checkLoadImage(&this->score6, scoreStringFinal, "OSU_SKIN_SCORE6");
        if(this->score6 == MISSING_TEXTURE) this->checkLoadImage(&this->score6, "score-6", "OSU_SKIN_SCORE6");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-7");
        this->checkLoadImage(&this->score7, scoreStringFinal, "OSU_SKIN_SCORE7");
        if(this->score7 == MISSING_TEXTURE) this->checkLoadImage(&this->score7, "score-7", "OSU_SKIN_SCORE7");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-8");
        this->checkLoadImage(&this->score8, scoreStringFinal, "OSU_SKIN_SCORE8");
        if(this->score8 == MISSING_TEXTURE) this->checkLoadImage(&this->score8, "score-8", "OSU_SKIN_SCORE8");
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-9");
        this->checkLoadImage(&this->score9, scoreStringFinal, "OSU_SKIN_SCORE9");
        if(this->score9 == MISSING_TEXTURE) this->checkLoadImage(&this->score9, "score-9", "OSU_SKIN_SCORE9");

        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-x");
        this->checkLoadImage(&this->scoreX, scoreStringFinal, "OSU_SKIN_SCOREX");
        // if (this->scoreX == MISSING_TEXTURE) checkLoadImage(&m_scoreX, "score-x", "OSU_SKIN_SCOREX"); // special
        // case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are
        // simply not drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-percent");
        this->checkLoadImage(&this->scorePercent, scoreStringFinal, "OSU_SKIN_SCOREPERCENT");
        // if (this->scorePercent == MISSING_TEXTURE) checkLoadImage(&m_scorePercent, "score-percent",
        // "OSU_SKIN_SCOREPERCENT"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing
        // extraneous things like the X are simply not drawn
        scoreStringFinal = scorePrefix;
        scoreStringFinal.append("-dot");
        this->checkLoadImage(&this->scoreDot, scoreStringFinal, "OSU_SKIN_SCOREDOT");
        // if (this->scoreDot == MISSING_TEXTURE) checkLoadImage(&m_scoreDot, "score-dot", "OSU_SKIN_SCOREDOT"); //
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
        if(this->combo0 == MISSING_TEXTURE)
            this->checkLoadImage(&this->combo0, "score-0",
                                 "OSU_SKIN_COMBO0");  // special cases: fallback to default skin combo numbers if the
                                                      // defined prefix doesn't point to any valid files
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-1");
        this->checkLoadImage(&this->combo1, comboStringFinal, "OSU_SKIN_COMBO1");
        if(this->combo1 == MISSING_TEXTURE) this->checkLoadImage(&this->combo1, "score-1", "OSU_SKIN_COMBO1");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-2");
        this->checkLoadImage(&this->combo2, comboStringFinal, "OSU_SKIN_COMBO2");
        if(this->combo2 == MISSING_TEXTURE) this->checkLoadImage(&this->combo2, "score-2", "OSU_SKIN_COMBO2");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-3");
        this->checkLoadImage(&this->combo3, comboStringFinal, "OSU_SKIN_COMBO3");
        if(this->combo3 == MISSING_TEXTURE) this->checkLoadImage(&this->combo3, "score-3", "OSU_SKIN_COMBO3");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-4");
        this->checkLoadImage(&this->combo4, comboStringFinal, "OSU_SKIN_COMBO4");
        if(this->combo4 == MISSING_TEXTURE) this->checkLoadImage(&this->combo4, "score-4", "OSU_SKIN_COMBO4");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-5");
        this->checkLoadImage(&this->combo5, comboStringFinal, "OSU_SKIN_COMBO5");
        if(this->combo5 == MISSING_TEXTURE) this->checkLoadImage(&this->combo5, "score-5", "OSU_SKIN_COMBO5");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-6");
        this->checkLoadImage(&this->combo6, comboStringFinal, "OSU_SKIN_COMBO6");
        if(this->combo6 == MISSING_TEXTURE) this->checkLoadImage(&this->combo6, "score-6", "OSU_SKIN_COMBO6");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-7");
        this->checkLoadImage(&this->combo7, comboStringFinal, "OSU_SKIN_COMBO7");
        if(this->combo7 == MISSING_TEXTURE) this->checkLoadImage(&this->combo7, "score-7", "OSU_SKIN_COMBO7");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-8");
        this->checkLoadImage(&this->combo8, comboStringFinal, "OSU_SKIN_COMBO8");
        if(this->combo8 == MISSING_TEXTURE) this->checkLoadImage(&this->combo8, "score-8", "OSU_SKIN_COMBO8");
        comboStringFinal = comboPrefix;
        comboStringFinal.append("-9");
        this->checkLoadImage(&this->combo9, comboStringFinal, "OSU_SKIN_COMBO9");
        if(this->combo9 == MISSING_TEXTURE) this->checkLoadImage(&this->combo9, "score-9", "OSU_SKIN_COMBO9");

        comboStringFinal = comboPrefix;
        comboStringFinal.append("-x");
        this->checkLoadImage(&this->comboX, comboStringFinal, "OSU_SKIN_COMBOX");
        // if (this->comboX == MISSING_TEXTURE) m_comboX = m_scoreX; // special case: ComboPrefix'd skins don't get
        // default fallbacks, instead missing extraneous things like the X are simply not drawn
    }

    this->randomizeFilePath();
    this->playSkip = this->createSkinImage("play-skip", vec2(193, 147), 94);
    this->randomizeFilePath();
    this->checkLoadImage(&this->playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
    this->playWarningArrow2 = this->createSkinImage("play-warningarrow", vec2(167, 129), 128);
    this->randomizeFilePath();
    this->checkLoadImage(&this->circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");
    this->randomizeFilePath();
    this->scorebarBg = this->createSkinImage("scorebar-bg", vec2(695, 44), 27.5f);
    this->scorebarColour = this->createSkinImage("scorebar-colour", vec2(645, 10), 6.25f);
    this->scorebarMarker = this->createSkinImage("scorebar-marker", vec2(24, 24), 15.0f);
    this->scorebarKi = this->createSkinImage("scorebar-ki", vec2(116, 116), 72.0f);
    this->scorebarKiDanger = this->createSkinImage("scorebar-kidanger", vec2(116, 116), 72.0f);
    this->scorebarKiDanger2 = this->createSkinImage("scorebar-kidanger2", vec2(116, 116), 72.0f);
    this->randomizeFilePath();
    this->sectionPassImage = this->createSkinImage("section-pass", vec2(650, 650), 400.0f);
    this->randomizeFilePath();
    this->sectionFailImage = this->createSkinImage("section-fail", vec2(650, 650), 400.0f);
    this->randomizeFilePath();
    this->inputoverlayBackground = this->createSkinImage("inputoverlay-background", vec2(193, 55), 34.25f);
    this->inputoverlayKey = this->createSkinImage("inputoverlay-key", vec2(43, 46), 26.75f);

    this->randomizeFilePath();
    this->hit0 = this->createSkinImage("hit0", vec2(128, 128), 42);
    this->hit0->setAnimationFramerate(60);
    this->hit50 = this->createSkinImage("hit50", vec2(128, 128), 42);
    this->hit50->setAnimationFramerate(60);
    this->hit50g = this->createSkinImage("hit50g", vec2(128, 128), 42);
    this->hit50g->setAnimationFramerate(60);
    this->hit50k = this->createSkinImage("hit50k", vec2(128, 128), 42);
    this->hit50k->setAnimationFramerate(60);
    this->hit100 = this->createSkinImage("hit100", vec2(128, 128), 42);
    this->hit100->setAnimationFramerate(60);
    this->hit100g = this->createSkinImage("hit100g", vec2(128, 128), 42);
    this->hit100g->setAnimationFramerate(60);
    this->hit100k = this->createSkinImage("hit100k", vec2(128, 128), 42);
    this->hit100k->setAnimationFramerate(60);
    this->hit300 = this->createSkinImage("hit300", vec2(128, 128), 42);
    this->hit300->setAnimationFramerate(60);
    this->hit300g = this->createSkinImage("hit300g", vec2(128, 128), 42);
    this->hit300g->setAnimationFramerate(60);
    this->hit300k = this->createSkinImage("hit300k", vec2(128, 128), 42);
    this->hit300k->setAnimationFramerate(60);

    this->randomizeFilePath();
    this->checkLoadImage(&this->particle50, "particle50", "OSU_SKIN_PARTICLE50", true);
    this->checkLoadImage(&this->particle100, "particle100", "OSU_SKIN_PARTICLE100", true);
    this->checkLoadImage(&this->particle300, "particle300", "OSU_SKIN_PARTICLE300", true);

    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
    this->randomizeFilePath();
    this->sliderb = this->createSkinImage("sliderb", vec2(128, 128), 64, false, "");
    this->sliderb->setAnimationFramerate(/*45.0f*/ 50.0f);
    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
    this->randomizeFilePath();
    this->sliderFollowCircle2 = this->createSkinImage("sliderfollowcircle", vec2(259, 259), 64);
    this->randomizeFilePath();
    this->checkLoadImage(
        &this->sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE",
        !this->bIsDefaultSkin);  // !m_bIsDefaultSkin ensures that default doesn't override user, in these special cases
    this->sliderStartCircle2 = this->createSkinImage("sliderstartcircle", vec2(128, 128), 64, !this->bIsDefaultSkin);
    this->checkLoadImage(&this->sliderStartCircleOverlay, "sliderstartcircleoverlay",
                         "OSU_SKIN_SLIDERSTARTCIRCLEOVERLAY", !this->bIsDefaultSkin);
    this->sliderStartCircleOverlay2 =
        this->createSkinImage("sliderstartcircleoverlay", vec2(128, 128), 64, !this->bIsDefaultSkin);
    this->sliderStartCircleOverlay2->setAnimationFramerate(2);
    this->randomizeFilePath();
    this->checkLoadImage(&this->sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE", !this->bIsDefaultSkin);
    this->sliderEndCircle2 = this->createSkinImage("sliderendcircle", vec2(128, 128), 64, !this->bIsDefaultSkin);
    this->checkLoadImage(&this->sliderEndCircleOverlay, "sliderendcircleoverlay", "OSU_SKIN_SLIDERENDCIRCLEOVERLAY",
                         !this->bIsDefaultSkin);
    this->sliderEndCircleOverlay2 =
        this->createSkinImage("sliderendcircleoverlay", vec2(128, 128), 64, !this->bIsDefaultSkin);
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
    this->selectionModEasy = this->createSkinImage("selection-mod-easy", vec2(68, 66), 38);
    this->selectionModNoFail = this->createSkinImage("selection-mod-nofail", vec2(68, 66), 38);
    this->selectionModHalfTime = this->createSkinImage("selection-mod-halftime", vec2(68, 66), 38);
    this->selectionModHardRock = this->createSkinImage("selection-mod-hardrock", vec2(68, 66), 38);
    this->selectionModSuddenDeath = this->createSkinImage("selection-mod-suddendeath", vec2(68, 66), 38);
    this->selectionModPerfect = this->createSkinImage("selection-mod-perfect", vec2(68, 66), 38);
    this->selectionModDoubleTime = this->createSkinImage("selection-mod-doubletime", vec2(68, 66), 38);
    this->selectionModNightCore = this->createSkinImage("selection-mod-nightcore", vec2(68, 66), 38);
    this->selectionModDayCore = this->createSkinImage("selection-mod-daycore", vec2(68, 66), 38);
    this->selectionModHidden = this->createSkinImage("selection-mod-hidden", vec2(68, 66), 38);
    this->selectionModFlashlight = this->createSkinImage("selection-mod-flashlight", vec2(68, 66), 38);
    this->selectionModRelax = this->createSkinImage("selection-mod-relax", vec2(68, 66), 38);
    this->selectionModAutopilot = this->createSkinImage("selection-mod-relax2", vec2(68, 66), 38);
    this->selectionModSpunOut = this->createSkinImage("selection-mod-spunout", vec2(68, 66), 38);
    this->selectionModAutoplay = this->createSkinImage("selection-mod-autoplay", vec2(68, 66), 38);
    this->selectionModNightmare = this->createSkinImage("selection-mod-nightmare", vec2(68, 66), 38);
    this->selectionModTarget = this->createSkinImage("selection-mod-target", vec2(68, 66), 38);
    this->selectionModScorev2 = this->createSkinImage("selection-mod-scorev2", vec2(68, 66), 38);
    this->selectionModTD = this->createSkinImage("selection-mod-touchdevice", vec2(68, 66), 38);
    this->selectionModCinema = this->createSkinImage("selection-mod-cinema", vec2(68, 66), 38);

    this->mode_osu = this->createSkinImage("mode-osu", vec2(32, 32), 32);
    this->mode_osu_small = this->createSkinImage("mode-osu-small", vec2(32, 32), 32);

    this->randomizeFilePath();
    this->checkLoadImage(&this->pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
    this->checkLoadImage(&this->pauseReplay, "pause-replay", "OSU_SKIN_PAUSE_REPLAY");
    this->checkLoadImage(&this->pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
    this->checkLoadImage(&this->pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
    this->checkLoadImage(&this->pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY");
    if(this->pauseOverlay == MISSING_TEXTURE)
        this->checkLoadImage(&this->pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY", true, "jpg");
    this->checkLoadImage(&this->failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND");
    if(this->failBackground == MISSING_TEXTURE)
        this->checkLoadImage(&this->failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND", true, "jpg");
    this->checkLoadImage(&this->unpause, "unpause", "OSU_SKIN_UNPAUSE");

    this->randomizeFilePath();
    this->checkLoadImage(&this->buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
    this->checkLoadImage(&this->buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
    this->checkLoadImage(&this->buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
    this->randomizeFilePath();
    this->menuBackImg = this->createSkinImage("menu-back", vec2(225, 87), 54);
    this->randomizeFilePath();

    // NOTE: scaling is ignored when drawing this specific element
    this->selectionMode = this->createSkinImage("selection-mode", vec2(90, 90), 38);

    this->selectionModeOver = this->createSkinImage("selection-mode-over", vec2(88, 90), 38);
    this->selectionMods = this->createSkinImage("selection-mods", vec2(74, 90), 38);
    this->selectionModsOver = this->createSkinImage("selection-mods-over", vec2(74, 90), 38);
    this->selectionRandom = this->createSkinImage("selection-random", vec2(74, 90), 38);
    this->selectionRandomOver = this->createSkinImage("selection-random-over", vec2(74, 90), 38);
    this->selectionOptions = this->createSkinImage("selection-options", vec2(74, 90), 38);
    this->selectionOptionsOver = this->createSkinImage("selection-options-over", vec2(74, 90), 38);

    this->randomizeFilePath();
    this->checkLoadImage(&this->songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
    this->checkLoadImage(&this->songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
    this->randomizeFilePath();
    this->checkLoadImage(&this->menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");
    this->menuButtonBackground2 = this->createSkinImage("menu-button-background", vec2(699, 103), 64.0f);
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

    this->rankingAsmall = this->createSkinImage("ranking-A-small", vec2(34, 40), 128);
    this->rankingBsmall = this->createSkinImage("ranking-B-small", vec2(34, 40), 128);
    this->rankingCsmall = this->createSkinImage("ranking-C-small", vec2(34, 40), 128);
    this->rankingDsmall = this->createSkinImage("ranking-D-small", vec2(34, 40), 128);
    this->rankingSsmall = this->createSkinImage("ranking-S-small", vec2(34, 40), 128);
    this->rankingSHsmall = this->createSkinImage("ranking-SH-small", vec2(34, 40), 128);
    this->rankingXsmall = this->createSkinImage("ranking-X-small", vec2(34, 40), 128);
    this->rankingXHsmall = this->createSkinImage("ranking-XH-small", vec2(34, 40), 128);

    this->rankingPerfect = this->createSkinImage("ranking-perfect", vec2(478, 150), 128);

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
    this->loadSound(this->normalHitNormal, "normal-hitnormal", "OSU_SKIN_NORMALHITNORMAL_SND", true, true, false, true,
                    0.8f);
    this->loadSound(this->normalHitWhistle, "normal-hitwhistle", "OSU_SKIN_NORMALHITWHISTLE_SND", true, true, false,
                    true, 0.85f);
    this->loadSound(this->normalHitFinish, "normal-hitfinish", "OSU_SKIN_NORMALHITFINISH_SND", true, true);
    this->loadSound(this->normalHitClap, "normal-hitclap", "OSU_SKIN_NORMALHITCLAP_SND", true, true, false, true,
                    0.85f);

    this->loadSound(this->normalSliderTick, "normal-slidertick", "OSU_SKIN_NORMALSLIDERTICK_SND", true, true);
    this->loadSound(this->normalSliderSlide, "normal-sliderslide", "OSU_SKIN_NORMALSLIDERSLIDE_SND", false, true, true);
    this->loadSound(this->normalSliderWhistle, "normal-sliderwhistle", "OSU_SKIN_NORMALSLIDERWHISTLE_SND", true, true);

    this->loadSound(this->softHitNormal, "soft-hitnormal", "OSU_SKIN_SOFTHITNORMAL_SND", true, true, false, true, 0.8f);
    this->loadSound(this->softHitWhistle, "soft-hitwhistle", "OSU_SKIN_SOFTHITWHISTLE_SND", true, true, false, true,
                    0.85f);
    this->loadSound(this->softHitFinish, "soft-hitfinish", "OSU_SKIN_SOFTHITFINISH_SND", true, true);
    this->loadSound(this->softHitClap, "soft-hitclap", "OSU_SKIN_SOFTHITCLAP_SND", true, true, false, true, 0.85f);

    this->loadSound(this->softSliderTick, "soft-slidertick", "OSU_SKIN_SOFTSLIDERTICK_SND", true, true);
    this->loadSound(this->softSliderSlide, "soft-sliderslide", "OSU_SKIN_SOFTSLIDERSLIDE_SND", false, true, true);
    this->loadSound(this->softSliderWhistle, "soft-sliderwhistle", "OSU_SKIN_SOFTSLIDERWHISTLE_SND", true, true);

    this->loadSound(this->drumHitNormal, "drum-hitnormal", "OSU_SKIN_DRUMHITNORMAL_SND", true, true, false, true, 0.8f);
    this->loadSound(this->drumHitWhistle, "drum-hitwhistle", "OSU_SKIN_DRUMHITWHISTLE_SND", true, true, false, true,
                    0.85f);
    this->loadSound(this->drumHitFinish, "drum-hitfinish", "OSU_SKIN_DRUMHITFINISH_SND", true, true);
    this->loadSound(this->drumHitClap, "drum-hitclap", "OSU_SKIN_DRUMHITCLAP_SND", true, true, false, true, 0.85f);

    this->loadSound(this->drumSliderTick, "drum-slidertick", "OSU_SKIN_DRUMSLIDERTICK_SND", true, true);
    this->loadSound(this->drumSliderSlide, "drum-sliderslide", "OSU_SKIN_DRUMSLIDERSLIDE_SND", false, true, true);
    this->loadSound(this->drumSliderWhistle, "drum-sliderwhistle", "OSU_SKIN_DRUMSLIDERWHISTLE_SND", true, true);

    this->loadSound(this->spinnerBonus, "spinnerbonus", "OSU_SKIN_SPINNERBONUS_SND", true, true);
    this->loadSound(this->spinnerSpinSound, "spinnerspin", "OSU_SKIN_SPINNERSPIN_SND", false, true, true);

    // others
    this->loadSound(this->combobreak, "combobreak", "OSU_SKIN_COMBOBREAK_SND", true, true);
    this->loadSound(this->failsound, "failsound", "OSU_SKIN_FAILSOUND_SND");
    this->loadSound(this->applause, "applause", "OSU_SKIN_APPLAUSE_SND");
    this->loadSound(this->menuHit, "menuhit", "OSU_SKIN_MENUHIT_SND", true, true);
    this->loadSound(this->menuHover, "menuclick", "OSU_SKIN_MENUCLICK_SND", true, true);
    this->loadSound(this->checkOn, "check-on", "OSU_SKIN_CHECKON_SND", true, true);
    this->loadSound(this->checkOff, "check-off", "OSU_SKIN_CHECKOFF_SND", true, true);
    this->loadSound(this->shutter, "shutter", "OSU_SKIN_SHUTTER_SND", true, true);
    this->loadSound(this->sectionPassSound, "sectionpass", "OSU_SKIN_SECTIONPASS_SND");
    this->loadSound(this->sectionFailSound, "sectionfail", "OSU_SKIN_SECTIONFAIL_SND");

    // UI feedback
    this->loadSound(this->messageSent, "key-confirm", "OSU_SKIN_MESSAGE_SENT_SND", true, true, false);
    this->loadSound(this->deletingText, "key-delete", "OSU_SKIN_DELETING_TEXT_SND", true, true, false);
    this->loadSound(this->movingTextCursor, "key-movement", "OSU_MOVING_TEXT_CURSOR_SND", true, true, false);
    this->loadSound(this->typing1, "key-press-1", "OSU_TYPING_1_SND", true, true, false);
    this->loadSound(this->typing2, "key-press-2", "OSU_TYPING_2_SND", true, true, false, false);
    this->loadSound(this->typing3, "key-press-3", "OSU_TYPING_3_SND", true, true, false, false);
    this->loadSound(this->typing4, "key-press-4", "OSU_TYPING_4_SND", true, true, false, false);
    this->loadSound(this->menuBack, "menuback", "OSU_MENU_BACK_SND", true, true, false, false);
    this->loadSound(this->closeChatTab, "click-close", "OSU_CLOSE_CHAT_TAB_SND", true, true, false, false);
    this->loadSound(this->clickButton, "click-short-confirm", "OSU_CLICK_BUTTON_SND", true, true, false, false);
    this->loadSound(this->hoverButton, "click-short", "OSU_HOVER_BUTTON_SND", true, true, false, false);
    this->loadSound(this->backButtonClick, "back-button-click", "OSU_BACK_BUTTON_CLICK_SND", true, true, false, false);
    this->loadSound(this->backButtonHover, "back-button-hover", "OSU_BACK_BUTTON_HOVER_SND", true, true, false, false);
    this->loadSound(this->clickMainMenuCube, "menu-play-click", "OSU_CLICK_MAIN_MENU_CUBE_SND", true, true, false,
                    false);
    this->loadSound(this->hoverMainMenuCube, "menu-play-hover", "OSU_HOVER_MAIN_MENU_CUBE_SND", true, true, false,
                    false);
    this->loadSound(this->clickSingleplayer, "menu-freeplay-click", "OSU_CLICK_SINGLEPLAYER_SND", true, true, false,
                    false);
    this->loadSound(this->hoverSingleplayer, "menu-freeplay-hover", "OSU_HOVER_SINGLEPLAYER_SND", true, true, false,
                    false);
    this->loadSound(this->clickMultiplayer, "menu-multiplayer-click", "OSU_CLICK_MULTIPLAYER_SND", true, true, false,
                    false);
    this->loadSound(this->hoverMultiplayer, "menu-multiplayer-hover", "OSU_HOVER_MULTIPLAYER_SND", true, true, false,
                    false);
    this->loadSound(this->clickOptions, "menu-options-click", "OSU_CLICK_OPTIONS_SND", true, true, false, false);
    this->loadSound(this->hoverOptions, "menu-options-hover", "OSU_HOVER_OPTIONS_SND", true, true, false, false);
    this->loadSound(this->clickExit, "menu-exit-click", "OSU_CLICK_EXIT_SND", true, true, false, false);
    this->loadSound(this->hoverExit, "menu-exit-hover", "OSU_HOVER_EXIT_SND", true, true, false, false);
    this->loadSound(this->expand, "select-expand", "OSU_EXPAND_SND", true, true, false);
    this->loadSound(this->selectDifficulty, "select-difficulty", "OSU_SELECT_DIFFICULTY_SND", true, true, false, false);
    this->loadSound(this->sliderbar, "sliderbar", "OSU_DRAG_SLIDER_SND", true, true, false);
    this->loadSound(this->matchConfirm, "match-confirm", "OSU_ALL_PLAYERS_READY_SND", true, true, false);
    this->loadSound(this->roomJoined, "match-join", "OSU_ROOM_JOINED_SND", true, true, false);
    this->loadSound(this->roomQuit, "match-leave", "OSU_ROOM_QUIT_SND", true, true, false);
    this->loadSound(this->roomNotReady, "match-notready", "OSU_ROOM_NOT_READY_SND", true, true, false);
    this->loadSound(this->roomReady, "match-ready", "OSU_ROOM_READY_SND", true, true, false);
    this->loadSound(this->matchStart, "match-start", "OSU_MATCH_START_SND", true, true, false);

    this->loadSound(this->pauseLoop, "pause-loop", "OSU_PAUSE_LOOP_SND", false, false, true, true);
    this->loadSound(this->pauseHover, "pause-hover", "OSU_PAUSE_HOVER_SND", true, true, false, false);
    this->loadSound(this->clickPauseBack, "pause-back-click", "OSU_CLICK_QUIT_SONG_SND", true, true, false, false);
    this->loadSound(this->hoverPauseBack, "pause-back-hover", "OSU_HOVER_QUIT_SONG_SND", true, true, false, false);
    this->loadSound(this->clickPauseContinue, "pause-continue-click", "OSU_CLICK_RESUME_SONG_SND", true, true, false,
                    false);
    this->loadSound(this->hoverPauseContinue, "pause-continue-hover", "OSU_HOVER_RESUME_SONG_SND", true, true, false,
                    false);
    this->loadSound(this->clickPauseRetry, "pause-retry-click", "OSU_CLICK_RETRY_SONG_SND", true, true, false, false);
    this->loadSound(this->hoverPauseRetry, "pause-retry-hover", "OSU_HOVER_RETRY_SONG_SND", true, true, false, false);

    if(this->clickButton == nullptr) this->clickButton = this->menuHit;
    if(this->hoverButton == nullptr) this->hoverButton = this->menuHover;
    if(this->pauseHover == nullptr) this->pauseHover = this->hoverButton;
    if(this->selectDifficulty == nullptr) this->selectDifficulty = this->clickButton;
    if(this->typing2 == nullptr) this->typing2 = this->typing1;
    if(this->typing3 == nullptr) this->typing3 = this->typing2;
    if(this->typing4 == nullptr) this->typing4 = this->typing3;
    if(this->backButtonClick == nullptr) this->backButtonClick = this->clickButton;
    if(this->backButtonHover == nullptr) this->backButtonHover = this->hoverButton;
    if(this->menuBack == nullptr) this->menuBack = this->clickButton;
    if(this->closeChatTab == nullptr) this->closeChatTab = this->clickButton;
    if(this->clickMainMenuCube == nullptr) this->clickMainMenuCube = this->clickButton;
    if(this->hoverMainMenuCube == nullptr) this->hoverMainMenuCube = this->menuHover;
    if(this->clickSingleplayer == nullptr) this->clickSingleplayer = this->clickButton;
    if(this->hoverSingleplayer == nullptr) this->hoverSingleplayer = this->menuHover;
    if(this->clickMultiplayer == nullptr) this->clickMultiplayer = this->clickButton;
    if(this->hoverMultiplayer == nullptr) this->hoverMultiplayer = this->menuHover;
    if(this->clickOptions == nullptr) this->clickOptions = this->clickButton;
    if(this->hoverOptions == nullptr) this->hoverOptions = this->menuHover;
    if(this->clickExit == nullptr) this->clickExit = this->clickButton;
    if(this->hoverExit == nullptr) this->hoverExit = this->menuHover;
    if(this->clickPauseBack == nullptr) this->clickPauseBack = this->clickButton;
    if(this->hoverPauseBack == nullptr) this->hoverPauseBack = this->pauseHover;
    if(this->clickPauseContinue == nullptr) this->clickPauseContinue = this->clickButton;
    if(this->hoverPauseContinue == nullptr) this->hoverPauseContinue = this->pauseHover;
    if(this->clickPauseRetry == nullptr) this->clickPauseRetry = this->clickButton;
    if(this->hoverPauseRetry == nullptr) this->hoverPauseRetry = this->pauseHover;

    // scaling
    // HACKHACK: this is pure cancer
    if(this->cursor != nullptr && this->cursor->getFilePath().find("@2x") != -1) this->bCursor2x = true;
    if(this->cursorTrail != nullptr && this->cursorTrail->getFilePath().find("@2x") != -1) this->bCursorTrail2x = true;
    if(this->cursorRipple != nullptr && this->cursorRipple->getFilePath().find("@2x") != -1)
        this->bCursorRipple2x = true;
    if(this->cursorSmoke != nullptr && this->cursorSmoke->getFilePath().find("@2x") != -1) this->bCursorSmoke2x = true;
    if(this->approachCircle != nullptr && this->approachCircle->getFilePath().find("@2x") != -1)
        this->bApproachCircle2x = true;
    if(this->reverseArrow != nullptr && this->reverseArrow->getFilePath().find("@2x") != -1)
        this->bReverseArrow2x = true;
    if(this->hitCircle != nullptr && this->hitCircle->getFilePath().find("@2x") != -1) this->bHitCircle2x = true;
    if(this->default0 != nullptr && this->default0->getFilePath().find("@2x") != -1) this->bIsDefault02x = true;
    if(this->default1 != nullptr && this->default1->getFilePath().find("@2x") != -1) this->bIsDefault12x = true;
    if(this->score0 != nullptr && this->score0->getFilePath().find("@2x") != -1) this->bIsScore02x = true;
    if(this->combo0 != nullptr && this->combo0->getFilePath().find("@2x") != -1) this->bIsCombo02x = true;
    if(this->spinnerApproachCircle != nullptr && this->spinnerApproachCircle->getFilePath().find("@2x") != -1)
        this->bSpinnerApproachCircle2x = true;
    if(this->spinnerBottom != nullptr && this->spinnerBottom->getFilePath().find("@2x") != -1)
        this->bSpinnerBottom2x = true;
    if(this->spinnerCircle != nullptr && this->spinnerCircle->getFilePath().find("@2x") != -1)
        this->bSpinnerCircle2x = true;
    if(this->spinnerTop != nullptr && this->spinnerTop->getFilePath().find("@2x") != -1) this->bSpinnerTop2x = true;
    if(this->spinnerMiddle != nullptr && this->spinnerMiddle->getFilePath().find("@2x") != -1)
        this->bSpinnerMiddle2x = true;
    if(this->spinnerMiddle2 != nullptr && this->spinnerMiddle2->getFilePath().find("@2x") != -1)
        this->bSpinnerMiddle22x = true;
    if(this->sliderScorePoint != nullptr && this->sliderScorePoint->getFilePath().find("@2x") != -1)
        this->bSliderScorePoint2x = true;
    if(this->sliderStartCircle != nullptr && this->sliderStartCircle->getFilePath().find("@2x") != -1)
        this->bSliderStartCircle2x = true;
    if(this->sliderEndCircle != nullptr && this->sliderEndCircle->getFilePath().find("@2x") != -1)
        this->bSliderEndCircle2x = true;

    if(this->circularmetre != nullptr && this->circularmetre->getFilePath().find("@2x") != -1)
        this->bCircularmetre2x = true;

    if(this->pauseContinue != nullptr && this->pauseContinue->getFilePath().find("@2x") != -1)
        this->bPauseContinue2x = true;

    if(this->menuButtonBackground != nullptr && this->menuButtonBackground->getFilePath().find("@2x") != -1)
        this->bMenuButtonBackground2x = true;
    if(this->star != nullptr && this->star->getFilePath().find("@2x") != -1) this->bStar2x = true;
    if(this->rankingPanel != nullptr && this->rankingPanel->getFilePath().find("@2x") != -1)
        this->bRankingPanel2x = true;
    if(this->rankingMaxCombo != nullptr && this->rankingMaxCombo->getFilePath().find("@2x") != -1)
        this->bRankingMaxCombo2x = true;
    if(this->rankingAccuracy != nullptr && this->rankingAccuracy->getFilePath().find("@2x") != -1)
        this->bRankingAccuracy2x = true;
    if(this->rankingA != nullptr && this->rankingA->getFilePath().find("@2x") != -1) this->bRankingA2x = true;
    if(this->rankingB != nullptr && this->rankingB->getFilePath().find("@2x") != -1) this->bRankingB2x = true;
    if(this->rankingC != nullptr && this->rankingC->getFilePath().find("@2x") != -1) this->bRankingC2x = true;
    if(this->rankingD != nullptr && this->rankingD->getFilePath().find("@2x") != -1) this->bRankingD2x = true;
    if(this->rankingS != nullptr && this->rankingS->getFilePath().find("@2x") != -1) this->bRankingS2x = true;
    if(this->rankingSH != nullptr && this->rankingSH->getFilePath().find("@2x") != -1) this->bRankingSH2x = true;
    if(this->rankingX != nullptr && this->rankingX->getFilePath().find("@2x") != -1) this->bRankingX2x = true;
    if(this->rankingXH != nullptr && this->rankingXH->getFilePath().find("@2x") != -1) this->bRankingXH2x = true;

    // HACKHACK: all of the <>2 loads are temporary fixes until I fix the checkLoadImage() function logic

    // custom
    Image *defaultCursor = resourceManager->getImage("OSU_SKIN_CURSOR_DEFAULT");
    Image *defaultCursor2 = this->cursor;
    if(defaultCursor != nullptr)
        this->defaultCursor = defaultCursor;
    else if(defaultCursor2 != nullptr)
        this->defaultCursor = defaultCursor2;

    Image *defaultButtonLeft = resourceManager->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
    Image *defaultButtonLeft2 = this->buttonLeft;
    if(defaultButtonLeft != nullptr)
        this->defaultButtonLeft = defaultButtonLeft;
    else if(defaultButtonLeft2 != nullptr)
        this->defaultButtonLeft = defaultButtonLeft2;

    Image *defaultButtonMiddle = resourceManager->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
    Image *defaultButtonMiddle2 = this->buttonMiddle;
    if(defaultButtonMiddle != nullptr)
        this->defaultButtonMiddle = defaultButtonMiddle;
    else if(defaultButtonMiddle2 != nullptr)
        this->defaultButtonMiddle = defaultButtonMiddle2;

    Image *defaultButtonRight = resourceManager->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
    Image *defaultButtonRight2 = this->buttonRight;
    if(defaultButtonRight != nullptr)
        this->defaultButtonRight = defaultButtonRight;
    else if(defaultButtonRight2 != nullptr)
        this->defaultButtonRight = defaultButtonRight2;

    // print some debug info
    debugLog("Skin: Version {:f}\n", this->fVersion);
    debugLog("Skin: HitCircleOverlap = {:d}\n", this->iHitCircleOverlap);

    // delayed error notifications due to resource loading potentially blocking engine time
    if(!parseSkinIni1Status && parseSkinIni2Status && cv::skin.getString() != "default")
        osu->notificationOverlay->addNotification("Error: Couldn't load skin.ini!", 0xffff0000);
    else if(!parseSkinIni2Status)
        osu->notificationOverlay->addNotification("Error: Couldn't load DEFAULT skin.ini!!!", 0xffff0000);

    resourceManager->resetSyncLoadMaxBatchSize();
}

void Skin::loadBeatmapOverride(const std::string & /*filepath*/) {
    // debugLog("Skin::loadBeatmapOverride( {:s} )\n", filepath.c_str());
    //  TODO: beatmap skin support
}

void Skin::reloadSounds() {
    std::vector<Resource *> soundResources;
    soundResources.reserve(this->sounds.size());

    for(auto &sound : this->sounds) {
        soundResources.push_back(sound);
    }

    resourceManager->reloadResources(soundResources, cv::skin_async.getBool());
}

bool Skin::parseSkinINI(std::string filepath) {
    std::vector<u8> rawData;
    size_t fileSize{0};
    {
        File file(filepath);
        if(!file.canRead() || !(fileSize = file.getFileSize())) {
            debugLog("OsuSkin Error: Couldn't load {:s}\n", filepath);
            return false;
        }
        rawData = file.takeFileBuffer();
        // close the file here
    }

    UString fileContent;

    // check for UTF-16 LE BOM and convert if needed
    if(fileSize >= 2 && rawData[0] == 0xFF && rawData[1] == 0xFE) {
        // convert UTF-16 LE to UTF-8
        std::string utf8Result;
        utf8Result.reserve(fileSize);

        size_t utf16Size = fileSize - 2;  // skip BOM
        const u8 *utf16Data = rawData.data() + 2;

        for(size_t i = 0; i < utf16Size - 1; i += 2) {
            uint16_t utf16Char = utf16Data[i] | (utf16Data[i + 1] << 8);

            if(utf16Char < 0x80)
                utf8Result += static_cast<char>(utf16Char);
            else if(utf16Char < 0x800) {
                utf8Result += static_cast<char>(0xC0 | (utf16Char >> 6));
                utf8Result += static_cast<char>(0x80 | (utf16Char & 0x3F));
            } else {
                utf8Result += static_cast<char>(0xE0 | (utf16Char >> 12));
                utf8Result += static_cast<char>(0x80 | ((utf16Char >> 6) & 0x3F));
                utf8Result += static_cast<char>(0x80 | (utf16Char & 0x3F));
            }
        }

        fileContent = UString(utf8Result.c_str());
    } else {
        // assume UTF-8/ASCII
        fileContent = UString(reinterpret_cast<char *>(rawData.data()), static_cast<int>(fileSize));
    }

    // process content line by line, handling different line endings
    int nonEmptyLineCounter = 0;
    int curBlock = 0;  // NOTE: was -1, but osu incorrectly defaults to [General] and loads properties even before the
                       // actual section start (just for this first section though)

    UString currentLine;
    for(int i = 0; i < fileContent.length(); i++) {
        wchar_t ch = fileContent[i];

        // accumulate characters until we hit a line ending
        if(ch != L'\r' && ch != L'\n') {
            currentLine += ch;
            continue;
        }

        // handle CRLF, CR, or LF
        if(ch == L'\r' && i + 1 < fileContent.length() && fileContent[i + 1] == L'\n') i++;  // skip LF in CRLF

        std::string curLine{currentLine.toUtf8()};
        currentLine.clear();  // reset for next line

        if(!curLine.empty()) nonEmptyLineCounter++;

        // skip comments
        size_t firstNonSpace = curLine.find_first_not_of(" \t");
        if(firstNonSpace != std::string::npos && curLine.substr(firstNonSpace).starts_with("//")) continue;

        // section detection
        if(curLine.find("[General]") != std::string::npos)
            curBlock = 0;
        else if(curLine.find("[Colours]") != std::string::npos || curLine.find("[Colors]") != std::string::npos)
            curBlock = 1;
        else if(curLine.find("[Fonts]") != std::string::npos)
            curBlock = 2;

        switch(curBlock) {
            // General
            case 0: {
                std::string version;
                if(Parsing::parse_value(curLine, "Version", &version)) {
                    if((version.find("latest") != std::string::npos) || (version.find("User") != std::string::npos)) {
                        this->fVersion = 2.5f;
                    } else {
                        Parsing::parse_value(curLine, "Version", &this->fVersion);
                    }
                }

                Parsing::parse_value(curLine, "CursorRotate", &this->bCursorRotate);
                Parsing::parse_value(curLine, "CursorCentre", &this->bCursorCenter);
                Parsing::parse_value(curLine, "CursorExpand", &this->bCursorExpand);
                Parsing::parse_value(curLine, "SliderBallFlip", &this->bSliderBallFlip);
                Parsing::parse_value(curLine, "AllowSliderBallTint", &this->bAllowSliderBallTint);
                Parsing::parse_value(curLine, "HitCircleOverlayAboveNumber", &this->bHitCircleOverlayAboveNumber);

                // https://osu.ppy.sh/community/forums/topics/314209
                Parsing::parse_value(curLine, "HitCircleOverlayAboveNumer", &this->bHitCircleOverlayAboveNumber);

                if(Parsing::parse_value(curLine, "SliderStyle", &this->iSliderStyle)) {
                    if(this->iSliderStyle != 1 && this->iSliderStyle != 2) this->iSliderStyle = 2;
                }

                if(Parsing::parse_value(curLine, "AnimationFramerate", &this->fAnimationFramerate)) {
                    if(this->fAnimationFramerate < 0.f) this->fAnimationFramerate = 0.f;
                }

                break;
            }

            // Colors
            case 1: {
                u8 comboNum;
                u8 r, g, b;

                // FIXME: actually use comboNum for ordering
                if(Parsing::parse_numbered_value(curLine, "Combo", &comboNum, &r, &g, &b)) {
                    if(comboNum >= 1 && comboNum <= 8) {
                        this->comboColors.push_back(rgb(r, g, b));
                    }
                } else if(Parsing::parse_value(curLine, "SpinnerApproachCircle", &r, &g, &b))
                    this->spinnerApproachCircleColor = rgb(r, g, b);
                else if(Parsing::parse_value(curLine, "SliderBall", &r, &g, &b))
                    this->sliderBallColor = rgb(r, g, b);
                else if(Parsing::parse_value(curLine, "SliderBorder", &r, &g, &b))
                    this->sliderBorderColor = rgb(r, g, b);
                else if(Parsing::parse_value(curLine, "SliderTrackOverride", &r, &g, &b)) {
                    this->sliderTrackOverride = rgb(r, g, b);
                    this->bSliderTrackOverride = true;
                } else if(Parsing::parse_value(curLine, "SongSelectActiveText", &r, &g, &b))
                    this->songSelectActiveText = rgb(r, g, b);
                else if(Parsing::parse_value(curLine, "SongSelectInactiveText", &r, &g, &b))
                    this->songSelectInactiveText = rgb(r, g, b);
                else if(Parsing::parse_value(curLine, "InputOverlayText", &r, &g, &b))
                    this->inputOverlayText = rgb(r, g, b);

                break;
            }

            // Fonts
            case 2: {
                Parsing::parse_value(curLine, "ComboOverlap", &this->iComboOverlap);
                Parsing::parse_value(curLine, "ScoreOverlap", &this->iScoreOverlap);
                Parsing::parse_value(curLine, "HitCircleOverlap", &this->iHitCircleOverlap);

                if(Parsing::parse_value(curLine, "ComboPrefix", &this->sComboPrefix)) {
                    // XXX: jank path normalization
                    for(int i = 0; i < this->sComboPrefix.length(); i++) {
                        if(this->sComboPrefix[i] == '\\') {
                            this->sComboPrefix.erase(i, 1);
                            this->sComboPrefix.insert(i, "/");
                        }
                    }
                }

                if(Parsing::parse_value(curLine, "ScorePrefix", &this->sScorePrefix)) {
                    // XXX: jank path normalization
                    for(int i = 0; i < this->sScorePrefix.length(); i++) {
                        if(this->sScorePrefix[i] == '\\') {
                            this->sScorePrefix.erase(i, 1);
                            this->sScorePrefix.insert(i, "/");
                        }
                    }
                }

                if(Parsing::parse_value(curLine, "HitCirclePrefix", &this->sHitCirclePrefix)) {
                    // XXX: jank path normalization
                    for(int i = 0; i < this->sHitCirclePrefix.length(); i++) {
                        if(this->sHitCirclePrefix[i] == '\\') {
                            this->sHitCirclePrefix.erase(i, 1);
                            this->sHitCirclePrefix.insert(i, "/");
                        }
                    }
                }

                break;
            }
        }
    }

    // process the last line if it doesn't end with a line terminator
    if(!currentLine.isEmpty()) nonEmptyLineCounter++;

    if(nonEmptyLineCounter < 1) return false;

    return true;
}

void Skin::onIgnoreBeatmapSampleVolumeChange() { this->resetSampleVolume(); }

void Skin::setSampleSet(int sampleSet) {
    if(this->iSampleSet == sampleSet) return;

    /// debugLog("sample set = {:d}\n", sampleSet);
    this->iSampleSet = sampleSet;
}

void Skin::resetSampleVolume() {
    this->setSampleVolume(std::clamp<float>((float)this->iSampleVolume / 100.0f, 0.0f, 1.0f), true);
}

void Skin::setSampleVolume(float volume, bool force) {
    if(cv::ignore_beatmap_sample_volume.getBool() &&
       (int)(cv::volume_effects.getFloat() * 100.0f) == this->iSampleVolume)
        return;

    const float newSampleVolume =
        (!cv::ignore_beatmap_sample_volume.getBool() ? volume : 1.0f) * cv::volume_effects.getFloat();

    if(!force && this->iSampleVolume == (int)(newSampleVolume * 100.0f)) return;

    this->iSampleVolume = (int)(newSampleVolume * 100.0f);
    /// debugLog("sample volume = {:f}\n", sampleVolume);
    for(auto &soundSample : this->soundSamples) {
        soundSample.sound->setVolume(newSampleVolume * soundSample.hardcodedVolumeMultiplier);
    }
}

Color Skin::getComboColorForCounter(int i, int offset) {
    i += cv::skin_color_index_add.getInt();
    i = std::max(i, 0);

    if(this->beatmapComboColors.size() > 0 && !cv::ignore_beatmap_combo_colors.getBool())
        return this->beatmapComboColors[(i + offset) % this->beatmapComboColors.size()];
    else if(this->comboColors.size() > 0)
        return this->comboColors[i % this->comboColors.size()];
    else
        return argb(255, 0, 255, 0);
}

void Skin::setBeatmapComboColors(std::vector<Color> colors) { this->beatmapComboColors = std::move(colors); }

void Skin::playHitCircleSound(int sampleType, float pan, long delta) {
    if(this->iSampleVolume <= 0) {
        return;
    }

    if(!cv::sound_panning.getBool() || (cv::mod_fposu.getBool() && !cv::mod_fposu_sound_panning.getBool()) ||
       (cv::mod_fps.getBool() && !cv::mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv::sound_panning_multiplier.getFloat();
    }

    f32 pitch = 0.f;
    if(cv::snd_pitch_hitsounds.getBool()) {
        auto bm = osu->getSelectedBeatmap();
        if(bm) {
            f32 range = bm->getHitWindow100();
            pitch = (f32)delta / range * cv::snd_pitch_hitsounds_factor.getFloat();
        }
    }

    int actualSampleSet = this->iSampleSet;
    if(cv::skin_force_hitsound_sample_set.getInt() > 0) actualSampleSet = cv::skin_force_hitsound_sample_set.getInt();

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

    if(!cv::sound_panning.getBool() || (cv::mod_fposu.getBool() && !cv::mod_fposu_sound_panning.getBool()) ||
       (cv::mod_fps.getBool() && !cv::mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv::sound_panning_multiplier.getFloat();
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
    if(!cv::sound_panning.getBool() || (cv::mod_fposu.getBool() && !cv::mod_fposu_sound_panning.getBool()) ||
       (cv::mod_fps.getBool() && !cv::mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv::sound_panning_multiplier.getFloat();
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
    if(this->spinnerSpinSound == nullptr) return;

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
    if(this->spinnerSpinSound == nullptr) return;

    if(this->spinnerSpinSound->isPlaying()) soundEngine->stop(this->spinnerSpinSound);
}

void Skin::randomizeFilePath() {
    if(this->bIsRandomElements && this->filepathsForRandomSkin.size() > 0)
        this->sFilePath = this->filepathsForRandomSkin[rand() % this->filepathsForRandomSkin.size()];
}

SkinImage *Skin::createSkinImage(const std::string &skinElementName, vec2 baseSizeForScaling2x, float osuSize,
                                 bool ignoreDefaultSkin, const std::string &animationSeparator) {
    auto *skinImage =
        new SkinImage(this, skinElementName, baseSizeForScaling2x, osuSize, animationSeparator, ignoreDefaultSkin);
    this->images.push_back(skinImage);

    const std::vector<std::string> &filepathsForExport = skinImage->getFilepathsForExport();
    this->filepathsForExport.insert(this->filepathsForExport.end(), filepathsForExport.begin(),
                                    filepathsForExport.end());

    return skinImage;
}

void Skin::checkLoadImage(Image **addressOfPointer, const std::string &skinElementName, const std::string &resourceName,
                          bool ignoreDefaultSkin, const std::string &fileExtension, bool forceLoadMipmaps) {
    if(*addressOfPointer != MISSING_TEXTURE) return;  // we are already loaded

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
    if(cv::skin_hd.getBool()) {
        // load default skin
        if(!ignoreDefaultSkin) {
            if(existsDefaultFilePath1) {
                std::string defaultResourceName = resourceName;
                defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                if(cv::skin_async.getBool()) resourceManager->requestNextLoadAsync();

                *addressOfPointer = resourceManager->loadImageAbs(defaultFilePath1, defaultResourceName,
                                                                  cv::skin_mipmaps.getBool() || forceLoadMipmaps);
            } else {
                // fallback to @1x
                if(existsDefaultFilePath2) {
                    std::string defaultResourceName = resourceName;
                    defaultResourceName.append("_DEFAULT");  // so we don't load the default skin twice

                    if(cv::skin_async.getBool()) resourceManager->requestNextLoadAsync();

                    *addressOfPointer = resourceManager->loadImageAbs(defaultFilePath2, defaultResourceName,
                                                                      cv::skin_mipmaps.getBool() || forceLoadMipmaps);
                }
            }
        }

        // load user skin
        if(existsFilepath1) {
            if(cv::skin_async.getBool()) resourceManager->requestNextLoadAsync();

            *addressOfPointer =
                resourceManager->loadImageAbs(filepath1, "", cv::skin_mipmaps.getBool() || forceLoadMipmaps);
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

            if(cv::skin_async.getBool()) resourceManager->requestNextLoadAsync();

            *addressOfPointer = resourceManager->loadImageAbs(defaultFilePath2, defaultResourceName,
                                                              cv::skin_mipmaps.getBool() || forceLoadMipmaps);
        }
    }

    // load user skin
    if(existsFilepath2) {
        if(cv::skin_async.getBool()) resourceManager->requestNextLoadAsync();

        *addressOfPointer =
            resourceManager->loadImageAbs(filepath2, "", cv::skin_mipmaps.getBool() || forceLoadMipmaps);
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

void Skin::loadSound(Sound *&sndRef, const std::string &skinElementName, std::string resourceName, bool isOverlayable,
                     bool isSample, bool loop, bool fallback_to_default, float hardcodedVolumeMultiplier) {
    if(sndRef != nullptr) return;  // we are already loaded

    // random skin support
    this->randomizeFilePath();

    bool was_first_load = false;

    auto try_load_sound = [isSample, isOverlayable, &was_first_load](const std::string &base_path, const std::string &filename,
                                                    bool loop, const std::string &resource_name,
                                                    bool default_skin) -> Sound * {
        const char *extensions[] = {".wav", ".mp3", ".ogg", ".flac"};
        for(auto &extension : extensions) {
            std::string fn = filename;
            fn.append(extension);

            std::string path = base_path;
            path.append(fn);

            // this check will fix up the filename casing
            if(env->fileExists(path)) {
                Sound *existing_sound = resourceManager->getSound(resource_name);

                // default already loaded, just return it
                if(default_skin && existing_sound) return existing_sound;

                was_first_load = true;

                // user skin, destroy old sound resource (blocking)
                if(existing_sound) resourceManager->destroyResource(existing_sound, true);

                if(cv::skin_async.getBool()) {
                    resourceManager->requestNextLoadAsync();
                }

                // load sound here
                return resourceManager->loadSoundAbs(path, resource_name, !isSample, isOverlayable, loop);
            }
        }
        return nullptr;
    };

    // load user skin
    bool loaded_user_skin = false;
    if(cv::skin_use_skin_hitsounds.getBool() || !isSample) {
        sndRef = try_load_sound(this->sFilePath, skinElementName, loop, resourceName, false);
        loaded_user_skin = (sndRef != nullptr);
    }

    if(fallback_to_default && !loaded_user_skin) {
        std::string defaultpath = MCENGINE_DATA_DIR "materials/default/";
        std::string defaultResourceName = std::move(resourceName);
        defaultResourceName.append("_DEFAULT");
        sndRef = try_load_sound(defaultpath, skinElementName, loop, defaultResourceName, true);
    }

    // failed both default and user
    if(sndRef == nullptr) {
        debugLog("Skin Warning: NULL sound {:s}!\n", skinElementName.c_str());
        return;
    }

    // force reload default skin sound anyway if the custom skin does not include it (e.g. audio device change)
    if(!loaded_user_skin && !was_first_load) {
        resourceManager->reloadResource(sndRef, cv::skin_async.getBool());
    }

    if(isSample) {
        this->soundSamples.push_back({
            .sound = sndRef,
            .hardcodedVolumeMultiplier = (hardcodedVolumeMultiplier >= 0.0f ? hardcodedVolumeMultiplier : 1.0f),
        });
    }

    this->sounds.push_back(sndRef);

    // export
    this->filepathsForExport.push_back(sndRef->getFilePath());
}

bool Skin::compareFilenameWithSkinElementName(const std::string &filename, const std::string &skinElementName) {
    if(filename.length() == 0 || skinElementName.length() == 0) return false;
    return filename.substr(0, filename.find_last_of('.')) == skinElementName;
}

Sound *Skin::getSpinnerBonus() const {
    //
    return this->spinnerBonus;
}

Sound *Skin::getSpinnerSpinSound() const {
    //
    return this->spinnerSpinSound;
}

Sound *Skin::getCombobreak() const {
    //
    return this->combobreak;
}

Sound *Skin::getFailsound() const {
    //
    return this->failsound;
}

Sound *Skin::getApplause() const {
    //
    return this->applause;
}

Sound *Skin::getMenuHit() const {
    //
    return this->menuHit;
}

Sound *Skin::getMenuHover() const {
    //
    return this->menuHover;
}

Sound *Skin::getCheckOn() const {
    //
    return this->checkOn;
}

Sound *Skin::getCheckOff() const {
    //
    return this->checkOff;
}

Sound *Skin::getShutter() const {
    //
    return this->shutter;
}

Sound *Skin::getSectionPassSound() const {
    //
    return this->sectionPassSound;
}

Sound *Skin::getSectionFailSound() const {
    //
    return this->sectionFailSound;
}

Sound *Skin::getExpandSound() const {
    //
    return this->expand;
}

Sound *Skin::getMessageSentSound() const {
    //
    return this->messageSent;
}

Sound *Skin::getDeletingTextSound() const {
    //
    return this->deletingText;
}

Sound *Skin::getMovingTextCursorSound() const {
    //
    return this->movingTextCursor;
}

Sound *Skin::getTyping1Sound() const {
    //
    return this->typing1;
}

Sound *Skin::getTyping2Sound() const {
    //
    return this->typing2;
}

Sound *Skin::getTyping3Sound() const {
    //
    return this->typing3;
}

Sound *Skin::getTyping4Sound() const {
    //
    return this->typing4;
}

Sound *Skin::getMenuBackSound() const {
    //
    return this->menuBack;
}

Sound *Skin::getCloseChatTabSound() const {
    //
    return this->closeChatTab;
}

Sound *Skin::getHoverButtonSound() const {
    //
    return this->hoverButton;
}

Sound *Skin::getClickButtonSound() const {
    //
    return this->clickButton;
}

Sound *Skin::getClickMainMenuCubeSound() const {
    //
    return this->clickMainMenuCube;
}

Sound *Skin::getHoverMainMenuCubeSound() const {
    //
    return this->hoverMainMenuCube;
}

Sound *Skin::getClickSingleplayerSound() const {
    //
    return this->clickSingleplayer;
}

Sound *Skin::getHoverSingleplayerSound() const {
    //
    return this->hoverSingleplayer;
}

Sound *Skin::getClickMultiplayerSound() const {
    //
    return this->clickMultiplayer;
}

Sound *Skin::getHoverMultiplayerSound() const {
    //
    return this->hoverMultiplayer;
}

Sound *Skin::getClickOptionsSound() const {
    //
    return this->clickOptions;
}

Sound *Skin::getHoverOptionsSound() const {
    //
    return this->hoverOptions;
}

Sound *Skin::getClickExitSound() const {
    //
    return this->clickExit;
}

Sound *Skin::getHoverExitSound() const {
    //
    return this->hoverExit;
}

Sound *Skin::getPauseLoopSound() const {
    //
    return this->pauseLoop;
}

Sound *Skin::getPauseHoverSound() const {
    //
    return this->pauseHover;
}

Sound *Skin::getClickPauseBackSound() const {
    //
    return this->clickPauseBack;
}

Sound *Skin::getHoverPauseBackSound() const {
    //
    return this->hoverPauseBack;
}

Sound *Skin::getClickPauseContinueSound() const {
    //
    return this->clickPauseContinue;
}

Sound *Skin::getHoverPauseContinueSound() const {
    //
    return this->hoverPauseContinue;
}

Sound *Skin::getClickPauseRetrySound() const {
    //
    return this->clickPauseRetry;
}

Sound *Skin::getHoverPauseRetrySound() const {
    //
    return this->hoverPauseRetry;
}

Sound *Skin::getBackButtonClickSound() const {
    //
    return this->backButtonClick;
}

Sound *Skin::getBackButtonHoverSound() const {
    //
    return this->backButtonHover;
}

Sound *Skin::getSelectDifficultySound() const {
    //
    return this->selectDifficulty;
}

Sound *Skin::getSliderbarSound() const {
    //
    return this->sliderbar;
}

Sound *Skin::getMatchConfirmSound() const {
    //
    return this->matchConfirm;
}

Sound *Skin::getRoomJoinedSound() const {
    //
    return this->roomJoined;
}

Sound *Skin::getRoomQuitSound() const {
    //
    return this->roomQuit;
}

Sound *Skin::getRoomNotReadySound() const {
    //
    return this->roomNotReady;
}

Sound *Skin::getRoomReadySound() const {
    //
    return this->roomReady;
}

Sound *Skin::getMatchStartSound() const {
    //
    return this->matchStart;
}
