//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		replay handler & parser
//
// $NoKeywords: $osr
//===============================================================================//

#include "OsuReplay.h"

#include <lzma.h>

#include "BanchoProtocol.h"
#include "Engine.h"

OsuReplay::BEATMAP_VALUES OsuReplay::getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS,
                                                                   float legacyOD, float legacyHP) {
    OsuReplay::BEATMAP_VALUES v;

    // HACKHACK: code duplication, see Osu::getRawSpeedMultiplier()
    v.speedMultiplier = 1.0f;
    {
        if(modsLegacy & ModFlags::HalfTime) v.speedMultiplier = 0.75f;
        if((modsLegacy & ModFlags::DoubleTime) || (modsLegacy & ModFlags::Nightcore)) v.speedMultiplier = 1.5f;
    }

    // HACKHACK: code duplication, see Osu::getDifficultyMultiplier()
    v.difficultyMultiplier = 1.0f;
    {
        if(modsLegacy & ModFlags::HardRock) v.difficultyMultiplier = 1.4f;
        if(modsLegacy & ModFlags::Easy) v.difficultyMultiplier = 0.5f;
    }

    // HACKHACK: code duplication, see Osu::getCSDifficultyMultiplier()
    v.csDifficultyMultiplier = 1.0f;
    {
        if(modsLegacy & ModFlags::HardRock) v.csDifficultyMultiplier = 1.3f;  // different!
        if(modsLegacy & ModFlags::Easy) v.csDifficultyMultiplier = 0.5f;
    }

    // apply legacy mods to legacy beatmap values
    v.AR = clamp<float>(legacyAR * v.difficultyMultiplier, 0.0f, 10.0f);
    v.CS = clamp<float>(legacyCS * v.csDifficultyMultiplier, 0.0f, 10.0f);
    v.OD = clamp<float>(legacyOD * v.difficultyMultiplier, 0.0f, 10.0f);
    v.HP = clamp<float>(legacyHP * v.difficultyMultiplier, 0.0f, 10.0f);

    return v;
}

std::vector<OsuReplay::Frame> OsuReplay::get_frames(uint8_t* replay_data, int32_t replay_size) {
    std::vector<OsuReplay::Frame> replay_frames;
    if(replay_size <= 0) return replay_frames;

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);
    if(ret != LZMA_OK) {
        debugLog("Failed to init lzma library (%d).\n", ret);
        return replay_frames;
    }

    long cur_music_pos = 0;
    std::stringstream ss;
    std::string frame_str;
    uint8_t outbuf[BUFSIZ];
    strm.next_in = replay_data;
    strm.avail_in = replay_size;
    do {
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);

        ret = lzma_code(&strm, LZMA_FINISH);
        if(ret != LZMA_OK && ret != LZMA_STREAM_END) {
            debugLog("Decompression error (%d).\n", ret);
            goto end;
        }

        ss.write((const char*)outbuf, sizeof(outbuf) - strm.avail_out);
    } while(strm.avail_out == 0);

    while(std::getline(ss, frame_str, ',')) {
        OsuReplay::Frame frame;
        sscanf(frame_str.c_str(), "%ld|%f|%f|%hhu", &frame.milliseconds_since_last_frame, &frame.x, &frame.y,
               &frame.key_flags);

        if(frame.milliseconds_since_last_frame != -12345) {
            cur_music_pos += frame.milliseconds_since_last_frame;
            frame.cur_music_pos = cur_music_pos;
            replay_frames.push_back(frame);
        }
    }

end:
    lzma_end(&strm);
    return replay_frames;
}

OsuReplay::Info OsuReplay::from_bytes(uint8_t* data, int s_data) {
    OsuReplay::Info info;

    Packet replay;
    replay.memory = data;
    replay.size = s_data;

    info.gamemode = read_byte(&replay);
    if(info.gamemode != 0) {
        debugLog("Replay has unexpected gamemode %d!", info.gamemode);
        return info;
    }

    info.osu_version = read_int32(&replay);
    info.diff2_md5 = read_string(&replay);
    info.username = read_string(&replay);
    info.replay_md5 = read_string(&replay);
    info.num300s = read_short(&replay);
    info.num100s = read_short(&replay);
    info.num50s = read_short(&replay);
    info.numGekis = read_short(&replay);
    info.numKatus = read_short(&replay);
    info.numMisses = read_short(&replay);
    info.score = read_int32(&replay);
    info.comboMax = read_short(&replay);
    info.perfect = read_byte(&replay);
    info.mod_flags = read_int32(&replay);
    info.life_bar_graph = read_string(&replay);
    info.timestamp = read_int64(&replay) / 10;

    int32_t replay_size = read_int32(&replay);
    if(replay_size <= 0) return info;
    auto replay_data = new uint8_t[replay_size];
    read_bytes(&replay, replay_data, replay_size);
    info.frames = OsuReplay::get_frames(replay_data, replay_size);
    delete[] replay_data;

    return info;
}
