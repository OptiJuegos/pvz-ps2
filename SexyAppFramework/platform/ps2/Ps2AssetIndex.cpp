#ifdef PS2_PLATFORM

#include "Ps2AssetIndex.h"
#include "Ps2IoLock.h"
#include "Ps2PvzServices.h" // Ps2GetResourcePrefix()

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <set>

static std::set<std::string> gAssetFiles;
static bool gIndexReady = false;
static bool gIndexUsable = false;
// Device prefix the scan opens paths under (e.g. "cdfs:/"). Keys stay relative,
// so game lookups (relative) still match; only opendir() sees the prefix.
static std::string gScanRoot;
// True once we know misses are authoritative (index reflects the whole tree),
// so an absent entry means "not on the media" and no real open is attempted.
static bool gForceUsable = false;

// Lowercase + forward slashes + no leading "./" + no ISO9660 ";version" suffix,
// so lookups match however the game spells the path (resource ids arrive
// uppercase; cdfs readdir may hand back "MAIN.PAK;1").
static std::string NormalizePath(const std::string& thePath)
{
	std::string aPath;
	aPath.reserve(thePath.size());
	size_t aStart = (thePath.size() >= 2 && thePath[0] == '.' && (thePath[1] == '/' || thePath[1] == '\\')) ? 2 : 0;
	for (size_t i = aStart; i < thePath.size(); i++)
	{
		char c = thePath[i];
		if (c == ';')          // ISO9660 version marker: drop ";1".."; end of name
		{
			while (i + 1 < thePath.size() && thePath[i + 1] != '/' && thePath[i + 1] != '\\')
				i++;
			continue;
		}
		if (c == '\\')
			c = '/';
		else if (c >= 'A' && c <= 'Z')
			c = (char)(c - 'A' + 'a');
		aPath += c;
	}
	return aPath;
}

// stat()/d_type are unreliable across the PS2 fio drivers; an entry is a
// directory iff opendir() on it succeeds. The scan runs once, so the extra
// opens are cheap. theDir is relative (the stored key); gScanRoot is prepended
// only for the actual opendir so cdfs sees "cdfs:/dir".
static void ScanDir(const std::string& theDir, int theDepth)
{
	std::string aOpen;
	if (theDir.empty())
		aOpen = gScanRoot.empty() ? "." : gScanRoot;
	else
		aOpen = gScanRoot + theDir;

	DIR* d = opendir(aOpen.c_str());
	if (d == NULL)
		return;

	while (dirent* e = readdir(d))
	{
		const char* aName = e->d_name;
		if (aName[0] == '\0' || strcmp(aName, ".") == 0 || strcmp(aName, "..") == 0)
			continue;

		std::string aPath = theDir.empty() ? aName : theDir + "/" + aName;

		// Every asset file has an extension, so only extensionless entries can
		// be directories worth the opendir() probe (one fio round-trip each).
		bool aMaybeDir = strchr(aName, '.') == NULL;
		DIR* aSub = (aMaybeDir && theDepth < 4) ? opendir((gScanRoot + aPath).c_str()) : NULL;
		if (aSub != NULL)
		{
			closedir(aSub);
			ScanDir(aPath, theDepth + 1);
		}
		else
		{
			gAssetFiles.insert(NormalizePath(aPath));
		}
	}
	closedir(d);
}

static void EnsureIndex()
{
	if (gIndexReady)
		return;

	Ps2IoLockAcquire();
	if (!gIndexReady)
	{
		// On a CD/DVD boot, scan the disc root (cdfs:/) rather than cwd (cdrom0:,
		// which mangles subpaths). This is essential, not optional: the resource
		// manager probes many optional companions per asset (multi-extension,
		// "_name"/"name_" alpha variants) that are NOT on the disc, and on cdfs
		// each miss is a ~300ms directory walk. With the index built, those
		// misses are rejected from RAM. Real assets come from main.pak anyway;
		// the disc tree is tiny (just the pak + a couple loose files), so the
		// scan is quick. Force "usable" so an absent key means "not on the disc"
		// even if the tree turned out empty.
		gScanRoot = Ps2GetResourcePrefix();
		gForceUsable = !gScanRoot.empty();

		ScanDir("", 0);
		gIndexUsable = gForceUsable || gAssetFiles.size() > 0;
		printf("[PS2] asset index: %u files (root '%s')%s\n",
			(unsigned)gAssetFiles.size(), gScanRoot.c_str(),
			gIndexUsable ? "" : " (empty, probing falls back to fio)");
		gIndexReady = true;
	}
	Ps2IoLockRelease();
}

bool Ps2AssetExists(const std::string& thePath)
{
	// Device-prefixed paths (mass:/, mc0:/, host:/...) live outside the
	// scanned tree; let those go to the real open.
	if (thePath.find(':') != std::string::npos)
		return true;

	EnsureIndex();
	if (!gIndexUsable)
		return true;
	return gAssetFiles.find(NormalizePath(thePath)) != gAssetFiles.end();
}

void Ps2AssetNoteCreated(const std::string& thePath)
{
	if (thePath.find(':') != std::string::npos)
		return;
	Ps2IoLockAcquire();
	gAssetFiles.insert(NormalizePath(thePath));
	Ps2IoLockRelease();
}

#endif
