//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		prepares a gauss kernel to use with the blur shader
//
// $NoKeywords: $gblur
//===============================================================================//

#ifndef GAUSSIANBLURKERNEL_H
#define GAUSSIANBLURKERNEL_H

#include "cbase.h"

class GaussianBlurKernel {
   public:
    GaussianBlurKernel(int kernelSize, float radius, int targetWidth, int targetHeight);
    ~GaussianBlurKernel();

    void rebuild() {
        this->release();
        this->build();
    }
    void release();

    inline const int getKernelSize() { return this->iKernelSize; }
    inline const float getRadius() { return this->fRadius; }

    inline float* getKernel() { return &this->kernel.front(); }
    inline float* getOffsetsHorizontal() { return &this->offsetsHorizontal.front(); }
    inline float* getOffsetsVertical() { return &this->offsetsVertical.front(); }

   private:
    void build();

    float fRadius;
    int iKernelSize;
    int iTargetWidth, iTargetHeight;

    std::vector<float> kernel;
    std::vector<float> offsetsHorizontal;
    std::vector<float> offsetsVertical;
};

#endif
