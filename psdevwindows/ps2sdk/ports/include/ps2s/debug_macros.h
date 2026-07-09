/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

// this file should NOT be enclosed in #ifndef/#define/#endif

#include "ps2s/macros.h"
#include <assert.h>
#include <stdio.h>
// #include "ps2s/timer.h"

#ifdef mAssert
#undef mAssert
#undef mError
#undef mErrorIf
#undef mWarn
#undef mWarnIf
#undef mDebugPrint
#undef MAKE_TIMER_NAME
#undef INIT_TIMERS
#undef START_TIMER
#undef STOP_TIMER
#undef DISPLAY_TIMERS
#undef UPDATE_TIMERS
#undef DELETE_TIMERS
#endif

#if (defined _DEBUG && !mExpandCat(DEBUG_MODULE_NAME, _NO_ERRORS)) \
    || mExpandCat(DEBUG_MODULE_NAME, _DO_ERRORS)
#define mAssert(_a) assert(_a);
#define mError(_msg, _args...)                                   \
    printf("ERROR: " __FILE__ ", " mExpandQuote(__LINE__) ": "); \
    printf(_msg, ##_args);                                       \
    printf("\n");                                                \
    mAssert(0);

#define mErrorIf(_cond, _msg, _args...) \
    if (_cond) {                        \
        mError(_msg, ##_args);          \
    }
#else
#define mAssert(_a)
#define mError(_msg, _args...)
#define mErrorIf(_cond, _msg, _args...)
#endif

// warnings

#if (defined _DEBUG && !mExpandCat(DEBUG_MODULE_NAME, _NO_WARNINGS)) \
    || mExpandCat(DEBUG_MODULE_NAME, _DO_WARNINGS)
#define mWarn(_msg, _args...)                                      \
    printf("WARNING: " __FILE__ ", " mExpandQuote(__LINE__) ": "); \
    printf(_msg, ##_args);                                         \
    printf("\n");
#define mWarnIf(_cond, _msg, _args...) \
    if (_cond) {                       \
        mWarn(_msg, ##_args);          \
    }
#else
#define mWarn(_msg, _args...)
#define mWarnIf(_cond, _msg, _args...)
#endif

// debug prints

#if (defined _DEBUG && !mExpandCat(DEBUG_MODULE_NAME, _NO_DPRINTS)) \
    || mExpandCat(DEBUG_MODULE_NAME, _DO_DPRINTS)
#define mDebugPrint(_msg, _args...)                              \
    printf("DEBUG: " __FILE__ ", " mExpandQuote(__LINE__) ": "); \
    printf(_msg, ##_args);                                       \
    printf("\n");
#else
#define mDebugPrint(_msg, _args...)
#endif

// timers

#if (defined _DEBUG && !mExpandCat(DEBUG_MODULE_NAME, _NO_TIMERS)) \
    || mExpandCat(DEBUG_MODULE_NAME, _DO_TIMERS)
#define MAKE_TIMER_NAME(__name) "Make uNiQuE" __name
#define INIT_TIMERS(__realTimer, __res) CHTimer::InitHierarchy(__realTimer, __res)
#define START_TIMER(__name) CHTimer::StartTimer(MAKE_TIMER_NAME(__name))
#define STOP_TIMER(__name) CHTimer::StopTimer(MAKE_TIMER_NAME(__name))
#define DISPLAY_TIMERS() CHTimer::DisplayHierarchy()
#define UPDATE_TIMERS() CHTimer::UpdateHierarchy()
#define DELETE_TIMERS() CHTimer::DeleteHierarchy()
#else
#define INIT_TIMERS(__realTimer, __res)
#define START_TIMER(__name)
#define STOP_TIMER(__name)
#define DISPLAY_TIMERS()
#define UPDATE_TIMERS()
#define DELETE_TIMERS()
#endif

#define mInitTimers INIT_TIMERS
#define mStartTimer START_TIMER
#define mStopTimer STOP_TIMER
#define mDisplayTimers DISPLAY_TIMERS
#define mUpdateTimers UPDATE_TIMERS
#define mDeleteTimers DELETE_TIMERS
