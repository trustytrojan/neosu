#include "BanchoNetworking.h"

#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoProtocol.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUICheckbox.h"
#include "Chat.h"
#include "ConVar.h"
#include "Database.h"
#include "Downloader.h"
#include "Engine.h"
#include "File.h"
#include "Lobby.h"
#include "OptionsMenu.h"
#include "ResourceManager.h"
#include "RoomScreen.h"
#include "SongBrowser.h"
#include "UIAvatar.h"
#include "UIButton.h"
#include "miniz.h"

// Bancho protocol
pthread_mutex_t outgoing_mutex = PTHREAD_MUTEX_INITIALIZER;
bool try_logging_in = false;
Packet login_packet;
Packet outgoing;
pthread_mutex_t incoming_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<Packet> incoming_queue;
time_t last_packet_tms = {0};
double seconds_between_pings = 1.0;

std::string auth_header = "";
UString cho_token = "";

// osu! private API
pthread_mutex_t api_requests_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t api_responses_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<APIRequest> api_request_queue;
std::vector<Packet> api_response_queue;

void disconnect() {
    pthread_mutex_lock(&outgoing_mutex);

    // Logout
    // This is a blocking call, but we *do* want this to block when quitting the game.
    if(bancho.is_online()) {
        Packet packet;
        write_u16(&packet, LOGOUT);
        write_u8(&packet, 0);
        write_u32(&packet, 0);

        CURL *curl = curl_easy_init();
        auto version_header = UString::format("x-mcosu-ver: %s", bancho.neosu_version.toUtf8());
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, auth_header.c_str());
        chunk = curl_slist_append(chunk, version_header.toUtf8());
        auto query_url = UString::format("https://c.%s/", bancho.endpoint.toUtf8());
        curl_easy_setopt(curl, CURLOPT_URL, query_url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, packet.memory);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, packet.pos);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu!");
#ifdef _WIN32
        // ABSOLUTELY RETARDED, FUCK WINDOWS
        curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif
        curl_easy_perform(curl);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        free(packet.memory);
    }

    try_logging_in = false;
    auth_header = "";
    free(outgoing.memory);
    outgoing = Packet();
    bancho.user_id = 0;
    bancho.server_icon_url = "";
    if(bancho.server_icon != nullptr) {
        engine->getResourceManager()->destroyResource(bancho.server_icon);
        bancho.server_icon = nullptr;
    }

    bancho.score_submission_policy = ServerPolicy::NO_PREFERENCE;
    bancho.set_fposu_flag = false;
    bancho.set_mirror_flag = false;
    bancho.osu->m_optionsMenu->updateLayout();

    bancho.osu->m_optionsMenu->logInButton->setText("Log in");
    bancho.osu->m_optionsMenu->logInButton->setColor(0xff00ff00);
    bancho.osu->m_optionsMenu->logInButton->is_loading = false;
    ConVars::sv_cheats.setValue(true);

    for(auto pair : online_users) {
        delete pair.second;
    }
    online_users.clear();
    friends.clear();

    bancho.osu->m_chat->onDisconnect();

    // XXX: We should toggle between "offline" sorting options and "online" ones
    //      Online ones would be "Local scores", "Global", "Country", "Selected mods" etc
    //      While offline ones would be "By score", "By pp", etc
    bancho.osu->m_songBrowser2->onSortScoresChange(UString("Sort By Score"), 0);

    abort_downloads();

    pthread_mutex_unlock(&outgoing_mutex);
}

