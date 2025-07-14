#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "Archival.h"
#include "BanchoNetworking.h"
#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "NetworkHandler.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "UpdateHandler.h"

const char *UpdateHandler::TEMP_UPDATE_DOWNLOAD_FILEPATH = "update.zip";

void *UpdateHandler::run(void *data) {
    // not using semaphores/mutexes here because it's not critical

    if(data == NULL) return NULL;

    UpdateHandler *handler = (UpdateHandler *)data;

    if(handler->_m_bKYS) return NULL;  // cancellation point

    // check for updates
    handler->_requestUpdate();

    if(handler->_m_bKYS) return NULL;  // cancellation point

    // continue if we have one. reset the thread in both cases after we're done
    if(handler->status != STATUS::STATUS_UP_TO_DATE) {
        // try to download and install the update
        if(handler->_downloadUpdate()) {
            if(handler->_m_bKYS) return NULL;  // cancellation point

            handler->_installUpdate(TEMP_UPDATE_DOWNLOAD_FILEPATH);
        }

        handler->updateThread = NULL;  // reset

        if(handler->_m_bKYS) return NULL;  // cancellation point

        // retry up to 3 times if something went wrong
        if(handler->getStatus() != STATUS::STATUS_SUCCESS_INSTALLATION) {
            handler->iNumRetries++;
            if(handler->iNumRetries < 4) handler->checkForUpdates();
        }
    } else {
        handler->updateThread = NULL;  // reset
    }

    return NULL;
}

UpdateHandler::UpdateHandler() {
    this->update_url = "";

    this->status = cv::auto_update.getBool() ? STATUS::STATUS_CHECKING_FOR_UPDATE : STATUS::STATUS_UP_TO_DATE;
    this->iNumRetries = 0;
    this->_m_bKYS = false;
}

UpdateHandler::~UpdateHandler() {}

void UpdateHandler::stop() {
    this->_m_bKYS = true;
    this->updateThread = NULL;
}

void UpdateHandler::wait() {
    if(this->updateThread != NULL) {
        this->updateThread->join();
    }
}

void UpdateHandler::checkForUpdates() {
    if(!cv::auto_update.getBool() || cv::debug.getBool() ||
       (this->updateThread != NULL && this->updateThread->joinable()))
        return;

    this->updateThread = new std::thread(UpdateHandler::run, (void *)this);
    this->updateThread->detach();

    if(this->iNumRetries > 0) debugLog("UpdateHandler::checkForUpdates() retry %i ...\n", this->iNumRetries);
}

void UpdateHandler::_requestUpdate() {
    debugLog("UpdateHandler::requestUpdate()\n");
    this->status = STATUS::STATUS_CHECKING_FOR_UPDATE;

    UString latestVersion = networkHandler->httpGet("https://" NEOSU_DOMAIN "/update/" OS_NAME "/latest-version.txt");
    float fLatestVersion = strtof(latestVersion.toUtf8(), NULL);
    if(fLatestVersion == 0.f) {
        this->status = STATUS::STATUS_UP_TO_DATE;
        debugLog("Failed to check for updates :/\n");
        return;
    }

    float current_version = cv::version.getFloat();
    if(current_version >= fLatestVersion) {
        // We're already up to date
        this->status = STATUS::STATUS_UP_TO_DATE;
        debugLog("We're already up to date (current v%.2f, latest v%.2f)\n", current_version, fLatestVersion);
        return;
    }

    debugLog("Downloading latest update... (current v%.2f, latest v%.2f)\n", current_version, fLatestVersion);
    this->update_url = UString::format("https://" NEOSU_DOMAIN "/update/" OS_NAME "/v%.2f.zip", fLatestVersion);
}

bool UpdateHandler::_downloadUpdate() {
    UString url = this->update_url;

    debugLog("UpdateHandler::downloadUpdate( %s )\n", url.toUtf8());
    this->status = STATUS::STATUS_DOWNLOADING_UPDATE;

    // setting the status in every error check return is retarded

    // download
    std::string data = networkHandler->httpDownload(url);
    if(data.length() < 2) {
        debugLog("UpdateHandler::downloadUpdate() error, downloaded file is too small (%i)!\n", data.length());
        this->status = STATUS::STATUS_ERROR;
        return false;
    }

    // write to disk
    debugLog("UpdateHandler: Downloaded file has %i length, writing ...\n", data.length());
    std::ofstream file(TEMP_UPDATE_DOWNLOAD_FILEPATH, std::ios::out | std::ios::binary);
    if(file.good()) {
        file.write(data.data(), data.length());
        file.close();
    } else {
        debugLog("UpdateHandler::downloadUpdate() error, can't write file!\n");
        this->status = STATUS::STATUS_ERROR;
        return false;
    }

    debugLog("UpdateHandler::downloadUpdate() finished successfully.\n");
    return true;
}

