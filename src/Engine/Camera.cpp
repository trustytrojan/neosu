//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		3d quaternion camera system
//
// $NoKeywords: $cam
//===============================================================================//

#include "Camera.h"

#include "Engine.h"

Matrix4 Camera::buildMatrixOrtho2D(float left, float right, float bottom, float top, float zn, float zf) {
    const float invX = 1.0f / (right - left);
    const float invY = 1.0f / (top - bottom);
    const float invZ = 1.0f / (zf - zn);

    return Matrix4((2.0f * invX), 0, 0, (-(right + left) * invX), 0, (2.0 * invY), 0, (-(top + bottom) * invY), 0, 0,
                   (-2.0f * invZ), (-(zf + zn) * invZ), 0, 0, 0, 1)
        .transpose();
}

Matrix4 Camera::buildMatrixOrtho2DGLLH(float left, float right, float bottom, float top, float zn, float zf) {
    const float invX = 1.0f / (right - left);
    const float invY = 1.0f / (top - bottom);
    const float invZ = 1.0f / (zf - zn);

    return Matrix4((2.0f * invX), 0, 0, (-(right + left) * invX), 0, (2.0 * invY), 0, (-(top + bottom) * invY), 0, 0,
                   (-2.0f * invZ), (-(zf + zn) * invZ), 0, 0, 0, 1)
        .transpose();
}

Matrix4 Camera::buildMatrixOrtho2DDXLH(float left, float right, float bottom, float top, float zn, float zf) {
    return Matrix4(2.0f / (right - left), 0, 0, (left + right) / (left - right), 0, 2.0f / (top - bottom), 0,
                   (top + bottom) / (bottom - top), 0, 0, 1.0f / (zf - zn), zn / (zn - zf), 0, 0, 0, 1)
        .transpose();
}

Matrix4 Camera::buildMatrixLookAt(Vector3 eye, Vector3 target, Vector3 up) {
    const Vector3 zAxis = (eye - target).normalize();
    const Vector3 xAxis = up.cross(zAxis).normalize();
    const Vector3 yAxis = zAxis.cross(xAxis);

    return Matrix4(xAxis.x, xAxis.y, xAxis.z, -xAxis.dot(eye), yAxis.x, yAxis.y, yAxis.z, -yAxis.dot(eye), zAxis.x,
                   zAxis.y, zAxis.z, -zAxis.dot(eye), 0, 0, 0, 1)
        .transpose();
}

Matrix4 Camera::buildMatrixLookAtLH(Vector3 eye, Vector3 target, Vector3 up) {
    const Vector3 zAxis = (target - eye).normalize();
    const Vector3 xAxis = up.cross(zAxis).normalize();
    const Vector3 yAxis = zAxis.cross(xAxis);

    return Matrix4(xAxis.x, xAxis.y, xAxis.z, -xAxis.dot(eye), yAxis.x, yAxis.y, yAxis.z, -yAxis.dot(eye), zAxis.x,
                   zAxis.y, zAxis.z, -zAxis.dot(eye), 0, 0, 0, 1)
        .transpose();
}

Matrix4 Camera::buildMatrixPerspectiveFov(float fovRad, float aspect, float zn, float zf) {
    const float f = 1.0f / std::tan(fovRad / 2.0f);

    const float q = (zf + zn) / (zn - zf);
    const float qn = (2 * zf * zn) / (zn - zf);

    return Matrix4(f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, q, qn, 0, 0, -1, 0).transpose();
}

Matrix4 Camera::buildMatrixPerspectiveFovVertical(float fovRad, float aspectRatioWidthToHeight, float zn, float zf) {
    const float f = 1.0f / std::tan(fovRad / 2.0f);

    const float q = (zf + zn) / (zn - zf);
    const float qn = (2 * zf * zn) / (zn - zf);

    return Matrix4(f / aspectRatioWidthToHeight, 0, 0, 0, 0, f, 0, 0, 0, 0, q, qn, 0, 0, -1, 0).transpose();
}

Matrix4 Camera::buildMatrixPerspectiveFovVerticalDXLH(float fovRad, float aspectRatioWidthToHeight, float zn,
                                                      float zf) {
    const float f = 1.0f / std::tan(fovRad / 2.0f);

    return Matrix4(f / aspectRatioWidthToHeight, 0, 0, 0, 0, f, 0, 0, 0, 0, zf / (zf - zn), -zn * zf / (zf - zn), 0, 0,
                   1, 0)
        .transpose();
}

