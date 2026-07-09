/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_types_h
#define ps2s_types_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus

// Custom 128-bit types (16-byte aligned) - not available in stdint.h
typedef unsigned int uint128_t __attribute__((mode(TI), aligned(16)));
typedef int int128_t __attribute__((mode(TI), aligned(16)));

#endif // __cplusplus

#endif // ps2s_types_h
