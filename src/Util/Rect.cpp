#include "Rect.h"

#include "cbase.h"



McRect::McRect(float x, float y, float width, float height, bool isCentered) {
    this->set(x, y, width, height, isCentered);
}

McRect::McRect(Vector2 pos, Vector2 size, bool isCentered) { this->set(pos.x, pos.y, size.x, size.y, isCentered); }

void McRect::set(float x, float y, float width, float height, bool isCentered) {
    if(isCentered) {
        this->fMinX = x - (width / 2.0f);
        this->fMaxX = x + (width / 2.0f);
        this->fMinY = y - (height / 2.0f);
        this->fMaxY = y + (height / 2.0f);
    } else {
        this->fMinX = x;
        this->fMaxX = x + width;
        this->fMinY = y;
        this->fMaxY = y + height;
    }
}

McRect McRect::intersect(const McRect &rect) const {
    McRect intersection;

    intersection.fMinX = std::max(this->fMinX, rect.fMinX);
    intersection.fMinY = std::max(this->fMinY, rect.fMinY);
    intersection.fMaxX = std::min(this->fMaxX, rect.fMaxX);
    intersection.fMaxY = std::min(this->fMaxY, rect.fMaxY);

    // if the rects don't intersect
    if(intersection.fMinX > intersection.fMaxX || intersection.fMinY > intersection.fMaxY) {
        // then reset the rect to a null rect
        intersection.fMinX = 0.0f;
        intersection.fMaxX = 0.0f;
        intersection.fMinY = 0.0f;
        intersection.fMaxY = 0.0f;
    }

    return intersection;
}

McRect McRect::Union(const McRect &rect) const {
    McRect Union;

    Union.fMinX = std::min(this->fMinX, rect.fMinX);
    Union.fMinY = std::min(this->fMinY, rect.fMinY);
    Union.fMaxX = std::max(this->fMaxX, rect.fMaxX);
    Union.fMaxY = std::max(this->fMaxY, rect.fMaxY);

    return Union;
}

bool McRect::intersects(const McRect &rect) const {
    const float minx = std::max(this->fMinX, rect.fMinX);
    const float miny = std::max(this->fMinY, rect.fMinY);
    const float maxx = std::min(this->fMaxX, rect.fMaxX);
    const float maxy = std::min(this->fMaxY, rect.fMaxY);

    return (minx < maxx && miny < maxy);
}
