#pragma once

#include "UString.h"
#include <atomic>

class UpdateHandler {
   public:
    enum class STATUS : int8_t {
        STATUS_IDLE,
        STATUS_CHECKING_FOR_UPDATE,
        STATUS_DOWNLOADING_UPDATE,
        STATUS_DOWNLOAD_COMPLETE,
        STATUS_ERROR,
    };

    UpdateHandler() = default;
    ~UpdateHandler() = default;

    UpdateHandler(const UpdateHandler&) = delete;
    UpdateHandler& operator=(const UpdateHandler&) = delete;
    UpdateHandler(UpdateHandler&&) = delete;
    UpdateHandler& operator=(UpdateHandler&&) = delete;

    void checkForUpdates(bool force_update);
    void installUpdate();

    [[nodiscard]] inline STATUS getStatus() const { return this->status.load(); }

    // release stream management
    void onBleedingEdgeChanged(float oldVal, float newVal);

   private:
    // async operation chain
    void onVersionCheckComplete(const std::string& response, bool success, bool force_update);
    void onDownloadComplete(const std::string& data, bool success, std::string hash);

    // status
    std::atomic<STATUS> status = STATUS::STATUS_IDLE;
};
