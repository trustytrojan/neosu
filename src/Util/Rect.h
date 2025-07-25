#pragma once
#include "Vectors.h"

class McRect {
   public:
    McRect(float x = 0, float y = 0, float width = 0, float height = 0, bool isCentered = false);
    McRect(const Vector2 &pos, const Vector2 &size, bool isCentered = false);

    void set(float x, float y, float width, float height, bool isCentered = false);
    void set(const Vector2 &pos, const Vector2 &size, bool isCentered = false);

    [[nodiscard]] inline bool contains(const Vector2 &point) const {
        return (point.x >= this->vMin.x && point.x <= this->vMax.x && point.y >= this->vMin.y && point.y <= this->vMax.y);
    }

    [[nodiscard]] McRect intersect(const McRect &rect) const;
    [[nodiscard]] bool intersects(const McRect &rect) const;
    [[nodiscard]] McRect Union(const McRect &rect) const;

    [[nodiscard]] inline Vector2 getPos() const { return this->vMin; }
    [[nodiscard]] inline Vector2 getSize() const { return this->vMax - this->vMin; }
    [[nodiscard]] inline Vector2 getCenter() const { return (this->vMin + this->vMax) * 0.5f; }
    [[nodiscard]] inline Vector2 getMin() const { return this->vMin; }
    [[nodiscard]] inline Vector2 getMax() const { return this->vMax; }

    [[nodiscard]] inline float getX() const { return this->vMin.x; }
    [[nodiscard]] inline float getY() const { return this->vMin.y; }
    [[nodiscard]] inline float getWidth() const { return this->vMax.x - this->vMin.x; }
    [[nodiscard]] inline float getHeight() const { return this->vMax.y - this->vMin.y; }

    [[nodiscard]] inline float getMinX() const { return this->vMin.x; }
    [[nodiscard]] inline float getMinY() const { return this->vMin.y; }
    [[nodiscard]] inline float getMaxX() const { return this->vMax.x; }
    [[nodiscard]] inline float getMaxY() const { return this->vMax.y; }

    inline void setMin(const Vector2 &min) { this->vMin = min; }
    inline void setMax(const Vector2 &max) { this->vMax = max; }
    inline void setMaxX(float maxx) { this->vMax.x = maxx; }
    inline void setMaxY(float maxy) { this->vMax.y = maxy; }
    inline void setMinX(float minx) { this->vMin.x = minx; }
    inline void setMinY(float miny) { this->vMin.y = miny; }

   private:
    Vector2 vMin;
    Vector2 vMax;
};
