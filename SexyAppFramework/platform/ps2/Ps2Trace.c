#ifdef PS2_PLATFORM

#include "Ps2Trace.h"

// this is for debugging
#if PS2_TRACE_ENABLE

#include <malloc.h>   // mallinfo() — heap figure for the trace and heartbeat
#include <stdio.h>

// Map a spectrum position (0..1535: red->yellow->green->cyan->blue->magenta) to
// RGB and paint the border. Shared by the progress/heap ramps.
static void ps2_ramp_to_border(unsigned pos)
{
    unsigned seg, t;
    unsigned char r, g, b;
    if (pos > 1535u) pos = 1535u;
    seg = pos >> 8;
    t   = pos & 0xFFu;
    switch (seg)
    {
        default:
        case 0: r = 255;                    g = (unsigned char)t;         b = 0;                      break;
        case 1: r = (unsigned char)(255-t); g = 255;                      b = 0;                      break;
        case 2: r = 0;                      g = 255;                      b = (unsigned char)t;       break;
        case 3: r = 0;                      g = (unsigned char)(255-t);   b = 255;                    break;
        case 4: r = (unsigned char)t;       g = 0;                        b = 255;                    break;
        case 5: r = 255;                    g = 0;                        b = (unsigned char)(255-t); break;
    }
    Ps2BootStage(r, g, b);
}

// Low-level primitive: paint the GS PCRTC background-colour register directly.
// No IO, no SIF RPC, works before (or without) any video init, so a black-screen
// or wedged-IOP hang still shows the colour of the last point reached.
void Ps2BootStage(unsigned char r, unsigned char g, unsigned char b)
{
    *((volatile unsigned int*)0x120000E0) =
        ((unsigned int)b << 16) | ((unsigned int)g << 8) | (unsigned int)r;
}

typedef struct
{
    unsigned char r, g, b;
    const char*   name;
} Ps2TraceInfo;

// Single colour table. Boot/init/first-frame stay DIM (<=96 per channel);
// loading-thread points are BRIGHT so the two phases never look alike. The
// loading wheel is spread for maximum on-a-TV distinctness because that is the
// phase we actively debug.
static const Ps2TraceInfo g_ps2TraceInfo[PS2_TRACE_COUNT] =
{
    /* -- boot: dim warm -- */
    {  48,   0,   0, "boot.main"        },
    {  96,   0,   0, "boot.sif"         },
    {  96,  48,   0, "boot.iop"         },
    {  96,  96,   0, "boot.blanktga"    },
    {   0,  96,   0, "boot.lawnapp"     },
    /* -- app init: dim cool -- */
    {   0,  96,  96, "init.sexy"        },
    {   0,  48,  96, "init.log"         },
    {   0,   0,  96, "init.resxml"      },
    {  64,   0,  96, "init.group"       },
    {   0,  80,  80, "init.done"        },
    /* -- first frame / main loop: dim purple/teal -- */
    {  96,   0,  96, "frame.draw"       },
    {  96,   0,  48, "frame.drawdone"   },
    {   0,  64,  64, "frame.redraw"     },
    {  40,  40,  40, "frame.gs"         },
    {   0,  32,  80, "mainloop"         },
    {   0,   0,  64, "frame.update"     },
    {   0,  64,  32, "frame.updatedone" },
    /* -- loading thread: BRIGHT, maximally distinct -- */
    { 128,   0,   0, "load.loaderbar"   },  // dark red
    { 255,   0,   0, "load.strings"     },  // red
    { 255,   0, 128, "load.counttasks"  },  // rose
    { 255, 128,   0, "load.images"      },  // orange
    { 255, 255,   0, "load.fonts"       },  // yellow
    {   0, 255,   0, "load.musicinit"   },  // green
    {   0, 255, 128, "load.systems"     },  // spring green
    {   0,   0, 255, "load.reanimcache" },  // pure blue
    {   0, 255, 255, "load.trails"      },  // cyan
    { 255,   0, 255, "load.particles"   },  // magenta
    { 160,   0, 255, "load.purge"       },  // violet
    { 255, 255, 255, "load.preload"     },  // white
    { 128, 128, 128, "load.sounds"      },  // grey
    {   0, 200, 160, "load.done"        },  // bright teal
};

static unsigned char s_lastR = 0, s_lastG = 0, s_lastB = 0;

void Ps2Trace(Ps2TracePoint thePoint)
{
    const Ps2TraceInfo* info;
    struct mallinfo mi;

    if ((unsigned)thePoint >= (unsigned)PS2_TRACE_COUNT)
        return;

    info = &g_ps2TraceInfo[thePoint];
    s_lastR = info->r;
    s_lastG = info->g;
    s_lastB = info->b;
    Ps2BootStage(info->r, info->g, info->b);

    /* printf lands in userdata/log.txt (see TodDebug.cpp), so each stage line
       is on disk before the next stage runs. */
    mi = mallinfo();
    printf("[TRACE] %-16s heapUsed=%.2fMB free=%.2fMB\n", info->name,
           (double)mi.uordblks / (1024.0 * 1024.0),
           (double)mi.fordblks / (1024.0 * 1024.0));
}

void Ps2TraceMainBeat(void)
{
    /* ~1Hz square wave on the blue channel of the last stage colour: proves
       the MAIN thread is alive without hiding the stage/heap hue underneath.
       s_lastB itself is not modified, so stage colours stay authoritative. */
    static unsigned sFrame = 0;
    static int sPhase = 0;
    if (((++sFrame) & 31) != 0)
        return;
    sPhase ^= 1;
    Ps2BootStage(s_lastR, s_lastG,
                 sPhase ? (unsigned char)(s_lastB ^ 0x80) : s_lastB);
}

void Ps2TraceBeat(void)
{
    // HEAP ramp: the border colour encodes live heap usage (uordblks), mapped
    // 0..32MB -> the spectrum. The border DRIFTS as the heap grows and FREEZES on
    // the colour of the heap level at the hang, so the stuck hue tells us how full
    // RAM was — the OOM test. Reference points (heapUsed):
    //   red ~0MB, yellow ~5MB, green ~10.7MB, cyan ~16MB, blue ~21.3MB
    //   (pressure valve), magenta ~26.7MB (~ceiling => OOM), red-again ~32MB.
    struct mallinfo mi = mallinfo();
    unsigned long used = (unsigned long)mi.uordblks;
    /* pos = used / 32MB * 1536 ; do it as used * 1536 >> 25 (32MB == 1<<25). */
    unsigned pos = (unsigned)((used >> 14) * 1536u >> 11); /* == used*1536/2^25 */
    ps2_ramp_to_border(pos);
}

#endif /* PS2_TRACE_ENABLE */

#endif // PS2_PLATFORM
