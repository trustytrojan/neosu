#pragma once
#include "Vectors.h"

class McRect {
   public:
    McRect(float x = 0, float y = 0, float width = 0, float height = 0, bool isCentered = false);
    McRect(Vector2 pos, Vector2 size, bool isCentered = false);

    void set(float x, float y, float width, float height, bool isCentered = false);

    inline bool contains(const Vector2 &point) const {
        return (point.x >= this->fMinX && point.x <= this->fMaxX && point.y >= this->fMinY && point.y <= this->fMaxY);
    }
    McRect intersect(const McRect &rect) const;
    bool intersects(const McRect &rect) const;
    McRect Union(const McRect &rect) const;

    inline Vector2 getPos() const { return Vector2(this->fMinX, this->fMinY); }
    inline Vector2 getSize() const { return Vector2(this->fMaxX - this->fMinX, this->fMaxY - this->fMinY); }

    inline float getX() const { return this->fMinX; }
    inline float getY() const { return this->fMinY; }
    inline float getWidth() const { return (this->fMaxX - this->fMinX); }
    inline float getHeight() const { return (this->fMaxY - this->fMinY); }

    inline float getMinX() const { return this->fMinX; }
    inline float getMinY() const { return this->fMinY; }
    inline float getMaxX() const { return this->fMaxX; }
    inline float getMaxY() const { return this->fMaxY; }

    inline void setMaxX(float maxx) { this->fMaxX = maxx; }
    inline void setMaxY(float maxy) { this->fMaxY = maxy; }
    inline void setMinX(float minx) { this->fMinX = minx; }
    inline void setMinY(float miny) { this->fMinY = miny; }

   private:
    float fMinX;
    float fMinY;
    float fMaxX;
    float fMaxY;
};
