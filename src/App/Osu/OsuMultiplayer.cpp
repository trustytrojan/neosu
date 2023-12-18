//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

#include "Bancho.h"
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

#include "OsuUISongBrowserInfoLabel.h"

ConVar osu_mp_freemod("osu_mp_freemod", false);
ConVar osu_mp_freemod_all("osu_mp_freemod_all", true, "allow everything, or only standard osu mods");
ConVar osu_mp_win_condition_accuracy("osu_mp_win_condition_accuracy", false);
ConVar osu_mp_allow_client_beatmap_select("osu_mp_sv_allow_client_beatmap_select", true);
ConVar osu_mp_broadcastcommand("osu_mp_broadcastcommand");
ConVar osu_mp_clientcastcommand("osu_mp_clientcastcommand");
ConVar osu_mp_broadcastforceclientbeatmapdownload("osu_mp_broadcastforceclientbeatmapdownload");
ConVar osu_mp_select_beatmap("osu_mp_select_beatmap");
ConVar osu_mp_request_beatmap_download("osu_mp_request_beatmap_download");

unsigned long long OsuMultiplayer::sortHackCounter = 0;
ConVar *OsuMultiplayer::m_cl_cmdrate = NULL;

OsuMultiplayer::OsuMultiplayer(Osu *osu)
{
	m_osu = osu;

	if (m_cl_cmdrate == NULL)
		m_cl_cmdrate = convar->getConVarByName("cl_cmdrate", false);

	// convar callbacks
	osu_mp_broadcastcommand.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onBroadcastCommand) );
	osu_mp_clientcastcommand.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onClientcastCommand) );

	osu_mp_broadcastforceclientbeatmapdownload.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPForceClientBeatmapDownload) );
	osu_mp_select_beatmap.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPSelectBeatmap) );
	osu_mp_request_beatmap_download.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPRequestBeatmapDownload) );

	m_fNextPlayerCmd = 0.0f;

	m_bMPSelectBeatmapScheduled = false;

	m_iLastClientBeatmapSelectID = 0;

	/*
	PLAYER ply;
	ply.name = "Player1";
	ply.missingBeatmap = false;
	ply.score = 1000000;
	ply.accuracy = 0.9123456f;
	ply.combo = 420;
	ply.dead = false;
	m_clientPlayers.push_back(ply);
	ply.name = "Player2";
	ply.missingBeatmap = true;
	ply.combo = 1;
	m_clientPlayers.push_back(ply);
	ply.name = "Player420";
	ply.missingBeatmap = false;
	ply.dead = true;
	ply.combo = 23;
	m_clientPlayers.push_back(ply);
	*/
}

OsuMultiplayer::~OsuMultiplayer()
{
}

void OsuMultiplayer::update()
{
	if (m_bMPSelectBeatmapScheduled)
	{
		// wait for songbrowser db load
		if (m_osu->getSongBrowser()->getDatabase()->isFinished())
		{
			setBeatmap(m_sMPSelectBeatmapScheduledMD5Hash);
			m_bMPSelectBeatmapScheduled = false; // NOTE: resetting flag below to avoid endless loops
		}
	}

	if (!isInMultiplayer()) return;

	// TODO @kiwec: add code below
}

