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
#include "SString.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "UpdateHandler.h"

UpdateHandler::UpdateHandler() {
    this->update_url = "";
    this->status = STATUS::STATUS_UP_TO_DATE;
    this->iNumRetries = 0;
}

void UpdateHandler::checkForUpdates() {
    this->status = STATUS::STATUS_CHECKING_FOR_UPDATE;

    UString versionUrl = "https://" NEOSU_DOMAIN;
    if(cv::bleedingedge.getBool()) {
        versionUrl.append("/bleedingedge/" OS_NAME ".txt");
    } else {
        versionUrl.append("/update/" OS_NAME "/latest-version.txt");
    }

    debugLogF("UpdateHandler: Checking for a newer version from {}\n", versionUrl);

    NetworkHandler::RequestOptions options;
    options.timeout = 10;
    options.connectTimeout = 5;

    networkHandler->httpRequestAsync(
        versionUrl,
        [this](const NetworkHandler::Response &response) {
            this->onVersionCheckComplete(response.body, response.success);
        },
        options);
}

void UpdateHandler::onVersionCheckComplete(const std::string &response, bool success) {
    if(!success || response.empty()) {
        this->status = STATUS::STATUS_UP_TO_DATE;
        debugLog("UpdateHandler ERROR: Failed to check for updates :/\n");
        return;
    }

    auto lines = SString::split(response, "\n");
    f32 latest_version = strtof(lines[0].c_str(), NULL);
    u64 latest_build_tms = 0;
    if(lines.size() > 1) {
        latest_build_tms = std::strtoull(lines[1].c_str(), NULL, 10);
    }

    if(latest_version == 0.f && latest_build_tms == 0) {
        this->status = STATUS::STATUS_UP_TO_DATE;
        debugLog("UpdateHandler ERROR: Failed to parse version number\n");
        return;
    }

    u64 current_build_tms = cv::build_timestamp.getU64();
    bool should_update = true;  //(cv::version.getFloat() < latest_version) || (current_build_tms < latest_build_tms);
    if(!should_update) {
        // We're already up to date
        this->status = STATUS::STATUS_UP_TO_DATE;
        debugLog("UpdateHandler: We're already up to date (current v%.2f (%d), latest v%.2f (%d))\n",
                 cv::version.getFloat(), current_build_tms, latest_version, latest_build_tms);
        return;
    }

    debugLog("UpdateHandler: Downloading latest update... (current v%.2f (%d), latest v%.2f (%d))\n",
             cv::version.getFloat(), current_build_tms, latest_version, latest_build_tms);

    this->update_url = "https://" NEOSU_DOMAIN;
    if(cv::bleedingedge.getBool()) {
        this->update_url.append("/bleedingedge/" OS_NAME ".zip");
    } else {
        this->update_url.append(UString::format("/update/" OS_NAME "/v%.2f.zip", latest_version));
    }

    this->downloadUpdate();
}

void UpdateHandler::downloadUpdate() {
    debugLog("UpdateHandler: Downloading %s\n", this->update_url.toUtf8());
    this->status = STATUS::STATUS_DOWNLOADING_UPDATE;

    NetworkHandler::RequestOptions options;
    options.timeout = 300;  // 5 minutes for large downloads
    options.connectTimeout = 10;
    options.followRedirects = true;

    networkHandler->httpRequestAsync(
        this->update_url,
        [this](const NetworkHandler::Response &response) { this->onDownloadComplete(response.body, response.success); },
        options);
}

