#include "Downloader.h"

#include <curl/curl.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>

#include "Archival.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Osu.h"
#include "SongBrowser/SongBrowser.h"
#include "curl_blob.h"

namespace {  // static
class DownloadManager {
   private:
    struct DownloadRequest {
        std::string url;
        std::atomic<float> progress{0.0f};
        std::atomic<int> response_code{0};
        std::vector<u8> data;
        std::mutex data_mutex;
        bool completed{false};
    };

    std::atomic<bool> shutting_down{false};
    std::thread worker_thread;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::queue<std::shared_ptr<DownloadRequest>> download_queue;
    std::unordered_map<std::string, std::shared_ptr<DownloadRequest>> active_downloads;
    std::mutex active_mutex;

    static size_t curl_writefunc(void *contents, size_t size, size_t nmemb, void *userp) {
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

    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
        auto* request = static_cast<DownloadRequest*>(clientp);
        if(dltotal > 0) {
            request->progress.store(static_cast<float>(dlnow) / static_cast<float>(dltotal));
        }
        return 0;
    }

    void worker_loop() {
        CURL* curl = curl_easy_init();
        if(!curl) {
            debugLog("Failed to initialize cURL for download worker!\n");
            return;
        }

        while(!this->shutting_down.load()) {
            Timing::sleepMS(100);  // wait 100ms between every download
            std::shared_ptr<DownloadRequest> request;

            // wait for work or shutdown
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->queue_cv.wait(lock,
                                    [this] { return !this->download_queue.empty() || this->shutting_down.load(); });

                if(this->shutting_down.load() && this->download_queue.empty()) {
                    break;
                }

                if(!this->download_queue.empty()) {
                    request = this->download_queue.front();
                    this->download_queue.pop();
                }
            }

            if(!request) {
                continue;
            }

            if(this->shutting_down.load()) {
                break;
            }

            // perform download
            Packet response{};

            curl_easy_reset(curl);
            curl_easy_setopt(curl, CURLOPT_URL, request->url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, bancho->user_agent.toUtf8());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writefunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, request.get());
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

            curl_easy_setopt_CAINFO_BLOB_embedded(curl);

            debugLog("Downloading %s\n", request->url.c_str());
            CURLcode res = curl_easy_perform(curl);

            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            // update request with results
            {
                std::scoped_lock lock(request->data_mutex);
                request->response_code.store(static_cast<int>(response_code));

                if(res == CURLE_OK && response_code == 200) {
                    request->data = std::vector<u8>(response.memory, response.memory + response.size);
                    request->progress.store(1.0f);
                    request->completed = true;
                } else {
                    if(res != CURLE_OK) {
                        debugLog("Failed to download %s: %s\n", request->url.c_str(), curl_easy_strerror(res));
                    }
                    if(response_code == 429) {
                        // rate limited, retry later (keep in active_downloads)
                        request->progress.store(0.0f);

                        // wait before retrying
                        std::this_thread::sleep_for(std::chrono::seconds(5));

                        // re-queue for retry
                        {
                            std::scoped_lock lock(this->queue_mutex);
                            this->download_queue.push(request);
                        }
                        this->queue_cv.notify_one();

                        free(response.memory);
                        continue;
                    } else {
                        request->progress.store(-1.0f);
                        request->completed = true;
                    }
                }
            }

            free(response.memory);
            // don't remove from active_downloads, keep completed requests cached (so future download requests can just
            // grab the already-downloaded data)
        }

