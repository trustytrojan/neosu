//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		asynchronous resource loading system
//
// $NoKeywords: $arl
//===============================================================================//

#include "AsyncResourceLoader.h"

#include <algorithm>

#include "App.h"
#include "ConVar.h"
#include "Engine.h"
#include "Thread.h"

using namespace std::chrono_literals;

//==================================
// LOADER THREAD
//==================================
class AsyncResourceLoader::LoaderThread final {
   public:
    std::unique_ptr<McThread> thread;
    AsyncResourceLoader *loader{};
    size_t threadIndex{};
    std::chrono::steady_clock::time_point lastWorkTime{};

    LoaderThread(AsyncResourceLoader *loader_ptr, size_t index) : loader(loader_ptr), threadIndex(index) {}
};

namespace {
void *asyncResourceLoaderThread(void *data, std::stop_token stopToken) {
    auto *loaderThread = static_cast<AsyncResourceLoader::LoaderThread *>(data);
    AsyncResourceLoader *loader = loaderThread->loader;
    const size_t threadIndex = loaderThread->threadIndex;

    loaderThread->lastWorkTime = std::chrono::steady_clock::now();
    loader->iActiveThreadCount.fetch_add(1);

    if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Thread #{} started\n", threadIndex);

    while(!stopToken.stop_requested() && !loader->bShuttingDown.load()) {
        // yield in case we're sharing a logical CPU, like on a single-core system
        Timing::sleep(0);

        auto work = loader->getNextPendingWork();

        if(!work) {
            std::unique_lock<std::mutex> lock(loader->workAvailableMutex);

            std::stop_callback stopCallback(stopToken, [&]() { loader->workAvailable.notify_all(); });

            auto waitTime = loader->threadIdleTimeout;
            bool workAvailable = loader->workAvailable.wait_for(lock, waitTime, [&]() {
                return stopToken.stop_requested() || loader->bShuttingDown.load() ||
                       loader->iActiveWorkCount.load() > 0;
            });

            if(!workAvailable && !(app && app->isInCriticalInteractiveSession())) {
                if(cv_debug_rm.getBool())
                    debugLogF("AsyncResourceLoader: Thread #{} terminating due to idle timeout\n", threadIndex);

                loaderThread->thread->requestStop();
                break;
            }

            if(stopToken.stop_requested() || loader->bShuttingDown.load()) break;

            continue;
        }

        loaderThread->lastWorkTime = std::chrono::steady_clock::now();

        Resource *resource = work->resource;
        work->state.store(AsyncResourceLoader::WorkState::ASYNC_IN_PROGRESS);

        if(cv_debug_rm.getBool())
            debugLogF("AsyncResourceLoader: Thread #{} loading {:s}\n", threadIndex, resource->getName());

        resource->loadAsync();

        work->state.store(AsyncResourceLoader::WorkState::ASYNC_COMPLETE);
        loader->markWorkAsyncComplete(std::move(work));

        if(cv_debug_rm.getBool())
            debugLogF("AsyncResourceLoader: Thread #{} finished async loading {:s}\n", threadIndex, resource->getName());
    }

    loader->iActiveThreadCount.fetch_sub(1);

    if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Thread #{} exiting\n", threadIndex);

    return nullptr;
}
}  // namespace

//==================================
// ASYNC RESOURCE LOADER
//==================================

AsyncResourceLoader::AsyncResourceLoader() {
    this->iMaxThreads = std::clamp(std::thread::hardware_concurrency() - 1, MIN_NUM_THREADS, 32u);
    this->threadIdleTimeout = THREAD_IDLE_TIMEOUT;

    // create initial threads
    for(size_t i = 0; i < MIN_NUM_THREADS; i++) {
        auto loaderThread = std::make_unique<LoaderThread>(this, this->iTotalThreadsCreated.fetch_add(1));

        loaderThread->thread = std::make_unique<McThread>(asyncResourceLoaderThread, loaderThread.get());
        if(!loaderThread->thread->isReady()) {
            engine->showMessageError("AsyncResourceLoader Error", "Couldn't create core thread!");
        } else {
            std::scoped_lock lock(this->threadsMutex);
            this->threads.push_back(std::move(loaderThread));
        }
    }
}

