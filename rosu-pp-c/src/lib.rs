use rosu_pp::osu::{OsuGradualPerformance, OsuScoreState};
use std::path::Path;
use std::ptr;

use libc;
use rosu_pp;

#[repr(C)]
pub struct pp_info {
    pub total_stars: f64,
    pub aim_stars: f64,
    pub speed_stars: f64,

    pub pp: f64,

    pub num_objects: u32,
    pub num_circles: u32,
    pub num_spinners: u32,

    pub aim_strains: *mut f64,
    pub aim_strains_len: usize,
    pub aim_strains_capacity: usize,
    pub speed_strains: *mut f64,
    pub speed_strains_len: usize,
    pub speed_strains_capacity: usize,

    pub ok: bool,
}

impl Default for pp_info {
    fn default() -> Self {
        pp_info {
            total_stars: 0.0,
            aim_stars: 0.0,
            speed_stars: 0.0,

            pp: 0.0,

            num_objects: 0,
            num_circles: 0,
            num_spinners: 0,

            aim_strains: std::ptr::null_mut(),
            aim_strains_len: 0,
            aim_strains_capacity: 0,
            speed_strains: std::ptr::null_mut(),
            speed_strains_len: 0,
            speed_strains_capacity: 0,

            ok: false,
        }
    }
}

#[no_mangle]
pub extern "C" fn free_pp_info(info: pp_info) {
    if !info.aim_strains.is_null() {
        unsafe {
            Vec::from_raw_parts(info.aim_strains, info.aim_strains_len, info.aim_strains_capacity);
        }
    }
    if !info.speed_strains.is_null() {
        unsafe {
            Vec::from_raw_parts(info.speed_strains, info.speed_strains_len, info.speed_strains_capacity);
        }
    }
}

#[no_mangle]
pub extern "C" fn calculate_full_pp(path: *const libc::c_char, mod_flags: u32, ar: f32, cs: f32, od: f32, speed_multiplier: f64) -> pp_info {
    let c_path = unsafe { std::ffi::CStr::from_ptr(path) };

    let path = Path::new(c_path.to_str().unwrap());
    let map = match rosu_pp::Beatmap::from_path(path) {
        Ok(val) => val,
        Err(_) => return pp_info::default()
    };

    let osu_map = match rosu_pp::osu::OsuBeatmap::try_from_owned(map) {
        Ok(val) => val,
        Err(_) => return pp_info::default()
    };

    let diff = rosu_pp::Difficulty::new()
        .mods(mod_flags)
        .ar(ar, false)
        .cs(cs, false)
        .od(od, false)
        .clock_rate(speed_multiplier);

    let diff_attrs = diff.with_mode().calculate(&osu_map);
    let diff_strains = diff.with_mode().strains(&osu_map);
    let mut aim_strains = std::mem::ManuallyDrop::new(diff_strains.aim);
    let mut speed_strains = std::mem::ManuallyDrop::new(diff_strains.speed);

    let mut result = pp_info {
        total_stars: diff_attrs.stars,
        aim_stars: diff_attrs.aim,
        speed_stars: diff_attrs.speed,

        pp: 0.0,

        num_objects: diff_attrs.n_circles + diff_attrs.n_sliders + diff_attrs.n_spinners,
        num_circles: diff_attrs.n_circles,
        num_spinners: diff_attrs.n_spinners,

        aim_strains: aim_strains.as_mut_ptr(),
        aim_strains_len: aim_strains.len(),
        aim_strains_capacity: aim_strains.capacity(),
        speed_strains: speed_strains.as_mut_ptr(),
        speed_strains_len: speed_strains.len(),
        speed_strains_capacity: speed_strains.capacity(),

        ok: true,
    };

    result.pp = rosu_pp::Performance::new(diff_attrs)
        .mods(mod_flags)
        .ar(ar, false)
        .cs(cs, false)
        .od(od, false)
        .clock_rate(speed_multiplier)
        .calculate()
        .pp();

    return result;
}

#[repr(C)]
pub struct pp_output {
    pub stars: f64,
    pub pp: f64,
    pub map: *mut gradual_data,
}

struct gradual_data {
    gradual: OsuGradualPerformance,
    last_hitobject_plus_1: usize
}

#[no_mangle]
pub extern "C" fn load_map(path: *const libc::c_char, mod_flags: u32, ar: f32, cs: f32, od: f32, speed_multiplier: f64) -> *mut gradual_data {
    let c_path = unsafe { std::ffi::CStr::from_ptr(path) };

    let path = Path::new(c_path.to_str().unwrap());
    let map = match rosu_pp::Beatmap::from_path(path) {
        Ok(val) => val,
        Err(_) => return ptr::null_mut()
    };

    let osu_map = match rosu_pp::osu::OsuBeatmap::try_from_owned(map) {
        Ok(val) => val,
        Err(_) => return ptr::null_mut()
    };

    let gradual = rosu_pp::Difficulty::new()
        .mods(mod_flags)
        .ar(ar, false)
        .cs(cs, false)
        .od(od, false)
        .clock_rate(speed_multiplier)
        .with_mode()
        .gradual_performance(&osu_map);

    return Box::into_raw(Box::new(gradual_data {
        gradual: gradual,
        last_hitobject_plus_1: 0,
    }));
}

#[no_mangle]
pub extern "C" fn process_map(gradual_ptr: *mut gradual_data, cur_hitobject: usize, max_combo: u32, num_300: u32, num_100: u32, num_50: u32, num_misses: u32) -> pp_output {
    if gradual_ptr.is_null() {
        return pp_output {
            stars: 0.0,
            pp: 0.0,
            map: ptr::null_mut()
        };
    }

    let data = unsafe { &mut *gradual_ptr };
    let mut state = OsuScoreState::new();
    state.max_combo = max_combo;
    state.n300 = num_300;
    state.n100 = num_100;
    state.n50 = num_50;
    state.misses = num_misses;

    match data.gradual.nth(state, cur_hitobject - data.last_hitobject_plus_1) {
        Some(attrs) => {
            data.last_hitobject_plus_1 = cur_hitobject + 1;
            return pp_output {
                stars: attrs.stars(),
                pp: attrs.pp(),
                map: gradual_ptr
            };
        },
        None => {
            unsafe { drop(Box::from_raw(gradual_ptr)); }
            return pp_output {
                stars: 0.0,
                pp: 0.0,
                map: ptr::null_mut()
            };
        }
    }
}

#[no_mangle]
pub extern "C" fn free_map(gradual_ptr: *mut gradual_data) {
    if !gradual_ptr.is_null() {
        unsafe { drop(Box::from_raw(gradual_ptr)); }
    }
}
