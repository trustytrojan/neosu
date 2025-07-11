#pragma once
#include "UString.h"

#include <memory>
#include <mutex>

struct Bancho;
class NetworkHandler {
   public:
    NetworkHandler();
    ~NetworkHandler();

    NetworkHandler(const NetworkHandler&) = delete;
    NetworkHandler& operator=(const NetworkHandler&) = delete;
    NetworkHandler(NetworkHandler&&) = delete;
    NetworkHandler& operator=(NetworkHandler&&) = delete;

    // curl stuff
    UString httpGet(const UString &url, long timeout = 5, long connectTimeout = 5);
    std::string httpDownload(const UString &url, long timeout = 60, long connectTimeout = 5);
    std::string curlReadBuffer;

   private:
    static size_t curlStringWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t curlStringStreamWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static std::once_flag curl_init_flag;
    static std::unique_ptr<Bancho> s_banchoInstance;
};

extern Bancho* bancho;
