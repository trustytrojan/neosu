//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		individual engine components to compile with
//
// $NoKeywords: $feat
//===============================================================================//

#ifndef ENGINEFEATURES_H
#define ENGINEFEATURES_H

#ifndef MCENGINE_DATA_DIR
#define MCENGINE_DATA_DIR "./"
#endif

#define MCENGINE_APP Osu

/*
 * std::thread/std::mutex support
 */
#ifndef MCENGINE_FEATURE_MULTITHREADING
#define MCENGINE_FEATURE_MULTITHREADING
#endif

/*
 * pthread support
 */
#ifndef MCENGINE_FEATURE_PTHREADS
#define MCENGINE_FEATURE_PTHREADS
#endif

/*
 * OpenGL graphics (Desktop, legacy + modern)
 */
#ifndef MCENGINE_FEATURE_OPENGL
#define MCENGINE_FEATURE_OPENGL
#endif

/*
 * OpenGL graphics (Mobile, ES/EGL)
 */
// #define MCENGINE_FEATURE_OPENGLES

/*
 * DirectX 11 graphics
 */
// #define MCENGINE_FEATURE_DIRECTX11

/*
 * ENet & CURL networking
 */
#define MCENGINE_FEATURE_NETWORKING

/*
 * OpenCL
 */
// #define MCENGINE_FEATURE_OPENCL

/*
 * Vulkan
 */
// #define MCENGINE_FEATURE_VULKAN

/*
 * OpenVR
 */
// #define MCENGINE_FEATURE_OPENVR

/*
 * Discord RPC (rich presence)
 */
// #define MCENGINE_FEATURE_DISCORD

#endif
