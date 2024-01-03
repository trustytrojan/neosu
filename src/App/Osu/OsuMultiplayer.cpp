//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

// Let's commit the full TODO list, why not?
// TODO @kiwec: test OsuRoom
// TODO @kiwec: logout on quit. needs a rework of the networking logic. disconnect != logout
// TODO @kiwec: show loading while logging in
// TODO @kiwec: add "personal best" in online score list
// TODO @kiwec: hardcode server list with capabilities (eg, does the server allow score submission)
//              or, fetch it when starting the game, just like it checks for updates
// TODO @kiwec: enable/disable some features based on what headers the servers sends back on login
// TODO @kiwec: fetch avatars and display in leaderboards, score browser, lobby list, etc
// TODO @kiwec: once logged in, gray out user/pw/server fields and switch log in button to log out button
// TODO @kiwec: comb over every single option, and every single convar and make sure no cheats are possible in multiplayer
// TODO @kiwec: make separate convar for toggling scoreboard in mp (ppy client sucks for removing this!!!)
// TODO @kiwec: reviving in multi is a mystery. how do the clients know when somebody revived? is it just health > 0?
// TODO @kiwec: what's the flow for a match ending because all the players in a team died?
// TODO @kiwec: make webpage for https://mcosu.kiwec.net/

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "OsuMultiplayer.h"

#include "Engine.h"
#include "ConVar.h"
#include "Console.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "File.h"

#include "Osu.h"
#include "OsuDatabase.h"
#include "OsuDatabaseBeatmap.h"

#include "OsuBeatmap.h"

#include "OsuMainMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"

unsigned long long OsuMultiplayer::sortHackCounter = 0;

OsuMultiplayer::OsuMultiplayer(Osu *osu)
{
	m_osu = osu;
	bancho.osu = osu;
}

OsuMultiplayer::~OsuMultiplayer()
{
}

// TODO @kiwec: on map select, make sure we check
// m_osu->getSongBrowser()->getDatabase()->isFinished()
// before we call setBeatmap()

void OsuMultiplayer::update() {
	receive_api_responses();
	receive_bancho_packets();
}

void OsuMultiplayer::onClientScoreChange()
{
	if(!bancho.is_playing_a_multi_map()) return;

	Packet packet = {0};
	packet.id = UPDATE_MATCH_SCORE;

	write_int(&packet, (int32_t)m_osu->getSelectedBeatmap()->getTime());

	uint8_t slot_id = 0;
	for(uint8_t i = 0; i < 16; i++) {
		if(bancho.room.slots[i].player_id == bancho.user_id) {
			slot_id = i;
			break;
		}
	}
	write_byte(&packet, slot_id);

	write_short(&packet, (uint16_t)m_osu->getScore()->getNum300s());
	write_short(&packet, (uint16_t)m_osu->getScore()->getNum100s());
	write_short(&packet, (uint16_t)m_osu->getScore()->getNum50s());
	write_short(&packet, (uint16_t)m_osu->getScore()->getNum300gs());
	write_short(&packet, (uint16_t)m_osu->getScore()->getNum100ks());
	write_short(&packet, (uint16_t)m_osu->getScore()->getNumMisses());
	write_int(&packet, (int32_t)m_osu->getScore()->getScore());
	write_short(&packet, (uint16_t)m_osu->getScore()->getCombo());
	write_short(&packet, (uint16_t)m_osu->getScore()->getComboMax());
	write_byte(&packet, m_osu->getScore()->getNumSliderBreaks() == 0 && m_osu->getScore()->getNumMisses() == 0); // TODO @kiwec: is this correct?
	write_byte(&packet, m_osu->getSelectedBeatmap()->getHealth() * 100); // TODO @kiwec: currently doing 0-100, might be 0-255
	write_byte(&packet, 0); // 4P, not supported
	write_byte(&packet, m_osu->getModScorev2());
	send_packet(packet);
}

bool OsuMultiplayer::onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap)
{
	return false;
}

void OsuMultiplayer::setBeatmap(OsuDatabaseBeatmap *beatmap)
{
	if (beatmap == NULL) return;
	setBeatmap(beatmap->getMD5Hash());
}

void OsuMultiplayer::setBeatmap(std::string md5hash)
{
	if (md5hash.length() < 32) return;

	// TODO @kiwec
}

bool OsuMultiplayer::isWaitingForPlayers() {
	return !bancho.room.all_players_loaded;
}

void OsuMultiplayer::onBeatmapDownloadFinished(const BeatmapDownloadState &dl)
{
	// TODO: validate inputs

	// create folder structure
	UString beatmapFolderPath = "tmp/";
	{
		if (!env->directoryExists(beatmapFolderPath))
			env->createDirectory(beatmapFolderPath);

		beatmapFolderPath.append(dl.osuFileMD5Hash);
		beatmapFolderPath.append("/");
		if (!env->directoryExists(beatmapFolderPath))
			env->createDirectory(beatmapFolderPath);
	}

	// write downloaded files to disk
	{
		UString osuFilePath = beatmapFolderPath;
		osuFilePath.append(dl.osuFileMD5Hash);
		osuFilePath.append(".osu");

		if (!env->fileExists(osuFilePath))
		{
			File osuFile(osuFilePath, File::TYPE::WRITE);
			osuFile.write((const char*)&dl.osuFileBytes[0], dl.osuFileBytes.size());
		}
	}
	{
		UString musicFilePath = beatmapFolderPath;
		musicFilePath.append(dl.musicFileName);

		if (!env->fileExists(musicFilePath))
		{
			File musicFile(musicFilePath, File::TYPE::WRITE);
			musicFile.write((const char*)&dl.musicFileBytes[0], dl.musicFileBytes.size());
		}
	}
	if (dl.backgroundFileName.length() > 1)
	{
		UString backgroundFilePath = beatmapFolderPath;
		backgroundFilePath.append(dl.backgroundFileName);

		if (!env->fileExists(backgroundFilePath))
		{
			File backgroundFile(backgroundFilePath, File::TYPE::WRITE);
			backgroundFile.write((const char*)&dl.backgroundFileBytes[0], dl.backgroundFileBytes.size());
		}
	}

	// and load the beatmap
	OsuDatabaseBeatmap *beatmap = m_osu->getSongBrowser()->getDatabase()->addBeatmap(beatmapFolderPath);
	if (beatmap != NULL)
	{
		m_osu->getSongBrowser()->addBeatmap(beatmap);
		m_osu->getSongBrowser()->updateSongButtonSorting();
		if (beatmap->getDifficulties().size() > 0) // NOTE: always assume only 1 diff exists for now
		{
			m_osu->getSongBrowser()->selectBeatmapMP(beatmap->getDifficulties()[0]);
			// TODO @kiwec: check for beatmap md5, if it's the room's, update
		}
	}
}
