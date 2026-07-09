#pragma once

#ifdef PS2_PLATFORM

#include <kernel.h>
#include <delaythread.h>
#include <stdint.h>
#include <time.h>
#include <timer.h>

typedef uint32_t Uint32;
typedef uint64_t Uint64;

#define SDL_INIT_TIMER 0x00000001u
#define SDL_QUIT      0x100u

typedef struct SDL_QuitEvent
{
	Uint32 type;
	Uint32 timestamp;
} SDL_QuitEvent;

typedef union SDL_Event
{
	Uint32 type;
	SDL_QuitEvent quit;
} SDL_Event;

static inline int SDL_Init(Uint32)
{
	return 0;
}

static inline Uint32 SDL_GetTicks()
{
	// newlib clock() is CPU-time based on PS2 and can stop advancing while the
	// main thread sleeps. The SexyApp loop waits for this value to move before
	// running the first update, so use the EE timer system clock instead.
	return (Uint32)(GetTimerSystemTime() / (kBUSCLK / 1000ULL));
}

static inline void SDL_Delay(Uint32 ms)
{
	DelayThread((unsigned int)ms * 1000);
}

static inline Uint64 SDL_GetPerformanceCounter()
{
	return (Uint64)GetTimerSystemTime();
}

static inline Uint64 SDL_GetPerformanceFrequency()
{
	return (Uint64)kBUSCLK;
}

static inline int SDL_PushEvent(SDL_Event*)
{
	return 0;
}

#endif
