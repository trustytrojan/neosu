//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		thread wrapper
//
// $NoKeywords: $thread
//===============================================================================//

#pragma once
#ifndef THREAD_H
#define THREAD_H

#ifdef __EXCEPTIONS
#include "Engine.h" // for debugLog
#endif

#include <functional>
#include <stop_token>
#include <thread>

class McThread final
{
public:
	

	typedef void *(*START_ROUTINE)(void *);
	typedef void *(*START_ROUTINE_WITH_STOP_TOKEN)(void *, std::stop_token);

public:
	// backward compatibility
	McThread(START_ROUTINE start_routine, void *arg)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		this->bReady = false;
		this->bUsesStopToken = false;
#ifdef __EXCEPTIONS
		try
		{
#endif
			// wrap the legacy function for jthreads
			this->thread = std::jthread([start_routine, arg](std::stop_token) -> void { start_routine(arg); });
			this->bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	// constructor which takes a stop token directly
	McThread(START_ROUTINE_WITH_STOP_TOKEN start_routine, void *arg)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		this->bReady = false;
		this->bUsesStopToken = true;
#ifdef __EXCEPTIONS
		try
		{
#endif
			this->thread = std::jthread([start_routine, arg](std::stop_token token) -> void { start_routine(arg, token); });
			this->bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	// ctor that takes a std::function directly
	McThread(std::function<void(std::stop_token)> func)
#ifndef __EXCEPTIONS
	    noexcept
#endif
	{
		this->bReady = false;
		this->bUsesStopToken = true;
#ifdef __EXCEPTIONS
		try
		{
#endif
			this->thread = std::jthread(std::move(func));
			this->bReady = true;
#ifdef __EXCEPTIONS
		}
		catch (const std::system_error &e)
		{
			debugLog("McThread Error: std::jthread constructor exception: {:s}\n", e.what());
		}
#endif
	}

	~McThread()
	{
		if (!this->bReady)
			return;

		this->bReady = false;

		// jthread automatically requests stop and joins in destructor
		// but we can be explicit about it
		if (this->thread.joinable())
		{
			this->thread.request_stop();
			this->thread.join();
		}
	}

	[[nodiscard]] inline bool isReady() const { return this->bReady; }
	[[nodiscard]] inline bool isStopRequested() const { return this->bReady && this->thread.get_stop_token().stop_requested(); }

	inline void requestStop()
	{
		if (this->bReady)
			this->thread.request_stop();
	}

	[[nodiscard]] inline std::stop_token getStopToken() const { return this->thread.get_stop_token(); }
	[[nodiscard]] inline bool usesStopToken() const { return this->bUsesStopToken; }

private:
	std::jthread thread;
	bool bReady;
	bool bUsesStopToken;
};

#endif
