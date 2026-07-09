/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_core_h
#define ps2s_core_h

#include <malloc.h>

//  #include "eetypes.h"
//  #include "eestruct.h"
#include "ps2s/debug.h"
#include "ps2s/types.h"

namespace Core {

/********************************************
    * MemMappings
    */

namespace MemMappings {
    static const uint32_t Normal       = 0x00000000,
                      Uncached     = 0x20000000,
                      UncachedAccl = 0x30000000,

                      SP = 0x70000000,

                      VU0Code = 0x11000000,
                      VU0Data = 0x11004000,
                      VU1Code = 0x11008000,
                      VU1Data = 0x1100c000;
}

template <class ptrType>
inline ptrType MakePtrNormal(ptrType ptr)
{
    return reinterpret_cast<ptrType>((uint32_t)ptr & 0x0fffffff);
}

template <class ptrType>
inline ptrType MakePtrUncached(ptrType ptr)
{
    return reinterpret_cast<ptrType>((uint32_t)MakePtrNormal(ptr) | MemMappings::Uncached);
}

template <class ptrType>
inline ptrType MakePtrUncachedAccl(ptrType ptr)
{
    return reinterpret_cast<ptrType>((uint32_t)MakePtrNormal(ptr) | MemMappings::UncachedAccl);
}

/********************************************
    * misc
    */

// used to override new() to allocate on qword boundaries (only for linux)

inline void* New16(size_t size)
{
    return ::operator new(size, std::align_val_t(16));
}

inline void Delete16(void* p)
{
    ::operator delete(p);
}

// cop0 counter

inline void ZeroCount(void);
inline uint32_t GetCount(void);

// fpu

inline uint32_t FToI4(float flp);

// cop0 performance counter

static const uint32_t COP0_NUM_PERF_COUNTERS = 2;
static const uint32_t COP0_NUM_PERF_EVENTS   = 17;

void SetupPerfCounters(uint32_t evt_0, uint32_t evt_1); // set up,also zero + halt

inline void HaltPerfCounters();      // stop both counters
inline void ZeroStartPerfCounters(); // clear and start both counters

inline uint32_t ReadPerfCounter0(); // read counter 0
inline uint32_t ReadPerfCounter1(); // read counter 1

// names of the events for people

extern const char* ev0_Name[COP0_NUM_PERF_EVENTS];
extern const char* ev1_Name[COP0_NUM_PERF_EVENTS];
}

/********************************************
 * Core inlines
 */

inline void
Core::ZeroCount(void)
{
    asm __volatile__("mtc0	$0, $9	\n"
                     "sync.p		  ");
}

inline uint32_t
Core::GetCount(void)
{
    uint32_t ret;

    asm __volatile__("mfc0	%0, $9	\n"
                     "sync.p		  "
                     : "=r"(ret));

    return ret;
}

inline uint32_t
Core::FToI4(float flp)
{
    uint32_t fip;

    asm __volatile__("qmtc2		%1, $vf1	\n"
                     "vftoi4	$vf1, $vf1	\n"
                     "qmfc2		%0, $vf1	\n"
                     : "=r"(fip)
                     : "r"(flp));

    return fip;
}

// performance counter inlines

inline void Core::HaltPerfCounters()
{
    asm("mtps	$0,0	\n"
        "sync.p		");
};

inline void Core::ZeroStartPerfCounters()
{
    asm __volatile__("	mtps	$0,0			# halt performance counters \n"
                     "	sync.p				# \n"
                     "  mtpc	$0,0 			# set perfcounter 0 to zero \n"
                     "  mtpc	$0,1 			# set perfcounter 1 to zero \n"
                     "  sync.p				# \n"
                     "  lui	v0,0x8000	    	# master enable \n"
                     "  mtps	v0,0 			# truly - we rule \n"
                     : // no output
                     : // no input
                     : "v0");
};

inline uint32_t Core::ReadPerfCounter0()
{
    uint32_t ret;
    asm __volatile__("	mfpc	%0,0	\n"
                     "	sync.p		"
                     : "=r"(ret));
    return ret;
}

inline uint32_t Core::ReadPerfCounter1()
{
    uint32_t ret;
    asm __volatile__("	mfpc	%0,1	\n"
                     "	sync.p		"
                     : "=r"(ret));
    return ret;
}

#endif // ps2s_core_h