void UpdateHandler::_installUpdate(const std::string &zipFilePath) {
    debugLog("UpdateHandler::installUpdate( %s )\n", zipFilePath.c_str());
    this->status = STATUS::STATUS_INSTALLING_UPDATE;

    if(!env->fileExists(zipFilePath)) {
        debugLog("UpdateHandler::installUpdate() error, \"%s\" does not exist!\n", zipFilePath.c_str());
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    Archive archive(zipFilePath);
    if(!archive.isValid()) {
        debugLog("UpdateHandler::installUpdate() error, couldn't open archive!\n");
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog("UpdateHandler::installUpdate() error, archive is empty!\n");
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    // find main directory (assuming first entry is the main subdirectory)
    std::string mainDirectory;
    if(!entries.empty() && entries[0].isDirectory()) {
        mainDirectory = entries[0].getFilename();
    } else {
        debugLog("UpdateHandler::installUpdate() error, first entry is not the main directory!\n");
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    // separate raw dirs and files
    std::vector<Archive::Entry> files, dirs;
    for(const auto &entry : entries) {
        if(entry.isDirectory()) {
            dirs.push_back(entry);
        } else {
            files.push_back(entry);
        }

        debugLog("UpdateHandler: Filename: \"%s\", isDir: %i, uncompressed size: %u\n", entry.getFilename().c_str(),
                 (int)entry.isDirectory(), (unsigned int)entry.getUncompressedSize());
    }

    // repair/create missing/new dirs
    std::string cfgDir = MCENGINE_DATA_DIR "cfg/";
    bool cfgDirExists = env->directoryExists(cfgDir);
    for(const auto &dir : dirs) {
        std::string dirName = dir.getFilename();
        int mainDirectoryOffset = dirName.find(mainDirectory);
        if(mainDirectoryOffset == 0 && dirName.length() - mainDirectoryOffset > 0 &&
           mainDirectoryOffset + mainDirectory.length() < dirName.length()) {
            std::string newDir = dirName.substr(mainDirectoryOffset + mainDirectory.length());

            if(!env->directoryExists(newDir)) {
                debugLog("UpdateHandler: Creating directory %s\n", newDir.c_str());
                env->createDirectory(newDir);
            }
        }
    }

    // extract and overwrite almost everything
    for(const auto &file : files) {
        std::string fileName = file.getFilename();

        // ignore cfg directory (don't want to overwrite user settings), except if it doesn't exist
        if(fileName.find(cfgDir) != std::string::npos && cfgDirExists) {
            debugLog("UpdateHandler: Ignoring file \"%s\"\n", fileName.c_str());
            continue;
        }

        int mainDirectoryOffset = fileName.find(mainDirectory);
        if(mainDirectoryOffset == 0 && fileName.length() - mainDirectoryOffset > 0 &&
           mainDirectoryOffset + mainDirectory.length() < fileName.length()) {
            std::string outFilePath = fileName.substr(mainDirectoryOffset + mainDirectory.length());

            // .exe and .dll can't be directly overwritten on windows
            if(outFilePath.length() > 4) {
                if(!strcmp(outFilePath.c_str() + outFilePath.length() - 4, ".exe") ||
                   !strcmp(outFilePath.c_str() + outFilePath.length() - 4, ".dll")) {
                    std::string old_path = outFilePath;
                    old_path.append(".old");
                    env->deleteFile(old_path);
                    env->renameFile(outFilePath, old_path);
                }
            }

            debugLog("UpdateHandler: Writing %s\n", outFilePath.c_str());
            if(!file.extractToFile(outFilePath)) {
                debugLog("UpdateHandler: Failed to extract file %s\n", outFilePath.c_str());
            }
        } else if(mainDirectoryOffset != 0) {
            debugLog("UpdateHandler::installUpdate() warning, ignoring file \"%s\" because it's not in the main dir!\n",
                     fileName.c_str());
        }
    }

    this->status = STATUS::STATUS_SUCCESS_INSTALLATION;
    env->deleteFile(zipFilePath);
}
