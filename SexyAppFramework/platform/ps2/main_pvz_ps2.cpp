#ifdef PS2_PLATFORM

#include <kernel.h>
#include <sifrpc.h>
#include <gsKit.h>
#include <stdio.h>
#include <sys/stat.h>

#include "LawnApp.h"
#include "Resources.h"
#include "Sexy.TodLib/TodStringFile.h"
#include "Ps2PvzServices.h"
#include "Ps2Trace.h"

using namespace Sexy;

// Definition of the gsGlobal pointer shared with gles_ps2.cpp, Window.cpp and Input.cpp.
GSGLOBAL* gsGlobal = nullptr;

// Function-pointer globals declared extern in LawnApp.h; defined here for PS2
// (on PC they live in main.cpp which is excluded from this build).
bool (*gAppCloseRequest)()        = nullptr;
bool (*gAppHasUsedCheatKeys)()    = nullptr;
SexyString (*gGetCurrentLevelName)() = nullptr;

// blank.tga is not included in some pak builds; write a minimal one to the host FS so
// the resource manager can find it at "images/blank".
static void EnsureBlankTga()
{
    // On a CD/DVD boot the current directory is the read-only disc, so this
    // fallback cannot (and need not) write: the shipped main.pak carries
    // images/blank.tga and the pak layer serves it directly. Skip the doomed
    // open/write pair to avoid a spurious warning.
    if (Ps2GetResourcePrefix()[0] != '\0')
        return;

    FILE* f = fopen("images/blank.tga", "rb");
    if (f) { fclose(f); return; }

    mkdir("images", 0755);   // create directory if missing (ignore errors)

    f = fopen("images/blank.tga", "wb");
    if (!f)
    {
        printf("[PvZ PS2] Warning: could not create images/blank.tga\n");
        return;
    }

    // Minimal uncompressed 32-bpp RGBA TGA: 4x4 white pixels.
    // 4x4 avoids potential issues with 1x1 textures on some PS2 texture units.
    static const unsigned char kHeader[18] = {
        0,    // ID length
        0,    // color map type: none
        2,    // image type: uncompressed true-color
        0,0,0,0,0,  // color map spec
        0,0,  // X origin (LE)
        0,0,  // Y origin (LE)
        4,0,  // width = 4 (LE)
        4,0,  // height = 4 (LE)
        32,   // pixel depth: 32 bpp
        0x28  // image descriptor: 8 alpha bits, top-left origin
    };
    fwrite(kHeader, 1, sizeof(kHeader), f);

    // 4*4 = 16 white BGRA pixels
    for (int i = 0; i < 16; ++i)
    {
        static const unsigned char kWhite[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        fwrite(kWhite, 1, 4, f);
    }
    fclose(f);
    printf("[PvZ PS2] Created images/blank.tga\n");
}

int main(int argc, char** argv)
{
    // argv[0] is the ELF boot path; its device prefix (cdrom0:/mass:/host:)
    // tells the services layer how we were launched so it can pick the asset
    // read root and save location. Guard against a launcher passing no args.
    const char* aArgv0 = (argc > 0 && argv != NULL && argv[0] != NULL) ? argv[0] : "";

    // Boot tracer: compiled out unless PS2_TRACE_ENABLE=1 (Ps2Trace.h). When
    // enabled, each point repaints the GS border colour so a black-screen hang
    // shows how far startup got — boot/init are dim, loading is bright.
    Ps2Trace(PS2_TRACE_BOOT_MAIN);

    // The kernel hands the ELF's main thread priority 0 — the highest on the
    // EE, above the audio mixer thread (20, Ps2SoundManager). A CPU-bound
    // gameplay frame at priority 0 never yields, so the mixer crawls to ~1
    // chunk/s, the SPU2 ring underruns, and playback sticks on a frozen buzz
    // (mixer health log: "STALLED" cycling spots 3/4/5 with pause=0). Drop
    // below the mixer so it can preempt us; it only needs ~1ms per 46ms
    // chunk. This call originally lived in startup_ps2.c and was lost when
    // that file was removed in the logging refactor — do not lose it again.
    ChangeThreadPriority(GetThreadId(), 32);

    SifInitRpc(0);
    Ps2Trace(PS2_TRACE_BOOT_SIF);
    printf("[PvZ PS2] main() entered\n");

    // IOP modules (memory card, USB mass storage, cdfs, audsrv) + save location
    // + asset read root.
    Ps2PvzInitServices(aArgv0);
    Sexy::SetAppDataFolder(Ps2GetSavePrefix());
    Ps2Trace(PS2_TRACE_BOOT_IOP);

    EnsureBlankTga();
    Ps2Trace(PS2_TRACE_BOOT_BLANKTGA);

    TodStringListSetColors(gLawnStringFormats, gLawnStringFormatCount);
    gGetCurrentLevelName    = LawnGetCurrentLevelName;
    gAppCloseRequest        = LawnGetCloseRequest;
    gAppHasUsedCheatKeys    = LawnHasUsedCheatKeys;
    gExtractResourcesByName = Sexy::ExtractResourcesByName;

    gLawnApp = new LawnApp();
    Ps2Trace(PS2_TRACE_BOOT_LAWNAPP);
    gLawnApp->Init();
    Ps2Trace(PS2_TRACE_INIT_DONE);
    gLawnApp->Start();
    gLawnApp->Shutdown();
    delete gLawnApp;

    return 0;
}

#endif // PS2_PLATFORM