void reconnect() {
    if(bancho.osu->m_iInstanceID > 0) {
        // XXX: To handle multiple instances you would have to do some complex IPC
        //      Would be great to be able to use neosu as tournament spectator client...
        //      But that's not in scope right now.
        return;
    }

    disconnect();

    // Disable autologin, in case there's an error while logging in
    // Will be reenabled after the login succeeds
    convar->getConVarByName("mp_autologin")->setValue(false);

    UString cv_password = convar->getConVarByName("mp_password")->getString();
    if(cv_password.length() == 0) {
        // No password: don't try to log in
        return;
    }

    const char *pw = cv_password.toUtf8();  // cv_password needs to stay in scope!
    bancho.pw_md5 = md5((u8 *)pw, strlen(pw));

    bancho.username = convar->getConVarByName("name")->getString();
    bancho.endpoint = convar->getConVarByName("mp_server")->getString();

    bancho.osu->m_optionsMenu->logInButton->is_loading = true;
    Packet new_login_packet = build_login_packet();

    pthread_mutex_lock(&outgoing_mutex);
    free(login_packet.memory);
    login_packet = new_login_packet;
    try_logging_in = true;
    pthread_mutex_unlock(&outgoing_mutex);
}

size_t curl_write(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Packet *mem = (Packet *)userp;

    u8 *ptr = (u8 *)realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        /* out of memory! */
        debugLog("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static void send_api_request(CURL *curl, APIRequest api_out) {
    // XXX: Use download()

    Packet response;
    response.id = api_out.type;
    response.extra = api_out.extra;
    response.extra_int = api_out.extra_int;
    response.memory = (u8 *)malloc(2048);

    struct curl_slist *chunk = NULL;
    auto query_url = UString::format("https://osu.%s%s", bancho.endpoint.toUtf8(), api_out.path.toUtf8());
    curl_easy_setopt(curl, CURLOPT_URL, query_url.toUtf8());
    if(api_out.type == SUBMIT_SCORE) {
        auto token_header = UString::format("token: %s", cho_token.toUtf8());
        chunk = curl_slist_append(chunk, token_header.toUtf8());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    }
    if(api_out.mime != nullptr) {
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, api_out.mime);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
#ifdef _WIN32
    // ABSOLUTELY RETARDED, FUCK WINDOWS
    curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif

    CURLcode res = curl_easy_perform(curl);
    if(res == CURLE_OK) {
        pthread_mutex_lock(&api_responses_mutex);
        api_response_queue.push_back(response);
        pthread_mutex_unlock(&api_responses_mutex);
    }

    if(api_out.mime) {
        curl_mime_free(api_out.mime);
    }
    curl_easy_reset(curl);
    curl_slist_free_all(chunk);
}

static void send_bancho_packet(CURL *curl, Packet outgoing) {
    Packet response;
    response.memory = (u8 *)malloc(2048);

    struct curl_slist *chunk = NULL;
    auto version_header = UString::format("x-mcosu-ver: %s", bancho.neosu_version.toUtf8());
    chunk = curl_slist_append(chunk, version_header.toUtf8());
    if(!auth_header.empty()) {
        chunk = curl_slist_append(chunk, auth_header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    auto query_url = UString::format("https://c.%s/", bancho.endpoint.toUtf8());
    curl_easy_setopt(curl, CURLOPT_URL, query_url.toUtf8());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, outgoing.memory);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, outgoing.pos);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu!");
#ifdef _WIN32
    // ABSOLUTELY RETARDED, FUCK WINDOWS
    curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif

    last_packet_tms = time(NULL);
    CURLcode res = curl_easy_perform(curl);
    CURLHcode hres;
    if(res != CURLE_OK) {
        debugLog("Failed to send packet, cURL error %d: %s\n", res, curl_easy_strerror(res));
        if(auth_header.empty()) {
            // XXX: Not thread safe, playing with fire here
            auto errmsg = UString::format("Failed to log in: %s", curl_easy_strerror(res));
            bancho.osu->getNotificationOverlay()->addNotification(errmsg);
        }
        goto end;
    }

    // Update auth token if applicable
    struct curl_header *header;
    hres = curl_easy_header(curl, "cho-token", 0, CURLH_HEADER, -1, &header);
    if(hres == CURLHE_OK) {
        auth_header = "osu-token: " + std::string(header->value);
        cho_token = UString(header->value);
    }
    hres = curl_easy_header(curl, "x-mcosu-features", 0, CURLH_HEADER, -1, &header);
    if(hres == CURLHE_OK) {
        if(strstr(header->value, "submit=0") != NULL) {
            bancho.score_submission_policy = ServerPolicy::NO;
            bancho.osu->m_optionsMenu->updateLayout();
            debugLog("Server doesn't want score submission. :(\n");
        } else if(strstr(header->value, "submit=1") != NULL) {
            bancho.score_submission_policy = ServerPolicy::YES;
            bancho.osu->m_optionsMenu->updateLayout();
            debugLog("Server wants score submission! :D\n");
        }

        if(strstr(header->value, "fposu=1") != NULL) {
            bancho.set_fposu_flag = true;
        }
        if(strstr(header->value, "mirror=1") != NULL) {
            bancho.set_mirror_flag = true;
        }
    }

    while(response.pos < response.size) {
        u16 packet_id = read_u16(&response);
        response.pos++;
        u32 packet_len = read_u32(&response);
        if(packet_len > 10485760) {
            debugLog("Received a packet over 10Mb! Dropping response.\n");
            goto end;
        }

        Packet incoming = {
            .id = packet_id,
            .memory = (u8 *)malloc(packet_len),
            .size = packet_len,
            .pos = 0,
        };
        memcpy(incoming.memory, response.memory + response.pos, packet_len);

        pthread_mutex_lock(&incoming_mutex);
        incoming_queue.push_back(incoming);
        pthread_mutex_unlock(&incoming_mutex);

        seconds_between_pings = 1.0;
        response.pos += packet_len;
    }

end:
    curl_easy_reset(curl);
    free(response.memory);
    curl_slist_free_all(chunk);
}

static void *do_networking(void *data) {
    (void)data;

    last_packet_tms = time(NULL);

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL, online functionality disabled.\n");
        return NULL;
    }

    while(bancho.osu != nullptr) {
        pthread_mutex_lock(&api_requests_mutex);
        if(api_request_queue.empty()) {
            pthread_mutex_unlock(&api_requests_mutex);
        } else {
            APIRequest api_out = api_request_queue.front();
            api_request_queue.erase(api_request_queue.begin());
            pthread_mutex_unlock(&api_requests_mutex);

            send_api_request(curl, api_out);
        }

        if(bancho.osu && bancho.osu->m_lobby->isVisible()) seconds_between_pings = 1;
        if(bancho.is_in_a_multi_room() && seconds_between_pings > 3) seconds_between_pings = 3;
        bool should_ping = difftime(time(NULL), last_packet_tms) > seconds_between_pings;
        if(bancho.user_id <= 0) should_ping = false;

        pthread_mutex_lock(&outgoing_mutex);
        if(try_logging_in) {
            Packet login = login_packet;
            login_packet = Packet();
            try_logging_in = false;
            pthread_mutex_unlock(&outgoing_mutex);
            send_bancho_packet(curl, login);
            free(login.memory);
            pthread_mutex_lock(&outgoing_mutex);
        } else if(should_ping && outgoing.pos == 0) {
            write_u16(&outgoing, PING);
            write_u8(&outgoing, 0);
            write_u32(&outgoing, 0);

            // Polling gets slower over time, but resets when we receive new data
            if(seconds_between_pings < 30.0) {
                seconds_between_pings += 1.0;
            }
        }

        if(outgoing.pos > 0) {
            Packet out = outgoing;
            outgoing = Packet();
            pthread_mutex_unlock(&outgoing_mutex);

            // DEBUG: If we're not sending the right amount of bytes, bancho.py just
            // chugs along! To try to detect it faster, we'll send two packets per request.
            write_u16(&out, PING);
            write_u8(&out, 0);
            write_u32(&out, 0);

            send_bancho_packet(curl, out);
            free(out.memory);
        } else {
            pthread_mutex_unlock(&outgoing_mutex);
        }

        env->sleep(1000);  // wait 1ms
    }

    curl_easy_cleanup(curl);

    return NULL;
}

