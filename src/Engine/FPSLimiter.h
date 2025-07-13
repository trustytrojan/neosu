//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		frame limiter, to be called at the end of a main loop interation
//
// $NoKeywords: $fps $limiter
//===============================================================================//

#pragma once

namespace FPSLimiter {
void limit_frames(int targetFPS);
void reset();
};  // namespace FPSLimiter
