#pragma once
#include "Vectors.h"

class McRect {
   public:
    McRect(float x = 0, float y = 0, float width = 0, float height = 0, bool isCentered = false);
    McRect(Vector2 pos, Vector2 size, bool isCentered = false);

    void set(float x, float y, float width, float height, bool isCentered = false);

    [[nodiscard]] inline bool contains(const Vector2 &point) const {
        return (point.x >= this->fMinX && point.x <= this->fMaxX && point.y >= this->fMinY && point.y <= this->fMaxY);
    }
    [[nodiscard]] McRect intersect(const McRect &rect) const;
    [[nodiscard]] bool intersects(const McRect &rect) const;
    [[nodiscard]] McRect Union(const McRect &rect) const;

    [[nodiscard]] inline Vector2 getPos() const { return Vector2(this->fMinX, this->fMinY); }
    [[nodiscard]] inline Vector2 getSize() const { return Vector2(this->fMaxX - this->fMinX, this->fMaxY - this->fMinY); }

    [[nodiscard]] inline float getX() const { return this->fMinX; }
    [[nodiscard]] inline float getY() const { return this->fMinY; }
    [[nodiscard]] inline float getWidth() const { return (this->fMaxX - this->fMinX); }
    [[nodiscard]] inline float getHeight() const { return (this->fMaxY - this->fMinY); }

    [[nodiscard]] inline float getMinX() const { return this->fMinX; }
    [[nodiscard]] inline float getMinY() const { return this->fMinY; }
    [[nodiscard]] inline float getMaxX() const { return this->fMaxX; }
    [[nodiscard]] inline float getMaxY() const { return this->fMaxY; }

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

// TODO: move this somewhere else (lol)
enum class AnchorPoint : uint8_t {
    CENTER,        // Default - image centered on x,y
    TOP_LEFT,      // x,y at top left corner
    TOP_RIGHT,     // x,y at top right corner
    BOTTOM_LEFT,   // x,y at bottom left corner
    BOTTOM_RIGHT,  // x,y at bottom right corner
    TOP,           // x,y at top center
    BOTTOM,        // x,y at bottom center
    LEFT,          // x,y at middle left
    RIGHT          // x,y at middle right
};
