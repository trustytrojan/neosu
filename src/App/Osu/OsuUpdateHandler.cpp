//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "BanchoNetworking.h"
#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "NetworkHandler.h"
#include "Osu.h"
#include "OsuUpdateHandler.h"
#include "ResourceManager.h"
#include "miniz.h"

const char *OsuUpdateHandler::TEMP_UPDATE_DOWNLOAD_FILEPATH = "update.zip";

void *OsuUpdateHandler::run(void *data) {
    // not using semaphores/mutexes here because it's not critical

    if(data == NULL) return NULL;

    OsuUpdateHandler *handler = (OsuUpdateHandler *)data;

    if(handler->_m_bKYS) return NULL;  // cancellation point

    // check for updates
    handler->_requestUpdate();

    if(handler->_m_bKYS) return NULL;  // cancellation point

    // continue if we have one. reset the thread in both cases after we're done
    if(handler->m_status != STATUS::STATUS_UP_TO_DATE) {
        // try to download and install the update
        if(handler->_downloadUpdate()) {
            if(handler->_m_bKYS) return NULL;  // cancellation point

            handler->_installUpdate(TEMP_UPDATE_DOWNLOAD_FILEPATH);
        }

        handler->m_updateThread = 0;  // reset

        if(handler->_m_bKYS) return NULL;  // cancellation point

        // retry up to 3 times if something went wrong
        if(handler->getStatus() != STATUS::STATUS_SUCCESS_INSTALLATION) {
            handler->m_iNumRetries++;
            if(handler->m_iNumRetries < 4) handler->checkForUpdates();
        }
    } else
        handler->m_updateThread = 0;  // reset

    return NULL;
}

OsuUpdateHandler::OsuUpdateHandler() {
    m_updateThread = 0;
    update_url = "";

    m_status = convar->getConVarByName("auto_update")->getBool() ? STATUS::STATUS_CHECKING_FOR_UPDATE
                                                                 : STATUS::STATUS_UP_TO_DATE;
    m_iNumRetries = 0;
    _m_bKYS = false;
}

OsuUpdateHandler::~OsuUpdateHandler() {
    if(m_updateThread != 0)
        engine->showMessageErrorFatal("Fatal Error",
                                      "OsuUpdateHandler was destroyed while the update thread is still running!!!");
}

void OsuUpdateHandler::stop() {
    if(m_updateThread != 0) {
        _m_bKYS = true;
        m_updateThread = 0;
    }
}

void OsuUpdateHandler::wait() {
    if(m_updateThread != 0) {
        pthread_join(m_updateThread, NULL);
        m_updateThread = 0;
    }
}

void OsuUpdateHandler::checkForUpdates() {
    if(!convar->getConVarByName("auto_update")->getBool() || Osu::debug->getBool() || m_updateThread != 0) return;

    int ret = pthread_create(&m_updateThread, NULL, OsuUpdateHandler::run, (void *)this);
    if(ret) {
        m_updateThread = 0;
        debugLog("OsuUpdateHandler: Error, pthread_create() returned %i!\n", ret);
        return;
    }

    if(m_iNumRetries > 0) debugLog("OsuUpdateHandler::checkForUpdates() retry %i ...\n", m_iNumRetries);
}

void OsuUpdateHandler::_requestUpdate() {
    debugLog("OsuUpdateHandler::requestUpdate()\n");
    m_status = STATUS::STATUS_CHECKING_FOR_UPDATE;

    UString latestVersion = engine->getNetworkHandler()->httpGet(MCOSU_UPDATE_URL "/latest-version.txt");
    float fLatestVersion = strtof(latestVersion.toUtf8(), NULL);
    if(fLatestVersion == 0.f) {
        m_status = STATUS::STATUS_UP_TO_DATE;
        debugLog("Failed to check for updates :/\n");
        return;
    }

    float current_version = convar->getConVarByName("osu_version")->getFloat();
    if(current_version >= fLatestVersion) {
        // We're already up to date
        m_status = STATUS::STATUS_UP_TO_DATE;
        debugLog("We're already up to date (current v%.2f, latest v%.2f)\n", current_version, fLatestVersion);
        return;
    }

#ifdef _WIN32
    const char *os = "windows";
#else
    const char *os = "linux";
#endif
    debugLog("Downloading latest update... (current v%.2f, latest v%.2f)\n", current_version, fLatestVersion);
    update_url = UString::format(MCOSU_UPDATE_URL "/mcosu-multiplayer-%s-v%.2f.zip", os, fLatestVersion);
}

bool OsuUpdateHandler::_downloadUpdate() {
    UString url = update_url;

    debugLog("OsuUpdateHandler::downloadUpdate( %s )\n", url.toUtf8());
    m_status = STATUS::STATUS_DOWNLOADING_UPDATE;

    // setting the status in every error check return is retarded

    // download
    std::string data = engine->getNetworkHandler()->httpDownload(url);
    if(data.length() < 2) {
        debugLog("OsuUpdateHandler::downloadUpdate() error, downloaded file is too small (%i)!\n", data.length());
        m_status = STATUS::STATUS_ERROR;
        return false;
    }

    // write to disk
    debugLog("OsuUpdateHandler: Downloaded file has %i length, writing ...\n", data.length());
    std::ofstream file(TEMP_UPDATE_DOWNLOAD_FILEPATH, std::ios::out | std::ios::binary);
    if(file.good()) {
        file.write(data.data(), data.length());
        file.close();
    } else {
        debugLog("OsuUpdateHandler::downloadUpdate() error, can't write file!\n");
        m_status = STATUS::STATUS_ERROR;
        return false;
    }

    debugLog("OsuUpdateHandler::downloadUpdate() finished successfully.\n");
    return true;
}