bool OsuMultiplayer::onClientReceiveInt(uint32_t id, void *data, uint32_t size, bool forceAcceptOnServer)
{
	const int wrapperSize = sizeof(PACKET_TYPE);
	if (size <= wrapperSize) return false;

	// unwrap the packet
	char *unwrappedPacket = (char*)data;
	unwrappedPacket += wrapperSize;
	const uint32_t unwrappedSize = (size - wrapperSize);

	const PACKET_TYPE type = (*(PACKET_TYPE*)data);
	switch (type)
	{
	case PLAYER_CHANGE_TYPE:
		if (unwrappedSize >= sizeof(PLAYER_CHANGE_PACKET))
		{
			// execute
			PLAYER_CHANGE_PACKET *pp = (struct PLAYER_CHANGE_PACKET*)unwrappedPacket;
			bool exists = false;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					exists = true;
					if (!pp->connected) // player disconnect
					{
						m_clientPlayers.erase(m_clientPlayers.begin() + i);

						debugLog("Player %i left the game.\n", pp->id);
					}
					else // player update
					{
					}
					break;
				}
			}

			if (!exists) // player connect
			{
				PLAYER ply;
				ply.id = pp->id;
				ply.name = UString(pp->name).substr(0, pp->size);

				ply.missingBeatmap = false;
				ply.downloadingBeatmap = false;
				ply.waiting = true;

				ply.combo = 0;
				ply.accuracy = 0.0f;
				ply.score = 0;
				ply.dead = false;

				ply.input.mouse1down = false;
				ply.input.mouse2down = false;
				ply.input.key1down = false;
				ply.input.key2down = false;

				ply.sortHack = sortHackCounter++;

				m_clientPlayers.push_back(ply);

				debugLog("Player %i joined the game.\n", pp->id);
			}
		}
		return true;

	case PLAYER_STATE_TYPE:
		if (unwrappedSize >= sizeof(PLAYER_STATE_PACKET))
		{
			// execute
			PLAYER_STATE_PACKET *pp = (struct PLAYER_STATE_PACKET*)unwrappedPacket;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					// player state update
					m_clientPlayers[i].missingBeatmap = pp->missingBeatmap;
					m_clientPlayers[i].downloadingBeatmap = pp->downloadingBeatmap;
					m_clientPlayers[i].waiting = pp->waiting;
					break;
				}
			}
		}
		return true;

	case CONVAR_TYPE:
		if (unwrappedSize >= sizeof(CONVAR_PACKET))
		{
			// execute
			CONVAR_PACKET *pp = (struct CONVAR_PACKET*)unwrappedPacket;
			Console::processCommand(UString(pp->str).substr(0, pp->len));
		}
		return true;

	case STATE_TYPE:
		if (unwrappedSize >= sizeof(GAME_STATE_PACKET))
		{
			// execute
			GAME_STATE_PACKET *pp = (struct GAME_STATE_PACKET*)unwrappedPacket;
			if (pp->state == SELECT)
			{
				bool found = false;
				const std::vector<OsuDatabaseBeatmap*> &beatmaps = m_osu->getSongBrowser()->getDatabase()->getDatabaseBeatmaps();
				for (int i=0; i<beatmaps.size(); i++)
				{
					OsuDatabaseBeatmap *beatmap = beatmaps[i];
					const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
					for (int d=0; d<diffs.size(); d++)
					{
						OsuDatabaseBeatmap *diff = diffs[d];
						bool uuidMatches = (diff->getMD5Hash().length() > 0);
						for (int u=0; u<32 && u<diff->getMD5Hash().length(); u++)
						{
							if (diff->getMD5Hash()[u] != pp->beatmapMD5Hash[u])
							{
								uuidMatches = false;
								break;
							}
						}

						if (uuidMatches)
						{
							found = true;
							m_osu->getSongBrowser()->selectBeatmapMP(diff);
							break;
						}
					}

					if (found)
						break;
				}

				if (!found)
				{
					m_sMPSelectBeatmapScheduledMD5Hash = std::string(pp->beatmapMD5Hash, sizeof(pp->beatmapMD5Hash));

					if (beatmaps.size() < 1)
					{
						if (m_osu->getMainMenu()->isVisible() && !m_bMPSelectBeatmapScheduled)
						{
							m_bMPSelectBeatmapScheduled = true;

							m_osu->toggleSongBrowser();
						}
						else
							m_osu->getNotificationOverlay()->addNotification("Database not yet loaded ...", 0xffffff00);
					}
					else
						m_osu->getNotificationOverlay()->addNotification((pp->beatmapId > 0 ? "Missing beatmap! -> ^^^ Click top left [Web] ^^^" : "Missing Beatmap!"), 0xffff0000);

					m_osu->getSongBrowser()->getInfoLabel()->setFromMissingBeatmap(pp->beatmapId);
				}

				// send status update to everyone
				onClientStatusUpdate(!found);
			}
			else if (m_osu->getSelectedBeatmap() != NULL)
			{
				if (pp->state == START)
				{
					if (m_osu->isInPlayMode())
						m_osu->getSelectedBeatmap()->stop(true);

					// only start if we actually have the correct beatmap
					OsuDatabaseBeatmap *diff2 = m_osu->getSelectedBeatmap()->getSelectedDifficulty2();
					if (diff2 != NULL)
					{
						bool uuidMatches = (diff2->getMD5Hash().length() > 0);
						for (int u=0; u<32 && u<diff2->getMD5Hash().length(); u++)
						{
							if (diff2->getMD5Hash()[u] != pp->beatmapMD5Hash[u])
							{
								uuidMatches = false;
								break;
							}
						}

						if (uuidMatches)
							m_osu->getSongBrowser()->onDifficultySelectedMP(m_osu->getSelectedBeatmap()->getSelectedDifficulty2(), true);
					}
				}
				else if (m_osu->isInPlayMode())
				{
					if (pp->state == SEEK)
						m_osu->getSelectedBeatmap()->seekMS(pp->seekMS);
					else if (pp->state == SKIP)
						m_osu->getSelectedBeatmap()->skipEmptySection();
					else if (pp->state == PAUSE)
						m_osu->getSelectedBeatmap()->pause(false);
					else if (pp->state == UNPAUSE)
						m_osu->getSelectedBeatmap()->pause(false);
					else if (pp->state == RESTART)
						m_osu->getSelectedBeatmap()->restart(pp->quickRestart);
					else if (pp->state == STOP)
						m_osu->getSelectedBeatmap()->stop();
				}
			}
		}
		return true;

	case SCORE_TYPE:
		if (unwrappedSize >= sizeof(SCORE_PACKET))
		{
			// execute
			SCORE_PACKET *pp = (struct SCORE_PACKET*)unwrappedPacket;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					m_clientPlayers[i].combo = pp->combo;
					m_clientPlayers[i].accuracy = pp->accuracy;
					m_clientPlayers[i].score = pp->score;
					m_clientPlayers[i].dead = pp->dead;
					break;
				}
			}

			// sort players
			struct PlayerSortComparator
			{
			    bool operator() (PLAYER const &a, PLAYER const &b) const
			    {
			    	// strict weak ordering!
			    	if (osu_mp_win_condition_accuracy.getBool())
			    	{
			    		if (a.accuracy == b.accuracy)
			    		{
			    			if (a.dead == b.dead)
			    				return (a.sortHack > b.sortHack);
			    			else
			    				return b.dead;
			    		}
			    		else
			    		{
			    			if (a.dead == b.dead)
			    				return (a.accuracy > b.accuracy);
			    			else
			    				return b.dead;
			    		}
			    	}
			    	else
			    	{
			    		if (a.score == b.score)
			    		{
			    			if (a.dead == b.dead)
			    				return (a.sortHack > b.sortHack);
			    			else
			    				return b.dead;
			    		}
			    		else
			    		{
			    			if (a.dead == b.dead)
			    				return (a.score > b.score);
			    			else
			    				return b.dead;
			    		}
			    	}
			    }
			};
			std::sort(m_clientPlayers.begin(), m_clientPlayers.end(), PlayerSortComparator());
		}
		return true;

	default:
		return false;
	}
}

