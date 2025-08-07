#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Resource.h"

class ConVar;

// everything is public because the class data should only be accessed by ResourceManager and the resource threads
// themselves
class AsyncResourceLoader final {
   public:
    AsyncResourceLoader();
    ~AsyncResourceLoader();

    // main interface for ResourceManager
    inline void resetMaxPerUpdate() { this->bMaxLoadsResetPending = true; }
    inline void setMaxPerUpdate(size_t num) { this->iLoadsPerUpdate = std::clamp<size_t>(num, 1, 512); }
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
    [[nodiscard]] inline size_t getMaxPerUpdate() const { return this->iLoadsPerUpdate; }

    enum class WorkState : uint8_t { PENDING = 0, ASYNC_IN_PROGRESS = 1, ASYNC_COMPLETE = 2, SYNC_COMPLETE = 3 };

    struct LoadingWork {
        Resource *resource;
        size_t workId;
        std::atomic<WorkState> state{WorkState::PENDING};

        LoadingWork(Resource *res, size_t id) : resource(res), workId(id) {}
    };

    class LoaderThread;
    friend class LoaderThread;

    // thread management
    void ensureThreadAvailable();
    void cleanupIdleThreads();

    // work queue management
    std::unique_ptr<LoadingWork> getNextPendingWork();
    void markWorkAsyncComplete(std::unique_ptr<LoadingWork> work);
    std::unique_ptr<LoadingWork> getNextAsyncCompleteWork();

    // set during ctor, dependent on hardware
    size_t iMaxThreads;
    static constexpr const size_t HARD_THREADCOUNT_LIMIT{32};

    // how many resources to load on update()
    // default is == max # threads (or 1 during gameplay)
    size_t iLoadsPerUpdate;
    bool bMaxLoadsResetPending{false};

    // thread idle configuration
    static constexpr std::chrono::milliseconds IDLE_GRACE_PERIOD{1000};  // 1 sec
    static constexpr std::chrono::milliseconds IDLE_TIMEOUT{15000};      // 15 sec
    std::chrono::steady_clock::time_point lastCleanupTime;

    // thread pool
    std::unordered_map<size_t, std::unique_ptr<LoaderThread>> threadpool;  // index to thread
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
    std::condition_variable_any workAvailable;
    std::mutex workAvailableMutex;

    // async destroy queue
    std::vector<Resource *> asyncDestroyQueue;
    std::mutex asyncDestroyMutex;

    // lifecycle flags
    std::atomic<bool> bShuttingDown{false};
};
