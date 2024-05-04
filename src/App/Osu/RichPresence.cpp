//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		generalized rich presence handler
//
// $NoKeywords: $rpt
//===============================================================================//

#include "RichPresence.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "DiscordInterface.h"
#include "Engine.h"
#include "Environment.h"
#include "ModSelector.h"
#include "Osu.h"
#include "RoomScreen.h"
#include "SongBrowser.h"
#include "score.h"

ConVar osu_rich_presence("osu_rich_presence", true, FCVAR_NONE, RichPresence::onRichPresenceChange);
ConVar osu_rich_presence_dynamic_windowtitle(
    "osu_rich_presence_dynamic_windowtitle", true, FCVAR_NONE,
    "should the window title show the currently playing beatmap Artist - Title and [Difficulty] name");
ConVar osu_rich_presence_show_recentplaystats("osu_rich_presence_show_recentplaystats", true, FCVAR_NONE);
ConVar osu_rich_presence_discord_show_totalpp("osu_rich_presence_discord_show_totalpp", true, FCVAR_NONE);

ConVar *RichPresence::m_name_ref = NULL;

const UString RichPresence::KEY_STEAM_STATUS = "status";
const UString RichPresence::KEY_DISCORD_STATUS = "state";
const UString RichPresence::KEY_DISCORD_DETAILS = "details";

UString last_status = "[neosu]\nWaking up";
Action last_action = IDLE;

void RichPresence::setBanchoStatus(Osu *osu, const char *info_text, Action action) {
    if(osu == NULL) return;

    MD5Hash map_md5("");
    u32 map_id = 0;

    auto selected_beatmap = osu->getSelectedBeatmap();
    if(selected_beatmap != nullptr) {
        auto diff = selected_beatmap->getSelectedDifficulty2();
        if(diff != nullptr) {
            map_md5 = diff->getMD5Hash();
            map_id = diff->getID();
        }
    }

    char fancy_text[1024] = {0};
    snprintf(fancy_text, 1023, "[neosu]\n%s", info_text);

    last_status = fancy_text;
    last_action = action;

    Packet packet;
    packet.id = CHANGE_ACTION;
    write<u8>(&packet, action);
    write_string(&packet, fancy_text);
    write_string(&packet, map_md5.hash);
    write<u32>(&packet, osu->m_modSelector->getModFlags());
    write<u8>(&packet, 0);  // osu!std
    write<u32>(&packet, map_id);
    send_packet(packet);
}

void RichPresence::updateBanchoMods() {
    MD5Hash map_md5("");
    u32 map_id = 0;

    auto selected_beatmap = bancho.osu->getSelectedBeatmap();
    if(selected_beatmap != nullptr) {
        auto diff = selected_beatmap->getSelectedDifficulty2();
        if(diff != nullptr) {
            map_md5 = diff->getMD5Hash();
            map_id = diff->getID();
        }
    }

    Packet packet;
    packet.id = CHANGE_ACTION;
    write<u8>(&packet, last_action);
    write_string(&packet, last_status.toUtf8());
    write_string(&packet, map_md5.hash);
    write<u32>(&packet, bancho.osu->m_modSelector->getModFlags());
    write<u8>(&packet, 0);  // osu!std
    write<u32>(&packet, map_id);
    send_packet(packet);

    // Servers like akatsuki send different leaderboards based on what mods
    // you have selected. Reset leaderboard when switching mods.
    bancho.osu->m_songBrowser2->m_db->m_online_scores.clear();
    bancho.osu->m_songBrowser2->rebuildScoreButtons();
}

void RichPresence::onMainMenu(Osu *osu) {
    setStatus(osu, "Main Menu");
    setBanchoStatus(osu, "Main Menu", AFK);
}

