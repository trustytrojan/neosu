//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by Beatmap for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#include "BackgroundStarCacheLoader.h"

#include "Beatmap.h"
#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"

BackgroundStarCacheLoader::BackgroundStarCacheLoader(Beatmap *beatmap) : Resource() {
    m_beatmap = beatmap;

    m_bDead = true;  // NOTE: start dead! need to revive() before use
    m_iProgress = 0;
}

void BackgroundStarCacheLoader::init() { m_bReady = true; }

void BackgroundStarCacheLoader::initAsync() {
    if(m_bDead.load()) {
        m_bAsyncReady = true;
        return;
    }

    // recalculate star cache
    DatabaseBeatmap *diff2 = m_beatmap->getSelectedDifficulty2();
    if(diff2 != NULL) {
        // precalculate cut star values for live pp

        // reset
        m_beatmap->m_aimStarsForNumHitObjects.clear();
        m_beatmap->m_aimSliderFactorForNumHitObjects.clear();
        m_beatmap->m_speedStarsForNumHitObjects.clear();
        m_beatmap->m_speedNotesForNumHitObjects.clear();

        const std::string &osuFilePath = diff2->getFilePath();
        const float AR = m_beatmap->getAR();
        const float CS = m_beatmap->getCS();
        const float OD = m_beatmap->getOD();
        const float speedMultiplier = osu->getSpeedMultiplier();  // NOTE: not beatmap->getSpeedMultiplier()!
        const bool relax = osu->getModRelax();
        const bool touchDevice = osu->getModTD();

        DatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres =
            DatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, AR, CS, speedMultiplier, false, m_bDead);

        for(size_t i = 0; i < diffres.diffobjects.size(); i++) {
            double aimStars = 0.0;
            double aimSliderFactor = 0.0;
            double speedStars = 0.0;
            double speedNotes = 0.0;

            DifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, speedMultiplier, relax,
                                                                 touchDevice, &aimStars, &aimSliderFactor, &speedStars,
                                                                 &speedNotes, i, m_bDead);

            m_beatmap->m_aimStarsForNumHitObjects.push_back(aimStars);
            m_beatmap->m_aimSliderFactorForNumHitObjects.push_back(aimSliderFactor);
            m_beatmap->m_speedStarsForNumHitObjects.push_back(speedStars);
            m_beatmap->m_speedNotesForNumHitObjects.push_back(speedNotes);

            m_iProgress = i;

            if(m_bDead.load()) {
                m_beatmap->m_aimStarsForNumHitObjects.clear();
                m_beatmap->m_aimSliderFactorForNumHitObjects.clear();
                m_beatmap->m_speedStarsForNumHitObjects.clear();
                m_beatmap->m_speedNotesForNumHitObjects.clear();

                break;
            }
        }
    }

    m_bAsyncReady = true;
}
