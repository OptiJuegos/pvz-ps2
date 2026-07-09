#ifndef __PS2TRACE_H__
#define __PS2TRACE_H__

// -----------------------------------------------------------------------------
// PS2 startup tracer (boot/load border colours).
//
// DISABLED by default. To turn it on, set the define below to 1 (or pass
// -DPS2_TRACE_ENABLE=1) and rebuild:
//
//     #define PS2_TRACE_ENABLE 1
//
// When enabled, Ps2Trace() writes the GS PCRTC background-colour register:
// no fio, no SIF RPC, and it keeps working even when the IOP is wedged, so the
// colour of the TV border/overscan tells you exactly how far startup got on a
// freeze or black screen on real hardware (emulators never reproduce those).
//
// One table, no collisions (see g_ps2TraceInfo in Ps2Trace.c). The rule is
// BRIGHTNESS = PHASE, so you can always tell boot from loading at a glance:
//   * boot + app-init + first-frame points are DIM (colour components <= 96),
//   * loading-thread points are BRIGHT, full-saturation primaries/secondaries.
//
// When disabled (the default), every entry point below compiles to nothing.
// -----------------------------------------------------------------------------

#ifndef PS2_TRACE_ENABLE
#define PS2_TRACE_ENABLE 0 // real-hardware load freeze hunt resolved; border trace back off
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    // ---- Boot: main_pvz_ps2.cpp (dim, warm) ----
    PS2_TRACE_BOOT_MAIN = 0,      // main() entered
    PS2_TRACE_BOOT_SIF,          // SIF RPC up
    PS2_TRACE_BOOT_IOP,          // IOP modules + save prefix done
    PS2_TRACE_BOOT_BLANKTGA,     // blank.tga ensured (host fs writable)
    PS2_TRACE_BOOT_LAWNAPP,      // LawnApp constructed

    // ---- App init: LawnApp::Init (dim, cool) ----
    PS2_TRACE_INIT_SEXY,         // SexyApp::Init done (pak mounted, savedata set)
    PS2_TRACE_INIT_LOG,          // debug log (re)opened at final location
    PS2_TRACE_INIT_RESXML,       // resources.xml parsed
    PS2_TRACE_INIT_GROUP,        // "Init" resource group loaded
    PS2_TRACE_INIT_DONE,         // LawnApp::Init() returned

    // ---- First frame / main loop: SexyAppBase (dim, purple/teal) ----
    PS2_TRACE_FRAME_DRAW,        // first WidgetManager::DrawScreen
    PS2_TRACE_FRAME_DRAWDONE,    // first DrawScreen returned
    PS2_TRACE_FRAME_REDRAW,      // first Redraw/GS flush about to run
    PS2_TRACE_FRAME_GS,          // first frame reached the GS
    PS2_TRACE_MAINLOOP,          // Start() entered the main loop
    PS2_TRACE_FIRST_UPDATE,      // first UpdateApp about to run
    PS2_TRACE_FIRST_UPDATE_DONE, // first update returned, waiting for draw

    // ---- Loading thread: LawnApp::LoadingThreadProc (BRIGHT) ----
    PS2_TRACE_LOAD_LOADERBAR,    // TodLoadResources("LoaderBar")
    PS2_TRACE_LOAD_STRINGS,      // TodStringListLoad(LawnStrings.txt)
    PS2_TRACE_LOAD_COUNTTASKS,   // task counting (GetNumResources/preload/music)
    PS2_TRACE_LOAD_IMAGES,       // LoadGroup("LoadingImages")
    PS2_TRACE_LOAD_FONTS,        // LoadGroup("LoadingFonts")
    PS2_TRACE_LOAD_MUSICINIT,    // mMusic->MusicInit() (mixer starts streaming)
    PS2_TRACE_LOAD_SYSTEMS,      // PoolEffect/ZenGarden init
    PS2_TRACE_LOAD_REANIMCACHE,  // ReanimatorCacheInitialize + Foley (big allocs)
    PS2_TRACE_LOAD_TRAILS,       // TrailLoadDefinitions
    PS2_TRACE_LOAD_PARTICLES,    // TodParticleLoadDefinitions
    PS2_TRACE_LOAD_PURGE,        // PurgeLazyImageBits before preload
    PS2_TRACE_LOAD_PRELOAD,      // PreloadForUser
    PS2_TRACE_LOAD_SOUNDS,       // LoadGroup("LoadingSounds")
    PS2_TRACE_LOAD_DONE,         // loading thread finished

    PS2_TRACE_COUNT
} Ps2TracePoint;

#if PS2_TRACE_ENABLE

// Set the border colour for thePoint and log its name + heap usage.
void Ps2Trace(Ps2TracePoint thePoint);

// Per-item heartbeat: the border colour encodes live heap usage as a spectrum
// ramp. A slow-but-alive loop makes the border drift; a genuine hang inside
// one item leaves it static. No fio/SIF.
void Ps2TraceBeat(void);

// Main-loop liveness tick (call once per frame): every ~half second it toggles
// the blue channel of the last trace colour. On a freeze this splits the
// diagnosis in two at a glance: border still blinking = only the loading
// thread is stuck; border fully static = the main thread is wedged too.
void Ps2TraceMainBeat(void);

// Low-level primitive: write GS BGCOLOUR directly.
void Ps2BootStage(unsigned char r, unsigned char g, unsigned char b);

#else /* !PS2_TRACE_ENABLE */

static __inline__ void Ps2Trace(Ps2TracePoint thePoint) { (void)thePoint; }
static __inline__ void Ps2TraceBeat(void) {}
static __inline__ void Ps2TraceMainBeat(void) {}
static __inline__ void Ps2BootStage(unsigned char r, unsigned char g, unsigned char b)
{
    (void)r; (void)g; (void)b;
}

#endif /* PS2_TRACE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // __PS2TRACE_H__
