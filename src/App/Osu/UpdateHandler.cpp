#include "UpdateHandler.h"

#include "Archival.h"
#include "BanchoNetworking.h"
#include "ConVar.h"
#include "crypto.h"
#include "Engine.h"
#include "File.h"
#include "NetworkHandler.h"
#include "SString.h"
#include "Osu.h"

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <fstream>
#include <iomanip>
#include <sstream>

using enum UpdateHandler::STATUS;

std::string UpdateHandler::getStreamArchiveName() {
    return cv::bleedingedge.getBool() ? "update-bleeding-" OS_NAME ".zip" : "update-" OS_NAME ".zip";
}

std::string UpdateHandler::getStreamCacheName() {
    return cv::bleedingedge.getBool() ? "update_cache-bleeding-" OS_NAME ".txt" : "update_cache-" OS_NAME ".txt";
}

void UpdateHandler::onBleedingEdgeChanged(float oldVal, float newVal) {
    const bool oldState = !!static_cast<int>(oldVal);
    const bool newState = !!static_cast<int>(newVal);

    if((oldState == newState) || ((this->getStreamArchiveName() == this->updateArchiveName) &&
                                  (this->getStreamCacheName() == this->updateCacheInfoPath))) {
        return;
    }

    const auto status = this->getStatus();
    const bool allowChange = (status == STATUS_INITIAL || status == STATUS_UP_TO_DATE ||
                              status == STATUS_DOWNLOAD_COMPLETE || status == STATUS_ERROR);
    if(allowChange) {
        // don't interfere with the normal update check if we got a change callback from reading a config
        // just keep the cvar set, the pending update will use the right paths (depending on current cv::bleedingedge)
        if(status != STATUS_INITIAL) {
            this->updateArchiveName = this->getStreamArchiveName();
            this->updateCacheInfoPath = this->getStreamCacheName();

            this->status = STATUS_UP_TO_DATE;
            this->iNumRetries = 0;
            this->checkForUpdates();
        }
    } else {
        debugLog("Can't change release stream while an update is in progress!\n");
        cv::bleedingedge.setValue(oldState, false);
    }
}

UpdateHandler::UpdateHandler() {
    this->update_url = "";
    this->status = STATUS_INITIAL;
    this->iNumRetries = 0;
    this->currentVersionContent = "";
    this->updateArchiveName = this->getStreamArchiveName();
    this->updateCacheInfoPath = this->getStreamCacheName();
    cv::bleedingedge.setCallback(SA::MakeDelegate<&UpdateHandler::onBleedingEdgeChanged>(this));
    cv::force_update.setCallback(SA::MakeDelegate<&UpdateHandler::checkForUpdates>(this));
}

