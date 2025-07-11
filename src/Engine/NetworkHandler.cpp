#include "NetworkHandler.h"
#include "curl_blob.h"

#include <curl/curl.h>

#include <sstream>

// #include "ConVar.h"
#include "Engine.h"
#include "Bancho.h"

std::unique_ptr<Bancho> NetworkHandler::s_banchoInstance = nullptr;
Bancho *bancho = nullptr;

std::once_flag NetworkHandler::curl_init_flag;

NetworkHandler::NetworkHandler() {
    std::call_once(curl_init_flag, []() {
        // XXX: run curl_global_cleanup() after waiting for network threads to terminate
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
    s_banchoInstance = std::make_unique<Bancho>();
    bancho = s_banchoInstance.get();
    runtime_assert(bancho, "Bancho handler failed to initialize!");
}

NetworkHandler::~NetworkHandler() {
    s_banchoInstance.reset();
    bancho = nullptr;
    // XXX: run after waiting for network threads to terminate
    // curl_global_cleanup();
}

UString NetworkHandler::httpGet(const UString &url, long timeout, long connectTimeout) {
    CURL *curl = curl_easy_init();

    if(curl != NULL) {
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &this->curlReadBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

        curl_easy_setopt_CAINFO_BLOB_embedded(curl);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            debugLogF("Error while fetching {:s}: {:s}\n", url.toUtf8(), curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);

        return UString{this->curlReadBuffer};
    } else {
        debugLogF("ERROR: curl == NULL!\n");
        return {""};
    }
}

std::string NetworkHandler::httpDownload(const UString &url, long timeout, long connectTimeout) {
    CURL *curl = curl_easy_init();

    if(curl != NULL) {
        std::stringstream curlWriteBuffer(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringStreamWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlWriteBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);

        curl_easy_setopt_CAINFO_BLOB_embedded(curl);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            curlWriteBuffer = std::stringstream();
            debugLogF("ERROR ({}): {:s}\n", static_cast<unsigned int>(res), curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);

        return std::string{curlWriteBuffer.str()};
    } else {
        debugLogF("ERROR: curl == NULL!\n");
        return {""};
    }
}

size_t NetworkHandler::curlStringWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

size_t NetworkHandler::curlStringStreamWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::stringstream *)userp)->write((const char *)contents, (size_t)size * nmemb);
    return size * nmemb;
}
