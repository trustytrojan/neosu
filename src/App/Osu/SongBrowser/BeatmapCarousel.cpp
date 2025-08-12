#include "BeatmapCarousel.h"
#include "CBaseUIContainer.h"

BeatmapCarousel::~BeatmapCarousel() {
    this->getContainer()->invalidate();
}
