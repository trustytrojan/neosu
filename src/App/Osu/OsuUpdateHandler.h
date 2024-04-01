//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#ifndef OSUUPDATECHECKER_H
#define OSUUPDATECHECKER_H

#include <pthread.h>

#include "cbase.h"

class OsuUpdateHandler {
   public:
    enum class STATUS {
        STATUS_UP_TO_DATE,
        STATUS_CHECKING_FOR_UPDATE,
        STATUS_DOWNLOADING_UPDATE,
        STATUS_INSTALLING_UPDATE,
        STATUS_SUCCESS_INSTALLATION,
        STATUS_ERROR
    };

    static void *run(void *data);

    OsuUpdateHandler();
    virtual ~OsuUpdateHandler();

    void stop();  // tells the update thread to stop at the next cancellation point
    void wait();  // blocks until the update thread is finished

    void checkForUpdates();

    inline STATUS getStatus() const { return m_status; }
    UString update_url;

   private:
    static const char *TEMP_UPDATE_DOWNLOAD_FILEPATH;

    // async
    void _requestUpdate();
    bool _downloadUpdate();
    void _installUpdate(std::string zipFilePath);

    pthread_t m_updateThread;
    bool _m_bKYS;

    // releases
    Environment::OS stringToOS(UString osString);

    // status
    STATUS m_status;
    int m_iNumRetries;
};

#endif
