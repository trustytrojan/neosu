#pragma once

#include "UString.h"
#include <atomic>

class UpdateHandler {
   public:
    enum class STATUS : uint8_t {
        STATUS_UP_TO_DATE,
        STATUS_CHECKING_FOR_UPDATE,
        STATUS_DOWNLOADING_UPDATE,
        STATUS_DOWNLOAD_COMPLETE,
        STATUS_INSTALLING_UPDATE,
        STATUS_SUCCESS_INSTALLATION,
        STATUS_ERROR
    };

    UpdateHandler();
    ~UpdateHandler() = default;

    UpdateHandler(const UpdateHandler&) = delete;
    UpdateHandler& operator=(const UpdateHandler&) = delete;
    UpdateHandler(UpdateHandler&&) = delete;
    UpdateHandler& operator=(UpdateHandler&&) = delete;

    void checkForUpdates();
    void installUpdate();

    [[nodiscard]] inline STATUS getStatus() const { return this->status.load(); }
    UString update_url;

   private:

    // async operation chain
    void onVersionCheckComplete(const std::string& response, bool success);
    void downloadUpdate();
    void onDownloadComplete(const std::string& data, bool success);

    // cache management
    void saveCacheInfo(const std::string& versionContent);
    bool isCachedUpdateValid(const std::string& versionContent);
    std::string calculateUpdateHash(const std::string& versionContent);

    // release stream management
    void onBleedingEdgeChanged(float oldVal, float newVal);

    // status
    std::atomic<STATUS> status;
    int iNumRetries;
    std::string updateArchiveName;
    std::string updateCacheInfoPath;
    std::string currentVersionContent;  // stored for cache operations
};
