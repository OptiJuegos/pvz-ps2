/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_vif_h
#define ps2s_vif_h

#include <ps2s/types.h>

namespace Vifs {

/********************************************
    * constants
    */

namespace Opcodes {
    static const uint32_t nop      = 0,
                      stcycl   = 1,
                      offset   = 2,
                      base     = 3,
                      itop     = 4,
                      stmod    = 5,
                      mskpath3 = 6,
                      mark     = 7,
                      flushe   = 16,
                      flush    = 17,
                      flusha   = 18,
                      mscal    = 20,
                      mscnt    = 23,
                      mscalf   = 21,
                      stmask   = 32,
                      strow    = 48,
                      stcol    = 49,
                      mpg      = 74,
                      direct   = 80,
                      directhl = 81;
}

namespace UnpackModes {
    static const uint32_t s_32  = 0,
                      s_16  = 1,
                      s_8   = 2,
                      v2_32 = 4,
                      v2_16 = 5,
                      v2_8  = 6,
                      v3_32 = 8,
                      v3_16 = 9,
                      v3_8  = 10,
                      v4_32 = 12,
                      v4_16 = 13,
                      v4_8  = 14,
                      v4_5  = 15;
}

namespace AddModes {
    static const uint32_t kNone       = 0,
                      kOffset     = 1,
                      kAccumulate = 2;
}

/********************************************
    * types
    */

typedef struct {
    uint32_t immediate : 16;
    uint32_t num : 8;
    uint32_t cmd : 8;
} tVifCode;

typedef struct {
    unsigned int m0 : 2;
    unsigned int m1 : 2;
    unsigned int m2 : 2;
    unsigned int m3 : 2;
    unsigned int m4 : 2;
    unsigned int m5 : 2;
    unsigned int m6 : 2;
    unsigned int m7 : 2;
    unsigned int m8 : 2;
    unsigned int m9 : 2;
    unsigned int m10 : 2;
    unsigned int m11 : 2;
    unsigned int m12 : 2;
    unsigned int m13 : 2;
    unsigned int m14 : 2;
    unsigned int m15 : 2;
} tMask;

} // namespace Vifs

#endif // ps2s_vif_h