Matrix4 Camera::buildMatrixPerspectiveFovHorizontal(float fovRad, float aspectRatioHeightToWidth, float zn, float zf) {
    const float f = 1.0f / std::tan(fovRad / 2.0f);

    const float q = (zf + zn) / (zn - zf);
    const float qn = (2 * zf * zn) / (zn - zf);

    return Matrix4(f, 0, 0, 0, 0, f / aspectRatioHeightToWidth, 0, 0, 0, 0, q, qn, 0, 0, -1, 0).transpose();
}

Matrix4 Camera::buildMatrixPerspectiveFovHorizontalDXLH(float fovRad, float aspectRatioHeightToWidth, float zn,
                                                        float zf) {
    const float f = 1.0f / std::tan(fovRad / 2.0f);

    return Matrix4(f, 0, 0, 0, 0, f / aspectRatioHeightToWidth, 0, 0, 0, 0, zf / (zf - zn), -zn * zf / (zf - zn), 0, 0,
                   1, 0)
        .transpose();
}

static Vector3 Vector3TransformCoord(const Vector3 &in, const Matrix4 &mat) {
    // return mat * in; // wtf?

    Vector3 out;

    const float norm = mat[3] * in.x + mat[7] * in.y + mat[11] * in.z + mat[15];

    if(norm) {
        out.x = (mat[0] * in.x + mat[4] * in.y + mat[8] * in.z + mat[12]) / norm;
        out.y = (mat[1] * in.x + mat[5] * in.y + mat[9] * in.z + mat[13]) / norm;
        out.z = (mat[2] * in.x + mat[6] * in.y + mat[10] * in.z + mat[14]) / norm;
    }

    return out;
}

//*************************//
//	Camera implementation  //
//*************************//

Camera::Camera(Vector3 pos, Vector3 viewDir, float fovDeg, CAMERA_TYPE camType) {
    this->vPos = pos;
    this->vViewDir = viewDir;
    this->fFov = deg2rad(fovDeg);
    this->camType = camType;

    this->fOrbitDistance = 5.0f;
    this->bOrbitYAxis = true;

    this->fPitch = 0;
    this->fYaw = 0;
    this->fRoll = 0;

    // base axes
    this->vWorldXAxis = Vector3(1, 0, 0);
    this->vWorldYAxis = Vector3(0, 1, 0);
    this->vWorldZAxis = Vector3(0, 0, 1);

    // derived axes
    this->vXAxis = this->vWorldXAxis;
    this->vYAxis = this->vWorldYAxis;
    this->vZAxis = this->vWorldZAxis;

    this->vViewRight = this->vWorldXAxis;
    this->vViewUp = this->vWorldYAxis;

    this->lookAt(pos + viewDir);
}

void Camera::updateVectors() {
    // update rotation
    if(this->camType == CAMERA_TYPE_FIRST_PERSON)
        this->rotation.fromEuler(this->fRoll, this->fYaw, -this->fPitch);
    else if(this->camType == CAMERA_TYPE_ORBIT) {
        this->rotation.identity();

        if(this->bOrbitYAxis) {
            // yaw
            Quaternion tempQuat;
            tempQuat.fromAxis(this->vYAxis, this->fYaw);

            this->rotation = tempQuat * this->rotation;

            // pitch
            tempQuat.fromAxis(this->vXAxis, -this->fPitch);
            this->rotation = this->rotation * tempQuat;

            this->rotation.normalize();
        }
    }

    // calculate new coordinate system
    this->vViewDir = (this->worldRotation * this->rotation) * this->vZAxis;
    this->vViewRight = (this->worldRotation * this->rotation) * this->vXAxis;
    this->vViewUp = (this->worldRotation * this->rotation) * this->vYAxis;

    // update pos if we are orbiting (with the new coordinate system from above)
    if(this->camType == CAMERA_TYPE_ORBIT) this->setPos(this->vOrbitTarget);

    this->updateViewFrustum();
}

