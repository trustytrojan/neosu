//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		asynchronous resource loading system
//
// $NoKeywords: $arl
//===============================================================================//

#pragma once
#ifndef ASYNCRESOURCELOADER_H
#define ASYNCRESOURCELOADER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <vector>

#include "Resource.h"

class ConVar;
class McThread;

// everything is public because the class data should only be accessed by ResourceManager and the resource threads
// themselves
class AsyncResourceLoader final {
   public:
    AsyncResourceLoader();
    ~AsyncResourceLoader();

    // main interface for ResourceManager
    void requestAsyncLoad(Resource *resource);
    void update(bool lowLatency);
    void shutdown();

    // resource lifecycle management
    void scheduleAsyncDestroy(Resource *resource);
    void reloadResources(const std::vector<Resource *> &resources);

    // status queries
    [[nodiscard]] inline bool isLoading() const { return this->iActiveWorkCount.load() > 0; }
    [[nodiscard]] bool isLoadingResource(Resource *resource) const;
    [[nodiscard]] size_t getNumLoadingWork() const { return this->iActiveWorkCount.load(); }
    [[nodiscard]] size_t getNumActiveThreads() const { return this->iActiveThreadCount.load(); }
    [[nodiscard]] inline size_t getNumLoadingWorkAsyncDestroy() const { return this->asyncDestroyQueue.size(); }

    enum class WorkState : uint8_t { PENDING = 0, ASYNC_IN_PROGRESS = 1, ASYNC_COMPLETE = 2, SYNC_COMPLETE = 3 };

    struct LoadingWork {
        Resource *resource;
        size_t workId;
        std::atomic<WorkState> state{WorkState::PENDING};

        LoadingWork(Resource *res, size_t id) : resource(res), workId(id) {}
    };

    class LoaderThread;
    friend class LoaderThread;

    // threading configuration
    static constexpr unsigned int MIN_NUM_THREADS = 1;
    static constexpr auto THREAD_IDLE_TIMEOUT = std::chrono::seconds{5};

    // thread management
    void ensureThreadAvailable();
    void cleanupIdleThreads();

    // work queue management
    std::unique_ptr<LoadingWork> getNextPendingWork();
    void markWorkAsyncComplete(std::unique_ptr<LoadingWork> work);
    std::unique_ptr<LoadingWork> getNextAsyncCompleteWork();

    size_t iMaxThreads;
    std::chrono::seconds threadIdleTimeout{5};

    // thread pool
    std::vector<std::unique_ptr<LoaderThread>> threads;
    mutable std::mutex threadsMutex;

    // thread lifecycle tracking
    std::atomic<size_t> iActiveThreadCount{0};
    std::atomic<size_t> iTotalThreadsCreated{0};

    // separate queues for different work states (avoids O(n) scanning)
    std::queue<std::unique_ptr<LoadingWork>> pendingWork;
    std::queue<std::unique_ptr<LoadingWork>> asyncCompleteWork;

    // single mutex for both work queues (they're accessed in sequence, not concurrently)
    mutable std::mutex workQueueMutex;

    // fast lookup for checking if a resource is being loaded
    std::unordered_set<Resource *> loadingResourcesSet;
    mutable std::mutex loadingResourcesMutex;

    // atomic counters for efficient status queries
    std::atomic<size_t> iActiveWorkCount{0};
    std::atomic<size_t> iWorkIdCounter{0};

    // work notification
    std::condition_variable workAvailable;
    std::mutex workAvailableMutex;

    // async destroy queue
    std::vector<Resource *> asyncDestroyQueue;
    std::mutex asyncDestroyMutex;

    // lifecycle flags
    std::atomic<bool> bShuttingDown{false};
};

#endif
