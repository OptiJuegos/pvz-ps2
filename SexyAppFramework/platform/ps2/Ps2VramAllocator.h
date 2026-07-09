#pragma once

#ifdef PS2_PLATFORM

#include <stdint.h>

uint32_t Ps2VramAlloc(uint32_t size);
void Ps2VramFree(uint32_t vram, uint32_t size);
void Ps2VramEvictAll();

extern "C" int ps2_dbg_vram_current_kb();
extern "C" int ps2_dbg_vram_free_blocks();
extern "C" int ps2_dbg_vram_recycled_kb();

#endif
