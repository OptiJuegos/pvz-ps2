#!/usr/bin/env python3
"""Build a PopCap-format main.pak from loose asset folders.

Written for the PS2 port: PakInterface::AddPakFile (PS2 branch) streams the
pak from disk, so packing the read-only asset tree replaces thousands of
per-file open()/close() round-trips over usbhdfsd (each one an IOP RPC plus a
FAT directory walk) with RAM index lookups plus seeks on one shared handle.

On-disk format (every byte XORed with 0xF7):
    u32  magic 0xBAC04AC0 (little-endian)
    u32  version (0)
    per file:
        u8   flags (0)
        u8   name length
        ...  name bytes (relative path, forward slashes)
        i32  size (little-endian)
        i64  filetime (unused by the PS2 build; written as 0)
    u8   0x80 (FILEFLAGS_END)
    ...  each file's raw bytes concatenated in record order

Usage:
    python tools/build_pak.py <media_root> [-o OUTPUT] [--dirs DIR [DIR ...]]

Example:
    python tools/build_pak.py "PVZ PS2 PORT V1.0 OptiJuegos/Version ELF"

After building, the script re-parses its own output exactly the way
AddPakFile does and byte-compares every record against the source file.
"""

import argparse
import os
import struct
import sys

XOR_TABLE = bytes(b ^ 0xF7 for b in range(256))
MAGIC = 0xBAC04AC0
FILEFLAGS_END = 0x80

# Read-only asset dirs served through p_fopen. sounds/ is included: Music.cpp
# and Ps2SoundManager already read every track/SFX through p_fopen, so packing
# them collapses the "multi-MB read, seconds over USB 1.1" open()+FAT-walk on
# every tune change (and every SFX re-decode after a purge) into a seek+
# chunked-read on the one already-open shared pak handle. savedata/ is written
# at runtime and stays loose.
DEFAULT_DIRS = ["images", "reanim", "particles", "compiled", "data", "properties", "drm", "sounds"]


def collect_files(root, dirs):
    files = []
    for d in dirs:
        base = os.path.join(root, d)
        if not os.path.isdir(base):
            print(f"warning: '{d}' not found under {root}, skipping")
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames.sort()
            for name in sorted(filenames):
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, root).replace("\\", "/")
                files.append((rel, full, os.path.getsize(full)))
    return files


def build_pak(root, out_path, dirs):
    files = collect_files(root, dirs)
    if not files:
        sys.exit("error: no files found; nothing to pack")

    too_long = [rel for rel, _, _ in files if len(rel.encode("latin-1")) > 255]
    if too_long:
        sys.exit(f"error: path over 255 bytes (name length is a u8): {too_long[0]}")

    header = bytearray()
    header += struct.pack("<II", MAGIC, 0)
    for rel, _, size in files:
        name = rel.encode("latin-1")
        header += struct.pack("<BB", 0, len(name))
        header += name
        header += struct.pack("<iq", size, 0)
    header.append(FILEFLAGS_END)

    total_data = sum(size for _, _, size in files)
    with open(out_path, "wb") as out:
        out.write(bytes(header).translate(XOR_TABLE))
        for rel, full, size in files:
            with open(full, "rb") as f:
                data = f.read()
            if len(data) != size:
                sys.exit(f"error: size of '{rel}' changed while packing")
            out.write(data.translate(XOR_TABLE))

    print(f"wrote {out_path}: {len(files)} records, "
          f"{len(header)} header bytes + {total_data} data bytes "
          f"({(len(header) + total_data) / (1024 * 1024):.1f} MB)")
    return files


def verify_pak(root, pak_path, files):
    """Re-parse the pak the way PakInterface::AddPakFile does and byte-compare
    every record against its source file."""
    with open(pak_path, "rb") as f:
        raw = f.read()
    data = raw.translate(XOR_TABLE)

    magic, version = struct.unpack_from("<II", data, 0)
    if magic != MAGIC or version != 0:
        sys.exit("verify FAILED: bad magic/version")

    pos = 8
    records = []
    while True:
        flags = data[pos]; pos += 1
        if flags & FILEFLAGS_END:
            break
        name_len = data[pos]; pos += 1
        name = data[pos:pos + name_len].decode("latin-1"); pos += name_len
        size, _filetime = struct.unpack_from("<iq", data, pos); pos += 12
        records.append((name, size))

    if len(records) != len(files):
        sys.exit(f"verify FAILED: {len(records)} records parsed, {len(files)} expected")

    offset = pos
    for (name, size), (rel, full, src_size) in zip(records, files):
        if name != rel or size != src_size:
            sys.exit(f"verify FAILED: record mismatch '{name}' vs '{rel}'")
        with open(full, "rb") as f:
            if data[offset:offset + size] != f.read():
                sys.exit(f"verify FAILED: data mismatch in '{rel}'")
        offset += size

    if offset != len(data):
        sys.exit(f"verify FAILED: {len(data) - offset} trailing bytes")
    print(f"verify OK: all {len(records)} records byte-identical to sources")


def main():
    ap = argparse.ArgumentParser(description="Build a PopCap main.pak from loose asset folders")
    ap.add_argument("root", help="media root (folder holding images/, reanim/, ...)")
    ap.add_argument("-o", "--output", default=None, help="output pak path (default: <root>/main.pak)")
    ap.add_argument("--dirs", nargs="+", default=DEFAULT_DIRS, help=f"folders to pack (default: {' '.join(DEFAULT_DIRS)})")
    args = ap.parse_args()

    out_path = args.output or os.path.join(args.root, "main.pak")
    files = build_pak(args.root, out_path, args.dirs)
    verify_pak(args.root, out_path, files)


if __name__ == "__main__":
    main()
