#include "pp.h"

#include "Beatmap.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Osu.h"
#include "Timer.h"

gradual_pp* init_gradual_pp(DatabaseBeatmap* diff, u32 mod_flags, f32 ar, f32 cs, f32 od, f64 speed_multiplier) {
    Timer t;
    t.start();

    gradual_pp* map = load_map(diff->m_sFilePath.c_str(), mod_flags, ar, cs, od, speed_multiplier);

    t.update();
    if(Osu::debug->getBool()) {
        debugLog("Time spent loading map into rosu-pp for gradual calc: %.2fms\n", t.getElapsedTime() * 1000.f);
    }

    return map;
}

gradual_pp* calculate_gradual_pp(gradual_pp* pp, i32 cur_hitobject, i32 max_combo, i32 num_300, i32 num_100, i32 num_50,
                                 i32 num_misses, f64* stars_out, f64* pp_out) {
    Timer t;
    t.start();

    pp_output out = process_map(pp, cur_hitobject, max_combo, num_300, num_100, num_50, num_misses);

    *stars_out = out.stars;
    *pp_out = out.pp;
    osu->getSelectedBeatmap()->last_calculated_hitobject = cur_hitobject;

    t.update();
    if(Osu::debug->getBool()) {
        debugLog("Time spent calculating up to hitobject %d: %.2fms\n", cur_hitobject, t.getElapsedTime() * 1000.f);
    }

    return out.map;
}

void free_gradual_pp(gradual_pp* pp) {
    if(pp == NULL) return;
    free_map(pp);
}
