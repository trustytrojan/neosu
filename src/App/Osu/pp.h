#pragma once
#include "cbase.h"

class DatabaseBeatmap;

extern "C" {
// rosu-pp bindings start
typedef struct {
    f64 total_stars;
    f64 aim_stars;
    f64 speed_stars;
    f64 pp;
    u32 num_objects;
    u32 num_circles;
    u32 num_spinners;
    bool ok;
} pp_info;

pp_info calculate_full_pp(const char* path, u32 mod_flags, f32 ar, f32 cs, f32 od, f64 speed_multiplier);

typedef struct gradual_pp gradual_pp;
typedef struct {
    f64 stars;
    f64 pp;
    gradual_pp* map;
} pp_output;

gradual_pp* load_map(const char* path, u32 mod_flags, f32 ar, f32 cs, f32 od, f64 speed_multiplier);
pp_output process_map(gradual_pp* map, size_t cur_hitobject, u32 max_combo, u32 num_300, u32 num_100, u32 num_50,
                      u32 num_misses);
void free_map(gradual_pp* map);
// rosu-pp bindings end
}

// These functions are blocking; you are expected to run them in a separate thread.
gradual_pp* init_gradual_pp(DatabaseBeatmap* diff, u32 mod_flags, f32 ar, f32 cs, f32 od, f64 speed_multiplier);
gradual_pp* calculate_gradual_pp(gradual_pp* pp, i32 cur_hitobject, i32 max_combo, i32 num_300, i32 num_100, i32 num_50,
                                 i32 num_misses, f64* stars_out, f64* pp_out);
void free_gradual_pp(gradual_pp* pp);
