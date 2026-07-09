#ifdef PS2_PLATFORM

#include "Ps2VramAllocator.h"

#include <gsKit.h>
#include <gsCore.h>

extern GSGLOBAL* gsGlobal;

// gsKit_vram_alloc() is a one-way bump allocator. This small free-list keeps
// texture churn bounded across PvZ screen/resource reloads; a full arena rewind
// is still available when fragmentation or working-set pressure wins.
#define PS2_MAX_VRAM_FREE 256

struct Ps2VramBlock
{
	uint32_t vram;
	uint32_t size;
};

static Ps2VramBlock s_vramFree[PS2_MAX_VRAM_FREE];
static int s_vramFreeCount = 0;
static uint32_t s_texVramBase = 0;

static void Ps2VramCaptureBase()
{
	if (!s_texVramBase && gsGlobal)
		s_texVramBase = gsGlobal->TexturePointer
			? gsGlobal->TexturePointer : gsGlobal->CurrentPointer;
}

uint32_t Ps2VramAlloc(uint32_t size)
{
	if (!gsGlobal)
		return GSKIT_ALLOC_ERROR;

	Ps2VramCaptureBase();

	int best = -1;
	for (int i = 0; i < s_vramFreeCount; i++)
	{
		if (s_vramFree[i].size >= size &&
			(best < 0 || s_vramFree[i].size < s_vramFree[best].size))
			best = i;
	}

	if (best >= 0)
	{
		// Freed blocks can still be referenced by queued GIF packets. Flush
		// before reusing their VRAM so in-flight draws sample the old contents.
		if (gsGlobal && gsGlobal->Os_Queue && gsGlobal->CurQueue == gsGlobal->Os_Queue)
		{
			gsKit_queue_exec(gsGlobal);
			gsKit_queue_reset(gsGlobal->Os_Queue);
		}

		uint32_t vram = s_vramFree[best].vram;
		s_vramFree[best] = s_vramFree[--s_vramFreeCount];
		return vram;
	}

	return gsKit_vram_alloc(gsGlobal, size, GSKIT_ALLOC_USERBUFFER);
}

void Ps2VramFree(uint32_t vram, uint32_t size)
{
	if (vram == GSKIT_ALLOC_ERROR || size == 0)
		return;

	if (s_vramFreeCount < PS2_MAX_VRAM_FREE)
	{
		s_vramFree[s_vramFreeCount].vram = vram;
		s_vramFree[s_vramFreeCount].size = size;
		s_vramFreeCount++;
	}
}

void Ps2VramEvictAll()
{
	s_vramFreeCount = 0;
	if (gsGlobal && s_texVramBase)
		gsGlobal->CurrentPointer = s_texVramBase;
}

extern "C" int ps2_dbg_vram_current_kb()
{
	return gsGlobal ? (int)(gsGlobal->CurrentPointer / 1024) : 0;
}

extern "C" int ps2_dbg_vram_free_blocks()
{
	return s_vramFreeCount;
}

extern "C" int ps2_dbg_vram_recycled_kb()
{
	uint32_t total = 0;
	for (int i = 0; i < s_vramFreeCount; ++i)
		total += s_vramFree[i].size;
	return (int)(total / 1024);
}

#endif
