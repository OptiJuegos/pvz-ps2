#include "fcaseopen.h"

#include "Ps2IoLock.h" // serialize SIF RPC file I/O on PS2 (no-op elsewhere)

#include <unistd.h> // fix "implicit declaration of function chdir"

#ifdef PS2_PLATFORM
#include <string.h>
#include <stdio.h>
#include "Ps2PvzServices.h" // Ps2GetResourcePrefix()

// On a CD/DVD boot, relative asset paths must resolve against the disc, not the
// launcher's cwd (which is cdrom0: — read-only and path-mangling). The services
// layer sets the read root to "cdfs:/"; prepend it to relative READ opens only.
// Writes keep going straight through: every writable path the game builds is
// already absolute (GetAppDataFolder() -> mc0:/mass:), so it carries a ':' and
// is left untouched. On host:/USB the prefix is empty -> zero behaviour change.
static const char* ps2_resolve_read_path(const char* thePath, const char* theMode,
                                          char* theBuf, size_t theBufLen)
{
    const char* aPrefix = Ps2GetResourcePrefix();
    if (aPrefix[0] == '\0' || thePath == NULL || thePath[0] == '\0')
        return thePath;                       // no CD read root: unchanged
    if (theMode == NULL || theMode[0] != 'r')
        return thePath;                       // writes are not asset reads
    if (strchr(thePath, ':') != NULL)
        return thePath;                       // already device-qualified
    snprintf(theBuf, theBufLen, "%s%s", aPrefix, thePath);
    return theBuf;
}
#endif

// casepath()/strsep() implement the case-insensitive fallback. It is disabled on
// PS2 (see fcaseopen/casechdir below), so skip compiling it there to avoid
// unused-function warnings and needless dirent usage.
#if !defined(_WIN32) && !defined(PS2_PLATFORM)
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>


#ifdef __HAIKU__
// this function seems to not exist under haiku??
char *strsep(char **stringp, const char *delim)
{
	char *begin, *end;
	begin = *stringp;
	if (begin == NULL) return NULL;

	if (delim[0] == '\0' || begin[0] == '\0')
	{
		*stringp = NULL;
		return begin;
	}

	end = strpbrk(begin, delim);
	if (end)
	{
		*end = '\0';
		*stringp = end + 1;
	}
	else
	{
		*stringp = NULL;
	}
	return begin;
}
#endif


// r must have strlen(path) + 3 bytes
static int casepath(char const *path, char *r)
{
    size_t l = strlen(path);
    char *p = alloca(l + 1);
    strcpy(p, path);
    size_t rl = 0;
    
    DIR *d;
    if (p[0] == '/')
    {
        d = opendir("/");
        p = p + 1;
    }
    else
    {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }
    
    int last = 0;
    char *c = strsep(&p, "/");
    while (c)
    {
        if (!d)
        {
            return 0;
        }
        
        if (last)
        {
            closedir(d);
            return 0;
        }
        
        r[rl] = '/';
        rl += 1;
        r[rl] = 0;
        
        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);
                
                break;
            }
            
            e = readdir(d);
        }
        
        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }
        
        c = strsep(&p, "/");
    }
    
    if (d) closedir(d);
    return 1;
}
#endif

FILE *fcaseopen(char const *path, char const *mode)
{
    Ps2IoLockAcquire();
#ifdef PS2_PLATFORM
    char aResolved[300];
    path = ps2_resolve_read_path(path, mode, aResolved, sizeof(aResolved));
#endif
    FILE *f = fopen(path, mode);
    // The casepath() fallback walks directories (opendir/readdir), and on PS2
    // every such call is an SIF RPC. It is disabled on PS2 because:
    //   * PCSX2's host: FS (backed by Windows/NTFS) is already case-insensitive,
    //     so a direct fopen resolves any file that actually exists;
    //   * all real assets live in the pak (matched via the record map, no host
    //     I/O), and the loose files we touch (main.pak, savedata, compiled/*)
    //     are named by the game itself with consistent case.
    // When a file is genuinely absent (e.g. auto-alpha companions "_Name.*" that
    // most images don't have), the ResourceManager probes several extensions;
    // the casepath fallback would turn each miss into a burst of opendir/readdir
    // SIF RPCs, flooding and eventually crashing the RPC queue (_request_end).
    // A single failing direct fopen per miss is cheap and correct.
#if !defined(_WIN32) && !defined(PS2_PLATFORM)
    if (!f)
    {
        char *r = alloca(strlen(path) + 3);
        if (casepath(path, r))
        {
            f = fopen(r, mode);
        }
    }
#endif
    Ps2IoLockRelease();
    return f;
}

void casechdir(char const *path)
{
    Ps2IoLockAcquire();
#if !defined(_WIN32) && !defined(PS2_PLATFORM)
    char *r = alloca(strlen(path) + 3);
    if (casepath(path, r))
    {
        chdir(r);
    }
    else
    {
        errno = ENOENT;
    }
#else
    chdir(path);
#endif
    Ps2IoLockRelease();
}