void OsuMultiplayer::onClientConnectedToServer()
{
	m_osu->getNotificationOverlay()->addNotification("Connected.", 0xff00ff00);

	// force send current score state to server (e.g. if the server died, or client connection died, this can recover a match result)
	onClientScoreChange(m_osu->getScore()->getComboMax(), m_osu->getScore()->getAccuracy(), m_osu->getScore()->getScore(), m_osu->getScore()->isDead(), true);
}

void OsuMultiplayer::onClientDisconnectedFromServer()
{
	m_osu->getNotificationOverlay()->addNotification("Disconnected.", 0xffff0000);

	m_clientPlayers.clear();

	// kill all downloads and uploads
	m_downloads.clear();
}

void OsuMultiplayer::onClientStatusUpdate(bool missingBeatmap, bool waiting, bool downloadingBeatmap)
{
	// TODO @kiwec: send status update packet
}

void OsuMultiplayer::onClientScoreChange(int combo, float accuracy, unsigned long long score, bool dead, bool reliable)
{
	// TODO @kiwec: send score change packet
}

bool OsuMultiplayer::onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap)
{
	return false;
}

void OsuMultiplayer::onClientBeatmapDownloadRequest()
{
	// TODO @kiwec: start downloading beatmap
}

void OsuMultiplayer::setBeatmap(OsuDatabaseBeatmap *beatmap)
{
	if (beatmap == NULL) return;
	setBeatmap(beatmap->getMD5Hash());
}

