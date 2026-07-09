#pragma once
#ifdef PS2_PLATFORM

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Boot-time IOP services for the PvZ PS2 build: loads the memory-card and
// USB-mass IOP modules (embedded in the ELF) plus audsrv for sound, then
// decides where persistent data lives and where assets are read from.
//
// theArgv0 is the ELF boot path (argv[0]). Its device prefix tells us how the
// game was launched: "cdrom0:..." means we booted from a CD/DVD (or a mounted
// ISO), so assets must be read from the disc through the cdfs driver while
// saves go to a writable device. Pass "" if argv is unavailable.
//
// Save location priority:  "mass:/PVZ/" (USB) -> "mc0:/PVZ/" (memory card)
//                          -> "" (current dir; PCSX2 host filesystem).
void Ps2PvzInitServices(const char* theArgv0);

// Prefix ending in '/' (or empty for cwd) where all persistent WRITES go
// (log, userdata saves, registry).
const char* Ps2GetSavePrefix(void);

// Prefix prepended to relative asset READ paths. Empty on host:/USB (assets
// live in the current directory). On a CD/DVD boot it is "cdfs:/", so the
// case-insensitive cdfs driver resolves "images/blank.tga" -> the disc's
// IMAGES/BLANK.TGA;1 without the game changing any of its path strings.
const char* Ps2GetResourcePrefix(void);

// True if the embedded audsrv module loaded (sound is available).
bool Ps2AudioAvailable(void);

#ifdef __cplusplus
}
#endif

#endif // PS2_PLATFORM
