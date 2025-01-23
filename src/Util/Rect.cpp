#include "Rect.h"

#include "cbase.h"

using namespace std;

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

    intersection.fMinX = max(this->fMinX, rect.fMinX);
    intersection.fMinY = max(this->fMinY, rect.fMinY);
    intersection.fMaxX = min(this->fMaxX, rect.fMaxX);
    intersection.fMaxY = min(this->fMaxY, rect.fMaxY);

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

    Union.fMinX = min(this->fMinX, rect.fMinX);
    Union.fMinY = min(this->fMinY, rect.fMinY);
    Union.fMaxX = max(this->fMaxX, rect.fMaxX);
    Union.fMaxY = max(this->fMaxY, rect.fMaxY);

    return Union;
}

bool McRect::intersects(const McRect &rect) const {
    const float minx = max(this->fMinX, rect.fMinX);
    const float miny = max(this->fMinY, rect.fMinY);
    const float maxx = min(this->fMaxX, rect.fMaxX);
    const float maxy = min(this->fMaxY, rect.fMaxY);

    return (minx < maxx && miny < maxy);
}
