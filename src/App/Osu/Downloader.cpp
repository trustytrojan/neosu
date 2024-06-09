#include "Downloader.h"

#include <curl/curl.h>
#include <mutex>
#include <thread>

#include <sstream>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"
#include "Osu.h"
#include "SongBrowser/SongBrowser.h"
#include "miniz.h"

struct DownloadResult {
    std::string url;
    std::vector<u8> data;
    float progress = 0.f;
    int response_code = 0;
};

struct DownloadThread {
    bool running;
    std::string endpoint;
    std::vector<DownloadResult*> downloads;
};

std::mutex threads_mtx;
std::vector<DownloadThread*> threads;

void abort_downloads() {
    std::lock_guard<std::mutex> lock(threads_mtx);
    for(auto thread : threads) {
        thread->running = false;
    }
}

int update_download_progress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                              curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;

    std::lock_guard<std::mutex> lock(threads_mtx);
    auto result = (DownloadResult*)clientp;
    if(dltotal == 0) {
        result->progress = 0.f;
    } else if(dlnow > 0) {
        result->progress = (float)dlnow / (float)dltotal;
    }

    return CURL_PROGRESSFUNC_CONTINUE;
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

        DownloadResult* result = NULL;
        std::string url;

        threads_mtx.lock();
        for(auto download : thread->downloads) {
            if(download->progress == 0.f) {
                result = download;
                url = download->url;
                break;
            }
        }
        threads_mtx.unlock();
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
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#ifdef _WIN32
        // ABSOLUTELY RETARDED, FUCK WINDOWS
        curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");
#endif
        CURLcode res = curl_easy_perform(curl);
        int response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if(res == CURLE_OK) {
            threads_mtx.lock();
            result->progress = 1.f;
            result->response_code = response_code;
            result->data = std::vector<u8>(response.memory, response.memory + response.size);
            threads_mtx.unlock();
        } else {
            debugLog("Failed to download %s: %s\n", url.c_str(), curl_easy_strerror(res));

            threads_mtx.lock();
            result->response_code = response_code;
            if(response_code == 429) {
                result->progress = 0.f;
            } else {
                result->data = std::vector<u8>(response.memory, response.memory + response.size);
                result->progress = -1.f;
            }
            threads_mtx.unlock();

            if(response_code == 429) {
                // Try again 5s later
                env->sleep(5000000);
            }
        }
    }

end_thread:
    curl_easy_cleanup(curl);

    threads_mtx.lock();
    std::vector<DownloadThread*> new_threads;
    for(auto dt : threads) {
        if(thread != dt) {
            new_threads.push_back(dt);
        }
    }
    threads = std::move(new_threads);
    threads_mtx.unlock();

    free(response.memory);
    for(auto result : thread->downloads) {
        delete result;
    }
    delete thread;
    return NULL;
}

void download(const char* url, float* progress, std::vector<u8>& out, int* response_code) {
    char* hostname = NULL;
    bool download_found = false;
    DownloadThread* matching_thread = NULL;

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

    threads_mtx.lock();
    for(auto thread : threads) {
        if(thread->running && !strcmp(thread->endpoint.c_str(), hostname)) {
            matching_thread = thread;
            break;
        }
    }
    if(matching_thread == NULL) {
        matching_thread = new DownloadThread();
        matching_thread->running = true;
        matching_thread->endpoint = std::string(hostname);
        auto dl_thread = std::thread(do_downloads, matching_thread);
        dl_thread.detach();
        threads.push_back(matching_thread);
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
    threads_mtx.unlock();

end:
    curl_free(hostname);
    curl_url_cleanup(urlu);
}

void download_beatmapset(u32 set_id, float* progress) {
    // Check if we already have downloaded it
    auto map_dir = UString::format(MCENGINE_DATA_DIR "maps/%d/", set_id);
    if(env->directoryExists(map_dir.toUtf8())) {
        *progress = 1.f;
        return;
    }

    std::vector<u8> data;
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
    if(!env->directoryExists(map_dir.toUtf8())) {
        env->createDirectory(map_dir.toUtf8());
    }
    for(mz_uint i = 0; i < num_files; i++) {
        if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
        if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

        auto folders = UString(file_stat.m_filename).split("/");
        std::string file_path = map_dir.toUtf8();
        for(auto folder : folders) {
            if(!env->directoryExists(file_path)) {
                env->createDirectory(file_path);
            }

            if(folder == UString("..")) {
                // Bro...
                goto skip_file;
            } else {
                file_path.append("/");
                file_path.append(folder.toUtf8());
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

std::unordered_map<i32, i32> beatmap_to_beatmapset;
DatabaseBeatmap* download_beatmap(i32 beatmap_id, MD5Hash beatmap_md5, float* progress) {
    static i32 queried_map_id = 0;

    auto beatmap = osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(beatmap_md5);
    if(beatmap != NULL) {
        *progress = 1.f;
        return beatmap;
    }

    // XXX: Currently, we do not try to find the difficulty from unloaded database, or from neosu downloaded maps
    auto it = beatmap_to_beatmapset.find(beatmap_id);
    if(it == beatmap_to_beatmapset.end()) {
        if(queried_map_id == beatmap_id) {
            // We already queried for the beatmapset ID, and are waiting for the response
            *progress = 0.f;
            return NULL;
        }

        APIRequest request;
        request.type = GET_BEATMAPSET_INFO;
        request.path = UString::format("/web/osu-search-set.php?b=%d&u=%s&h=%s", beatmap_id, bancho.username.toUtf8(),
                                       bancho.pw_md5.toUtf8());
        request.extra_int = beatmap_id;
        send_api_request(request);

        queried_map_id = beatmap_id;

        *progress = 0.f;
        return NULL;
    }

    i32 set_id = it->second;
    if(set_id == 0) {
        // Already failed to load the beatmap
        *progress = -1.f;
        return NULL;
    }

    download_beatmapset(set_id, progress);
    if(*progress == -1.f) {
        // Download failed, don't retry
        beatmap_to_beatmapset[beatmap_id] = 0;
        return NULL;
    }

    // Download not finished
    if(*progress != 1.f) return NULL;

    auto mapset_path = UString::format(MCENGINE_DATA_DIR "maps/%d/", set_id);
    // XXX: Make a permanent database for auto-downloaded songs, so we can load them like osu!.db's
    osu->m_songBrowser2->getDatabase()->addBeatmap(mapset_path.toUtf8());
    osu->m_songBrowser2->updateSongButtonSorting();
    debugLog("Finished loading beatmapset %d.\n", set_id);

    beatmap = osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(beatmap_md5);
    if(beatmap == NULL) {
        beatmap_to_beatmapset[beatmap_id] = 0;
        *progress = -1.f;
        return NULL;
    }

    *progress = 1.f;
    return beatmap;
}

void process_beatmapset_info_response(Packet packet) {
    i32 map_id = packet.extra_int;
    if(packet.size == 0) {
        beatmap_to_beatmapset[map_id] = 0;
        return;
    }

    // {set_id}.osz|{artist}|{title}|{creator}|{status}|10.0|{last_update}|{set_id}|0|0|0|0|0
    auto tokens = UString((char*)packet.memory).split("|");
    if(tokens.size() < 13) return;

    beatmap_to_beatmapset[map_id] = strtoul(tokens[7].toUtf8(), NULL, 10);
}
