// Copyright (c) 2025, WH, All rights reserved.

#include "Thread.h"
#include "UString.h"

#if defined(_WIN32)
#include "WinDebloatDefs.h"
#include <processthreadsapi.h>
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#include <libloaderapi.h>
#include "dynutils.h"

namespace {
dynutils::lib_obj *kernel32_handle{nullptr};
decltype(SetThreadDescription) *pset_thread_desc{nullptr};
bool stdesc_load_attempted{false};
}  // namespace

#endif
#else
#include <pthread.h>
#endif

namespace McThread {
// WARNING: must be called from within the thread itself! otherwise, the main process name will be changed
bool set_current_thread_name([[maybe_unused]] const UString &name) {
#if defined(_WIN32)
    if(!stdesc_load_attempted) {
        stdesc_load_attempted = true;
        kernel32_handle = reinterpret_cast<dynutils::lib_obj *>(
            LoadLibraryEx(TEXT("kernel32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
        if(kernel32_handle)
            pset_thread_desc = load_func<decltype(SetThreadDescription)>(kernel32_handle, "SetThreadDescription");
    }
    if(pset_thread_desc) {
        HANDLE handle = GetCurrentThread();
        HRESULT hr = pset_thread_desc(handle, name.wc_str());
        return SUCCEEDED(hr);
    }
    return false;
#elif defined(__linux__)
    auto truncated_name = name.substr<std::string>(0, 15);
    return pthread_setname_np(pthread_self(), truncated_name.c_str()) == 0;
#elif defined(__APPLE__)
    return pthread_setname_np(name.toUtf8()) == 0;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    pthread_set_name_np(pthread_self(), name.toUtf8());
    return true;
#else
    return false;
#endif
}
};  // namespace McThread
