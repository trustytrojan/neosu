#include "Rect.h"

McRect::McRect(float x, float y, float width, float height, bool isCentered) {
    this->set(x, y, width, height, isCentered);
}

McRect::McRect(const Vector2 &pos, const Vector2 &size, bool isCentered) { 
    this->set(pos, size, isCentered); 
}

void McRect::set(float x, float y, float width, float height, bool isCentered) {
    this->set(Vector2(x, y), Vector2(width, height), isCentered);
}

void McRect::set(const Vector2 &pos, const Vector2 &size, bool isCentered) {
    if(isCentered) {
        Vector2 halfSize = size * 0.5f;
        this->vMin = pos - halfSize;
    } else {
        this->vMin = pos;
    }
    this->vSize = size;
}

McRect McRect::intersect(const McRect &rect) const {
    McRect intersection;
    
    Vector2 thisMax = this->vMin + this->vSize;
    Vector2 rectMax = rect.vMin + rect.vSize;

    intersection.vMin = glm::max(static_cast<const glm::vec2 &>(this->vMin), 
                                static_cast<const glm::vec2 &>(rect.vMin));
    Vector2 intersectMax = glm::min(static_cast<const glm::vec2 &>(thisMax), 
                                   static_cast<const glm::vec2 &>(rectMax));

    if(glm::any(glm::greaterThan(static_cast<const glm::vec2 &>(intersection.vMin),
                                 static_cast<const glm::vec2 &>(intersectMax)))) {
        intersection.vMin.zero();
        intersection.vSize.zero();
    } else {
        intersection.vSize = intersectMax - intersection.vMin;
    }

    return intersection;
}

McRect McRect::Union(const McRect &rect) const {
    McRect result;
    
    Vector2 thisMax = this->vMin + this->vSize;
    Vector2 rectMax = rect.vMin + rect.vSize;

    result.vMin = glm::min(static_cast<const glm::vec2 &>(this->vMin), 
                          static_cast<const glm::vec2 &>(rect.vMin));
    Vector2 resultMax = glm::max(static_cast<const glm::vec2 &>(thisMax), 
                                static_cast<const glm::vec2 &>(rectMax));
    result.vSize = resultMax - result.vMin;

    return result;
}

bool McRect::intersects(const McRect &rect) const {
    Vector2 thisMax = this->vMin + this->vSize;
    Vector2 rectMax = rect.vMin + rect.vSize;
    
    Vector2 intersectMin = glm::max(static_cast<const glm::vec2 &>(this->vMin), 
                                   static_cast<const glm::vec2 &>(rect.vMin));
    Vector2 intersectMax = glm::min(static_cast<const glm::vec2 &>(thisMax), 
                                   static_cast<const glm::vec2 &>(rectMax));

    return glm::all(glm::lessThan(static_cast<const glm::vec2 &>(intersectMin), 
                                  static_cast<const glm::vec2 &>(intersectMax)));
}
