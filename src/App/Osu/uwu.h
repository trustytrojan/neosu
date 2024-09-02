#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

// Unified Wait Utilities (◕‿◕✿)
namespace uwu {

// Promise for queuing work, but only the latest function will be run.
template <typename Func, typename Ret>
struct lazy_promise {
    lazy_promise(Ret default_ret) {
        m_ret = default_ret;
        m_thread = std::thread(&lazy_promise::run, this);
    }

    ~lazy_promise() {
        keep_running = false;
        m_cv.notify_one();
        m_thread.join();
    }

    void enqueue(Func func) {
        m_funcs_mtx.lock();
        m_funcs.push_back(func);
        m_funcs_mtx.unlock();
        m_cv.notify_one();
    }

    Ret get() {
        m_ret_mtx.lock();
        Ret out = m_ret;
        m_ret_mtx.unlock();
        return out;
    }
    void set(Ret ret) {
        m_ret_mtx.lock();
        m_ret = ret;
        m_ret_mtx.unlock();
    }

   private:
    void run() {
        for(;;) {
            std::unique_lock<std::mutex> lock(m_funcs_mtx);
            m_cv.wait(lock, [this]() { return !m_funcs.empty() || !keep_running; });
            if(!keep_running) break;
            if(m_funcs.empty()) {
                lock.unlock();
                continue;
            }

            Func func = m_funcs[m_funcs.size() - 1];
            m_funcs.clear();
            lock.unlock();

            set(func());
        }
    }

    bool keep_running = true;
    std::thread m_thread;
    std::mutex m_funcs_mtx;
    std::condition_variable m_cv;
    std::vector<Func> m_funcs;

    Ret m_ret;
    std::mutex m_ret_mtx;
};

};  // namespace uwu
