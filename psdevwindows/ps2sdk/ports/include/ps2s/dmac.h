/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_dmac_h
#define ps2s_dmac_h

// PLIN
#include "ps2s/types.h"

/********************************************
 * typedefs
 */

// PLIN
typedef struct tD_CHCR_t {
    unsigned DIR : 1; // Direction
    unsigned p0 : 1;
    unsigned MOD : 2; // Mode
    unsigned ASP : 2; // Address stack pointer
    unsigned TTE : 1; // Tag trasfer enable
    unsigned TIE : 1; // Tag interrupt enable
    unsigned STR : 1; // start
    unsigned p1 : 7;
    unsigned TAG : 16; // DMAtag
} tD_CHCR;

typedef struct tSourceChainTag_t {
    uint64_t QWC : 16;
    uint64_t pad : 10;
    uint64_t PCE : 2;
    uint64_t ID : 3;
    uint64_t IRQ : 1;
    uint64_t ADDR : 31;
    uint64_t SPR : 1;
    uint32_t opt1;
    uint32_t opt2;
} __attribute__((packed,aligned(16))) tSourceChainTag;

typedef tSourceChainTag tDmaTag;

typedef struct {
    tD_CHCR chcr;
    unsigned int p0[3]; // channel control
    void* madr;
    unsigned int p1[3]; // memory address
    unsigned int qwc;
    unsigned int p2[3]; // transfer count
    tDmaTag* tadr;
    unsigned int p3[3]; // tag address
    void* as0;
    unsigned int p4[3]; // address stack
    void* as1;
    unsigned int p5[3]; // address stack
    unsigned int p6[4]; // pad
    unsigned int p7[4]; // pad
    void* sadr;
    unsigned int p8[3]; // spr address
} tDmaChannel;

/********************************************
 * DMAC
 */

namespace DMAC {
namespace Channels {
    enum eChannels { vif0,
        vif1,
        gif,
        fromIpu,
        toIpu,
        sif0,
        sif1,
        sif2,
        fromSpr,
        toSpr };
}

enum { kRefe,
    kCnt,
    kNext,
    kRef,
    kRefs,
    kCall,
    kRet,
    kEnd };

/*
	namespace ChannelPtrs {
		const volatile tDmaChannel *vif0 = (volatile tDmaChannel*)D0_CHCR,
			*vif1 = (volatile tDmaChannel*)D1_CHCR,
			*gif = (volatile tDmaChannel*)D2_CHCR,
			*fromIpu = (volatile tDmaChannel*)D3_CHCR,
			*toIpu = (volatile tDmaChannel*)D4_CHCR,
			*sif0 = (volatile tDmaChannel*)D5_CHCR,
			*sif1 = (volatile tDmaChannel*)D6_CHCR,
			*sif2 = (volatile tDmaChannel*)D7_CHCR,
			*fromSpr = (volatile tDmaChannel*)D8_CHCR,
			*toSpr = (volatile tDmaChannel*)D9_CHCR;
	}
	*/
}

typedef DMAC::Channels::eChannels tDmaChannelId;

#endif // ps2s_dmac_h
