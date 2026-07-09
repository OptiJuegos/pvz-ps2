/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_gs_h
#define ps2s_gs_h

/********************************************
 * includes
 */

#include "ps2s/debug.h"
#include "ps2s/gs_reg_types.h"
#include "ps2s/types.h"

class CSCDmaPacket;

namespace GS {

/********************************************
    * constants
    */

typedef enum { kContext1,
    kContext2 } tContext;

typedef enum { kPsm32 = 0,
    kPsm24            = 1,
    kPsm16            = 2,
    kPsm16s           = 10,
    kPsm8             = 19,
    kPsm8h            = 27,
    kPsm4             = 20,
    kPsm4hh           = 44,
    kPsm4hl           = 36,

    kPsmz32  = 48,
    kPsmz24  = 49,
    kPsmz16  = 50,
    kPsmz16s = 58,

    kInvalidPsm = -1
} tPSM;

/********************************************
    * register addresses
    */

// "normal" registers

namespace RegAddrs {
    static const int prim       = 0x00;
    static const int rgbaq      = 0x01;
    static const int st         = 0x02;
    static const int uv         = 0x03;
    static const int xyzf2      = 0x04;
    static const int xyz2       = 0x05;
    static const int tex0_1     = 0x06;
    static const int tex0_2     = 0x07;
    static const int clamp_1    = 0x08;
    static const int clamp_2    = 0x09;
    static const int fog        = 0x0a;
    static const int xyzf3      = 0x0c;
    static const int xyz3       = 0x0d;
    static const int tex1_1     = 0x14;
    static const int tex1_2     = 0x15;
    static const int tex2_1     = 0x16;
    static const int tex2_2     = 0x17;
    static const int xyoffset_1 = 0x18;
    static const int xyoffset_2 = 0x19;
    static const int prmodecont = 0x1a;
    static const int prmode     = 0x1b;
    static const int texclut    = 0x1c;
    static const int scanmsk    = 0x22;
    static const int miptbp1_1  = 0x34;
    static const int miptbp1_2  = 0x35;
    static const int miptbp2_1  = 0x36;
    static const int miptbp2_2  = 0x37;
    static const int texa       = 0x3b;
    static const int fogcol     = 0x3d;
    static const int texflush   = 0x3f;
    static const int scissor_1  = 0x40;
    static const int scissor_2  = 0x41;
    static const int alpha_1    = 0x42;
    static const int alpha_2    = 0x43;
    static const int dimx       = 0x44;
    static const int dthe       = 0x45;
    static const int colclamp   = 0x46;
    static const int test_1     = 0x47;
    static const int test_2     = 0x48;
    static const int pabe       = 0x49;
    static const int fba_1      = 0x4a;
    static const int fba_2      = 0x4b;
    static const int frame_1    = 0x4c;
    static const int frame_2    = 0x4d;
    static const int zbuf_1     = 0x4e;
    static const int zbuf_2     = 0x4f;
    static const int bitbltbuf  = 0x50;
    static const int trxpos     = 0x51;
    static const int trxreg     = 0x52;
    static const int trxdir     = 0x53;
    static const int hwreg      = 0x54;
    static const int signal     = 0x60;
    static const int finish     = 0x61;
    static const int label      = 0x62;
    static const int nop        = 0x7f;
}

// the "special" registers

namespace ControlRegAddrs {

    static const int pmode    = 0x00;
    static const int smode2   = 0x02;
    static const int dispfb1  = 0x07;
    static const int display1 = 0x08;
    static const int dispfb2  = 0x09;
    static const int display2 = 0x0a;
    static const int extbuf   = 0x0b;
    static const int extdata  = 0x0c;
    static const int extwrite = 0x0d;
    static const int bgcolor  = 0x0e;
    static const int csr      = 0x40;
    static const int imr      = 0x41;
    static const int busdir   = 0x44;
    static const int siglblid = 0x48;
}

namespace ControlRegs {

    static volatile void* const pmode    = (volatile void*)0x12000000;
    static volatile void* const smode2   = (volatile void*)0x12000020;
    static volatile void* const dispfb1  = (volatile void*)0x12000070;
    static volatile void* const display1 = (volatile void*)0x12000080;
    static volatile void* const dispfb2  = (volatile void*)0x12000090;
    static volatile void* const display2 = (volatile void*)0x120000a0;
    static volatile void* const extbuf   = (volatile void*)0x120000b0;
    static volatile void* const extdata  = (volatile void*)0x120000c0;
    static volatile void* const extwrite = (volatile void*)0x120000d0;
    static volatile void* const bgcolor  = (volatile void*)0x120000e0;
    static volatile void* const csr      = (volatile void*)0x12001000;
    static volatile void* const imr      = (volatile void*)0x12001010;
    static volatile void* const busdir   = (volatile void*)0x12001040;
    static volatile void* const siglblid = (volatile void*)0x12001080;
}

/********************************************
    * methods
    */

inline uint32_t PackRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}
inline uint32_t PackRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

void Init(void);

void Flush(void);
void Flush(CSCDmaPacket& packet);

inline unsigned int GetBitsPerPixel(tPSM psm);
void ReorderClut(uint32_t* oldClut, uint32_t* newClut);

} // namespace GS

namespace GIF {
namespace Registers {
    static volatile uint32_t* const ctrl = (volatile uint32_t*)0x10003000;
    static volatile uint128_t* const fifo = (volatile uint128_t*)0x10006000;
}
}


typedef struct tGifTag_t {
    unsigned long long NLOOP : 15;
    unsigned long long EOP : 1;
    unsigned long long pad0 : 16;
    unsigned long long id : 14;
    unsigned long long PRE : 1;
    unsigned long long PRIM : 11;
    unsigned long long FLG : 2;
    unsigned long long NREG : 4;
    unsigned long long REGS0 : 4;
    unsigned long long REGS1 : 4;
    unsigned long long REGS2 : 4;
    unsigned long long REGS3 : 4;
    unsigned long long REGS4 : 4;
    unsigned long long REGS5 : 4;
    unsigned long long REGS6 : 4;
    unsigned long long REGS7 : 4;
    unsigned long long REGS8 : 4;
    unsigned long long REGS9 : 4;
    unsigned long long REGS10 : 4;
    unsigned long long REGS11 : 4;
    unsigned long long REGS12 : 4;
    unsigned long long REGS13 : 4;
    unsigned long long REGS14 : 4;
    unsigned long long REGS15 : 4;
} __attribute__((packed,aligned(16))) tGifTag;

inline unsigned int
GS::GetBitsPerPixel(tPSM psm)
{
    uint32_t bpp = 0;

    switch (psm) {
    case kPsm32:
    case kPsmz32:
        bpp = 32;
        break;
    case kPsm24:
    case kPsmz24:
        bpp = 24;
        break;
    case kPsm16:
    case kPsm16s:
    case kPsmz16:
    case kPsmz16s:
        bpp = 16;
        break;
    case kPsm8:
    case kPsm8h:
        bpp = 8;
        break;
    case kPsm4:
    case kPsm4hl:
    case kPsm4hh:
        bpp = 4;
        break;
    default:
        mAssert(false);
        break;
    }

    return bpp;
}

#endif // ps2s_gs_h
