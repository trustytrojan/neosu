#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include "types.h"

class DatabaseBeatmap;
class VolNormalization {
   public:
    VolNormalization() = default;
    ~VolNormalization() {
        // only clean up this instance's resources
        abort_instance();
    }

    // non-copyable, non-movable
    VolNormalization(const VolNormalization&) = delete;
    VolNormalization& operator=(const VolNormalization&) = delete;
    VolNormalization(VolNormalization&&) = delete;
    VolNormalization& operator=(VolNormalization&&) = delete;

    static inline void start_calc(const std::vector<DatabaseBeatmap*>& maps_to_calc) {
        get_instance().start_calc_instance(maps_to_calc);
    }

    static inline u32 get_total() {
        auto* inst = get_instance_ptr();
        return inst ? inst->get_total_instance() : 0;
    }

    static inline u32 get_computed() {
        auto* inst = get_instance_ptr();
        return inst ? inst->get_computed_instance() : 0;
    }

    static inline void abort() {
        auto* inst = get_instance_ptr();
        if(inst) {
            inst->abort_instance();
        }
    }

    // shutdown the singleton
    static inline void shutdown() {
        std::call_once(shutdown_flag, []() {
            if(instance) {
                instance->abort_instance();
                instance.reset();
            }
        });
    }

   private:
    static VolNormalization& get_instance();
    static VolNormalization* get_instance_ptr();

    void start_calc_instance(const std::vector<DatabaseBeatmap*>& maps_to_calc);

    u32 get_total_instance();
    u32 get_computed_instance();

    void abort_instance();

    struct LoudnessCalcThread;
    std::vector<LoudnessCalcThread*> threads;

    static std::unique_ptr<VolNormalization> instance;
    static std::once_flag instance_flag;
    static std::once_flag shutdown_flag;
};
