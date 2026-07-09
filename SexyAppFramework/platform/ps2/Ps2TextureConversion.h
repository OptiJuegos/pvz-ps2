#pragma once

#ifdef PS2_PLATFORM

#include <stdint.h>

// Source bytes are BGRA: Sexy stores 0xAARRGGBB uint32 values on little-endian
// EE, and glTexImage2D receives them as GL_BGRA / GL_UNSIGNED_INT_8_8_8_8_REV.
static inline uint32_t Ps2RgbaToPsmct32(const uint8_t* src)
{
	return ((uint32_t)(src[3] >> 1) << 24) |
		   ((uint32_t)src[0]        << 16) |
		   ((uint32_t)src[1]        <<  8) |
			(uint32_t)src[2];
}

static inline uint16_t Ps2RgbaToPsmct16(const uint8_t* src)
{
	uint16_t r = (uint16_t)(src[2] >> 3);
	uint16_t g = (uint16_t)(src[1] >> 3);
	uint16_t b = (uint16_t)(src[0] >> 3);
	uint16_t a = (uint16_t)(src[3] >= 128 ? 1 : 0);
	return (uint16_t)((a << 15) | (b << 10) | (g << 5) | r);
}

int Ps2ClutCsm1Pos(int index);
int Ps2PalettizeT8(const uint8_t* src, unsigned int npx, uint8_t* idx, unsigned int* paletteCT32);
int Ps2Ct16Dist(uint16_t a, unsigned int b32);

#endif