void OsuUpdateHandler::_installUpdate(std::string zipFilePath) {
    debugLog("OsuUpdateHandler::installUpdate( %s )\n", zipFilePath.c_str());
    m_status = STATUS::STATUS_INSTALLING_UPDATE;

    // setting the status in every error check return is retarded

    if(!env->fileExists(zipFilePath)) {
        debugLog("OsuUpdateHandler::installUpdate() error, \"%s\" does not exist!\n", zipFilePath.c_str());
        m_status = STATUS::STATUS_ERROR;
        return;
    }

    // load entire file
    File f(zipFilePath);
    if(!f.canRead()) {
        debugLog("OsuUpdateHandler::installUpdate() error, can't read file!\n");
        m_status = STATUS::STATUS_ERROR;
        return;
    }
    const uint8_t *content = f.readFile();

    // initialize zip
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if(!mz_zip_reader_init_mem(&zip_archive, content, f.getFileSize(), 0)) {
        debugLog("OsuUpdateHandler::installUpdate() error, couldn't mz_zip_reader_init_mem()!\n");
        m_status = STATUS::STATUS_ERROR;
        return;
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
    if(numFiles <= 0) {
        debugLog("OsuUpdateHandler::installUpdate() error, %u files!\n", numFiles);
        m_status = STATUS::STATUS_ERROR;
        return;
    }
    if(!mz_zip_reader_is_file_a_directory(&zip_archive, 0)) {
        debugLog("OsuUpdateHandler::installUpdate() error, first index is not the main directory!\n");
        m_status = STATUS::STATUS_ERROR;
        return;
    }

    // get main dir name (assuming that the first file is the main subdirectory)
    mz_zip_archive_file_stat file_stat;
    mz_zip_reader_file_stat(&zip_archive, 0, &file_stat);
    std::string mainDirectory = file_stat.m_filename;

    // split raw dirs and files
    std::vector<std::string> files;
    std::vector<std::string> dirs;
    for(int i = 1; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            debugLog("OsuUpdateHandler::installUpdate() warning, couldn't mz_zip_reader_file_stat() index %i!\n", i);
            continue;
        }

        if(mz_zip_reader_is_file_a_directory(&zip_archive, i))
            dirs.push_back(file_stat.m_filename);
        else
            files.push_back(file_stat.m_filename);

        debugLog("OsuUpdateHandler: Filename: \"%s\", isDir: %i, uncompressed size: %u, compressed size: %u\n",
                 file_stat.m_filename, (int)mz_zip_reader_is_file_a_directory(&zip_archive, i),
                 (unsigned int)file_stat.m_uncomp_size, (unsigned int)file_stat.m_comp_size);
    }

    // repair/create missing/new dirs
    std::string cfgDir = MCENGINE_DATA_DIR "cfg/";
    bool cfgDirExists = env->directoryExists(cfgDir);
    for(int i = 0; i < dirs.size(); i++) {
        int mainDirectoryOffset = dirs[i].find(mainDirectory);
        if(mainDirectoryOffset == 0 && dirs[i].length() - mainDirectoryOffset > 0 &&
           mainDirectoryOffset + mainDirectory.length() < dirs[i].length()) {
            std::string newDir = dirs[i].substr(mainDirectoryOffset + mainDirectory.length());

            if(!env->directoryExists(newDir)) {
                debugLog("OsuUpdateHandler: Creating directory %s\n", newDir.c_str());
                env->createDirectory(newDir);
            }
        }
    }

    // on windows it's impossible to overwrite a running executable, but we can simply rename/move it
    // this is not necessary on other operating systems, but it doesn't break anything so
    std::string executablePath = env->getExecutablePath();
    if(executablePath.length() > 0) {
        std::string oldExecutablePath = executablePath;
        oldExecutablePath.append(".old");
        env->deleteFile(oldExecutablePath);  // must delete potentially existing file from previous update, otherwise
                                             // renaming will not work
        env->renameFile(executablePath, oldExecutablePath);
    }

    // extract and overwrite almost everything
    for(int i = 0; i < files.size(); i++) {
        // ignore cfg directory (don't want to overwrite user settings), except if it doesn't exist
        if(files[i].find(cfgDir) != -1 && cfgDirExists) {
            debugLog("OsuUpdateHandler: Ignoring file \"%s\"\n", files[i].c_str());
            continue;
        }

        int mainDirectoryOffset = files[i].find(mainDirectory);

        if(mainDirectoryOffset == 0 && files[i].length() - mainDirectoryOffset > 0 &&
           mainDirectoryOffset + mainDirectory.length() < files[i].length()) {
            std::string outFilePath = files[i].substr(mainDirectoryOffset + mainDirectory.length());
            debugLog("OsuUpdateHandler: Writing %s\n", outFilePath.c_str());
            mz_zip_reader_extract_file_to_file(&zip_archive, files[i].c_str(), outFilePath.c_str(), 0);
        } else if(mainDirectoryOffset != 0)
            debugLog(
                "OsuUpdateHandler::installUpdate() warning, ignoring file \"%s\" because it's not in the main dir!\n",
                files[i].c_str());
    }

    mz_zip_reader_end(&zip_archive);

#ifndef _WIN32
    chmod(executablePath.c_str(), S_IRWXU);
#endif

    m_status = STATUS::STATUS_SUCCESS_INSTALLATION;
}

Environment::OS OsuUpdateHandler::stringToOS(UString osString) {
    Environment::OS os = Environment::OS::OS_NULL;
    if(osString.find("windows") != -1)
        os = Environment::OS::OS_WINDOWS;
    else if(osString.find("linux") != -1)
        os = Environment::OS::OS_LINUX;
    else if(osString.find("macos") != -1)
        os = Environment::OS::OS_MACOS;

    return os;
}
