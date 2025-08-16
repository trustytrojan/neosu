// Copyright (c) 2018, PG, All rights reserved.
#include "DiscordInterface.h"

static bool initialized = false;

#ifdef MCENGINE_PLATFORM_WINDOWS

#include "Bancho.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "Osu.h"
#include "Sound.h"

#define DISCORD_CLIENT_ID 1288141291686989846

namespace  // static
{
struct Application {
    struct IDiscordCore *core;
    struct IDiscordUserManager *users;
    struct IDiscordAchievementManager *achievements;
    struct IDiscordActivityManager *activities;
    struct IDiscordRelationshipManager *relationships;
    struct IDiscordApplicationManager *application;
    struct IDiscordLobbyManager *lobbies;
    DiscordUserId user_id;
} dapp{};

struct IDiscordActivityEvents activities_events {};
struct IDiscordRelationshipEvents relationships_events {};
struct IDiscordUserEvents users_events {};

#ifdef _WIN64
void on_discord_log(void * /*cdata*/, enum EDiscordLogLevel level, const char *message) {
    //(void)cdata;
    if(level == DiscordLogLevel_Error) {
        Engine::logRaw("[Discord] ERROR: {:s}\n", message);
    } else {
        Engine::logRaw("[Discord] {:s}\n", message);
    }
}
#endif

}  // namespace
#endif

void init_discord_sdk() {
#ifdef _WIN32
    memset(&dapp, 0, sizeof(dapp));
    memset(&activities_events, 0, sizeof(activities_events));
    memset(&relationships_events, 0, sizeof(relationships_events));
    memset(&users_events, 0, sizeof(users_events));

    // users_events.on_current_user_update = OnUserUpdated;
    // relationships_events.on_refresh = OnRelationshipsRefresh;

    struct DiscordCreateParams params {};
    params.client_id = DISCORD_CLIENT_ID;
    params.flags = DiscordCreateFlags_NoRequireDiscord;
    params.event_data = &dapp;
    params.activity_events = &activities_events;
    params.relationship_events = &relationships_events;
    params.user_events = &users_events;

    int res = DiscordCreate(DISCORD_VERSION, &params, &dapp.core);
    if(res != DiscordResult_Ok) {
        debugLog("Failed to initialize Discord SDK! (error {:d})\n", res);
        return;
    }

#ifdef _WIN64
    dapp.core->set_log_hook(dapp.core, DiscordLogLevel_Warn, NULL, on_discord_log);
#endif
    dapp.activities = dapp.core->get_activity_manager(dapp.core);

    dapp.activities->register_command(dapp.activities, "neosu://run");

    // dapp.users = dapp.core->get_user_manager(dapp.core);
    // dapp.achievements = dapp.core->get_achievement_manager(dapp.core);
    // dapp.application = dapp.core->get_application_manager(dapp.core);
    // dapp.lobbies = dapp.core->get_lobby_manager(dapp.core);
    // dapp.lobbies->connect_lobby_with_activity_secret(dapp.lobbies, "invalid_secret", &app, OnLobbyConnect);
    // dapp.application->get_oauth2_token(dapp.application, &app, OnOAuth2Token);
    // dapp.relationships = dapp.core->get_relationship_manager(dapp.core);

    initialized = true;
#else
    // not enabled on linux cuz the sdk is broken there
#endif
}

void tick_discord_sdk() {
    if(!initialized) return;

#ifdef _WIN32
    dapp.core->run_callbacks(dapp.core);
#else
        // not enabled on linux cuz the sdk is broken there
#endif
}

void destroy_discord_sdk() {
    // not doing anything because it will fucking CRASH if you close discord first
}

void clear_discord_presence() {
    if(!initialized) return;

#ifdef _WIN32
    // TODO @kiwec: test if this works
    struct DiscordActivity activity {};
    memset(&activity, 0, sizeof(activity));
    dapp.activities->update_activity(dapp.activities, &activity, NULL, NULL);
#else
        // not enabled on linux cuz the sdk is broken there
#endif
}

void set_discord_presence([[maybe_unused]] struct DiscordActivity *activity) {
    if(!initialized) return;

#ifdef _WIN32
    if(!cv::rich_presence.getBool()) return;

    // activity->type: int
    //     DiscordActivityType_Playing,
    //     DiscordActivityType_Streaming,
    //     DiscordActivityType_Listening,
    //     DiscordActivityType_Watching,

    // activity->details: char[128]; // what the player is doing
    // activity->state:   char[128]; // party status

    // Only set "end" if in multiplayer lobby, else it doesn't make sense since user can pause
    // Keep "start" across retries
    // activity->timestamps->start: int64_t
    // activity->timestamps->end:   int64_t

    // activity->party: DiscordActivityParty
    // activity->secrets: DiscordActivitySecrets
    // currently unused. should be lobby id, etc

    activity->application_id = DISCORD_CLIENT_ID;
    strcpy(&activity->name[0], "neosu");
    strcpy(&activity->assets.large_image[0], "neosu_icon");
    activity->assets.large_text[0] = '\0';
    strcpy(&activity->assets.small_image[0], "None");
    activity->assets.small_text[0] = '\0';

    auto diff2 = osu->getSelectedBeatmap()->getSelectedDifficulty2();
    auto music = osu->getSelectedBeatmap()->getMusic();
    bool listening = diff2 != NULL && music != NULL && music->isPlaying();
    bool playing = diff2 != NULL && osu->isInPlayMode();
    if(listening || playing) {
        auto url = UString::format("https://assets.ppy.sh/beatmaps/%d/covers/list@2x.jpg?%d", diff2->getSetID(),
                                   diff2->getID());
        strncpy(&activity->assets.large_image[0], url.toUtf8(), 127);

        if(bancho->server_icon_url.length() > 0 && cv::main_menu_use_server_logo.getBool()) {
            strncpy(&activity->assets.small_image[0], bancho->server_icon_url.toUtf8(), 127);
            strncpy(&activity->assets.small_text[0], bancho->endpoint.c_str(), 127);
        } else {
            strcpy(&activity->assets.small_image[0], "neosu_icon");
            activity->assets.small_text[0] = '\0';
        }
    }

    dapp.activities->update_activity(dapp.activities, activity, NULL, NULL);
#else
        // not enabled on linux cuz the sdk is broken there
#endif
}

// void (DISCORD_API *send_request_reply)(struct IDiscordActivityManager* manager, DiscordUserId user_id, enum
//     EDiscordActivityJoinRequestReply reply, void* callback_data, void (DISCORD_API *callback)(void* callback_data,
//     enum EDiscordResult result));

// void (DISCORD_API *send_invite)(struct IDiscordActivityManager* manager, DiscordUserId user_id, enum
//     EDiscordActivityActionType type, const char* content, void* callback_data, void (DISCORD_API *callback)(void*
//     callback_data, enum EDiscordResult result));

// void (DISCORD_API *accept_invite)(struct IDiscordActivityManager* manager, DiscordUserId user_id, void*
//     callback_data, void (DISCORD_API *callback)(void* callback_data, enum EDiscordResult result));