AsyncResourceLoader::~AsyncResourceLoader() { shutdown(); }

void AsyncResourceLoader::shutdown() {
    this->bShuttingDown = true;

    // request all this->threads to stop
    {
        std::scoped_lock lock(this->threadsMutex);
        for(auto &thread : this->threads) {
            if(thread->thread) thread->thread->requestStop();
        }
    }

    // wake up all this->threads so they can exit
    this->workAvailable.notify_all();

    // wait for this->threads to stop
    {
        std::scoped_lock lock(this->threadsMutex);
        this->threads.clear();
    }

    // cleanup remaining work items
    {
        std::scoped_lock lock(this->workQueueMutex);
        while(!this->pendingWork.empty()) {
            this->pendingWork.pop();
        }
        while(!this->asyncCompleteWork.empty()) {
            this->asyncCompleteWork.pop();
        }
    }

    // cleanup loading resources tracking
    {
        std::scoped_lock lock(this->loadingResourcesMutex);
        this->loadingResourcesSet.clear();
    }

    // cleanup async destroy queue
    for(auto &rs : this->asyncDestroyQueue) {
        SAFE_DELETE(rs);
    }
    this->asyncDestroyQueue.clear();
}

void AsyncResourceLoader::requestAsyncLoad(Resource *resource) {
    auto work = std::make_unique<LoadingWork>(resource, this->iWorkIdCounter.fetch_add(1));

    // add to tracking set
    {
        std::scoped_lock lock(this->loadingResourcesMutex);
        this->loadingResourcesSet.insert(resource);
    }

    // add to work queue
    {
        std::scoped_lock lock(this->workQueueMutex);
        this->pendingWork.push(std::move(work));
    }

    this->iActiveWorkCount.fetch_add(1);
    ensureThreadAvailable();
    this->workAvailable.notify_one();
}

void AsyncResourceLoader::update(bool lowLatency) {
    if(!lowLatency) cleanupIdleThreads();

    const size_t amountToProcess = lowLatency ? 1 : this->iMaxThreads;

    // process completed async work
    size_t numProcessed = 0;

    while(numProcessed < amountToProcess) {
        auto work = getNextAsyncCompleteWork();
        if(!work) break;

        Resource *rs = work->resource;

        if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Sync init for {:s}\n", rs->getName());

        rs->load();
        work->state.store(WorkState::SYNC_COMPLETE);

        // remove from tracking set
        {
            std::scoped_lock lock(this->loadingResourcesMutex);
            this->loadingResourcesSet.erase(rs);
        }

        this->iActiveWorkCount.fetch_sub(1);
        numProcessed++;

        // work will be automatically destroyed when unique_ptr goes out of scope
    }

    // process async destroy queue
    std::vector<Resource *> resourcesReadyForDestroy;

    {
        std::scoped_lock lock(asyncDestroyMutex);
        for(size_t i = 0; i < this->asyncDestroyQueue.size(); i++) {
            bool canBeDestroyed = true;

            {
                std::scoped_lock loadingLock(this->loadingResourcesMutex);
                if(this->loadingResourcesSet.find(this->asyncDestroyQueue[i]) != this->loadingResourcesSet.end()) {
                    canBeDestroyed = false;
                }
            }

            if(canBeDestroyed) {
                resourcesReadyForDestroy.push_back(this->asyncDestroyQueue[i]);
                this->asyncDestroyQueue.erase(this->asyncDestroyQueue.begin() + i);
                i--;
            }
        }
    }

    for(Resource *rs : resourcesReadyForDestroy) {
        if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Async destroy of resource {:s}\n", rs->getName());

        SAFE_DELETE(rs);
    }
}

void AsyncResourceLoader::scheduleAsyncDestroy(Resource *resource) {
    if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Scheduled async destroy of {:s}\n", resource->getName());

    std::scoped_lock lock(asyncDestroyMutex);
    this->asyncDestroyQueue.push_back(resource);
}