void Camera::updateViewFrustum() {
    // TODO: this function is broken due to matrix changes, refactor

    const Matrix4 viewMatrix = this->rotation.getMatrix();

    const Matrix4 projectionMatrix = buildMatrixPerspectiveFov(
        this->fFov, (float)engine->getScreenWidth() / (float)engine->getScreenHeight(), 0.0f, 1.0f);

    const Matrix4 viewProj = (viewMatrix * projectionMatrix);

    // left plane
    this->viewFrustum[0].a = viewProj[3] + viewProj[0];
    this->viewFrustum[0].b = viewProj[7] + viewProj[4];
    this->viewFrustum[0].c = viewProj[11] + viewProj[8];
    this->viewFrustum[0].d = viewProj[15] + viewProj[12];

    // right plane
    this->viewFrustum[1].a = viewProj[3] - viewProj[0];
    this->viewFrustum[1].b = viewProj[7] - viewProj[4];
    this->viewFrustum[1].c = viewProj[11] - viewProj[8];
    this->viewFrustum[1].d = viewProj[15] - viewProj[12];

    // top plane
    this->viewFrustum[2].a = viewProj[3] - viewProj[1];
    this->viewFrustum[2].b = viewProj[7] - viewProj[5];
    this->viewFrustum[2].c = viewProj[11] - viewProj[9];
    this->viewFrustum[2].d = viewProj[15] - viewProj[13];

    // bottom plane
    this->viewFrustum[3].a = viewProj[3] + viewProj[1];
    this->viewFrustum[3].b = viewProj[7] + viewProj[5];
    this->viewFrustum[3].c = viewProj[11] + viewProj[9];
    this->viewFrustum[3].d = viewProj[15] + viewProj[13];

    // normalize planes
    for(int i = 0; i < 4; i++) {
        const float norm = std::sqrt(this->viewFrustum[i].a * this->viewFrustum[i].a +
                                     this->viewFrustum[i].b * this->viewFrustum[i].b +
                                     this->viewFrustum[i].c * this->viewFrustum[i].c);
        if(norm) {
            this->viewFrustum[i].a = this->viewFrustum[i].a / norm;
            this->viewFrustum[i].b = this->viewFrustum[i].b / norm;
            this->viewFrustum[i].c = this->viewFrustum[i].c / norm;
            this->viewFrustum[i].d = this->viewFrustum[i].d / norm;
        } else {
            this->viewFrustum[i].a = 0.0f;
            this->viewFrustum[i].b = 0.0f;
            this->viewFrustum[i].c = 0.0f;
            this->viewFrustum[i].d = 0.0f;
        }
    }
}

void Camera::rotateX(float pitchDeg) {
    this->fPitch += pitchDeg;

    if(this->fPitch > 89.8f)
        this->fPitch = 89.8f;
    else if(this->fPitch < -89.8f)
        this->fPitch = -89.8f;

    this->updateVectors();
}

void Camera::rotateY(float yawDeg) {
    this->fYaw += yawDeg;

    if(this->fYaw > 360.0f)
        this->fYaw = this->fYaw - 360.0f;
    else if(this->fYaw < 0.0f)
        this->fYaw = 360.0f + this->fYaw;

    this->updateVectors();
}

void Camera::lookAt(Vector3 target) { this->lookAt(this->vPos, target); }

void Camera::lookAt(Vector3 eye, Vector3 target) {
    if((eye - target).length() < 0.001f) return;

    this->vPos = eye;

    // https://stackoverflow.com/questions/1996957/conversion-euler-to-matrix-and-matrix-to-euler
    // https://gamedev.stackexchange.com/questions/50963/how-to-extract-euler-angles-from-transformation-matrix

    Matrix4 lookAtMatrix = buildMatrixLookAt(eye, target, this->vYAxis);

    float yaw = std::atan2(-lookAtMatrix[8], lookAtMatrix[0]);
    float pitch = std::asin(-lookAtMatrix[6]);
    /// float roll = atan2(lookAtMatrix[4], lookAtMatrix[5]);

    this->fYaw = 180.0f + rad2deg(yaw);
    this->fPitch = rad2deg(pitch);

    this->updateVectors();
}

void Camera::setType(CAMERA_TYPE camType) {
    if(camType == this->camType) return;

    this->camType = camType;

    if(this->camType == CAMERA_TYPE_ORBIT)
        this->setPos(this->vOrbitTarget);
    else
        this->vPos = this->vOrbitTarget;
}

