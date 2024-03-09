#pragma once
#include "cbase.h"

class NetworkHandler {
   public:
    NetworkHandler();
    ~NetworkHandler();

    // curl stuff
    UString httpGet(UString url, long timeout = 5, long connectTimeout = 5);
    std::string httpDownload(UString url, long timeout = 60, long connectTimeout = 5);
    std::string curlReadBuffer;

   private:
    static size_t curlStringWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t curlStringStreamWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
};
