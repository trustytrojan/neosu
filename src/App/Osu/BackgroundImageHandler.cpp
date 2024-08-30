#include "BackgroundImageHandler.h"

#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "ResourceManager.h"

BackgroundImageHandler::BackgroundImageHandler() { m_bFrozen = false; }

BackgroundImageHandler::~BackgroundImageHandler() {
    for(size_t i = 0; i < m_cache.size(); i++) {
        engine->getResourceManager()->destroyResource(m_cache[i].backgroundImagePathLoader);
        engine->getResourceManager()->destroyResource(m_cache[i].image);
    }
    m_cache.clear();
}

void BackgroundImageHandler::update(bool allowEviction) {
    for(size_t i = 0; i < m_cache.size(); i++) {
        ENTRY &entry = m_cache[i];

        // NOTE: avoid load/unload jitter if framerate is below eviction delay
        const bool wasUsedLastFrame = entry.wasUsedLastFrame;
        entry.wasUsedLastFrame = false;

        // check and handle evictions
        if(!wasUsedLastFrame &&
           (engine->getTime() >= entry.evictionTime && engine->getFrameCount() >= entry.evictionTimeFrameCount)) {
            if(allowEviction) {
                if(!m_bFrozen && !engine->isMinimized()) {
                    if(entry.backgroundImagePathLoader != NULL) entry.backgroundImagePathLoader->interruptLoad();
                    if(entry.image != NULL) entry.image->interruptLoad();

                    engine->getResourceManager()->destroyResource(entry.backgroundImagePathLoader);
                    engine->getResourceManager()->destroyResource(entry.image);

                    m_cache.erase(m_cache.begin() + i);
                    i--;
                    continue;
                }
            } else {
                entry.evictionTime = engine->getTime() + cv_background_image_eviction_delay_seconds.getFloat();
                entry.evictionTimeFrameCount =
                    engine->getFrameCount() + (unsigned long)max(0, cv_background_image_eviction_delay_frames.getInt());
            }
        } else if(wasUsedLastFrame) {
            // check and handle scheduled loads
            if(entry.isLoadScheduled) {
                if(engine->getTime() >= entry.loadingTime) {
                    entry.isLoadScheduled = false;

                    if(entry.backgroundImageFileName.length() < 2) {
                        // if the backgroundImageFileName is not loaded, then we have to create a full
                        // DatabaseBeatmapBackgroundImagePathLoader
                        entry.image = NULL;
                        handleLoadPathForEntry(entry);
                    } else {
                        // if backgroundImageFileName is already loaded/valid, then we can directly load the image
                        entry.backgroundImagePathLoader = NULL;
                        handleLoadImageForEntry(entry);
                    }
                }
            } else {
                // no load scheduled (potential load-in-progress if it was necessary), handle backgroundImagePathLoader
                // loading finish
                if(entry.image == NULL && entry.backgroundImagePathLoader != NULL &&
                   entry.backgroundImagePathLoader->isReady()) {
                    if(entry.backgroundImagePathLoader->getLoadedBackgroundImageFileName().length() > 1) {
                        entry.backgroundImageFileName =
                            entry.backgroundImagePathLoader->getLoadedBackgroundImageFileName();
                        handleLoadImageForEntry(entry);
                    }

                    engine->getResourceManager()->destroyResource(entry.backgroundImagePathLoader);
                    entry.backgroundImagePathLoader = NULL;
                }
            }
        }
    }

    // reset flags
    m_bFrozen = false;

    // DEBUG:
    // debugLog("m_cache.size() = %i\n", (int)m_cache.size());
}

void BackgroundImageHandler::handleLoadPathForEntry(ENTRY &entry) {
    entry.backgroundImagePathLoader = new DatabaseBeatmapBackgroundImagePathLoader(entry.osuFilePath);

    // start path load
    engine->getResourceManager()->requestNextLoadAsync();
    engine->getResourceManager()->loadResource(entry.backgroundImagePathLoader);
}

