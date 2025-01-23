//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		a simple quaternion
//
// $NoKeywords: $quat
//===============================================================================//

#ifndef QUATERNION_H
#define QUATERNION_H

#include "cbase.h"

class Quaternion {
   public:
    float x, y, z, w;

    Quaternion() { this->identity(); }
    Quaternion(float x, float y, float z, float w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    void identity() {
        this->x = this->y = this->z = 0.0f;
        this->w = 1.0f;
    }
    void set(float x, float y, float z, float w);
    void normalize();

    void fromAxis(const Vector3 &axis, float angleDeg);
    void fromEuler(float yawDeg, float pitchDeg, float rollDeg);

    inline float getYaw() const {
        return rad2deg(std::atan2(2.0f * (this->y * this->z + this->w * this->x),
                                  this->w * this->w - this->x * this->x - this->y * this->y + this->z * this->z));
    }
    inline float getPitch() const { return rad2deg(std::asin(-2.0f * (this->x * this->z - this->w * this->y))); }
    inline float getRoll() const {
        return rad2deg(std::atan2(2.0f * (this->x * this->y + this->w * this->z),
                                  this->w * this->w + this->x * this->x - this->y * this->y - this->z * this->z));
    }

    inline Quaternion getConjugate() const { return Quaternion(-this->x, -this->y, -this->z, this->w); }
    Matrix4 getMatrix() const;
    Matrix3 getMatrix3() const;

    Quaternion operator*(const Quaternion &quat) const;
    Vector3 operator*(const Vector3 &vec) const;

   private:
    static constexpr float EPSILON = 0.00001f;
};

#endif
