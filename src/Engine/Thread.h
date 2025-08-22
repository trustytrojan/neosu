#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef THREAD_H
#define THREAD_H

class UString;

namespace McThread {
// WARNING: must be called from within the thread itself! otherwise, the main process name will be changed
bool set_current_thread_name(const UString &name);
}

#endif
