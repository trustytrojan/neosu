#pragma once
#include "UString.h"

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include <map>
#include <atomic>

#include <curl/curl.h>

struct Bancho;

// forward declare for async requests
struct NetworkRequest;

class NetworkHandler {
   public:
    // async request options
    struct RequestOptions {
        RequestOptions() { ; }  // ?
        std::map<std::string, std::string> headers;
        std::string postData;
        std::string userAgent;
        curl_mime* mimeData{nullptr};
        long timeout{5};
        long connectTimeout{5};
        bool followRedirects{false};
    };

    // async response data
    struct Response {
        std::string body;
        long responseCode{0};
        std::map<std::string, std::string> headers;
        bool success{false};
    };

    using AsyncCallback = std::function<void(Response response)>;

    NetworkHandler();
    ~NetworkHandler();

    NetworkHandler(const NetworkHandler&) = delete;
    NetworkHandler& operator=(const NetworkHandler&) = delete;
    NetworkHandler(NetworkHandler&&) = delete;
    NetworkHandler& operator=(NetworkHandler&&) = delete;

    // synchronous API
    UString httpGet(const UString& url, long timeout = 5, long connectTimeout = 5);
    std::string httpDownload(const UString& url, long timeout = 60, long connectTimeout = 5);

    // asynchronous API
    void httpRequestAsync(const UString& url, AsyncCallback callback, const RequestOptions& options = {});

    // sync request for special cases like logout
    Response performSyncRequest(const UString& url, const RequestOptions& options);

   private:
    static std::once_flag curl_init_flag;

    // curl_multi implementation
    CURLM* multi_handle;
    std::thread network_thread;
    std::atomic<bool> should_stop;

    // request queuing
    std::mutex request_queue_mutex;
    std::condition_variable request_queue_cv;
    std::queue<std::unique_ptr<NetworkRequest>> pending_requests;

    // active requests tracking
    std::mutex active_requests_mutex;
    std::map<CURL*, std::unique_ptr<NetworkRequest>> active_requests;

    // sync request support
    std::mutex sync_requests_mutex;
    std::map<void*, std::condition_variable*> sync_request_cvs;
    std::map<void*, Response> sync_responses;

    void networkThreadFunc();
    void processNewRequests();
    void processCompletedRequests();
    std::unique_ptr<NetworkRequest> createRequest(const UString& url, AsyncCallback callback,
                                                  const RequestOptions& options);
    void setupCurlHandle(CURL* handle, NetworkRequest* request);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

// allocate the global Bancho here, early in engine startup
// it's a bit out of place but that's fine
extern std::unique_ptr<Bancho> bancho;