void BackgroundImageHandler::handleLoadImageForEntry(ENTRY &entry) {
    std::string fullBackgroundImageFilePath = entry.folder;
    fullBackgroundImageFilePath.append(entry.backgroundImageFileName);

    // start image load
    engine->getResourceManager()->requestNextLoadAsync();
    engine->getResourceManager()->requestNextLoadUnmanaged();
    entry.image = engine->getResourceManager()->loadImageAbsUnnamed(fullBackgroundImageFilePath);
}

Image *BackgroundImageHandler::getLoadBackgroundImage(const DatabaseBeatmap *beatmap) {
    if(beatmap == NULL || !cv_load_beatmap_background_images.getBool() || !beatmap->draw_background) return NULL;

    // NOTE: no references to beatmap are kept anywhere (database can safely be deleted/reloaded without having to
    // notify the BackgroundImageHandler)

    const float newLoadingTime = engine->getTime() + cv_background_image_loading_delay.getFloat();
    const float newEvictionTime = engine->getTime() + cv_background_image_eviction_delay_seconds.getFloat();
    const unsigned long newEvictionTimeFrameCount =
        engine->getFrameCount() + (unsigned long)max(0, cv_background_image_eviction_delay_frames.getInt());

    // 1) if the path or image is already loaded, return image ref immediately (which may still be NULL) and keep track
    // of when it was last requested
    for(size_t i = 0; i < m_cache.size(); i++) {
        ENTRY &entry = m_cache[i];

        if(entry.osuFilePath == beatmap->getFilePath()) {
            entry.wasUsedLastFrame = true;
            entry.evictionTime = newEvictionTime;
            entry.evictionTimeFrameCount = newEvictionTimeFrameCount;

            // HACKHACK: to improve future loading speed, if we have already loaded the backgroundImageFileName, force
            // update the database backgroundImageFileName and fullBackgroundImageFilePath this is similar to how it
            // worked before the rework, but 100% safe(r) since we are not async
            if(m_cache[i].image != NULL && m_cache[i].backgroundImageFileName.length() > 1 &&
               beatmap->getBackgroundImageFileName().length() < 2) {
                const_cast<DatabaseBeatmap *>(beatmap)->m_sBackgroundImageFileName = m_cache[i].backgroundImageFileName;
                const_cast<DatabaseBeatmap *>(beatmap)->m_sFullBackgroundImageFilePath = beatmap->getFolder();
                const_cast<DatabaseBeatmap *>(beatmap)->m_sFullBackgroundImageFilePath.append(
                    m_cache[i].backgroundImageFileName);
            }

            return entry.image;
        }
    }

    // 2) not found in cache, so create a new entry which will get handled in the next update
    {
        // try evicting stale not-yet-loaded-nor-started-loading entries on overflow
        const int maxCacheEntries = cv_background_image_cache_size.getInt();
        {
            if(m_cache.size() >= maxCacheEntries) {
                for(size_t i = 0; i < m_cache.size(); i++) {
                    if(m_cache[i].isLoadScheduled && !m_cache[i].wasUsedLastFrame) {
                        m_cache.erase(m_cache.begin() + i);
                        i--;
                        continue;
                    }
                }
            }
        }

        // create entry
        ENTRY entry;
        {
            entry.isLoadScheduled = true;
            entry.wasUsedLastFrame = true;
            entry.loadingTime = newLoadingTime;
            entry.evictionTime = newEvictionTime;
            entry.evictionTimeFrameCount = newEvictionTimeFrameCount;

            entry.osuFilePath = beatmap->getFilePath();
            entry.folder = beatmap->getFolder();
            entry.backgroundImageFileName = beatmap->getBackgroundImageFileName();

            entry.backgroundImagePathLoader = NULL;
            entry.image = NULL;
        }
        if(m_cache.size() < maxCacheEntries) m_cache.push_back(entry);
    }

    return NULL;
}
