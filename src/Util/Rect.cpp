#include "Rect.h"

#include "cbase.h"

McRect::McRect(float x, float y, float width, float height, bool isCentered) { set(x, y, width, height, isCentered); }

McRect::McRect(Vector2 pos, Vector2 size, bool isCentered) { set(pos.x, pos.y, size.x, size.y, isCentered); }

void McRect::set(float x, float y, float width, float height, bool isCentered) {
    if(isCentered) {
        m_fMinX = x - (width / 2.0f);
        m_fMaxX = x + (width / 2.0f);
        m_fMinY = y - (height / 2.0f);
        m_fMaxY = y + (height / 2.0f);
    } else {
        m_fMinX = x;
        m_fMaxX = x + width;
        m_fMinY = y;
        m_fMaxY = y + height;
    }
}

McRect McRect::intersect(const McRect &rect) const {
    McRect intersection;

    intersection.m_fMinX = max(m_fMinX, rect.m_fMinX);
    intersection.m_fMinY = max(m_fMinY, rect.m_fMinY);
    intersection.m_fMaxX = min(m_fMaxX, rect.m_fMaxX);
    intersection.m_fMaxY = min(m_fMaxY, rect.m_fMaxY);

    // if the rects don't intersect
    if(intersection.m_fMinX > intersection.m_fMaxX || intersection.m_fMinY > intersection.m_fMaxY) {
        // then reset the rect to a null rect
        intersection.m_fMinX = 0.0f;
        intersection.m_fMaxX = 0.0f;
        intersection.m_fMinY = 0.0f;
        intersection.m_fMaxY = 0.0f;
    }

    return intersection;
}

McRect McRect::Union(const McRect &rect) const {
    McRect Union;

    Union.m_fMinX = min(m_fMinX, rect.m_fMinX);
    Union.m_fMinY = min(m_fMinY, rect.m_fMinY);
    Union.m_fMaxX = max(m_fMaxX, rect.m_fMaxX);
    Union.m_fMaxY = max(m_fMaxY, rect.m_fMaxY);

    return Union;
}

bool McRect::intersects(const McRect &rect) const {
    const float minx = max(m_fMinX, rect.m_fMinX);
    const float miny = max(m_fMinY, rect.m_fMinY);
    const float maxx = min(m_fMaxX, rect.m_fMaxX);
    const float maxy = min(m_fMaxY, rect.m_fMaxY);

    return (minx < maxx && miny < maxy);
}