        curl_easy_cleanup(curl);
    }

   public:
    DownloadManager() { this->worker_thread = std::thread(&DownloadManager::worker_loop, this); }

    ~DownloadManager() { this->shutdown(); }

    DownloadManager& operator=(const DownloadManager&) = delete;
    DownloadManager& operator=(DownloadManager&&) = delete;
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager(DownloadManager&&) = delete;

    void shutdown() {
        if(!this->shutting_down.exchange(true)) {
            // signal shutdown and wake up worker
            {
                std::scoped_lock lock(this->queue_mutex);
                // clear queue to prevent new work
                while(!this->download_queue.empty()) {
                    this->download_queue.pop();
                }
            }
            this->queue_cv.notify_all();

            if(this->worker_thread.joinable()) {
                this->worker_thread.join();
            }
        }
    }

    std::shared_ptr<DownloadRequest> start_download(const std::string& url) {
        std::scoped_lock lock(this->active_mutex);

        // check if already downloading
        auto it = this->active_downloads.find(url);
        if(it != this->active_downloads.end()) {
            return it->second;
        }

        // create new download request
        auto request = std::make_shared<DownloadRequest>();
        request->url = url;

        this->active_downloads[url] = request;

        // queue for download
        {
            std::scoped_lock queue_lock(this->queue_mutex);
            this->download_queue.push(request);
        }
        this->queue_cv.notify_one();

        return request;
    }
};

// shared global instance
std::unique_ptr<DownloadManager> s_download_manager;

// helper
std::unordered_map<i32, i32> beatmap_to_beatmapset;

i32 get_beatmapset_id_from_osu_file(const u8* osu_data, size_t s_osu_data) {
    i32 set_id = -1;
    std::string line;
    for(size_t i = 0; i < s_osu_data; i++) {
        if(osu_data[i] == '\n') {
            if(line.find("//") != 0) {
                sscanf(line.c_str(), " BeatmapSetID : %i \n", &set_id);
                if(set_id != -1) return set_id;
            }

            line = "";
        } else {
            line.push_back(osu_data[i]);
        }
    }

    if(line.find("//") != 0) {
        sscanf(line.c_str(), " BeatmapSetID : %i \n", &set_id);
    }

    return -1;
}
}  // namespace