void UpdateHandler::onDownloadComplete(const std::string &data, bool success) {
    if(!success || data.length() < 2) {
        debugLog("UpdateHandler ERROR: downloaded file is too small or failed (%zu bytes)!\n", data.length());
        this->status = STATUS::STATUS_ERROR;

        // retry logic
        this->iNumRetries++;
        if(this->iNumRetries < 4) {
            this->checkForUpdates();
        }
        return;
    }

    // write to disk
    debugLog("UpdateHandler: Downloaded file has %zu bytes, writing ...\n", data.length());
    std::ofstream file(TEMP_UPDATE_DOWNLOAD_FILEPATH, std::ios::out | std::ios::binary);
    if(file.good()) {
        file.write(data.data(), static_cast<std::streamsize>(data.length()));
        file.close();

        debugLog("UpdateHandler: Update finished successfully.\n");
        this->installUpdate(TEMP_UPDATE_DOWNLOAD_FILEPATH);
    } else {
        debugLog("UpdateHandler ERROR: Can't write file!\n");
        this->status = STATUS::STATUS_ERROR;

        // retry logic
        this->iNumRetries++;
        if(this->iNumRetries < 4) {
            this->checkForUpdates();
        }
    }
}

void UpdateHandler::installUpdate(const std::string &zipFilePath) {
    debugLog("UpdateHandler: installing %s\n", zipFilePath.c_str());
    this->status = STATUS::STATUS_INSTALLING_UPDATE;

    if(!env->fileExists(zipFilePath)) {
        debugLog("UpdateHandler ERROR: \"%s\" does not exist!\n", zipFilePath.c_str());
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    Archive archive(zipFilePath);
    if(!archive.isValid()) {
        debugLog("UpdateHandler ERROR: couldn't open archive!\n");
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog("UpdateHandler ERROR: archive is empty!\n");
        this->status = STATUS::STATUS_ERROR;
        return;
    }

    // separate raw dirs and files
    std::string mainDirectory = "neosu/";
    std::vector<Archive::Entry> files, dirs;
    for(const auto &entry : entries) {
        auto fileName = entry.getFilename();
        if(fileName.find(mainDirectory) != 0) {
            debugLog("UpdateHandler WARNING: Ignoring \"%s\" because it's not in the main dir!\n", fileName.c_str());
            continue;
        }

        if(entry.isDirectory()) {
            dirs.push_back(entry);
        } else {
            files.push_back(entry);
        }

        debugLog("UpdateHandler: Filename: \"%s\", isDir: %i, uncompressed size: %u\n", entry.getFilename().c_str(),
                 (int)entry.isDirectory(), (unsigned int)entry.getUncompressedSize());
    }

    // repair/create missing/new dirs
    for(const auto &dir : dirs) {
        std::string newDir = dir.getFilename().substr(mainDirectory.length());
        if(newDir.length() == 0) continue;
        if(env->directoryExists(newDir)) continue;

        debugLog("UpdateHandler: Creating directory %s\n", newDir.c_str());
        env->createDirectory(newDir);
    }

    // extract and overwrite almost everything
    for(const auto &file : files) {
        std::string outFilePath = file.getFilename().substr(mainDirectory.length());
        if(outFilePath.length() == 0) continue;

        // .exe and .dll can't be directly overwritten on windows
        std::string old_path{};
        bool temp_created = false;
        if(outFilePath.length() > 4) {
            if(!strcasecmp(outFilePath.c_str() + outFilePath.length() - 4, ".exe") ||
               !strcasecmp(outFilePath.c_str() + outFilePath.length() - 4, ".dll")) {
                old_path = outFilePath;
                old_path.append(".old");
                env->deleteFile(old_path);
                env->renameFile(outFilePath, old_path);
                temp_created = true;
            }
        }

        debugLog("UpdateHandler: Writing %s\n", outFilePath.c_str());
        if(!file.extractToFile(outFilePath)) {
            debugLog("UpdateHandler: Failed to extract file %s\n", outFilePath.c_str());
            if(temp_created) {
                env->deleteFile(outFilePath);
                env->renameFile(old_path, outFilePath);
            }
            this->status = STATUS::STATUS_ERROR;
            return;
        }
    }

    this->status = STATUS::STATUS_SUCCESS_INSTALLATION;
    env->deleteFile(zipFilePath);
}
