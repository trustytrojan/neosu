#include "Replay.h"

#include <lzma.h>

#include "Bancho.h"
#include "BanchoProtocol.h"
#include "Beatmap.h"
#include "Database.h"
#include "Engine.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "SongBrowser.h"
#include "score.h"

Replay::BEATMAP_VALUES Replay::getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS,
                                                             float legacyOD, float legacyHP) {
    Replay::BEATMAP_VALUES v;

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

std::vector<Replay::Frame> Replay::get_frames(uint8_t* replay_data, int32_t replay_size) {
    std::vector<Replay::Frame> replay_frames;
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
        Replay::Frame frame;
        sscanf(frame_str.c_str(), "%lld|%f|%f|%hhu", &frame.milliseconds_since_last_frame, &frame.x, &frame.y,
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

void Replay::compress_frames(const std::vector<Replay::Frame>& frames, uint8_t** compressed, size_t* s_compressed) {
    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_options_lzma options;
    lzma_lzma_preset(&options, LZMA_PRESET_DEFAULT);
    lzma_ret ret = lzma_alone_encoder(&stream, &options);
    if(ret != LZMA_OK) {
        debugLog("Failed to initialize lzma encoder: error %d\n", ret);
        *compressed = NULL;
        *s_compressed = 0;
        return;
    }

    std::string replay_string;
    for(auto frame : frames) {
        auto frame_str = UString::format("%lld|%.4f|%.4f|%hhu,", frame.milliseconds_since_last_frame, frame.x, frame.y,
                                         frame.key_flags);
        replay_string.append(frame_str.toUtf8(), frame_str.lengthUtf8());
    }

    // osu!stable doesn't consider a replay valid unless it ends with this
    replay_string.append("-12345|0.0000|0.0000|0,");

    *s_compressed = replay_string.length();
    *compressed = (uint8_t*)malloc(*s_compressed);

    stream.avail_in = replay_string.length();
    stream.next_in = (const uint8_t*)replay_string.c_str();
    stream.avail_out = *s_compressed;
    stream.next_out = *compressed;
    do {
        ret = lzma_code(&stream, LZMA_FINISH);
        if(ret == LZMA_OK) {
            *s_compressed *= 2;
            *compressed = (uint8_t*)realloc(*compressed, *s_compressed);
            stream.avail_out = *s_compressed - stream.total_out;
            stream.next_out = *compressed + stream.total_out;
        } else if(ret != LZMA_STREAM_END) {
            debugLog("Error while compressing replay: error %d\n", ret);
            *compressed = NULL;
            *s_compressed = 0;
            lzma_end(&stream);
            return;
        }
    } while(ret != LZMA_STREAM_END);

    *s_compressed = stream.total_out;
    lzma_end(&stream);
}

Replay::Info Replay::from_bytes(uint8_t* data, int s_data) {
    Replay::Info info;

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
    info.frames = Replay::get_frames(replay_data, replay_size);
    delete[] replay_data;

    return info;
}

bool Replay::load_from_disk(FinishedScore* score) {
    if(score->legacyReplayTimestamp > 0) {
        auto osu_folder = convar->getConVarByName("osu_folder")->getString();
        auto path = UString::format("%s/Data/r/%s-%llu.osr", osu_folder.toUtf8(), score->md5hash.hash,
                                    score->legacyReplayTimestamp);

        FILE* replay_file = fopen(path.toUtf8(), "rb");
        if(replay_file == NULL) return false;

        fseek(replay_file, 0, SEEK_END);
        size_t s_full_replay = ftell(replay_file);
        rewind(replay_file);

        uint8_t* full_replay = new uint8_t[s_full_replay];
        fread(full_replay, s_full_replay, 1, replay_file);
        fclose(replay_file);
        auto info = Replay::from_bytes(full_replay, s_full_replay);
        score->replay = info.frames;
        delete[] full_replay;
    } else {
        auto path = UString::format(MCENGINE_DATA_DIR "replays/%s/%llu.replay.lzma", score->server.c_str(),
                                    score->unixTimestamp);

        FILE* replay_file = fopen(path.toUtf8(), "rb");
        if(replay_file == NULL) return false;

        fseek(replay_file, 0, SEEK_END);
        size_t s_compressed_replay = ftell(replay_file);
        rewind(replay_file);

        uint8_t* compressed_replay = new uint8_t[s_compressed_replay];
        fread(compressed_replay, s_compressed_replay, 1, replay_file);
        fclose(replay_file);
        score->replay = Replay::get_frames(compressed_replay, s_compressed_replay);
        delete[] compressed_replay;
    }

    auto& map_scores = (*(bancho.osu->getSongBrowser()->getDatabase()->getScores()))[score->md5hash];
    for(auto& db_score : map_scores) {
        if(db_score.unixTimestamp != score->unixTimestamp) continue;
        if(&db_score != score) {
            db_score.replay = score->replay;
        }

        break;
    }

    return true;
}

void Replay::load_and_watch(FinishedScore score) {
    // Check if replay is loaded
    if(score.replay.empty()) {
        if(!load_from_disk(&score)) {
            if(strcmp(score.server.c_str(), bancho.endpoint.toUtf8()) != 0) {
                auto msg = UString::format("Please connect to %s to view this replay!", score.server.c_str());
                bancho.osu->m_notificationOverlay->addNotification(msg);
            }

            // Need to allocate with calloc since APIRequests free() the .extra
            void* mem = calloc(1, sizeof(FinishedScore));
            FinishedScore* score_cpy = new(mem) FinishedScore;
            *score_cpy = score;

            APIRequest request;
            request.type = GET_REPLAY;
            request.path = UString::format("/web/osu-getreplay.php?u=%s&h=%s&m=0&c=%d", bancho.username.toUtf8(),
                                           bancho.pw_md5.toUtf8(), score.online_score_id);
            request.mime = NULL;
            request.extra = (uint8_t*)score_cpy;
            send_api_request(request);

            bancho.osu->m_notificationOverlay->addNotification("Downloading replay...");
            return;
        }
    }

    // We tried loading from memory, we tried loading from file, we tried loading from server... RIP
    if(score.replay.empty()) {
        bancho.osu->m_notificationOverlay->addNotification("Failed to load replay");
        return;
    }

    auto beatmap = bancho.osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(score.md5hash.hash);
    if(beatmap == nullptr) {
        // XXX: Auto-download beatmap
        bancho.osu->m_notificationOverlay->addNotification("Missing beatmap for this replay");
    } else {
        bancho.osu->getSongBrowser()->onDifficultySelected(beatmap, false);
        bancho.osu->getSelectedBeatmap()->watch(score, 0.f);
    }
}