void RichPresence::onSongBrowser(Osu *osu) {
    setStatus(osu, "Song Selection");

    if(osu->m_room->isVisible()) {
        setBanchoStatus(osu, "Picking a map", MULTIPLAYER);
    } else {
        setBanchoStatus(osu, "Song selection", IDLE);
    }

    // also update window title
    if(osu_rich_presence_dynamic_windowtitle.getBool()) env->setWindowTitle("neosu");
}

void RichPresence::onPlayStart(Osu *osu) {
    UString playingInfo /*= "Playing "*/;
    playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getArtist().c_str());
    playingInfo.append(" - ");
    playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getTitle().c_str());
    playingInfo.append(" [");
    playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getDifficultyName().c_str());
    playingInfo.append("]");

    setStatus(osu, playingInfo);
    setBanchoStatus(osu, playingInfo.toUtf8(), bancho.is_in_a_multi_room() ? MULTIPLAYER : PLAYING);

    // also update window title
    if(osu_rich_presence_dynamic_windowtitle.getBool()) {
        UString windowTitle = UString(playingInfo);
        windowTitle.insert(0, "neosu - ");
        env->setWindowTitle(windowTitle);
    }
}

void RichPresence::onPlayEnd(Osu *osu, bool quit) {
    if(!quit && osu_rich_presence_show_recentplaystats.getBool()) {
        // e.g.: 230pp 900x 95.50% HDHRDT 6*

        // pp
        UString scoreInfo = UString::format("%ipp", (int)(std::round(osu->getScore()->getPPv2())));

        // max combo
        scoreInfo.append(UString::format(" %ix", osu->getScore()->getComboMax()));

        // accuracy
        scoreInfo.append(UString::format(" %.2f%%", osu->getScore()->getAccuracy() * 100.0f));

        // mods
        UString mods = osu->getScore()->getModsStringForRichPresence();
        if(mods.length() > 0) {
            scoreInfo.append(" ");
            scoreInfo.append(mods);
        }

        // stars
        scoreInfo.append(UString::format(" %.2f*", osu->getScore()->getStarsTomTotal()));

        setStatus(osu, scoreInfo);
        setBanchoStatus(osu, scoreInfo.toUtf8(), SUBMITTING);
    }
}

void RichPresence::setStatus(Osu *osu, UString status, bool force) {
    if(!osu_rich_presence.getBool() && !force) return;

    // discord
    discord->setRichPresence("largeImageKey", "logo_512", true);
    discord->setRichPresence("smallImageKey", "logo_discord_512_blackfill", true);
    discord->setRichPresence("largeImageText",
                             osu_rich_presence_discord_show_totalpp.getBool()
                                 ? "Top = Status / Recent Play; Bottom = Total weighted pp (neosu scores only!)"
                                 : "",
                             true);
    discord->setRichPresence("smallImageText",
                             osu_rich_presence_discord_show_totalpp.getBool()
                                 ? "Total weighted pp only work after the database has been loaded!"
                                 : "",
                             true);
    discord->setRichPresence(KEY_DISCORD_DETAILS, status);

    if(osu != NULL) {
        if(osu_rich_presence_discord_show_totalpp.getBool()) {
            if(m_name_ref == NULL) m_name_ref = convar->getConVarByName("name");

            const int ppRounded = (int)(std::round(
                osu->getSongBrowser()->getDatabase()->calculatePlayerStats(m_name_ref->getString()).pp));
            if(ppRounded > 0) discord->setRichPresence(KEY_DISCORD_STATUS, UString::format("%ipp (Mc)", ppRounded));
        }
    } else if(force && status.length() < 1)
        discord->setRichPresence(KEY_DISCORD_STATUS, "");
}

void RichPresence::onRichPresenceChange(UString oldValue, UString newValue) {
    if(!osu_rich_presence.getBool())
        onRichPresenceDisable();
    else
        onRichPresenceEnable();
}

void RichPresence::onRichPresenceEnable() { setStatus(NULL, "..."); }

void RichPresence::onRichPresenceDisable() { setStatus(NULL, "", true); }
