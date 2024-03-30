#include "BanchoDownloader.h"

#include <curl/curl.h>
#include <pthread.h>

#include <sstream>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "Engine.h"
#include "miniz.h"

struct DownloadThread {
    uint32_t id = 0;
    DownloadStatus status = DOWNLOADING;
    float progress = 0.f;
    pthread_t thread = {0};
};

pthread_mutex_t downloading_mapsets_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<DownloadThread*> downloading_mapsets;

void update_download_progress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                              curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;

    pthread_mutex_lock(&downloading_mapsets_mutex);
    auto dt = (DownloadThread*)clientp;
    if(dltotal == 0) {
        dt->progress = 0.f;
    } else if(dlnow > 0) {
        dt->progress = (float)dlnow / (float)dltotal;
    }
    pthread_mutex_unlock(&downloading_mapsets_mutex);
}

void* run_mapset_download_thread(void* arg) {
    DownloadThread* dt = (DownloadThread*)arg;
    downloading_mapsets.push_back(dt);
    pthread_mutex_unlock(&downloading_mapsets_mutex);

    CURL* curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL\n");
        pthread_mutex_lock(&downloading_mapsets_mutex);
        dt->progress = 1.f;
        dt->status = FAILURE;
        pthread_mutex_unlock(&downloading_mapsets_mutex);
        return NULL;
    }

    const int NB_MIRRORS = 6;
    const char* mirrors[NB_MIRRORS] = {
        "https://api.osu.direct/d/%d", "https://chimu.moe/d/%d",     "https://api.nerinyan.moe/d/%d",
        "https://catboy.best/s/%d",    "https://osu.gatari.pw/d/%d", "https://osu.sayobot.cn/osu.php?s=%d",
    };

    Packet response;
    response.memory = (uint8_t*)malloc(2048);
    bool download_success = false;
    for(int i = 0; i < 5; i++) {
        mz_zip_archive zip = {0};
        mz_zip_archive_file_stat file_stat;
        mz_uint num_files = 0;
        std::stringstream ss;
        ss << MCENGINE_DATA_DIR "maps/";
        ss << dt->id;
        auto extract_to = ss.str();

        auto query_url = UString::format(mirrors[i], dt->id);
        debugLog("Downloading %s\n", query_url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_URL, query_url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, bancho.user_agent.toUtf8());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, dt);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, update_download_progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#ifdef _WIN32
        // ABSOLUTELY RETARDED, FUCK WINDOWS
        curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            debugLog("Failed to download %s: %s\n", query_url.toUtf8(), curl_easy_strerror(res));
            goto reset;
        }

        debugLog("Extracting beatmapset %d (%d bytes)\n", dt->id, response.size);
        if(!mz_zip_reader_init_mem(&zip, response.memory, response.size, 0)) {
            debugLog("Failed to open .osz file\n");
            goto reset;
        }
        num_files = mz_zip_reader_get_num_files(&zip);
        if(num_files <= 0) {
            debugLog(".osz file is empty!\n");
            goto reset;
        }
        if(!env->directoryExists(extract_to)) {
            env->createDirectory(extract_to);
        }
        for(mz_uint i = 0; i < num_files; i++) {
            if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
            if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

            char* saveptr = NULL;
            char* folder = strtok_r(file_stat.m_filename, "/", &saveptr);
            std::string file_path = extract_to;
            while(folder != NULL) {
                if(!strcmp(folder, "..")) {
                    // Bro...
                    goto skip_file;
                }

                file_path.append("/");
                file_path.append(folder);
                folder = strtok_r(NULL, "/", &saveptr);
                if(folder != NULL) {
                    if(!env->directoryExists(file_path)) {
                        env->createDirectory(file_path);
                    }
                }
            }

            mz_zip_reader_extract_to_file(&zip, i, file_path.c_str(), 0);

        skip_file:;
            // When a file can't be extracted we just ignore it (as long as the archive is valid).
            // We'll check for errors when loading the beatmap.
        }

        // Success
        download_success = true;
        mz_zip_reader_end(&zip);
        break;

    reset:
        free(response.memory);
        response = Packet();
        response.memory = (uint8_t*)malloc(2048);
        curl_easy_reset(curl);
        pthread_mutex_lock(&downloading_mapsets_mutex);
        dt->progress = 0.f;
        pthread_mutex_unlock(&downloading_mapsets_mutex);
        continue;
    }

    // We're finally done downloading & extracting
    free(response.memory);
    curl_easy_cleanup(curl);

    pthread_mutex_lock(&downloading_mapsets_mutex);
    if(download_success) {
        auto it = std::find(downloading_mapsets.begin(), downloading_mapsets.end(), dt);
        downloading_mapsets.erase(it);
        delete dt;
    } else {
        dt->progress = 1.f;
        dt->status = FAILURE;
    }
    pthread_mutex_unlock(&downloading_mapsets_mutex);

    return NULL;
}

BeatmapDownloadStatus download_mapset(uint32_t set_id) {
    // Check if we're already downloading
    BeatmapDownloadStatus status = {0};
    bool is_downloading = false;
    pthread_mutex_lock(&downloading_mapsets_mutex);
    for(auto dt : downloading_mapsets) {
        if(dt->id == set_id) {
            status.progress = dt->progress;
            status.status = dt->status;
            is_downloading = true;
            break;
        }
    }
    pthread_mutex_unlock(&downloading_mapsets_mutex);
    if(is_downloading) {
        return status;
    }

    // Check if we already have downloaded it
    std::stringstream ss;
    ss << MCENGINE_DATA_DIR "maps/" << set_id;
    auto map_dir = ss.str();
    if(env->directoryExists(map_dir)) {
        status.progress = 1.f;
        status.status = SUCCESS;
        return status;
    }

    // Start download
    auto dt = new DownloadThread();
    dt->id = set_id, dt->progress = 0.f;
    dt->status = DOWNLOADING;
    dt->thread = {0};
    pthread_mutex_lock(&downloading_mapsets_mutex);
    int ret = pthread_create(&dt->thread, NULL, run_mapset_download_thread, dt);
    if(ret) {
        pthread_mutex_unlock(&downloading_mapsets_mutex);
        debugLog("Failed to start download thread: pthread_create() returned %i\n", ret);
        delete dt;
        status.progress = 1.f;
        status.status = FAILURE;
        return status;
    } else {
        status.progress = 0.f;
        status.status = DOWNLOADING;
        return status;
    }
}
