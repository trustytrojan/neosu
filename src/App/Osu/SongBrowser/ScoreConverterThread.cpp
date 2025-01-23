#include "ScoreConverterThread.h"

#include <chrono>

#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "SimulatedBeatmap.h"
#include "score.h"

std::atomic<u32> sct_computed = 0;
std::atomic<u32> sct_total = 0;

static std::thread thr;
static std::atomic<bool> dead = true;
static std::vector<FinishedScore> scores;

static void run_sct() {
    debugLog("Started score converter thread\n");

    i32 idx = 0;
    for(auto score : scores) {
        while(osu->should_pause_background_threads.load() && !dead.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if(dead.load()) return;

        // TODO @kiwec: compute ppv2 if missing

        if(score.replay.empty()) {
            if(!LegacyReplay::load_from_disk(&score, false)) {
                debugLog("Failed to load replay for score %d\n", idx);
                sct_computed++;
                idx++;
                continue;
            }
        }

        auto diff = db->getBeatmapDifficulty(score.beatmap_hash);
        SimulatedBeatmap smap(diff, score.mods);
        smap.spectated_replay = score.replay;
        smap.simulate_to(diff->getLengthMS());

        if(score.comboMax != smap.live_score.getComboMax())
            debugLog("Score %d: comboMax was %d, simulated %d\n", idx, score.comboMax, smap.live_score.getComboMax());
        if(score.num300s != smap.live_score.getNum300s())
            debugLog("Score %d: n300 was %d, simulated %d\n", idx, score.num300s, smap.live_score.getNum300s());
        if(score.num100s != smap.live_score.getNum100s())
            debugLog("Score %d: n100 was %d, simulated %d\n", idx, score.num100s, smap.live_score.getNum100s());
        if(score.num50s != smap.live_score.getNum50s())
            debugLog("Score %d: n50 was %d, simulated %d\n", idx, score.num50s, smap.live_score.getNum50s());
        if(score.numMisses != smap.live_score.getNumMisses())
            debugLog("Score %d: nMisses was %d, simulated %d\n", idx, score.numMisses, smap.live_score.getNumMisses());

        db->scores_mtx.lock();
        for(auto& other : (*db->getScores())[score.beatmap_hash]) {
            if(other.unixTimestamp == score.unixTimestamp) {
                // @PPV3: currently hitdeltas is always empty
                other.hitdeltas = score.hitdeltas;
                break;
            }
        }
        db->scores_mtx.unlock();

        // TODO @kiwec: update & save scores/pp

        sct_computed++;
        idx++;
    }

    sct_computed++;
}

void sct_calc(std::vector<FinishedScore> scores_to_calc) {
    return;  // TODO @kiwec: disabled for now

    sct_abort();
    if(scores_to_calc.empty()) return;

    dead = false;
    scores = scores_to_calc;
    sct_computed = 0;
    sct_total = scores.size() + 1;
    thr = std::thread(run_sct);
}

void sct_abort() {
    if(dead.load()) return;

    dead = true;
    thr.join();

    sct_total = 0;
    sct_computed = 0;
}