void Camera::setPos(Vector3 pos) {
    this->vOrbitTarget = pos;

    if(this->camType == CAMERA_TYPE_ORBIT)
        this->vPos = this->vOrbitTarget + this->vViewDir * -this->fOrbitDistance;
    else if(this->camType == CAMERA_TYPE_FIRST_PERSON)
        this->vPos = pos;
}

void Camera::setOrbitDistance(float orbitDistance) {
    this->fOrbitDistance = orbitDistance;
    if(this->fOrbitDistance < 0) this->fOrbitDistance = 0;
}

void Camera::setRotation(float yawDeg, float pitchDeg, float rollDeg) {
    this->fYaw = yawDeg;
    this->fPitch = pitchDeg;
    this->fRoll = rollDeg;
    this->updateVectors();
}

void Camera::setYaw(float yawDeg) {
    this->fYaw = yawDeg;
    this->updateVectors();
}

void Camera::setPitch(float pitchDeg) {
    this->fPitch = pitchDeg;
    this->updateVectors();
}

void Camera::setRoll(float rollDeg) {
    this->fRoll = rollDeg;
    this->updateVectors();
}

Vector3 Camera::getNextPosition(Vector3 velocity) const {
    return this->vPos + ((this->worldRotation * this->rotation) * velocity);
}

Vector3 Camera::getProjectedVector(Vector3 point, float screenWidth, float screenHeight, float zn, float zf) const {
    // TODO: fix this shit

    // build matrices
    const Matrix4 worldMatrix = Matrix4().translate(-this->vPos);
    const Matrix4 viewMatrix = this->rotation.getMatrix();
    const Matrix4 projectionMatrix = buildMatrixPerspectiveFov(this->fFov, screenWidth / screenHeight, zn, zf);

    // complete matrix
    Matrix4 worldViewProj = (worldMatrix * viewMatrix * projectionMatrix);

    // transform 3d point to 2d
    point = Vector3TransformCoord(point, worldViewProj);

    // convert projected screen coordinates to real screen coordinates
    point.x = screenWidth * ((point.x + 1.0f) / 2.0f);
    point.y = screenHeight * ((point.y + 1.0f) / 2.0f);
    point.z = zn + point.z * (zf - zn);

    return point;
}

Vector3 Camera::getUnProjectedVector(Vector2 point, float screenWidth, float screenHeight, float zn, float zf) const {
    Matrix4 projectionMatrix = buildMatrixPerspectiveFov(this->fFov, screenWidth / screenHeight, zn, zf);

    // transform pick position from screen space into 3d space
    Vector4 v;
    v.x = ((2.0f * point.x) / screenWidth) - 1.0f;
    v.y = ((2.0f * point.y) / screenHeight) - 1.0f;
    v.z = (0.0f - zn) / (zf - zn);  // this is very important
    v.w = 1.0f;

    const Matrix4 inverseProjectionMatrix = projectionMatrix.invert();
    const Matrix4 inverseViewMatrix = this->rotation.getMatrix().invert();

    v = inverseViewMatrix * inverseProjectionMatrix * v;

    return this->vPos - Vector3(v.x, v.y, v.z);
}

bool Camera::isPointVisibleFrustum(Vector3 point) const {
    float epsilon = 0.01f;

    // left
    float d11 = planeDotCoord(this->viewFrustum[0], point);

    // right
    float d21 = planeDotCoord(this->viewFrustum[1], point);

    // top
    float d31 = planeDotCoord(this->viewFrustum[2], point);

    // bottom
    float d41 = planeDotCoord(this->viewFrustum[3], point);

    if((d11 < epsilon) || (d21 < epsilon) || (d31 < epsilon) || (d41 < epsilon)) return false;

    return true;
}

bool Camera::isPointVisiblePlane(Vector3 point) const {
    float epsilon = 0.0f;

    if(!(planeDotCoord(this->vViewDir, this->vPos, point) < epsilon)) return true;

    return false;
}

float Camera::planeDotCoord(CAM_PLANE plane, Vector3 point) {
    return ((plane.a * point.x) + (plane.b * point.y) + (plane.c * point.z) + plane.d);
}

float Camera::planeDotCoord(Vector3 planeNormal, Vector3 planePoint, Vector3 &pv) {
    return planeNormal.dot(pv - planePoint);
}
