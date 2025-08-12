#pragma once
// Copyright (c) 2025, WH, All rights reserved.

// TODO: this currently does absolutely nothing besides passing through to CBaseUIScrollView

#include "CBaseUIScrollView.h"

class BeatmapCarousel : public CBaseUIScrollView {
   public:
    BeatmapCarousel(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, const UString &name = "")
        : CBaseUIScrollView(xPos, yPos, xSize, ySize, name) {}
    ~BeatmapCarousel() override;

    BeatmapCarousel &operator=(const BeatmapCarousel &) = delete;
    BeatmapCarousel &operator=(BeatmapCarousel &&) = delete;
    BeatmapCarousel(const BeatmapCarousel &) = delete;
    BeatmapCarousel(BeatmapCarousel &&) = delete;
};
