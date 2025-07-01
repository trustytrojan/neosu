#pragma once

// Windows headers shit
#include "cbase.h"

#ifdef MCENGINE_PLATFORM_WINDOWS
#pragma pack(push, 8)
#include "discord_game_sdk.h"
#pragma pack(pop)
#else
enum DiscordActivityType : uint8_t { DiscordActivityType_Listening, DiscordActivityType_Playing };
struct DiscordActivity {
    struct {
        struct {
            int current_size{0};
            int max_size{0};
        } size{};
    } party{};
    struct {
        int start{0};
        int end{0};
    } timestamps{};
    DiscordActivityType type{};
    char details[512]{};
    char state[512]{};
};
#endif

void init_discord_sdk();
void tick_discord_sdk();
void destroy_discord_sdk();
void clear_discord_presence();
void set_discord_presence(struct DiscordActivity *activity);
