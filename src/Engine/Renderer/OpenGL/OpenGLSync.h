#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef OPENGLSYNC_H
#define OPENGLSYNC_H

#include "cbase.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)
#include <deque>

typedef struct __GLsync *GLsync;

class OpenGLSync final {
   public:
    OpenGLSync();
    ~OpenGLSync();

    bool init();  // call once after OpenGL is loaded

    void begin();  // call at the beginning of beginScene()
    void end();    // call in endScene()

   private:
    enum SYNC_RESULT {
        SYNC_OBJECT_NOT_READY,  // sync object not created or already signaled
        SYNC_ALREADY_SIGNALED,  // GPU already done with the work
        SYNC_TIMEOUT_EXPIRED,   // waited but timed out
        SYNC_WAIT_FAILED,       // error during waiting
        SYNC_GPU_COMPLETED      // GPU just completed the work during this wait
    };

    SYNC_RESULT waitForSyncObject(GLsync syncObject,
                                  uint64_t timeoutNs);  // wait for a specific sync object to be reached
    void deleteSyncObject(GLsync syncObject);           // delete a sync object
    void setMaxFramesInFlight(int maxFrames);           // set maximum frames in flight (default: 2)
    [[nodiscard]] int getMaxFramesInFlight() const {
        return this->iMaxFramesInFlight;
    }  // get current maximum frames in flight
    [[nodiscard]] int getCurrentFramesInFlight() const {
        return this->frameSyncQueue.size();
    }  // get actual frames in flight
    void manageFrameSyncQueue(bool forceWait = false);  // Clean up expired sync objects and limit frames in flight

    typedef struct {
        GLsync syncObject;
        unsigned int frameNumber;
    } FrameSyncPoint;

    std::deque<FrameSyncPoint> frameSyncQueue;  // queue of frame sync points
    int iMaxFramesInFlight;                     // maximum allowed frames in flight
    unsigned int iSyncFrameCount;               // counter for sync-related tracking
    bool bEnabled;                              // enabledness
    bool bAvailable;                            // if the opengl feature is even available at all

    // callbacks
    inline void onSyncBehaviorChanged(const float newValue) {
        this->bEnabled = (this->init() && !!static_cast<int>(newValue));
    }
    void onFramecountNumChanged(const float newValue);
};

#endif
#endif