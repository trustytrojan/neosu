///////////////////////////////////////////////////////////////////////////////
// Matrice.cpp
// ===========
// NxN Matrix Math classes
//
// The elements of the matrix are stored as column major order.
// | 0 2 |    | 0 3 6 |    |  0  4  8 12 |
// | 1 3 |    | 1 4 7 |    |  1  5  9 13 |
//            | 2 5 8 |    |  2  6 10 14 |
//                         |  3  7 11 15 |
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2005-06-24
// UPDATED: 2014-09-21
//
// Copyright (C) 2005 Song Ho Ahn
///////////////////////////////////////////////////////////////////////////////

#include "Matrices.h"

#include <algorithm>
#include <cmath>

const float DEG2RAD = 3.141593f / 180;
const float EPSILON = 0.00001f;

///////////////////////////////////////////////////////////////////////////////
// transpose 2x2 matrix
///////////////////////////////////////////////////////////////////////////////
Matrix2& Matrix2::transpose() {
    std::swap(this->m[1], this->m[2]);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// return the determinant of 2x2 matrix
///////////////////////////////////////////////////////////////////////////////
float Matrix2::getDeterminant() { return this->m[0] * this->m[3] - this->m[1] * this->m[2]; }

///////////////////////////////////////////////////////////////////////////////
// inverse of 2x2 matrix
// If cannot find inverse, set identity matrix
///////////////////////////////////////////////////////////////////////////////
Matrix2& Matrix2::invert() {
    float determinant = this->getDeterminant();
    if(fabs(determinant) <= EPSILON) {
        return this->identity();
    }

    float tmp = this->m[0];  // copy the first element
    float invDeterminant = 1.0f / determinant;
    this->m[0] = invDeterminant * this->m[3];
    this->m[1] = -invDeterminant * this->m[1];
    this->m[2] = -invDeterminant * this->m[2];
    this->m[3] = invDeterminant * tmp;

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// transpose 3x3 matrix
///////////////////////////////////////////////////////////////////////////////
Matrix3& Matrix3::transpose() {
    std::swap(this->m[1], this->m[3]);
    std::swap(this->m[2], this->m[6]);
    std::swap(this->m[5], this->m[7]);

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// return determinant of 3x3 matrix
///////////////////////////////////////////////////////////////////////////////
float Matrix3::getDeterminant() {
    return this->m[0] * (this->m[4] * this->m[8] - this->m[5] * this->m[7]) -
           this->m[1] * (this->m[3] * this->m[8] - this->m[5] * this->m[6]) +
           this->m[2] * (this->m[3] * this->m[7] - this->m[4] * this->m[6]);
}

///////////////////////////////////////////////////////////////////////////////
// inverse 3x3 matrix
// If cannot find inverse, set identity matrix
///////////////////////////////////////////////////////////////////////////////
Matrix3& Matrix3::invert() {
    float determinant, invDeterminant;
    float tmp[9];

    tmp[0] = this->m[4] * this->m[8] - this->m[5] * this->m[7];
    tmp[1] = this->m[2] * this->m[7] - this->m[1] * this->m[8];
    tmp[2] = this->m[1] * this->m[5] - this->m[2] * this->m[4];
    tmp[3] = this->m[5] * this->m[6] - this->m[3] * this->m[8];
    tmp[4] = this->m[0] * this->m[8] - this->m[2] * this->m[6];
    tmp[5] = this->m[2] * this->m[3] - this->m[0] * this->m[5];
    tmp[6] = this->m[3] * this->m[7] - this->m[4] * this->m[6];
    tmp[7] = this->m[1] * this->m[6] - this->m[0] * this->m[7];
    tmp[8] = this->m[0] * this->m[4] - this->m[1] * this->m[3];

    // check determinant if it is 0
    determinant = this->m[0] * tmp[0] + this->m[1] * tmp[3] + this->m[2] * tmp[6];
    if(fabs(determinant) <= EPSILON) {
        return this->identity();  // cannot inverse, make it idenety matrix
    }

    // divide by the determinant
    invDeterminant = 1.0f / determinant;
    this->m[0] = invDeterminant * tmp[0];
    this->m[1] = invDeterminant * tmp[1];
    this->m[2] = invDeterminant * tmp[2];
    this->m[3] = invDeterminant * tmp[3];
    this->m[4] = invDeterminant * tmp[4];
    this->m[5] = invDeterminant * tmp[5];
    this->m[6] = invDeterminant * tmp[6];
    this->m[7] = invDeterminant * tmp[7];
    this->m[8] = invDeterminant * tmp[8];

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// transpose 4x4 matrix
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::transpose() {
    std::swap(this->m[1], this->m[4]);
    std::swap(this->m[2], this->m[8]);
    std::swap(this->m[3], this->m[12]);
    std::swap(this->m[6], this->m[9]);
    std::swap(this->m[7], this->m[13]);
    std::swap(this->m[11], this->m[14]);

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// inverse 4x4 matrix
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::invert() {
    // If the 4th row is [0,0,0,1] then it is affine matrix and
    // it has no projective transformation.
    if(this->m[3] == 0 && this->m[7] == 0 && this->m[11] == 0 && this->m[15] == 1)
        this->invertAffine();
    else {
        this->invertGeneral();
        /*@@ invertProjective() is not optimized (slower than generic one)
        if(fabs(m[0]*m[5] - m[1]*m[4]) > EPSILON)
            this->invertProjective();   // inverse using matrix partition
        else
            this->invertGeneral();      // generalized inverse
        */
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// compute the inverse of 4x4 Euclidean transformation matrix
//
// Euclidean transformation is translation, rotation, and reflection.
// With Euclidean transform, only the position and orientation of the object
// will be changed. Euclidean transform does not change the shape of an object
// (no scaling). Length and angle are reserved.
//
// Use inverseAffine() if the matrix has scale and shear transformation.
//
// M = [ R | T ]
//     [ --+-- ]    (R denotes 3x3 rotation/reflection matrix)
//     [ 0 | 1 ]    (T denotes 1x3 translation matrix)
//
// y = M*x  ->  y = R*x + T  ->  x = R^-1*(y - T)  ->  x = R^T*y - R^T*T
// (R is orthogonal,  R^-1 = R^T)
//
//  [ R | T ]-1    [ R^T | -R^T * T ]    (R denotes 3x3 rotation matrix)
//  [ --+-- ]   =  [ ----+--------- ]    (T denotes 1x3 translation)
//  [ 0 | 1 ]      [  0  |     1    ]    (R^T denotes R-transpose)
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::invertEuclidean() {
    // transpose 3x3 rotation matrix part
    // | R^T | 0 |
    // | ----+-- |
    // |  0  | 1 |
    float tmp;
    tmp = this->m[1];
    this->m[1] = this->m[4];
    this->m[4] = tmp;
    tmp = this->m[2];
    this->m[2] = this->m[8];
    this->m[8] = tmp;
    tmp = this->m[6];
    this->m[6] = this->m[9];
    this->m[9] = tmp;

    // compute translation part -R^T * T
    // | 0 | -R^T x |
    // | --+------- |
    // | 0 |   0    |
    float x = this->m[12];
    float y = this->m[13];
    float z = this->m[14];
    this->m[12] = -(this->m[0] * x + this->m[4] * y + this->m[8] * z);
    this->m[13] = -(this->m[1] * x + this->m[5] * y + this->m[9] * z);
    this->m[14] = -(this->m[2] * x + this->m[6] * y + this->m[10] * z);

    // last row should be unchanged (0,0,0,1)

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// compute the inverse of a 4x4 affine transformation matrix
//
// Affine transformations are generalizations of Euclidean transformations.
// Affine transformation includes translation, rotation, reflection, scaling,
// and shearing. Length and angle are NOT preserved.
// M = [ R | T ]
//     [ --+-- ]    (R denotes 3x3 rotation/scale/shear matrix)
//     [ 0 | 1 ]    (T denotes 1x3 translation matrix)
//
// y = M*x  ->  y = R*x + T  ->  x = R^-1*(y - T)  ->  x = R^-1*y - R^-1*T
//
//  [ R | T ]-1   [ R^-1 | -R^-1 * T ]
//  [ --+-- ]   = [ -----+---------- ]
//  [ 0 | 1 ]     [  0   +     1     ]
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::invertAffine() {
    // R^-1
    Matrix3 r(this->m[0], this->m[1], this->m[2], this->m[4], this->m[5], this->m[6], this->m[8], this->m[9],
              this->m[10]);
    r.invert();
    this->m[0] = r[0];
    this->m[1] = r[1];
    this->m[2] = r[2];
    this->m[4] = r[3];
    this->m[5] = r[4];
    this->m[6] = r[5];
    this->m[8] = r[6];
    this->m[9] = r[7];
    this->m[10] = r[8];

    // -R^-1 * T
    float x = this->m[12];
    float y = this->m[13];
    float z = this->m[14];
    this->m[12] = -(r[0] * x + r[3] * y + r[6] * z);
    this->m[13] = -(r[1] * x + r[4] * y + r[7] * z);
    this->m[14] = -(r[2] * x + r[5] * y + r[8] * z);

    // last row should be unchanged (0,0,0,1)
    // m[3] = m[7] = m[11] = 0.0f;
    // m[15] = 1.0f;

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// inverse matrix using matrix partitioning (blockwise inverse)
// It devides a 4x4 matrix into 4 of 2x2 matrices. It works in case of where
// det(A) != 0. If not, use the generic inverse method
// inverse formula.
// M = [ A | B ]    A, B, C, D are 2x2 matrix blocks
//     [ --+-- ]    det(M) = |A| * |D - ((C * A^-1) * B)|
//     [ C | D ]
//
// M^-1 = [ A' | B' ]   A' = A^-1 - (A^-1 * B) * C'
//        [ ---+--- ]   B' = (A^-1 * B) * -D'
//        [ C' | D' ]   C' = -D' * (C * A^-1)
//                      D' = (D - ((C * A^-1) * B))^-1
//
// NOTE: I wrap with () if it it used more than once.
//       The matrix is invertable even if det(A)=0, so must check det(A) before
//       calling this function, and use invertGeneric() instead.
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::invertProjective() {
    // partition
    Matrix2 a(this->m[0], this->m[1], this->m[4], this->m[5]);
    Matrix2 b(this->m[8], this->m[9], this->m[12], this->m[13]);
    Matrix2 c(this->m[2], this->m[3], this->m[6], this->m[7]);
    Matrix2 d(this->m[10], this->m[11], this->m[14], this->m[15]);

    // pre-compute repeated parts
    a.invert();              // A^-1
    Matrix2 ab = a * b;      // A^-1 * B
    Matrix2 ca = c * a;      // C * A^-1
    Matrix2 cab = ca * b;    // C * A^-1 * B
    Matrix2 dcab = d - cab;  // D - C * A^-1 * B

    // check determinant if |D - C * A^-1 * B| = 0
    // NOTE: this function assumes det(A) is already checked. if |A|=0 then,
    //      cannot use this function.
    float determinant = dcab[0] * dcab[3] - dcab[1] * dcab[2];
    if(fabs(determinant) <= EPSILON) {
        return this->identity();
    }

    // compute D' and -D'
    Matrix2 d1 = dcab;  //  (D - C * A^-1 * B)
    d1.invert();        //  (D - C * A^-1 * B)^-1
    Matrix2 d2 = -d1;   // -(D - C * A^-1 * B)^-1

    // compute C'
    Matrix2 c1 = d2 * ca;  // -D' * (C * A^-1)

    // compute B'
    Matrix2 b1 = ab * d2;  // (A^-1 * B) * -D'

    // compute A'
    Matrix2 a1 = a - (ab * c1);  // A^-1 - (A^-1 * B) * C'

    // assemble inverse matrix
    this->m[0] = a1[0];
    this->m[4] = a1[2]; /*|*/
    this->m[8] = b1[0];
    this->m[12] = b1[2];
    this->m[1] = a1[1];
    this->m[5] = a1[3]; /*|*/
    this->m[9] = b1[1];
    this->m[13] = b1[3];
    /*-----------------------------+-----------------------------*/
    this->m[2] = c1[0];
    this->m[6] = c1[2]; /*|*/
    this->m[10] = d1[0];
    this->m[14] = d1[2];
    this->m[3] = c1[1];
    this->m[7] = c1[3]; /*|*/
    this->m[11] = d1[1];
    this->m[15] = d1[3];

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// compute the inverse of a general 4x4 matrix using Cramer's Rule
// If cannot find inverse, return indentity matrix
// M^-1 = adj(M) / det(M)
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::invertGeneral() {
    // get cofactors of minor matrices
    float cofactor0 = this->getCofactor(this->m[5], this->m[6], this->m[7], this->m[9], this->m[10], this->m[11],
                                        this->m[13], this->m[14], this->m[15]);
    float cofactor1 = this->getCofactor(this->m[4], this->m[6], this->m[7], this->m[8], this->m[10], this->m[11],
                                        this->m[12], this->m[14], this->m[15]);
    float cofactor2 = this->getCofactor(this->m[4], this->m[5], this->m[7], this->m[8], this->m[9], this->m[11],
                                        this->m[12], this->m[13], this->m[15]);
    float cofactor3 = this->getCofactor(this->m[4], this->m[5], this->m[6], this->m[8], this->m[9], this->m[10],
                                        this->m[12], this->m[13], this->m[14]);

    // get determinant
    float determinant =
        this->m[0] * cofactor0 - this->m[1] * cofactor1 + this->m[2] * cofactor2 - this->m[3] * cofactor3;
    if(fabs(determinant) <= EPSILON) {
        return this->identity();
    }

    // get rest of cofactors for adj(M)
    float cofactor4 = this->getCofactor(this->m[1], this->m[2], this->m[3], this->m[9], this->m[10], this->m[11],
                                        this->m[13], this->m[14], this->m[15]);
    float cofactor5 = this->getCofactor(this->m[0], this->m[2], this->m[3], this->m[8], this->m[10], this->m[11],
                                        this->m[12], this->m[14], this->m[15]);
    float cofactor6 = this->getCofactor(this->m[0], this->m[1], this->m[3], this->m[8], this->m[9], this->m[11],
                                        this->m[12], this->m[13], this->m[15]);
    float cofactor7 = this->getCofactor(this->m[0], this->m[1], this->m[2], this->m[8], this->m[9], this->m[10],
                                        this->m[12], this->m[13], this->m[14]);

    float cofactor8 = this->getCofactor(this->m[1], this->m[2], this->m[3], this->m[5], this->m[6], this->m[7],
                                        this->m[13], this->m[14], this->m[15]);
    float cofactor9 = this->getCofactor(this->m[0], this->m[2], this->m[3], this->m[4], this->m[6], this->m[7],
                                        this->m[12], this->m[14], this->m[15]);
    float cofactor10 = this->getCofactor(this->m[0], this->m[1], this->m[3], this->m[4], this->m[5], this->m[7],
                                         this->m[12], this->m[13], this->m[15]);
    float cofactor11 = this->getCofactor(this->m[0], this->m[1], this->m[2], this->m[4], this->m[5], this->m[6],
                                         this->m[12], this->m[13], this->m[14]);

    float cofactor12 = this->getCofactor(this->m[1], this->m[2], this->m[3], this->m[5], this->m[6], this->m[7],
                                         this->m[9], this->m[10], this->m[11]);
    float cofactor13 = this->getCofactor(this->m[0], this->m[2], this->m[3], this->m[4], this->m[6], this->m[7],
                                         this->m[8], this->m[10], this->m[11]);
    float cofactor14 = this->getCofactor(this->m[0], this->m[1], this->m[3], this->m[4], this->m[5], this->m[7],
                                         this->m[8], this->m[9], this->m[11]);
    float cofactor15 = this->getCofactor(this->m[0], this->m[1], this->m[2], this->m[4], this->m[5], this->m[6],
                                         this->m[8], this->m[9], this->m[10]);

    // build inverse matrix = adj(M) / det(M)
    // adjugate of M is the transpose of the cofactor matrix of M
    float invDeterminant = 1.0f / determinant;
    this->m[0] = invDeterminant * cofactor0;
    this->m[1] = -invDeterminant * cofactor4;
    this->m[2] = invDeterminant * cofactor8;
    this->m[3] = -invDeterminant * cofactor12;

    this->m[4] = -invDeterminant * cofactor1;
    this->m[5] = invDeterminant * cofactor5;
    this->m[6] = -invDeterminant * cofactor9;
    this->m[7] = invDeterminant * cofactor13;

    this->m[8] = invDeterminant * cofactor2;
    this->m[9] = -invDeterminant * cofactor6;
    this->m[10] = invDeterminant * cofactor10;
    this->m[11] = -invDeterminant * cofactor14;

    this->m[12] = -invDeterminant * cofactor3;
    this->m[13] = invDeterminant * cofactor7;
    this->m[14] = -invDeterminant * cofactor11;
    this->m[15] = invDeterminant * cofactor15;

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// return determinant of 4x4 matrix
///////////////////////////////////////////////////////////////////////////////
float Matrix4::getDeterminant() {
    return this->m[0] * this->getCofactor(this->m[5], this->m[6], this->m[7], this->m[9], this->m[10], this->m[11],
                                          this->m[13], this->m[14], this->m[15]) -
           this->m[1] * this->getCofactor(this->m[4], this->m[6], this->m[7], this->m[8], this->m[10], this->m[11],
                                          this->m[12], this->m[14], this->m[15]) +
           this->m[2] * this->getCofactor(this->m[4], this->m[5], this->m[7], this->m[8], this->m[9], this->m[11],
                                          this->m[12], this->m[13], this->m[15]) -
           this->m[3] * this->getCofactor(this->m[4], this->m[5], this->m[6], this->m[8], this->m[9], this->m[10],
                                          this->m[12], this->m[13], this->m[14]);
}

///////////////////////////////////////////////////////////////////////////////
// compute cofactor of 3x3 minor matrix without sign
// input params are 9 elements of the minor matrix
// NOTE: The caller must know its sign.
///////////////////////////////////////////////////////////////////////////////
float Matrix4::getCofactor(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8) {
    return m0 * (m4 * m8 - m5 * m7) - m1 * (m3 * m8 - m5 * m6) + m2 * (m3 * m7 - m4 * m6);
}

///////////////////////////////////////////////////////////////////////////////
// translate this matrix by (x, y, z)
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::translate(const Vector3& v) { return this->translate(v.x, v.y, v.z); }

Matrix4& Matrix4::translate(float x, float y, float z) {
    this->m[0] += this->m[3] * x;
    this->m[4] += this->m[7] * x;
    this->m[8] += this->m[11] * x;
    this->m[12] += this->m[15] * x;
    this->m[1] += this->m[3] * y;
    this->m[5] += this->m[7] * y;
    this->m[9] += this->m[11] * y;
    this->m[13] += this->m[15] * y;
    this->m[2] += this->m[3] * z;
    this->m[6] += this->m[7] * z;
    this->m[10] += this->m[11] * z;
    this->m[14] += this->m[15] * z;

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// uniform scale
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::scale(float s) { return this->scale(s, s, s); }

Matrix4& Matrix4::scale(float x, float y, float z) {
    this->m[0] *= x;
    this->m[4] *= x;
    this->m[8] *= x;
    this->m[12] *= x;
    this->m[1] *= y;
    this->m[5] *= y;
    this->m[9] *= y;
    this->m[13] *= y;
    this->m[2] *= z;
    this->m[6] *= z;
    this->m[10] *= z;
    this->m[14] *= z;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// build a rotation matrix with given angle(degree) and rotation axis, then
// multiply it with this object
///////////////////////////////////////////////////////////////////////////////
Matrix4& Matrix4::rotate(float angle, const Vector3& axis) { return this->rotate(angle, axis.x, axis.y, axis.z); }

Matrix4& Matrix4::rotate(float angle, float x, float y, float z) {
    float c = cosf(angle * DEG2RAD);  // cosine
    float s = sinf(angle * DEG2RAD);  // sine
    float c1 = 1.0f - c;              // 1 - c
    float m0 = this->m[0], m4 = this->m[4], m8 = this->m[8], m12 = this->m[12], m1 = this->m[1], m5 = this->m[5],
          m9 = this->m[9], m13 = this->m[13], m2 = this->m[2], m6 = this->m[6], m10 = this->m[10], m14 = this->m[14];

    // build rotation matrix
    float r0 = x * x * c1 + c;
    float r1 = x * y * c1 + z * s;
    float r2 = x * z * c1 - y * s;
    float r4 = x * y * c1 - z * s;
    float r5 = y * y * c1 + c;
    float r6 = y * z * c1 + x * s;
    float r8 = x * z * c1 + y * s;
    float r9 = y * z * c1 - x * s;
    float r10 = z * z * c1 + c;

    // multiply rotation matrix
    this->m[0] = r0 * m0 + r4 * m1 + r8 * m2;
    this->m[1] = r1 * m0 + r5 * m1 + r9 * m2;
    this->m[2] = r2 * m0 + r6 * m1 + r10 * m2;
    this->m[4] = r0 * m4 + r4 * m5 + r8 * m6;
    this->m[5] = r1 * m4 + r5 * m5 + r9 * m6;
    this->m[6] = r2 * m4 + r6 * m5 + r10 * m6;
    this->m[8] = r0 * m8 + r4 * m9 + r8 * m10;
    this->m[9] = r1 * m8 + r5 * m9 + r9 * m10;
    this->m[10] = r2 * m8 + r6 * m9 + r10 * m10;
    this->m[12] = r0 * m12 + r4 * m13 + r8 * m14;
    this->m[13] = r1 * m12 + r5 * m13 + r9 * m14;
    this->m[14] = r2 * m12 + r6 * m13 + r10 * m14;

    return *this;
}

Matrix4& Matrix4::rotateX(float angle) {
    float c = cosf(angle * DEG2RAD);
    float s = sinf(angle * DEG2RAD);
    float m1 = this->m[1], m2 = this->m[2], m5 = this->m[5], m6 = this->m[6], m9 = this->m[9], m10 = this->m[10],
          m13 = this->m[13], m14 = this->m[14];

    this->m[1] = m1 * c + m2 * -s;
    this->m[2] = m1 * s + m2 * c;
    this->m[5] = m5 * c + m6 * -s;
    this->m[6] = m5 * s + m6 * c;
    this->m[9] = m9 * c + m10 * -s;
    this->m[10] = m9 * s + m10 * c;
    this->m[13] = m13 * c + m14 * -s;
    this->m[14] = m13 * s + m14 * c;

    return *this;
}

Matrix4& Matrix4::rotateY(float angle) {
    float c = cosf(angle * DEG2RAD);
    float s = sinf(angle * DEG2RAD);
    float m0 = this->m[0], m2 = this->m[2], m4 = this->m[4], m6 = this->m[6], m8 = this->m[8], m10 = this->m[10],
          m12 = this->m[12], m14 = this->m[14];

    this->m[0] = m0 * c + m2 * s;
    this->m[2] = m0 * -s + m2 * c;
    this->m[4] = m4 * c + m6 * s;
    this->m[6] = m4 * -s + m6 * c;
    this->m[8] = m8 * c + m10 * s;
    this->m[10] = m8 * -s + m10 * c;
    this->m[12] = m12 * c + m14 * s;
    this->m[14] = m12 * -s + m14 * c;

    return *this;
}

Matrix4& Matrix4::rotateZ(float angle) {
    float c = cosf(angle * DEG2RAD);
    float s = sinf(angle * DEG2RAD);
    float m0 = this->m[0], m1 = this->m[1], m4 = this->m[4], m5 = this->m[5], m8 = this->m[8], m9 = this->m[9],
          m12 = this->m[12], m13 = this->m[13];

    this->m[0] = m0 * c + m1 * -s;
    this->m[1] = m0 * s + m1 * c;
    this->m[4] = m4 * c + m5 * -s;
    this->m[5] = m4 * s + m5 * c;
    this->m[8] = m8 * c + m9 * -s;
    this->m[9] = m8 * s + m9 * c;
    this->m[12] = m12 * c + m13 * -s;
    this->m[13] = m12 * s + m13 * c;

    return *this;
}
