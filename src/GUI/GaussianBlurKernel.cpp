//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		prepares a gauss kernel to use with the blur shader
//
// $NoKeywords: $gblur
//===============================================================================//

#include "GaussianBlurKernel.h"

#include "Engine.h"

GaussianBlurKernel::GaussianBlurKernel(int kernelSize, float radius, int targetWidth, int targetHeight) {
    this->iKernelSize = kernelSize;
    this->fRadius = radius;
    this->iTargetWidth = targetWidth;
    this->iTargetHeight = targetHeight;

    this->build();
}

GaussianBlurKernel::~GaussianBlurKernel() { this->release(); }

void GaussianBlurKernel::build() {
    // allocate kernel
    this->kernel.reserve(this->iKernelSize);

    // allocate offsets
    this->offsetsHorizontal.reserve(this->iKernelSize);
    this->offsetsVertical.reserve(this->iKernelSize);

    // calculate kernel
    int center = this->iKernelSize / 2;
    double sum = 0.0f;
    double result = 0.0f;
    double _sigma = this->fRadius;
    double sigmaRoot = (double)std::sqrt(2 * _sigma * _sigma * PI);

    // dummy fill kernel
    for(int i = 0; i < this->iKernelSize; i++) {
        this->kernel.push_back(0.0f);
    }

    // now set the real values
    for(int i = 0; i < center; i++) {
        result = exp(-(i * i) / (double)(2 * _sigma * _sigma)) / sigmaRoot;
        this->kernel[center + i] = this->kernel[center - i] = (float)result;
        sum += result;
        if(i != 0) sum += result;
    }

    // normalize kernel
    for(int i = 0; i < center; i++) {
        this->kernel[center + i] = this->kernel[center - i] /= (float)sum;
    }

    // calculate offsets
    double xInc = 1.0 / (double)this->iTargetWidth;
    double yInc = 1.0 / (double)this->iTargetHeight;

    for(int i = -center; i < center; i++) {
        this->offsetsHorizontal.push_back((float)(i * xInc));
        this->offsetsVertical.push_back((float)(i * yInc));
    }
}

void GaussianBlurKernel::release() {
    this->kernel = std::vector<float>();
    this->offsetsHorizontal = std::vector<float>();
    this->offsetsVertical = std::vector<float>();
}
