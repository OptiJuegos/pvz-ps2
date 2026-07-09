/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_eetimer_h
#define ps2s_eetimer_h

/********************************************
 * includes
 */

// PLIN
// #include <eeregs.h>

#include "ps2s/types.h"

/********************************************
 * register types
 */

namespace Timers {

typedef struct {
    unsigned COUNT : 16; // Counter Value
    unsigned p0 : 16;
} tCount;

typedef struct {
    unsigned CLKS : 2; // Clock Select
    unsigned GATE : 1; // Gate Function
    unsigned GATS : 1; // Gate Select
    unsigned GATM : 2; // Gate mode
    unsigned ZRET : 1; // Zero Return
    unsigned CUE : 1;  // Count Up Enable
    unsigned CMPE : 1; // Interrupt by compare
    unsigned OVFE : 1; // Interrupt by overflow
    unsigned EQUF : 1; // Equal Flag
    unsigned OVFF : 1; // overflow Flag
    unsigned p0 : 20;
} tMode;

namespace Timer0 {
    static volatile void* const count = (volatile void*)0x10000000;
    static volatile void* const mode  = (volatile void*)0x10000010;
    static volatile void* const comp  = (volatile void*)0x10000020;
    static volatile void* const hold  = (volatile void*)0x10000030;
}
namespace Timer1 {
    static volatile void* const count = (volatile void*)0x10000800;
    static volatile void* const mode  = (volatile void*)0x10000810;
    static volatile void* const comp  = (volatile void*)0x10000820;
    static volatile void* const hold  = (volatile void*)0x10000830;
}
namespace Timer2 {
    static volatile void* const count = (volatile void*)0x10001000;
    static volatile void* const mode  = (volatile void*)0x10001010;
    static volatile void* const comp  = (volatile void*)0x10001020;
    // static volatile void* const hold = (volatile void*)0x10001030;
}
namespace Timer3 {
    static volatile void* const count = (volatile void*)0x10001800;
    static volatile void* const mode  = (volatile void*)0x10001810;
    static volatile void* const comp  = (volatile void*)0x10001820;
    // static volatile void* const hold = (volatile void*)0x10001830;
}
}

/********************************************
 * class def
 */

class CEETimer {
public:
    typedef enum { BusClock = 0,
        BusClock_16th,
        BusClock_256th,
        ScanLine } tResolution;
    typedef enum { HBlank,
        VBlank } tGateSource;
    typedef enum { CountWhileLow,
        RestartOnRising,
        RestartOnFalling,
        RestartOnBoth } tGateMode;
    typedef enum { Timer0,
        Timer1,
        Timer2,
        Timer3 } tTimer;

    static const unsigned int TicksPerFrame[4];

    CEETimer(tTimer whichTimer);

    // accessors

    inline unsigned int GetTicks(void) const { return *pCount; }
    inline tResolution GetResolution(void) const { return Resolution; }
    inline unsigned int GetTicksPerFrame(void) const { return TicksPerFrame[Resolution]; }

    inline bool HasReachedCompValue() const
    {
        Timers::tMode mode = *(Timers::tMode*)pMode;
        return (mode.EQUF == 1);
    }

    inline bool HasOverflowed() const
    {
        Timers::tMode mode = *(Timers::tMode*)pMode;
        return (mode.OVFF == 1);
    }

    // mutators

    inline void SetGateEnabled(bool enabled) { tMode.GATE = enabled; }
    inline void SetGateSource(tGateSource source) { tMode.GATS = source; }
    inline void SetGateMode(tGateMode mode) { tMode.GATM = mode; }
    inline void SetZeroReturn(bool zr) { tMode.ZRET = zr; }
    inline void SetCompareIntEnabled(bool enabled) { tMode.CMPE = enabled; }
    inline void SetOverflowIntEnabled(bool enabled) { tMode.OVFE = enabled; }

    inline void ClearCompareFlag()
    {
        tMode.EQUF = 1;
        *pMode     = *(unsigned int*)&tMode;
        tMode.EQUF = 0;
    }

    inline void ClearOverflow()
    {
        tMode.OVFF = 1;
        *pMode     = *(unsigned int*)&tMode;
        tMode.OVFF = 0;
    }

    inline void SetCompareValue(uint16_t val)
    {
        tComp  = val;
        *pComp = tComp;
    }

    inline void SetResolution(tResolution res)
    {
        Resolution = res;
        tMode.CLKS = (unsigned int)res;
    }

    inline void Start(void)
    {
        tMode.CUE    = 1;
        tCount.COUNT = 0;
        *pCount      = *((unsigned int*)&tCount);
        *pMode       = *((unsigned int*)&tMode);
    }

    inline void SetMode()
    {
        *pMode = *((unsigned int*)&tMode);
    }

    inline unsigned int Stop(void)
    {
        tMode.CUE = 0;
        *pMode    = *((unsigned int*)&tMode);
        return GetTicks();
    }

protected:
    Timers::tMode tMode;
    Timers::tCount tCount;
    uint16_t tComp;
    tResolution Resolution;

    volatile unsigned int* pMode;
    volatile unsigned int* pCount;
    volatile unsigned int* pComp;
    volatile unsigned int* pHold;
};

/********************************************
 * PerfTimer
 */

class PerfTimer {
public:
    inline static void Start(void)
    {
        asm __volatile__("	mtc0	$0, $9	\n"
                         "	sync.p		  ");
    }
    inline static unsigned int GetTicks(void)
    {
        unsigned int ret;

        asm __volatile__("    mfc0	%0, $9	\n"
                         "    sync.p		  "
                         : "=r"(ret));

        return ret;
    }
    inline static unsigned int Stop(void)
    {
        return GetTicks();
    }
};

#endif // ps2s_eetimer_h
