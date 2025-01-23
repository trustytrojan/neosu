#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

// Unified Wait Utilities (◕‿◕✿)
namespace uwu {

// Promise for queuing work, but only the latest function will be run.
template <typename Func, typename Ret>
struct lazy_promise {
    lazy_promise(Ret default_ret) {
        this->ret = default_ret;
        this->thread = std::thread(&lazy_promise::run, this);
    }

    ~lazy_promise() {
        this->keep_running = false;
        this->cv.notify_one();
        this->thread.join();
    }

    void enqueue(Func func) {
        this->funcs_mtx.lock();
        this->funcs.push_back(func);
        this->funcs_mtx.unlock();
        this->cv.notify_one();
    }

    Ret get() {
        this->ret_mtx.lock();
        Ret out = this->ret;
        this->ret_mtx.unlock();
        return out;
    }
    void set(Ret ret) {
        this->ret_mtx.lock();
        this->ret = ret;
        this->ret_mtx.unlock();
    }

   private:
    void run() {
        for(;;) {
            std::unique_lock<std::mutex> lock(this->funcs_mtx);
            this->cv.wait(lock, [this]() { return !this->funcs.empty() || !this->keep_running; });
            if(!this->keep_running) break;
            if(this->funcs.empty()) {
                lock.unlock();
                continue;
            }

            Func func = this->funcs[this->funcs.size() - 1];
            this->funcs.clear();
            lock.unlock();

            this->set(func());
        }
    }

    bool keep_running = true;
    std::thread thread;
    std::mutex funcs_mtx;
    std::condition_variable cv;
    std::vector<Func> funcs;

    Ret ret;
    std::mutex ret_mtx;
};

};  // namespace uwu
