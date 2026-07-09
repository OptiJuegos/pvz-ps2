/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_math_h
#define ps2s_math_h

#include "ps2s/types.h"
#include <math.h>

namespace Math {

static const float kPi = 3.141592654f;

inline float
Abs(float t)
{
    return (t < 0.0f) ? -t : t;
}

inline float
Clamp(float t, float a, float b)
{
    if (a <= t && t <= b)
        return t;
    else if (t < a)
        return a;
    else
        return b;
}

inline int
Clamp(int t, int a, int b)
{
    if (a <= t && t <= b)
        return t;
    else if (t < a)
        return a;
    else
        return b;
}

inline float
Bias(float b, float x)
{
    return (float)powf(x, logf(b) / logf(0.5));
}

inline float
Ceil(float a)
{
    return (float)((int)a + (a > 0.0f && a != (int)a));
}

inline float
DegToRad(float deg)
{
    return deg * (kPi / 180.0f);
}

inline uint32_t
DivUp(uint32_t num, uint32_t divisor)
{
    return (num % divisor) ? num / divisor + 1 : num / divisor;
}

inline bool
FuzzyEquals(float a, float b, float epsilon)
{
    return (b - epsilon <= a && a <= b + epsilon);
}

inline bool
FuzzyEqualsi(int a, int b, int epsilon)
{
    return (b - epsilon <= a && a <= b + epsilon);
}

inline float
Gain(float g, float x)
{
    if (x < 0.5f)
        return (float)(Bias((float)(1.0f - g), (float)(2.0f * x)) / 2.0f);
    else
        return (float)(1.0f - Bias((float)(1.0f - g), (float)(2.0f - 2.0f * x)) / 2.0f);
}

inline float
Floor(float a)
{
    float ret;

    asm __volatile__(
        "	.set	noreorder						\n"

        "	cvt.w.s	%0, %1	/* ret = (int)a */				\n"
        "	nop								\n"

        "	mfc1	%2, %1							\n"
        "	nop								\n"

        "	cvt.s.w	%0, %0	/* ret = (float)ret */				\n"
        "	nop								\n"

        "	bgtz	%2, 0f	/* quit if positive */				\n"
        "	nop								\n"

        "	/* negative */							\n"
        "	c.eq.s	%0, %1							\n"
        "	nop								\n"

        "	bc1t	0f	/* quit if a == (float)(int)a */		\n"
        "	nop								\n"

        "	li.s	%1, 1.0							\n"

        "	sub.s	%0, %0, %1						\n"
        "	nop								\n"

        "	mtc1	%2, %1	/* reset %1 since we clobbered it with 1.0 */	\n"
        "	nop								\n"

        " 0:	/* end */							\n"

        "	.set	reorder							\n"
        : "=&f"(ret)
        : "f"(a), "r"(a)
        : "$1", "cc");

    return ret;

    // return (float)( (int)a - (a < 0.0f && a != (int)a) );
}

inline int
Floori(float a)
{
    return ((int)a - (a < 0.0f && a != (int)a));
}

inline bool
IsOdd(int a)
{
    return (a & 1);
}

inline bool
IsEven(int a)
{
    return !IsOdd(a);
}

inline bool
IsPow2(uint32_t num)
{
    return ((num & (num - 1)) == 0);
}

inline float
Lerp(float t, float f1, float f2)
{
    return (f1 + t * (f2 - f1));
}

inline uint32_t
Log2(uint32_t num)
{
    uint32_t logNum = 0;
    while (num > 1) {
        num = num >> 1;
        logNum++;
    }
    return logNum;
}

inline int
Max(int a, int b)
{
    return (a > b) ? a : b;
}

inline int
Max(unsigned int a, unsigned int b)
{
    return (a > b) ? a : b;
}

inline int
Min(int a, int b)
{
    return (a < b) ? a : b;
}

inline int
Min(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

inline uint32_t
Min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

inline float
Max(float a, float b)
{
    return (a > b) ? a : b;
}

inline float
Min(float f1, float f2)
{
    return ((f1 < f2) ? f1 : f2);
}

// This is the rand from K&R.  rand() is supposed to be portable, but..
extern int Rand(void);
extern void SRand(unsigned int seed);

inline float
RadToDeg(float rad)
{
    return rad * 180.0f / kPi;
}

inline int
Round(float f)
{
    if (fabsf((float)(f - (int)f)) >= 0.5f)
        return (int)f + ((f < 0.0f) ? -1 : 1);
    else
        return (int)f;
}

inline float
SmoothStep(float t)
{
    return (t * t * (3.0f - 2.0f * t));
}

} // namespace Math

#endif // ps2s_math_h