void UpdateHandler::checkForUpdates() {
    if(cv::force_update.getBool() &&
       !(this->getStatus() == STATUS_UP_TO_DATE || this->getStatus() == STATUS_DOWNLOAD_COMPLETE ||
         this->getStatus() == STATUS_ERROR)) {
        debugLog("Can't force an update check now, wait until the update is finished!\n");
        cv::force_update.setValue(false, false);
        return;
    }

    this->status = STATUS_CHECKING_FOR_UPDATE;

    this->updateArchiveName = this->getStreamArchiveName();
    this->updateCacheInfoPath = this->getStreamCacheName();

    UString versionUrl = "https://" NEOSU_DOMAIN;
    if(cv::bleedingedge.getBool()) {
        versionUrl.append("/bleedingedge/" OS_NAME ".txt");
    } else {
        versionUrl.append("/update/" OS_NAME "/latest-version.txt");
    }

    debugLog("UpdateHandler: Checking for a newer version from {}\n", versionUrl);

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
        this->status = STATUS_UP_TO_DATE;
        debugLog("UpdateHandler ERROR: Failed to check for updates :/\n");
        return;
    }

    // store version content for cache operations
    this->currentVersionContent = response;

    auto lines = SString::split(response, "\n");
    f32 latest_version = strtof(lines[0].c_str(), NULL);
    u64 latest_build_tms = 0;
    if(lines.size() > 1) {
        latest_build_tms = std::strtoull(lines[1].c_str(), NULL, 10);
    }

    if(latest_version == 0.f && latest_build_tms == 0) {
        this->status = STATUS_UP_TO_DATE;
        debugLog("UpdateHandler ERROR: Failed to parse version number\n");
        return;
    }

    u64 current_build_tms = cv::build_timestamp.getU64();
    bool should_update = cv::force_update.getBool() || (cv::version.getFloat() < latest_version) ||
                         (current_build_tms < latest_build_tms);
    if(!should_update) {
        // We're already up to date
        this->status = STATUS_UP_TO_DATE;
        debugLog("UpdateHandler: We're already up to date (current v{:.2f} ({:d}), latest v{:.2f} ({:d}))\n",
                 cv::version.getFloat(), current_build_tms, latest_version, latest_build_tms);
        return;
    }

    // check if we have a cached update that matches this version
    if(this->isCachedUpdateValid(response)) {
        debugLog("UpdateHandler: Found valid cached update, skipping download\n");
        this->status = STATUS_DOWNLOAD_COMPLETE;
        return;
    }

    debugLog("UpdateHandler: Downloading latest update... (current v{:.2f} ({:d}), latest v{:.2f} ({:d}))\n",
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
    debugLog("UpdateHandler: Downloading {:s}\n", this->update_url.toUtf8());
    this->status = STATUS_DOWNLOADING_UPDATE;

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
        debugLog("UpdateHandler ERROR: downloaded file is too small or failed ({:d} bytes)!\n", data.length());
        this->status = STATUS_ERROR;

        // retry logic
        this->iNumRetries++;
        if(this->iNumRetries < 4) {
            this->checkForUpdates();
        }
        return;
    }

    // write to disk
    debugLog("UpdateHandler: Downloaded file has {:d} bytes, writing ...\n", data.length());
    std::ofstream file(this->updateArchiveName, std::ios::out | std::ios::binary);
    if(file.good()) {
        file.write(data.data(), static_cast<std::streamsize>(data.length()));
        file.close();

        debugLog("UpdateHandler: Update finished successfully.\n");
        this->status = STATUS_DOWNLOAD_COMPLETE;

        // save cache info for future use
        this->saveCacheInfo(this->currentVersionContent);
    } else {
        debugLog("UpdateHandler ERROR: Can't write file!\n");
        this->status = STATUS_ERROR;

        // retry logic
        this->iNumRetries++;
        if(this->iNumRetries < 4) {
            this->checkForUpdates();
        }
    }
}

