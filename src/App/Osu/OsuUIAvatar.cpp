#include "OsuUIAvatar.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Engine.h"
#include "Osu.h"
#include "OsuSkin.h"
#include "OsuUIUserContextMenu.h"
#include "ResourceManager.h"

int avatar_downloading_thread_id = 0;
pthread_mutex_t avatars_mtx = PTHREAD_MUTEX_INITIALIZER;
std::vector<uint32_t> avatars_to_load;
std::vector<uint32_t> avatars_loaded;

void *avatar_downloading_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&avatars_mtx);
    avatar_downloading_thread_id++;
    int thread_id = avatar_downloading_thread_id;
    pthread_mutex_unlock(&avatars_mtx);

    UString endpoint = bancho.endpoint;

    std::stringstream ss;
    ss << MCENGINE_DATA_DIR "avatars/" << endpoint.toUtf8();
    auto server_dir = ss.str();
    if(!env->directoryExists(server_dir)) {
        if(!env->directoryExists(MCENGINE_DATA_DIR "avatars")) {
            env->createDirectory(MCENGINE_DATA_DIR "avatars");
        }
        env->createDirectory(server_dir);
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL, avatar downloading disabled.\n");
        return NULL;
    }

    std::vector<uint32_t> blacklist;
    blacklist.push_back(0);  // make sure we don't try to load user id 0

    while(thread_id == avatar_downloading_thread_id) {
    loop:
        env->sleep(100000);  // wait 100ms between every download

        pthread_mutex_lock(&avatars_mtx);
        if(avatars_to_load.empty()) {
            pthread_mutex_unlock(&avatars_mtx);
            continue;
        }
        uint32_t avatar_id = avatars_to_load.front();
        for(auto bl : blacklist) {
            if(avatar_id == bl) {
                avatars_to_load.erase(avatars_to_load.begin());
                pthread_mutex_unlock(&avatars_mtx);
                goto loop;
            }
        }
        pthread_mutex_unlock(&avatars_mtx);

        auto img_url = UString::format("https://a.%s/%d", endpoint.toUtf8(), avatar_id);
        debugLog("Downloading %s\n", img_url.toUtf8());
        Packet response;
        response.memory = new uint8_t[2048];
        curl_easy_setopt(curl, CURLOPT_URL, img_url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, bancho.user_agent.toUtf8());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
#ifdef _WIN32
        // ABSOLUTELY RETARDED, FUCK WINDOWS
        curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            debugLog("Failed to download %s: %s\n", img_url.toUtf8(), curl_easy_strerror(res));

            int response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if(response_code == 429) {
                // Fetching avatars too quickly, try again 5s later
                env->sleep(5000000);
            } else {
                // Failed to load avatar, don't try to fetch it again
                blacklist.push_back(avatar_id);
            }

            // We still save the avatar if it 404s, since the server will return the default avatar
            if(response_code != 404) {
                delete response.memory;
                curl_easy_reset(curl);
                continue;
            }
        }

        std::stringstream ss2;
        ss2 << server_dir << "/" << std::to_string(avatar_id);
        auto img_path = ss2.str();
        FILE *file = fopen(img_path.c_str(), "wb");
        if(file != NULL) {
            fwrite(response.memory, response.size, 1, file);
            fflush(file);
            fclose(file);
        }
        delete response.memory;
        curl_easy_reset(curl);

        pthread_mutex_lock(&avatars_mtx);
        if(thread_id == avatar_downloading_thread_id) {
            avatars_to_load.erase(avatars_to_load.begin());
            avatars_loaded.push_back(avatar_id);
        }
        pthread_mutex_unlock(&avatars_mtx);
    }

    pthread_mutex_lock(&avatars_mtx);
    avatars_to_load.clear();
    avatars_loaded.clear();
    pthread_mutex_unlock(&avatars_mtx);

    curl_easy_cleanup(curl);
    return NULL;
}

OsuUIAvatar::OsuUIAvatar(uint32_t player_id, float xPos, float yPos, float xSize, float ySize)
    : CBaseUIButton(xPos, yPos, xSize, ySize, "avatar", "") {
    m_player_id = player_id;

    avatar_path = UString::format(MCENGINE_DATA_DIR "avatars/%s/%d", bancho.endpoint.toUtf8(), player_id);
    setClickCallback(fastdelegate::MakeDelegate(this, &OsuUIAvatar::onAvatarClicked));

    struct stat attr;
    bool exists = (stat(avatar_path.c_str(), &attr) == 0);
    if(exists) {
        // File exists, but if it's more than 7 days old assume it's expired
        time_t now = time(NULL);
        struct tm expiration_date = *localtime(&attr.st_mtime);
        expiration_date.tm_mday += 7;
        if(now > mktime(&expiration_date)) {
            exists = false;
        }
    }

    if(exists) {
        avatar = engine->getResourceManager()->loadImageAbs(avatar_path, avatar_path);
    }
}

void OsuUIAvatar::draw(Graphics *g) {
    if(!on_screen) return;  // Comment when you need to debug on_screen logic

    if(avatar != nullptr) {
        g->pushTransform();
        g->setColor(0xffffffff);
        g->scale(m_vSize.x / avatar->getWidth(), m_vSize.y / avatar->getHeight());
        g->translate(m_vPos.x + m_vSize.x / 2.0f, m_vPos.y + m_vSize.y / 2.0f);
        g->drawImage(avatar);
        g->popTransform();
    }

    // For debugging purposes
    // if(on_screen) {
    //     g->pushTransform();
    //     g->setColor(0xff00ff00);
    //     g->drawQuad((int)m_vPos.x, (int)m_vPos.y, (int)m_vSize.x, (int)m_vSize.y);
    //     g->popTransform();
    // } else {
    //     g->pushTransform();
    //     g->setColor(0xffff0000);
    //     g->drawQuad((int)m_vPos.x, (int)m_vPos.y, (int)m_vSize.x, (int)m_vSize.y);
    //     g->popTransform();
    // }
}

void OsuUIAvatar::mouse_update(bool *propagate_clicks) {
    CBaseUIButton::mouse_update(propagate_clicks);

    if(avatar == nullptr && m_player_id != 0) {
        pthread_mutex_lock(&avatars_mtx);

        // Check if it has finished downloading
        auto it = std::find(avatars_loaded.begin(), avatars_loaded.end(), m_player_id);
        if(it != avatars_loaded.end()) {
            avatars_loaded.erase(it);
            avatar = engine->getResourceManager()->loadImageAbs(avatar_path, avatar_path);
        }

        // Check if avatar is on screen and *still* not downloaded yet
        if(avatar == nullptr && on_screen) {
            // Request download if not done so already
            auto it = std::find(avatars_to_load.begin(), avatars_to_load.end(), m_player_id);
            if(it == avatars_to_load.end()) {
                debugLog("Adding avatar %d to download queue\n", m_player_id);
                avatars_to_load.push_back(m_player_id);
            }
        }
        pthread_mutex_unlock(&avatars_mtx);
    }
}

void OsuUIAvatar::onAvatarClicked(CBaseUIButton *btn) {
    if(bancho.osu->isInPlayMode()) {
        // Don't want context menu to pop up while playing a map
        return;
    }

    bancho.osu->m_user_actions->open(m_player_id);
}
