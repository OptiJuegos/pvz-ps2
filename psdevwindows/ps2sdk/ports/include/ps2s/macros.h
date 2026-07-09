/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_macros_h
#define ps2s_macros_h

#define mCat(_a, _b) _a##_b
#define mExpandCat(_a, _b) mCat(_a, _b)

#define mQuote(_a) #_a
#define mExpandQuote(_a) mQuote(_a)

#endif // ps2s_macros_h