void UpdateHandler::installUpdate() {
    if(this->getStatus() != STATUS_DOWNLOAD_COMPLETE) {
        debugLog("UpdateHandler: not ready! current status: {}\n", static_cast<int8_t>(this->getStatus()));
        return;
    }
    debugLog("UpdateHandler: installing {:s}\n", this->updateArchiveName);
    this->status = STATUS_INSTALLING_UPDATE;

    if(!env->fileExists(this->updateArchiveName)) {
        debugLog("UpdateHandler ERROR: \"{:s}\" does not exist!\n", this->updateArchiveName);
        this->status = STATUS_ERROR;
        return;
    }

    Archive archive(this->updateArchiveName);
    if(!archive.isValid()) {
        debugLog("UpdateHandler ERROR: couldn't open archive!\n");
        this->status = STATUS_ERROR;
        return;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog("UpdateHandler ERROR: archive is empty!\n");
        this->status = STATUS_ERROR;
        return;
    }

    // separate raw dirs and files
    std::string mainDirectory = "neosu/";
    std::vector<Archive::Entry> files, dirs;
    for(const auto &entry : entries) {
        auto fileName = entry.getFilename();
        if(!fileName.starts_with(mainDirectory)) {
            debugLog("UpdateHandler WARNING: Ignoring \"{:s}\" because it's not in the main dir!\n", fileName.c_str());
            continue;
        }

        if(entry.isDirectory()) {
            dirs.push_back(entry);
        } else {
            files.push_back(entry);
        }

        debugLog("UpdateHandler: Filename: \"{:s}\", isDir: {}, uncompressed size: {}\n", entry.getFilename().c_str(),
                 entry.isDirectory(), entry.getUncompressedSize());
    }

    // repair/create missing/new dirs
    for(const auto &dir : dirs) {
        std::string newDir = dir.getFilename().substr(mainDirectory.length());
        if(newDir.length() == 0) continue;
        if(env->directoryExists(newDir)) continue;

        debugLog("UpdateHandler: Creating directory {:s}\n", newDir.c_str());
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

        debugLog("UpdateHandler: Writing {:s}\n", outFilePath.c_str());
        if(!file.extractToFile(outFilePath)) {
            debugLog("UpdateHandler: Failed to extract file {:s}\n", outFilePath.c_str());
            if(temp_created) {
                env->deleteFile(outFilePath);
                env->renameFile(old_path, outFilePath);
            }
            this->status = STATUS_ERROR;
            return;
        }
    }

    this->status = STATUS_SUCCESS_INSTALLATION;
    env->deleteFile(this->updateArchiveName);
    env->deleteFile(this->updateCacheInfoPath);  // clear cache after successful installation
}

std::string UpdateHandler::calculateUpdateHash(const std::string &versionContent) {
    if(!env->fileExists(this->updateArchiveName)) {
        return "";
    }

    // get sha256 of file
    std::array<u8, 32> file_hash{};
    crypto::hash::sha256_f(this->updateArchiveName.c_str(), file_hash.data());

    if(file_hash == std::array<u8, 32>{}) {
        return "";
    }

    std::string combined_string{versionContent + std::string_view(reinterpret_cast<const char *>(file_hash.data())) +
                                '\0'};

    // get sha256 of (version + archive hash)
    std::array<u8, 32> combined_hash{};
    crypto::hash::sha256(combined_string.data(), combined_string.size(), combined_hash.data());

    std::stringstream ss;
    for(unsigned char i : combined_hash) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(i);
    }

    return ss.str();
}

void UpdateHandler::saveCacheInfo(const std::string &versionContent) {
    std::string hash = this->calculateUpdateHash(versionContent);
    if(hash.empty()) {
        debugLog("UpdateHandler: Failed to calculate hash for cache\n");
        return;
    }

    std::ofstream cacheFile(this->updateCacheInfoPath);
    if(cacheFile.good()) {
        cacheFile << hash;
        cacheFile.close();
        debugLog("UpdateHandler: Saved cache info with hash: {:s}\n", hash.c_str());
    } else {
        debugLog("UpdateHandler: Failed to save cache info\n");
    }
}

bool UpdateHandler::isCachedUpdateValid(const std::string &versionContent) {
    // check if cache file exists
    if(!env->fileExists(this->updateCacheInfoPath)) {
        return false;
    }

    // check if zip file exists
    if(!env->fileExists(this->updateArchiveName)) {
        return false;
    }

    // read cached hash
    std::ifstream cacheFile(this->updateCacheInfoPath);
    if(!cacheFile.good()) {
        return false;
    }

    std::string cachedHash;
    std::getline(cacheFile, cachedHash);
    cacheFile.close();

    if(cachedHash.empty()) {
        return false;
    }

    // calculate current hash
    std::string currentHash = this->calculateUpdateHash(versionContent);
    if(currentHash.empty()) {
        return false;
    }

    const bool isValid = (cachedHash == currentHash);
    debugLog("UpdateHandler: Cache validation - cached: {:s}, current: {:s}, valid: {}\n", cachedHash.c_str(),
             currentHash.c_str(), isValid);

    return isValid;
}