static void handle_api_response(Packet packet) {
    switch(packet.id) {
        case GET_BEATMAPSET_INFO: {
            RoomScreen::process_beatmapset_info_response(packet);
            break;
        }

        case GET_MAP_LEADERBOARD: {
            process_leaderboard_response(packet);
            break;
        }

        case GET_REPLAY: {
            if(packet.size == 0) {
                // Most likely, 404
                bancho.osu->m_notificationOverlay->addNotification("Failed to download replay");
                break;
            }

            FinishedScore *score = (FinishedScore *)packet.extra;
            std::stringstream replay_path;
            replay_path << MCENGINE_DATA_DIR "replays/" << score->server << "/" << score->unixTimestamp
                        << ".replay.lzma";

            // XXX: this is blocking main thread
            auto replay_path_str = replay_path.str();
            FILE *replay_file = fopen(replay_path_str.c_str(), "wb");
            if(replay_file == NULL) {
                bancho.osu->m_notificationOverlay->addNotification("Failed to save replay");
                break;
            }

            fwrite(packet.memory, packet.size, 1, replay_file);
            fclose(replay_file);
            Replay::load_and_watch(*score);
            break;
        }

        case MARK_AS_READ: {
            // (nothing to do)
            break;
        }

        case SUBMIT_SCORE: {
            // TODO @kiwec: handle response
            debugLog("Score submit result: %s\n", packet.memory);

            // Reset leaderboards so new score will appear
            bancho.osu->getSongBrowser()->m_db->m_online_scores.clear();
            bancho.osu->getSongBrowser()->rebuildScoreButtons();
            break;
        }

        default: {
            // NOTE: API Response type is same as API Request type
            debugLog("No handler for API response type %d!\n", packet.id);
        }
    }
}

