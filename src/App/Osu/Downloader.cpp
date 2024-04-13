#include "Downloader.h"

#include <curl/curl.h>
#include <pthread.h>

#include <sstream>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Engine.h"
#include "miniz.h"

struct DownloadResult {
    std::string url;
    std::vector<uint8_t> data;
    float progress = 0.f;
    int response_code = 0;
};

struct DownloadThread {
    bool running;
    pthread_t id;
    std::string endpoint;
    std::vector<DownloadResult*> downloads;
};

pthread_mutex_t threads_mtx = PTHREAD_MUTEX_INITIALIZER;
std::vector<DownloadThread*> threads;

void abort_downloads() {
    pthread_mutex_lock(&threads_mtx);
    for(auto thread : threads) {
        thread->running = false;
    }
    pthread_mutex_unlock(&threads_mtx);
}

void update_download_progress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                              curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;

    pthread_mutex_lock(&threads_mtx);
    auto result = (DownloadResult*)clientp;
    if(dltotal == 0) {
        result->progress = 0.f;
    } else if(dlnow > 0) {
        result->progress = (float)dlnow / (float)dltotal;
    }
    pthread_mutex_unlock(&threads_mtx);
}

void* do_downloads(void* arg) {
    auto thread = (DownloadThread*)arg;

    Packet response;
    CURL* curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL!\n");
        goto end_thread;
    }

    while(thread->running) {
        env->sleep(100000);  // wait 100ms between every download

        DownloadResult* result = nullptr;
        std::string url;

        pthread_mutex_lock(&threads_mtx);
        for(auto download : thread->downloads) {
            if(download->progress == 0.f) {
                result = download;
                url = download->url;
                break;
            }
        }
        pthread_mutex_unlock(&threads_mtx);
        if(!result) continue;

        free(response.memory);
        response = Packet();

        debugLog("Downloading %s\n", url.c_str());
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, bancho.user_agent.toUtf8());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, result);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, update_download_progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#ifdef _WIN32
        // ABSOLUTELY RETARDED, FUCK WINDOWS
        curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif
        CURLcode res = curl_easy_perform(curl);
        int response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if(res == CURLE_OK) {
            pthread_mutex_lock(&threads_mtx);
            result->progress = 1.f;
            result->response_code = response_code;
            result->data = std::vector<uint8_t>(response.memory, response.memory + response.size);
            pthread_mutex_unlock(&threads_mtx);
        } else {
            debugLog("Failed to download %s: %s\n", url.c_str(), curl_easy_strerror(res));

            pthread_mutex_lock(&threads_mtx);
            result->response_code = response_code;
            if(response_code == 429) {
                result->progress = 0.f;
            } else {
                result->data = std::vector<uint8_t>(response.memory, response.memory + response.size);
                result->progress = -1.f;
            }
            pthread_mutex_unlock(&threads_mtx);

            if(response_code == 429) {
                // Try again 5s later
                env->sleep(5000000);
            }
        }
    }

end_thread:
    curl_easy_cleanup(curl);

    pthread_mutex_lock(&threads_mtx);
    std::vector<DownloadThread*> new_threads;
    for(auto dt : threads) {
        if(thread != dt) {
            new_threads.push_back(dt);
        }
    }
    threads = std::move(new_threads);
    pthread_mutex_unlock(&threads_mtx);

    free(response.memory);
    for(auto result : thread->downloads) {
        delete result;
    }
    delete thread;
    return NULL;
}

void download(const char* url, float* progress, std::vector<uint8_t>& out, int* response_code) {
    char* hostname = NULL;
    bool download_found = false;
    DownloadThread* matching_thread = nullptr;

    CURLU* urlu = curl_url();
    if(!urlu) {
        *progress = -1.f;
        return;
    }

    if(curl_url_set(urlu, CURLUPART_URL, url, 0) != CURLUE_OK) {
        *progress = -1.f;
        goto end;
    }

    if(curl_url_get(urlu, CURLUPART_HOST, &hostname, 0) != CURLUE_OK) {
        *progress = -1.f;
        goto end;
    }

    pthread_mutex_lock(&threads_mtx);
    for(auto thread : threads) {
        if(thread->running && !strcmp(thread->endpoint.c_str(), hostname)) {
            matching_thread = thread;
            break;
        }
    }
    if(matching_thread == nullptr) {
        matching_thread = new DownloadThread();
        matching_thread->running = true;
        matching_thread->endpoint = std::string(hostname);
        int ret = pthread_create(&matching_thread->id, NULL, do_downloads, matching_thread);
        if(ret) {
            debugLog("Failed to start download thread: pthread_create() returned %i\n", ret);
            *progress = -1.f;
            delete matching_thread;
            goto end;
        } else {
            threads.push_back(matching_thread);
        }
    }

    for(int i = 0; i < matching_thread->downloads.size(); i++) {
        DownloadResult* result = matching_thread->downloads[i];

        if(result->url == url) {
            *progress = result->progress;
            *response_code = result->response_code;
            if(*response_code != 0) {
                out = result->data;
                delete matching_thread->downloads[i];
                matching_thread->downloads.erase(matching_thread->downloads.begin() + i);
            }

            download_found = true;
            break;
        }
    }

    if(!download_found) {
        auto newdl = new DownloadResult{.url = url};
        matching_thread->downloads.push_back(newdl);
        *progress = 0.f;
    }
    pthread_mutex_unlock(&threads_mtx);

end:
    curl_free(hostname);
    curl_url_cleanup(urlu);
}

void download_beatmapset(uint32_t set_id, float* progress) {
    // Check if we already have downloaded it
    std::stringstream ss;
    ss << MCENGINE_DATA_DIR "maps/" << std::to_string(set_id) << "/";
    auto map_dir = ss.str();
    if(env->directoryExists(map_dir)) {
        *progress = 1.f;
        return;
    }

    std::vector<uint8_t> data;
    auto mirror = convar->getConVarByName("beatmap_mirror")->getString();
    mirror.append(UString::format("%d", set_id));
    int response_code = 0;
    download(mirror.toUtf8(), progress, data, &response_code);
    if(response_code != 200) return;

    // Download succeeded: save map to disk
    mz_zip_archive zip = {0};
    mz_zip_archive_file_stat file_stat;
    mz_uint num_files = 0;

    debugLog("Extracting beatmapset %d (%d bytes)\n", set_id, data.size());
    if(!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0)) {
        debugLog("Failed to open .osz file\n");
        *progress = -1.f;
        return;
    }

    num_files = mz_zip_reader_get_num_files(&zip);
    if(num_files <= 0) {
        debugLog(".osz file is empty!\n");
        mz_zip_reader_end(&zip);
        *progress = -1.f;
        return;
    }
    if(!env->directoryExists(map_dir)) {
        env->createDirectory(map_dir);
    }
    for(mz_uint i = 0; i < num_files; i++) {
        if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
        if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

        char* saveptr = NULL;
        char* folder = strtok_r(file_stat.m_filename, "/", &saveptr);
        std::string file_path = map_dir;
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
    mz_zip_reader_end(&zip);
}