void AsyncResourceLoader::reloadResources(const std::vector<Resource *> &resources) {
    if(resources.empty()) {
        if(cv_debug_rm.getBool())
            debugLogF("AsyncResourceLoader Warning: reloadResources with empty resources vector!\n");
        return;
    }

    if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Async reloading {} resources\n", resources.size());

    std::vector<Resource *> resourcesToReload;
    for(Resource *rs : resources) {
        if(rs == nullptr) continue;

        if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader: Async reloading {:s}\n", rs->getName());

        bool isBeingLoaded = isLoadingResource(rs);

        if(!isBeingLoaded) {
            rs->release();
            resourcesToReload.push_back(rs);
        } else if(cv_debug_rm.getBool()) {
            debugLogF("AsyncResourceLoader: Resource {:s} is currently being loaded, skipping reload\n", rs->getName());
        }
    }

    for(Resource *rs : resourcesToReload) {
        requestAsyncLoad(rs);
    }
}

bool AsyncResourceLoader::isLoadingResource(Resource *resource) const {
    std::scoped_lock lock(this->loadingResourcesMutex);
    return this->loadingResourcesSet.find(resource) != this->loadingResourcesSet.end();
}

void AsyncResourceLoader::ensureThreadAvailable() {
    size_t activeThreads = this->iActiveThreadCount.load();
    size_t activeWorkCount = this->iActiveWorkCount.load();

    if(activeWorkCount > activeThreads && activeThreads < this->iMaxThreads) {
        std::scoped_lock lock(this->threadsMutex);

        if(this->threads.size() < this->iMaxThreads) {
            auto loaderThread = std::make_unique<LoaderThread>(this, this->iTotalThreadsCreated.fetch_add(1));

            loaderThread->thread = std::make_unique<McThread>(asyncResourceLoaderThread, loaderThread.get());
            if(!loaderThread->thread->isReady()) {
                if(cv_debug_rm.getBool()) debugLogF("AsyncResourceLoader Warning: Couldn't create dynamic thread!\n");
            } else {
                if(cv_debug_rm.getBool())
                    debugLogF("AsyncResourceLoader: Created dynamic thread #{} (total: {})\n", loaderThread->threadIndex,
                             this->threads.size() + 1);

                this->threads.push_back(std::move(loaderThread));
            }
        }
    }
}

void AsyncResourceLoader::cleanupIdleThreads() {
    if(this->threads.size() == 0) return;

    std::scoped_lock lock(this->threadsMutex);

    if(this->threads.size() == 0) return;

    // always remove this->threads that have requested stop, regardless of minimum
    // the minimum will be maintained by ensureThreadAvailable() when needed
    std::erase_if(this->threads, [&](const std::unique_ptr<LoaderThread> &thread) {
        if(thread->thread && thread->thread->isStopRequested()) {
            if(cv_debug_rm.getBool())
                Engine::logRaw("[cleanupIdleThreads] AsyncResourceLoader: Cleaning up thread #{}\n",
                               thread->threadIndex);
            return true;
        }
        return false;
    });
}

std::unique_ptr<AsyncResourceLoader::LoadingWork> AsyncResourceLoader::getNextPendingWork() {
    std::scoped_lock lock(this->workQueueMutex);

    if(this->pendingWork.empty()) return nullptr;

    auto work = std::move(this->pendingWork.front());
    this->pendingWork.pop();
    return work;
}

void AsyncResourceLoader::markWorkAsyncComplete(std::unique_ptr<LoadingWork> work) {
    std::scoped_lock lock(this->workQueueMutex);
    this->asyncCompleteWork.push(std::move(work));
}

std::unique_ptr<AsyncResourceLoader::LoadingWork> AsyncResourceLoader::getNextAsyncCompleteWork() {
    std::scoped_lock lock(this->workQueueMutex);

    if(this->asyncCompleteWork.empty()) return nullptr;

    auto work = std::move(this->asyncCompleteWork.front());
    this->asyncCompleteWork.pop();
    return work;
}
