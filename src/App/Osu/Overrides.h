#pragma once
#include "UString.h"

struct MapOverrides {
    MD5Hash map_md5;
    i16 local_offset = 0;
    i16 online_offset = 0;
    u16 nb_circles = 0;
    u16 nb_sliders = 0;
    u16 nb_spinners = 0;
    f64 star_rating = 0.0;
    i32 min_bpm = 0;
    i32 max_bpm = 0;
    i32 avg_bpm = 0;
    u8 draw_background = 1;
};