namespace Downloader {

void abort_downloads() {
    if(s_download_manager) {
        s_download_manager->shutdown();  // this will block until the worker thread finishes
        s_download_manager.reset();
    }
}

void download(const char* url, float* progress, std::vector<u8>& out, int* response_code) {
    if(!s_download_manager) {
        s_download_manager = std::make_unique<DownloadManager>();
    }

    auto request = s_download_manager->start_download(std::string(url));

    *progress = request->progress.load();
    *response_code = request->response_code.load();

    if(request->completed) {
        std::scoped_lock lock(request->data_mutex);
        if(*response_code == 200) {
            out = request->data;
        }
    }
}

i32 extract_beatmapset_id(const u8* data, size_t data_s) {
    debugLog("Reading beatmapset (%d bytes)\n", data_s);

    Archive archive(data, data_s);
    if(!archive.isValid()) {
        debugLog("Failed to open .osz file\n");
        return -1;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog(".osz file is empty!\n");
        return -1;
    }

    for(const auto& entry : entries) {
        if(entry.isDirectory()) continue;

        std::string filename = entry.getFilename();
        if(env->getFileExtensionFromFilePath(filename).compare("osu") != 0) continue;

        auto osu_data = entry.extractToMemory();
        if(osu_data.empty()) continue;

        i32 set_id = get_beatmapset_id_from_osu_file(osu_data.data(), osu_data.size());
        if(set_id != -1) return set_id;
    }

    return -1;
}

bool extract_beatmapset(const u8* data, size_t data_s, const std::string& map_dir) {
    debugLog("Extracting beatmapset (%d bytes)\n", data_s);

    Archive archive(data, data_s);
    if(!archive.isValid()) {
        debugLog("Failed to open .osz file\n");
        return false;
    }

    auto entries = archive.getAllEntries();
    if(entries.empty()) {
        debugLog(".osz file is empty!\n");
        return false;
    }

    if(!env->directoryExists(map_dir)) {
        env->createDirectory(map_dir);
    }

    for(const auto& entry : entries) {
        if(entry.isDirectory()) continue;

        std::string filename = entry.getFilename();
        auto folders = UString(filename).split("/");
        std::string file_path = map_dir;

        for(const auto& folder : folders) {
            if(!env->directoryExists(file_path)) {
                env->createDirectory(file_path);
            }

            if(folder == UString("..")) {
                // security check: skip files with path traversal attempts
                goto skip_file;
            } else {
                file_path.append("/");
                file_path.append(folder.toUtf8());
            }
        }

        if(!entry.extractToFile(file_path)) {
            debugLog("Failed to extract file %s\n", filename.c_str());
        }

    skip_file:;
        // when a file can't be extracted we just ignore it (as long as the archive is valid)
        // we'll check for errors when loading the beatmap
    }

    return true;
}

void download_beatmapset(u32 set_id, float* progress) {
    // Check if we already have downloaded it
    auto map_dir = fmt::format(MCENGINE_DATA_DIR "maps/{}/", set_id);
    if(env->directoryExists(map_dir)) {
        *progress = 1.f;
        return;
    }

    std::vector<u8> data;

    auto scheme = cv::use_https.getBool() ? "https://" : "http://";
    auto download_url = fmt::format("{:s}osu.{:s}/d/", scheme, bancho->endpoint.toUtf8());
    if(cv::beatmap_mirror_override.getString().length() > 0) {
        download_url = cv::beatmap_mirror_override.getString();
    }
    download_url.append(fmt::format("{:d}", set_id));

    int response_code = 0;
    download(download_url.c_str(), progress, data, &response_code);
    if(response_code != 200) return;

    // Download succeeded: save map to disk
    if(!extract_beatmapset(data.data(), data.size(), map_dir)) {
        *progress = -1.f;
        return;
    }
}

DatabaseBeatmap* download_beatmap(i32 beatmap_id, MD5Hash beatmap_md5, float* progress) {
    static i32 queried_map_id = 0;

    auto beatmap = db->getBeatmapDifficulty(beatmap_md5);
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
        request.path = UString::format("/web/osu-search-set.php?b=%d&u=%s&h=%s", beatmap_id, bancho->username.toUtf8(),
                                       bancho->pw_md5.toUtf8());
        request.extra_int = beatmap_id;
        BANCHO::Net::send_api_request(request);

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
    db->addBeatmapSet(mapset_path.toUtf8(), set_id);
    debugLog("Finished loading beatmapset %d.\n", set_id);

    beatmap = db->getBeatmapDifficulty(beatmap_md5);
    if(beatmap == NULL) {
        beatmap_to_beatmapset[beatmap_id] = 0;
        *progress = -1.f;
        return NULL;
    }

    // Some beatmaps don't provide beatmap/beatmapset IDs in the .osu files
    // While we're clueless on the beatmap IDs of the other maps in the set,
    // we can still make sure at least the one we wanted is correct.
    beatmap->iID = beatmap_id;

    *progress = 1.f;
    return beatmap;
}

DatabaseBeatmap* download_beatmap(i32 beatmap_id, i32 beatmapset_id, float* progress) {
    static i32 queried_map_id = 0;

    auto beatmap = db->getBeatmapDifficulty(beatmap_id);
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

        // We already have the set ID, skip the API request
        if(beatmapset_id != 0) {
            beatmap_to_beatmapset[beatmap_id] = beatmapset_id;
            *progress = 0.f;
            return NULL;
        }

        APIRequest request;
        request.type = GET_BEATMAPSET_INFO;
        request.path = UString::format("/web/osu-search-set.php?b=%d&u=%s&h=%s", beatmap_id, bancho->username.toUtf8(),
                                       bancho->pw_md5.toUtf8());
        request.extra_int = beatmap_id;
        BANCHO::Net::send_api_request(request);

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
    db->addBeatmapSet(mapset_path.toUtf8());
    debugLog("Finished loading beatmapset %d.\n", set_id);

    beatmap = db->getBeatmapDifficulty(beatmap_id);
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
}  // namespace Downloader
