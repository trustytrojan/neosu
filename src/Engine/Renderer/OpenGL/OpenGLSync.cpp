//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		opengl explicit synchronization implementation
//
// $NoKeywords: $gls
//===============================================================================//

#include "OpenGLSync.h"
#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)
#include "OpenGLHeaders.h"

#include "ConVar.h"
#include "Engine.h"

OpenGLSync::OpenGLSync() {
    this->iSyncFrameCount = 0;
    this->bEnabled = false;
    this->iMaxFramesInFlight = cv::r_sync_max_frames.getVal<int>();

    cv::r_sync_max_frames.setCallback(SA::MakeDelegate<&OpenGLSync::onFramecountNumChanged>(this));
    cv::r_sync_enabled.setCallback(SA::MakeDelegate<&OpenGLSync::onSyncBehaviorChanged>(this));
}

OpenGLSync::~OpenGLSync() {
    if(this->bAvailable) {
        while(!this->frameSyncQueue.empty()) {
            deleteSyncObject(this->frameSyncQueue.front().syncObject);
            this->frameSyncQueue.pop_front();
        }
    }
}

bool OpenGLSync::init() {
#ifndef MCENGINE_PLATFORM_WASM
    this->bAvailable = GLVersion.major > 3 || (GLVersion.major == 3 && GLVersion.minor >= 2);
#else
    this->bAvailable = false;
#endif
    this->bEnabled = cv::r_sync_enabled.getBool() && this->bAvailable;
    return this->bEnabled;
}

void OpenGLSync::begin() {
    if(this->bEnabled) manageFrameSyncQueue();  // this will block and wait if we have too many frames in flight already
}

void OpenGLSync::end() {
    if(this->bEnabled) {
        // create a new fence sync, to be signaled when current rendering commands complete on the GPU
        GLsync syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        if(syncObj) {
            // add it to the queue
            FrameSyncPoint syncPoint;
            syncPoint.syncObject = syncObj;
            syncPoint.frameNumber = this->iSyncFrameCount++;

            this->frameSyncQueue.push_back(syncPoint);

            if(cv::r_sync_debug.getBool())
                debugLogF("Created sync object for frame {:d} (frames in flight: {:d}/{:d})\n", syncPoint.frameNumber,
                          this->frameSyncQueue.size(), this->iMaxFramesInFlight);
        }
    }
}

void OpenGLSync::manageFrameSyncQueue(bool forceWait) {
    if(this->frameSyncQueue.empty()) return;

    const bool debug = cv::r_sync_debug.getBool();

    // check front of queue to see if it's complete
    bool needToWait = forceWait || (this->frameSyncQueue.size() >= this->iMaxFramesInFlight);

    // first, try a non-blocking check on the oldest sync object
    FrameSyncPoint &oldestSync = this->frameSyncQueue.front();
    GLint signaled = 0;

    GLsync sync = oldestSync.syncObject;
    glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

    // if oldest frame is done, we can remove it (and any other completed frames)
    if(signaled == GL_SIGNALED) {
        if(debug) debugLogF("Frame {:d} already completed (no wait needed)\n", oldestSync.frameNumber);

        deleteSyncObject(oldestSync.syncObject);
        this->frameSyncQueue.pop_front();

        // check if more frames are done
        while(!this->frameSyncQueue.empty()) {
            FrameSyncPoint &nextSync = this->frameSyncQueue.front();
            signaled = 0;

            sync = nextSync.syncObject;
            glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

            if(signaled == GL_SIGNALED) {
                if(debug) debugLogF("Frame {:d} also completed\n", nextSync.frameNumber);

                deleteSyncObject(nextSync.syncObject);
                this->frameSyncQueue.pop_front();
            } else {
                break;
            }
        }
    }
    // if we need to wait and the frame isn't done yet, do a blocking wait with timeout
    else if(needToWait) {
        if(debug)
            debugLogF("Waiting for frame {:d} to complete (frames in flight: {:d}/{:d})\n", oldestSync.frameNumber,
                      this->frameSyncQueue.size(), this->iMaxFramesInFlight);

        SYNC_RESULT result = waitForSyncObject(oldestSync.syncObject, cv::r_sync_timeout.getInt());

        if(debug) {
            switch(result) {
                case SYNC_OBJECT_NOT_READY:
                    debugLogF("Frame {:d} sync object was not ready\n", oldestSync.frameNumber);
                    break;
                case SYNC_ALREADY_SIGNALED:
                    debugLogF("Frame {:d} sync object was already signaled\n", oldestSync.frameNumber);
                    break;
                case SYNC_GPU_COMPLETED:
                    debugLogF("Frame {:d} sync object was just completed\n", oldestSync.frameNumber);
                    break;
                case SYNC_TIMEOUT_EXPIRED:
                    debugLogF("Frame {:d} sync object timed out after {:d} microseconds\n", oldestSync.frameNumber,
                              cv::r_sync_timeout.getInt());
                    break;
                case SYNC_WAIT_FAILED:
                    debugLogF("Frame {:d} sync wait failed\n", oldestSync.frameNumber);
                    break;
            }
        }

        // always remove the sync object we waited for, even if it timed out (don't get stuck if the GPU falls far
        // behind)
        deleteSyncObject(oldestSync.syncObject);
        this->frameSyncQueue.pop_front();

        // check if other frames are done too
        if(result == SYNC_ALREADY_SIGNALED || result == SYNC_GPU_COMPLETED) {
            while(!this->frameSyncQueue.empty()) {
                FrameSyncPoint &nextSync = this->frameSyncQueue.front();
                signaled = 0;

                sync = nextSync.syncObject;
                glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

                if(signaled == GL_SIGNALED) {
                    if(debug) debugLogF("Frame {:d} also completed\n", nextSync.frameNumber);

                    deleteSyncObject(nextSync.syncObject);
                    this->frameSyncQueue.pop_front();
                } else {
                    break;
                }
            }
        }
    }
}

// wait for a sync object with timeout
OpenGLSync::SYNC_RESULT OpenGLSync::waitForSyncObject(GLsync syncObject, uint64_t timeoutNs) {
    if(!syncObject) return SYNC_OBJECT_NOT_READY;

    GLsync sync = syncObject;

    // do a non-blocking check if already signaled
    GLint signaled = 0;
    glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &signaled);

    if(signaled == GL_SIGNALED) return SYNC_ALREADY_SIGNALED;

    // wait with timeout
    GLenum result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeoutNs);

    switch(result) {
        case GL_ALREADY_SIGNALED:
            return SYNC_ALREADY_SIGNALED;
        case GL_TIMEOUT_EXPIRED:
            return SYNC_TIMEOUT_EXPIRED;
        case GL_WAIT_FAILED:
            return SYNC_WAIT_FAILED;
        case GL_CONDITION_SATISFIED:
            return SYNC_GPU_COMPLETED;
        default:
            return SYNC_WAIT_FAILED;
    }
}

void OpenGLSync::deleteSyncObject(GLsync syncObject) {
    if(syncObject) glDeleteSync(syncObject);
}

void OpenGLSync::setMaxFramesInFlight(int maxFrames) {
    this->iMaxFramesInFlight = maxFrames;

    // may need to wait on some frames if the limit is being reduced
    if(this->bEnabled && this->frameSyncQueue.size() > this->iMaxFramesInFlight) {
        if(cv::r_sync_debug.getBool())
            debugLogF("Max frames reduced to {:d}, waiting for excess frames\n", this->iMaxFramesInFlight);
        // wait
        manageFrameSyncQueue(true);
    }
}

#endif