void OsuMultiplayer::setBeatmap(std::string md5hash)
{
	if (md5hash.length() < 32) return;

	GAME_STATE_PACKET pp;
	pp.state = STATE::SELECT;
	pp.seekMS = 0;
	pp.quickRestart = false;
	for (int i=0; i<32 && i<md5hash.length(); i++)
	{
		pp.beatmapMD5Hash[i] = md5hash[i];
	}
	pp.beatmapId = /*beatmap->getSelectedDifficulty()->beatmapId*/0;
	size_t size = sizeof(GAME_STATE_PACKET);

	PACKET_TYPE wrap = STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	onClientReceiveInt(0, wrappedPacket, size + wrapperSize);
}

bool OsuMultiplayer::isInMultiplayer()
{
	return bancho.is_connected;
}

bool OsuMultiplayer::isMissingBeatmap()
{
	for (size_t i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].id == bancho.user_id)
			return m_clientPlayers[i].missingBeatmap;
	}
	return false;
}

bool OsuMultiplayer::isWaitingForPlayers()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].waiting || m_clientPlayers[i].downloadingBeatmap)
			return true;
	}
	return false;
}

bool OsuMultiplayer::isWaitingForClient()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].id == bancho.user_id)
			return (m_clientPlayers[i].waiting || m_clientPlayers[i].downloadingBeatmap);
	}
	return false;
}

float OsuMultiplayer::getDownloadBeatmapPercentage() const
{
	if (!isDownloadingBeatmap() || m_downloads.size() < 1) return 0.0f;

	const size_t totalDownloadedBytes = m_downloads[0].numDownloadedOsuFileBytes + m_downloads[0].numDownloadedMusicFileBytes + m_downloads[0].numDownloadedBackgroundFileBytes;

	return ((float)totalDownloadedBytes / (float)m_downloads[0].totalDownloadBytes);
}

void OsuMultiplayer::onBroadcastCommand(UString command)
{
	onClientCommandInt(command, false);
}

void OsuMultiplayer::onClientcastCommand(UString command)
{
	onClientCommandInt(command, true);
}

void OsuMultiplayer::onClientCommandInt(UString string, bool executeLocallyToo)
{
}

void OsuMultiplayer::onMPForceClientBeatmapDownload()
{
}

void OsuMultiplayer::onMPSelectBeatmap(UString md5hash)
{
	setBeatmap(std::string(md5hash.toUtf8()));
}

void OsuMultiplayer::onMPRequestBeatmapDownload()
{
	onClientBeatmapDownloadRequest();
}

void OsuMultiplayer::onBeatmapDownloadFinished(const BeatmapDownloadState &dl)
{
	// update client state to no-longer-downloading
	onClientStatusUpdate(true, true, false);

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

			// and update our state if we have the correct beatmap now
			if (beatmap->getDifficulties()[0]->getMD5Hash() == m_sMPSelectBeatmapScheduledMD5Hash)
				onClientStatusUpdate(false, true, false);
		}
	}
}