void receive_api_responses() {
    pthread_mutex_lock(&api_responses_mutex);
    while(!api_response_queue.empty()) {
        Packet incoming = api_response_queue.front();
        api_response_queue.erase(api_response_queue.begin());
        handle_api_response(incoming);
        free(incoming.memory);
        free(incoming.extra);
    }
    pthread_mutex_unlock(&api_responses_mutex);
}

void receive_bancho_packets() {
    pthread_mutex_lock(&incoming_mutex);
    while(!incoming_queue.empty()) {
        Packet incoming = incoming_queue.front();
        incoming_queue.erase(incoming_queue.begin());
        handle_packet(&incoming);
        free(incoming.memory);
    }
    pthread_mutex_unlock(&incoming_mutex);
}

void send_api_request(APIRequest request) {
    if(bancho.user_id <= 0) {
        debugLog("Cannot send API request of type %d since we are not logged in.\n", request.type);
        return;
    }

    pthread_mutex_lock(&api_requests_mutex);

    // Jank way to do things... remove outdated requests now
    api_request_queue.erase(std::remove_if(api_request_queue.begin(), api_request_queue.end(),
                                           [request](APIRequest r) { return r.type = request.type; }),
                            api_request_queue.end());

    api_request_queue.push_back(request);

    pthread_mutex_unlock(&api_requests_mutex);
}

void send_packet(Packet &packet) {
    if(bancho.user_id <= 0) {
        // Don't queue any packets until we're logged in
        return;
    }

    // debugLog("Sending packet of type %hu: ", packet.id);
    // for (int i = 0; i < packet.pos; i++) {
    //     debugLog("%02x ", packet.memory[i]);
    // }
    // debugLog("\n");

    pthread_mutex_lock(&outgoing_mutex);

    // We're not sending it immediately, instead we just add it to the pile of
    // packets to send
    write_u16(&outgoing, packet.id);
    write_u8(&outgoing, 0);
    write_u32(&outgoing, packet.pos);
    write_bytes(&outgoing, packet.memory, packet.pos);

    pthread_mutex_unlock(&outgoing_mutex);
}

void init_networking_thread() {
    pthread_t dummy = 0;
    int ret = pthread_create(&dummy, NULL, do_networking, NULL);
    if(ret) {
        debugLog(
            "Failed to start networking thread: pthread_create() returned "
            "code %i\n",
            ret);
    }
}
