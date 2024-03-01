//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#ifndef OSUUPDATECHECKER_H
#define OSUUPDATECHECKER_H

#include "cbase.h"

#ifdef MCENGINE_FEATURE_PTHREADS

#include <pthread.h>

#endif

class OsuUpdateHandler
{
public:
	enum class STATUS
	{
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

	void stop(); // tells the update thread to stop at the next cancellation point
	void wait(); // blocks until the update thread is finished

	void checkForUpdates();

	inline STATUS getStatus() const {return m_status;}
	UString update_url;

private:
	static const char *TEMP_UPDATE_DOWNLOAD_FILEPATH;
	static ConVar *m_osu_release_stream_ref;

	// async
	void _requestUpdate();
	bool _downloadUpdate();
	void _installUpdate(std::string zipFilePath);

#ifdef MCENGINE_FEATURE_PTHREADS

	pthread_t m_updateThread;

#endif

	bool _m_bKYS;

	// releases
	enum class STREAM
	{
		STREAM_NULL,
		STREAM_DESKTOP,
		STREAM_VR
	};

	STREAM stringToStream(UString streamString);
	Environment::OS stringToOS(UString osString);
	STREAM getReleaseStream();

	// status
	STATUS m_status;
	int m_iNumRetries;
};

#endif
