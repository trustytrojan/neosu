//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		3d quaternion camera system
//
// $NoKeywords: $cam
//===============================================================================//

#ifndef CAMERA_H
#define CAMERA_H

#include "Quaternion.h"
#include "cbase.h"

class Camera {
   public:
    static Matrix4 buildMatrixOrtho2D(float left, float right, float bottom, float top, float zn,
                                      float zf);  // DEPRECATED (OpenGL specific)
    static Matrix4 buildMatrixOrtho2DGLLH(float left, float right, float bottom, float top, float zn,
                                          float zf);  // OpenGL
    static Matrix4 buildMatrixOrtho2DDXLH(float left, float right, float bottom, float top, float zn,
                                          float zf);                            // DirectX
    static Matrix4 buildMatrixLookAt(Vector3 eye, Vector3 target, Vector3 up);  // DEPRECATED
    static Matrix4 buildMatrixLookAtLH(Vector3 eye, Vector3 target, Vector3 up);
    static Matrix4 buildMatrixPerspectiveFov(float fovRad, float aspect, float zn,
                                             float zf);  // DEPRECATED (OpenGL specific)
    static Matrix4 buildMatrixPerspectiveFovVertical(float fovRad, float aspectRatioWidthToHeight, float zn,
                                                     float zf);  // DEPRECATED
    static Matrix4 buildMatrixPerspectiveFovVerticalDXLH(float fovRad, float aspectRatioWidthToHeight, float zn,
                                                         float zf);
    static Matrix4 buildMatrixPerspectiveFovHorizontal(float fovRad, float aspectRatioHeightToWidth, float zn,
                                                       float zf);  // DEPRECATED
    static Matrix4 buildMatrixPerspectiveFovHorizontalDXLH(float fovRad, float aspectRatioHeightToWidth, float zn,
                                                           float zf);

   public:
    enum CAMERA_TYPE : uint8_t { CAMERA_TYPE_FIRST_PERSON, CAMERA_TYPE_ORBIT };

    Camera(Vector3 pos = Vector3(0, 0, 0), Vector3 viewDir = Vector3(0, 0, 1), float fovDeg = 90.0f,
           CAMERA_TYPE camType = CAMERA_TYPE_FIRST_PERSON);

    void rotateX(float pitchDeg);
    void rotateY(float yawDeg);

    void lookAt(Vector3 target);

    // set
    void setType(CAMERA_TYPE camType);
    void setPos(Vector3 pos);
    void setFov(float fovDeg) { this->fFov = glm::radians(fovDeg); }
    void setFovRad(float fovRad) { this->fFov = fovRad; }
    void setOrbitDistance(float orbitDistance);
    void setOrbitYAxis(bool orbitYAxis) { this->bOrbitYAxis = orbitYAxis; }

    void setRotation(float yawDeg, float pitchDeg, float rollDeg);
    void setYaw(float yawDeg);
    void setPitch(float pitchDeg);
    void setRoll(float rollDeg);
    void setWorldOrientation(Quaternion worldRotation) {
        this->worldRotation = worldRotation;
        this->updateVectors();
    }

    // get
    [[nodiscard]] inline CAMERA_TYPE getType() const { return this->camType; }
    [[nodiscard]] inline Vector3 getPos() const { return this->vPos; }
    [[nodiscard]] Vector3 getNextPosition(Vector3 velocity) const;

    [[nodiscard]] inline float getFov() const { return glm::degrees(this->fFov); }
    [[nodiscard]] inline float getFovRad() const { return this->fFov; }
    [[nodiscard]] inline float getOrbitDistance() const { return this->fOrbitDistance; }

    [[nodiscard]] inline Vector3 getWorldXAxis() const { return this->worldRotation * this->vXAxis; }
    [[nodiscard]] inline Vector3 getWorldYAxis() const { return this->worldRotation * this->vYAxis; }
    [[nodiscard]] inline Vector3 getWorldZAxis() const { return this->worldRotation * this->vZAxis; }

    [[nodiscard]] inline Vector3 getViewDirection() const { return this->vViewDir; }
    [[nodiscard]] inline Vector3 getViewUp() const { return this->vViewUp; }
    [[nodiscard]] inline Vector3 getViewRight() const { return this->vViewRight; }

    [[nodiscard]] inline float getPitch() const { return this->fPitch; }
    [[nodiscard]] inline float getYaw() const { return this->fYaw; }
    [[nodiscard]] inline float getRoll() const { return this->fRoll; }

    [[nodiscard]] inline Quaternion getRotation() const { return this->rotation; }

    [[nodiscard]] Vector3 getProjectedVector(Vector3 point, float screenWidth, float screenHeight, float zn = 0.1f,
                               float zf = 1.0f) const;
    [[nodiscard]] Vector3 getUnProjectedVector(Vector2 point, float screenWidth, float screenHeight, float zn = 0.1f,
                                 float zf = 1.0f) const;

    [[nodiscard]] bool isPointVisibleFrustum(Vector3 point) const;  // within our viewing frustum
    [[nodiscard]] bool isPointVisiblePlane(Vector3 point) const;    // just in front of the camera plane

   private:
    struct CAM_PLANE {
        float a, b, c, d;
    };

    static float planeDotCoord(CAM_PLANE plane, Vector3 point);
    static float planeDotCoord(Vector3 planeNormal, Vector3 planePoint, Vector3 &pv);

    void updateVectors();
    void updateViewFrustum();

    void lookAt(Vector3 eye, Vector3 target);

    // vars
    CAMERA_TYPE camType;
    Vector3 vPos;
    Vector3 vOrbitTarget;
    float fFov;
    float fOrbitDistance;
    bool bOrbitYAxis;

    // base axes
    Vector3 vWorldXAxis;
    Vector3 vWorldYAxis;
    Vector3 vWorldZAxis;

    // derived axes
    Vector3 vXAxis;
    Vector3 vYAxis;
    Vector3 vZAxis;

    // rotation
    Quaternion rotation;
    Quaternion worldRotation;
    float fPitch;
    float fYaw;
    float fRoll;

    // relative coordinate system
    Vector3 vViewDir;
    Vector3 vViewRight;
    Vector3 vViewUp;

    // custom
    CAM_PLANE viewFrustum[4];
};

#endif
