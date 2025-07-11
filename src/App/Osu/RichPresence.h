#pragma once
#include "BanchoProtocol.h"
#include "cbase.h"

class ConVar;

class RichPresence {
   public:
    static void onMainMenu();
    static void onSongBrowser();
    static void onPlayStart();
    static void onPlayEnd(bool quit);
    static void onMultiplayerLobby();

    static void onRichPresenceChange(const UString &oldValue, const UString &newValue);
    static void setBanchoStatus(const char *info_text, Action action);
    static void updateBanchoMods();

   private:
    static const UString KEY_DISCORD_STATUS;
    static const UString KEY_DISCORD_DETAILS;
};
