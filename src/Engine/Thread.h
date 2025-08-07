#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef THREAD_H
#define THREAD_H

#include "UString.h"

#if defined(_WIN32) && defined(WINVER) && (WINVER >= 0x0A00)
#include "WinDebloatDefs.h"
#include <processthreadsapi.h>
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#else
#include <pthread.h>
#endif

namespace McThread {
// WARNING: must be called from within the thread itself! otherwise, the main process name will be changed
static inline bool set_current_thread_name([[maybe_unused]] const UString &name) {
#if defined(_WIN32) && defined(WINVER) && (WINVER >= 0x0A00)
    HANDLE handle = GetCurrentThread();
    HRESULT hr = SetThreadDescription(handle, name.wc_str());
    return SUCCEEDED(hr);
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

#endif
