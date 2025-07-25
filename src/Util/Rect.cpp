#include "Rect.h"

McRect::McRect(float x, float y, float width, float height, bool isCentered) {
    this->set(x, y, width, height, isCentered);
}

McRect::McRect(const Vector2 &pos, const Vector2 &size, bool isCentered) { this->set(pos, size, isCentered); }

void McRect::set(float x, float y, float width, float height, bool isCentered) {
    this->set(Vector2(x, y), Vector2(width, height), isCentered);
}

void McRect::set(const Vector2 &pos, const Vector2 &size, bool isCentered) {
    if(isCentered) {
        Vector2 halfSize = size * 0.5f;
        vMin = pos - halfSize;
        vMax = pos + halfSize;
    } else {
        vMin = pos;
        vMax = pos + size;
    }
}

McRect McRect::intersect(const McRect &rect) const {
    McRect intersection;

    intersection.vMin.x = std::max(this->vMin.x, rect.vMin.x);
    intersection.vMin.y = std::max(this->vMin.y, rect.vMin.y);
    intersection.vMax.x = std::min(this->vMax.x, rect.vMax.x);
    intersection.vMax.y = std::min(this->vMax.y, rect.vMax.y);

    // if the rects don't intersect, reset to null rect
    if(intersection.vMin.x > intersection.vMax.x || intersection.vMin.y > intersection.vMax.y) {
        intersection.vMin.zero();
        intersection.vMax.zero();
    }

    return intersection;
}

McRect McRect::Union(const McRect &rect) const {
    McRect result;

    // use vector component-wise min/max operations
    result.vMin.x = std::min(this->vMin.x, rect.vMin.x);
    result.vMin.y = std::min(this->vMin.y, rect.vMin.y);
    result.vMax.x = std::max(this->vMax.x, rect.vMax.x);
    result.vMax.y = std::max(this->vMax.y, rect.vMax.y);

    return result;
}

bool McRect::intersects(const McRect &rect) const {
    const float minx = std::max(this->vMin.x, rect.vMin.x);
    const float miny = std::max(this->vMin.y, rect.vMin.y);
    const float maxx = std::min(this->vMax.x, rect.vMax.x);
    const float maxy = std::min(this->vMax.y, rect.vMax.y);

    return (minx < maxx && miny < maxy);
}
