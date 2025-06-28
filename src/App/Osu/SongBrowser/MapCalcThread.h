#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Database.h"

class DatabaseBeatmap;

class MapCalcThread {
   public:
    MapCalcThread() = default;
    ~MapCalcThread() { abort(); }

    // non-copyable, non-movable
    MapCalcThread(const MapCalcThread&) = delete;
    MapCalcThread& operator=(const MapCalcThread&) = delete;
    MapCalcThread(MapCalcThread&&) = delete;
    MapCalcThread& operator=(MapCalcThread&&) = delete;

    struct mct_result {
        DatabaseBeatmap* diff2{};
        u32 nb_circles{};
        u32 nb_sliders{};
        u32 nb_spinners{};
        f32 star_rating{};
        u32 min_bpm{};
        u32 max_bpm{};
        u32 avg_bpm{};
    };

    // FIXME: maps_to_calc copied due to mysterious lifetime issues (?)
    static inline void start_calc(std::vector<BeatmapDifficulty*> maps_to_calc) {
        get_instance().start_calc_instance(maps_to_calc);
    }
    static inline void abort() { get_instance().abort_instance(); }

    // progress tracking
    static inline bool get_computed() { return get_instance().computed_count.load(); }
    static inline bool get_total() { return get_instance().total_count.load(); }

    static inline bool is_finished() {
        const auto& instance = get_instance();
        const u32 computed = instance.computed_count.load();
        const u32 total = instance.total_count.load();
        return total > 0 && computed >= total;
    }

    // results access
    static inline std::vector<mct_result>& get_results() { return get_instance().results; }

   private:
    void run();

    void start_calc_instance(const std::vector<BeatmapDifficulty*>& maps_to_calc);
    void abort_instance();

    // singleton access
    static MapCalcThread& get_instance();

    std::thread worker_thread;
    std::atomic<bool> should_stop{true};
    std::atomic<u32> computed_count{0};
    std::atomic<u32> total_count{0};
    std::vector<mct_result> results;
    const std::vector<BeatmapDifficulty*>* maps_to_process{nullptr};

    static std::unique_ptr<MapCalcThread> instance;
    static std::once_flag instance_flag;
};
