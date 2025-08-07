#pragma once
// Copyright (c) 2016, PG, All rights reserved.

#include "Vectors.h"
#include "Color.h"

#include <vector>

class Shader;
class VertexArrayObject;

namespace SliderRenderer {
VertexArrayObject *generateVAO(const std::vector<Vector2> &points, float hitcircleDiameter,
                               Vector3 translation = Vector3(0, 0, 0), bool skipOOBPoints = true);

void draw(const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter,
          float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f,
          float alpha = 1.0f, long sliderTimeForRainbow = 0);
void draw(VertexArrayObject *vao, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale,
          float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff,
          float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0,
          bool doEnableRenderTarget = true, bool doDisableRenderTarget = true,
          bool doDrawSliderFrameBufferToScreen = true);
};  // namespace SliderRenderer
