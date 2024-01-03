//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

#ifndef OSUMULTIPLAYER_H
#define OSUMULTIPLAYER_H

#include "cbase.h"

class File;

class Osu;
class OsuDatabaseBeatmap;

class OsuMultiplayer
{
public:
	OsuMultiplayer(Osu *osu);
	~OsuMultiplayer();

	void update();

	// clientside game events
	void onClientStatusUpdate(bool missingBeatmap, bool waiting = true, bool downloadingBeatmap = false);
	void onClientScoreChange();
	bool onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap);

	// tourney events
	void setBeatmap(OsuDatabaseBeatmap *beatmap);
	void setBeatmap(std::string md5hash);

	bool isInMultiplayer();

	bool isMissingBeatmap(); // are we missing the serverside beatmap
	bool isWaitingForPlayers();	// are we waiting for any player
	bool isWaitingForClient();	// is the waiting state set for the local player

	inline bool isDownloadingBeatmap() const {return false;} // TODO @kiwec

	float getDownloadBeatmapPercentage() const;

private:
	struct BeatmapDownloadState
	{
		uint32_t serial;

		size_t totalDownloadBytes;

		size_t numDownloadedOsuFileBytes;
		size_t numDownloadedMusicFileBytes;
		size_t numDownloadedBackgroundFileBytes;

		UString osuFileMD5Hash;
		UString musicFileName;
		UString backgroundFileName;

		std::vector<char> osuFileBytes;
		std::vector<char> musicFileBytes;
		std::vector<char> backgroundFileBytes;
	};

private:
	static unsigned long long sortHackCounter;

	void onBeatmapDownloadFinished(const BeatmapDownloadState &dl);

private:
	Osu *m_osu;
};

#endif
