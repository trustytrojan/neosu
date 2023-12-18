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
	enum STATE
	{
		SELECT,
		START,
		SEEK,
		SKIP,
		PAUSE,
		UNPAUSE,
		RESTART,
		STOP
	};

	struct PLAYER_INPUT
	{
		long time;
		Vector2 cursorPos;
		bool mouse1down;
		bool mouse2down;
		bool key1down;
		bool key2down;
	};

	struct PLAYER
	{
		unsigned int id;
		UString name;

		// state
		bool missingBeatmap;
		bool downloadingBeatmap;
		bool waiting;

		// score
		int combo;
		float accuracy;
		unsigned long long score;
		bool dead;

		// input
		PLAYER_INPUT input;
		std::vector<PLAYER_INPUT> inputBuffer;

		unsigned long long sortHack;
	};

	OsuMultiplayer(Osu *osu);
	~OsuMultiplayer();

	void update();

	// receive
	bool onClientReceiveInt(uint32_t id, void *data, uint32_t size, bool forceAcceptOnServer = false);

	// client events
	void onClientConnectedToServer();
	void onClientDisconnectedFromServer();

	// clientside game events
	void onClientStatusUpdate(bool missingBeatmap, bool waiting = true, bool downloadingBeatmap = false);
	void onClientScoreChange(int combo, float accuracy, unsigned long long score, bool dead, bool reliable = false);
	bool onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap);
	void onClientBeatmapDownloadRequest();

	// tourney events
	void setBeatmap(OsuDatabaseBeatmap *beatmap);
	void setBeatmap(std::string md5hash);

	bool isInMultiplayer();

	bool isMissingBeatmap(); // are we missing the serverside beatmap
	bool isWaitingForPlayers();	// are we waiting for any player
	bool isWaitingForClient();	// is the waiting state set for the local player

	inline bool isDownloadingBeatmap() const {return m_downloads.size() > 0;}

	inline std::vector<PLAYER> *getPlayers() {return &m_clientPlayers;}

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
	static ConVar *m_cl_cmdrate;

	void onBeatmapDownloadFinished(const BeatmapDownloadState &dl);

private:
	enum PACKET_TYPE
	{
		PLAYER_CHANGE_TYPE,
		PLAYER_STATE_TYPE,
		CONVAR_TYPE,
		STATE_TYPE,
		SCORE_TYPE,
	};

#pragma pack(1)

	struct PLAYER_CHANGE_PACKET
	{
		uint32_t id;
		bool connected;
		size_t size;
		wchar_t name[255];
	};

	struct PLAYER_STATE_PACKET
	{
		uint32_t id;
		bool missingBeatmap;	// this is only used visually
		bool downloadingBeatmap;// this is also only used visually
		bool waiting;			// this will block all players until everyone is ready
	};

	struct CONVAR_PACKET
	{
		wchar_t str[2048];
		size_t len;
	};

	struct GAME_STATE_PACKET
	{
		STATE state;
		unsigned long seekMS;
		bool quickRestart;
		char beatmapMD5Hash[32];
		long beatmapId;
	};

	struct SCORE_PACKET
	{
		uint32_t id;
		int32_t combo;
		float accuracy;
		uint64_t score;
		bool dead;
	};

#pragma pack()

private:
	Osu *m_osu;
	std::vector<PLAYER> m_clientPlayers;

	float m_fNextPlayerCmd;

	bool m_bMPSelectBeatmapScheduled;
	std::string m_sMPSelectBeatmapScheduledMD5Hash;

	std::vector<BeatmapDownloadState> m_downloads;

	unsigned int m_iLastClientBeatmapSelectID;
};

#endif
