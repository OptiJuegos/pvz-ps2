/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_utils_h
#define ps2s_utils_h

#include "ps2s/debug.h"
#include "ps2s/types.h"
#include <stdio.h>

/********************************************
	 * Utils
	 */

namespace Utils {
inline void MemCpy128(uint128_t* dest, const uint128_t* src, uint32_t numQwords);

inline void QwordHexDump(uint32_t* mem, uint32_t numQwords);
inline void QwordDecDump(uint32_t* mem, uint32_t numQwords);
inline void QwordFloatDump(float* mem, uint32_t numQwords);
}

/********************************************
 * Utils inlines
 */

inline void
Utils::MemCpy128(uint128_t* dest, const uint128_t* src, uint32_t numQwords)
{
    mAssert(((uint32_t)dest & 0xf) == 0 && ((uint32_t)src & 0xf) == 0);
    while (numQwords-- > 0)
        *(dest++) = *(src++);
}

inline void
Utils::QwordHexDump(uint32_t* mem, uint32_t numQwords)
{
    uint32_t i;
    for (i = 0; i < numQwords; i++, mem += 4)
        printf("%08lx: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", (uint32_t)mem, mem[0], mem[1], mem[2], mem[3]);
}

inline void
Utils::QwordDecDump(uint32_t* mem, uint32_t numQwords)
{
    uint32_t i;
    for (i = 0; i < numQwords; i++, mem += 4)
        printf("%08lx: %ld %ld %ld %ld\n", (uint32_t)mem, mem[0], mem[1], mem[2], mem[3]);
}

inline void
Utils::QwordFloatDump(float* mem, uint32_t numQwords)
{
    uint32_t i;
    for (i = 0; i < numQwords; i++, mem += 4)
        printf("%08lx: %f %f %f %f\n", (uint32_t)mem, mem[0], mem[1], mem[2], mem[3]);
}

#endif // ps2s_utils_h
