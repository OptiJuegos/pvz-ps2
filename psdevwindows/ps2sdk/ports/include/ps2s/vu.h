/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_vu_h
#define ps2s_vu_h

#include "ps2s/debug.h"
#include "ps2s/types.h"

/********************************************
	 * common
	 */

namespace VUs {
}

/********************************************
	 * VU0 specific
	 */

namespace VU0 {
inline void CopyQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords);
inline void CopyEvenQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords);
inline void CopyQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords);
inline void CopyEvenQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords);
}

/********************************************
	 * VU1 specific
	 */

namespace VU1 {
}

/********************************************
 * VU0 inlines
 */

void VU0::CopyQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemSrc & 0xf) == 0);

    asm volatile("
                         .set noreorder
                             ctc2
                     % 2,
                 $vi03
                     sll $8,
                 % 0, 4 /* num bytes */
                 addu $8,
                 % 1, $8 /* ending address */
                          .align 8 0
                 : lqc2 vf02, 0(% 1) addiu % 1, % 1, 16 bne % 1, $8, 0b vsqi vf02, ($vi03++).set reorder "
                 : "+r"(numQwords), "+r"(mainMemSrc)
                 : "r"(vu0QwordOffset)
                 : "cc", "$8");
}

void VU0::CopyEvenQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemSrc & 0xf) == 0);
    mErrorIf(numQwords & 1, "numQwords must be EVEN!");

    asm volatile("
                         .set noreorder

                             ctc2
                     % 2,
                 $vi04
                     sll $8,
                 % 0, 4 /* num bytes */
                 addu $8,
                 % 1, $8 /* ending address */
                          .balign 8 0
                 : lqc2 vf02, 0(% 1) addiu % 1, % 1, 16 lqc2 vf03, 0(% 1) addiu % 1, % 1, 16 vsqi vf02, ($vi04++)nop bne % 1, $8, 0b vsqi vf03, ($vi04++)

                                                                                                                                                    .set reorder "
                 : "+r"(numQwords), "+r"(mainMemSrc)
                 : "r"(vu0QwordOffset)
                 : "cc", "$8");
}

void VU0::CopyQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemDest & 0xf) == 0);

    asm volatile("
                         .set noreorder

                             ctc2
                     % 2,
                 $vi02
                         addi
                     % 0,
                 % 0, -16 sll $8, % 1, 4 /* num bytes */
                 addu $8,
                 $8, % 0 /* ending address */
                           .balign 8 0
                 : vlqi vf01, ($vi02++)addiu % 0, % 0, 16 bne % 0, $8, 0b sqc2 vf01, 0(% 0)

                                                                                         .set reorder "
                 : "+r"(mainMemDest)
                 : "r"(numQwords), "r"(vu0QwordOffset)
                 : "$8", "cc", "memory");
}

void VU0::CopyEvenQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemDest & 0xf) == 0);
    mErrorIf(numQwords & 1, "numQwords must be EVEN!");

    asm volatile("
                         .set noreorder

                             ctc2
                     % 2,
                 $vi02
                         addi
                     % 0,
                 % 0, -16 sll $8, % 1, 4 /* num bytes */
                 addu $8,
                 $8, % 0 /* ending address */
                           .balign 8 0
                 : addiu % 0, % 0, 16 vlqi vf01, ($vi02++)

                                                     vlqi vf02,
                 ($vi02++)
                     nop

                         sqc2 vf01,
                 0(% 0)
                         nop

                             addu
                     % 0,
                 % 0, 16 nop

                          bne
                     % 0,
                 $8, 0b sqc2 vf02, 0(% 0)

                                       .set reorder "
                 : "+r"(mainMemDest)
                 : "r"(numQwords), "r"(vu0QwordOffset)
                 : "$8", "cc", "memory");
}

#endif // ps2s_vu_h
