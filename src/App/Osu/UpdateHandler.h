#pragma once

#include "UString.h"
#include <atomic>

class UpdateHandler {
   public:
    enum class STATUS : uint8_t {
        STATUS_UP_TO_DATE,
        STATUS_CHECKING_FOR_UPDATE,
        STATUS_DOWNLOADING_UPDATE,
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

    [[nodiscard]] inline STATUS getStatus() const { return this->status.load(); }
    UString update_url;

   private:
    static constexpr const char* TEMP_UPDATE_DOWNLOAD_FILEPATH = "update.zip";

    // async operation chain
    void requestUpdate();
    void onVersionCheckComplete(const std::string& response, bool success);
    void downloadUpdate();
    void onDownloadComplete(const std::string& data, bool success);
    void installUpdate(const std::string& zipFilePath);

    // status
    std::atomic<STATUS> status;
    int iNumRetries;
};
