#ifndef __PS2ASSETINDEX_H__
#define __PS2ASSETINDEX_H__

#include <string>

// One-time recursive scan of the asset directories next to the ELF, kept as
// an in-memory set. Image loading probes many candidate paths per asset
// (alpha companions x directories x 4 extensions); each miss used to be a
// full fio open round-trip. With the index a miss is a set lookup.
//
// Fail-open: if the scan finds nothing (unexpected mount layout), every
// query returns true and loading behaves exactly as before.
bool Ps2AssetExists(const std::string& thePath);

// Record a file created at runtime so later reads of it are not blocked by
// the index (the scan only sees files present at startup).
void Ps2AssetNoteCreated(const std::string& thePath);

#endif
