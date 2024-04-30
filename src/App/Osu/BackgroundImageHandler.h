//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		handles thumbnail/background image loading async, cache
//
// $NoKeywords: $osubgldr
//===============================================================================//

#ifndef OSUBACKGROUNDIMAGEHANDLER_H
#define OSUBACKGROUNDIMAGEHANDLER_H

#include "cbase.h"

class Image;

class DatabaseBeatmap;
class DatabaseBeatmapBackgroundImagePathLoader;

class BackgroundImageHandler {
   public:
    BackgroundImageHandler();
    ~BackgroundImageHandler();

    void update(bool allowEviction);

    void scheduleFreezeCache() { m_bFrozen = true; }

    Image *getLoadBackgroundImage(const DatabaseBeatmap *beatmap);

   private:
    struct ENTRY {
        bool isLoadScheduled;
        bool wasUsedLastFrame;
        float loadingTime;
        float evictionTime;
        unsigned long evictionTimeFrameCount;

        std::string osuFilePath;
        std::string folder;
        std::string backgroundImageFileName;

        DatabaseBeatmapBackgroundImagePathLoader *backgroundImagePathLoader;
        Image *image;
    };

    void handleLoadPathForEntry(ENTRY &entry);
    void handleLoadImageForEntry(ENTRY &entry);

    std::vector<ENTRY> m_cache;
    bool m_bFrozen;
};

#endif
