#include <time.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdexcept>

#ifdef PS2_PLATFORM
#include <malloc.h> // mallinfo() for heap diagnostics
#include <stdio.h>
// On PS2 every printf is a SIF fio RPC to the IOP (TTY/host write). The fio
// service is not thread-safe: the loading pthread and the main thread both
// log heavily during loads, and unserialized writes wedge the IOP on real
// hardware (works in emulators, which serialize SIF internally). Route the
// tracing here through the same global IO lock as the pak reader.
#include "Ps2IoLock.h"
#include "Ps2Trace.h" // Ps2BootStage: freeze forensics inside the log write

// Master switch for the on-disk debug log (userdata/log.txt). Every line costs
// an fopen+fwrite+fclose round-trip on the save device — deliberate (the last
// line must survive a freeze), but thousands of lines per load are a real
// slowdown on USB 1.1. At 0, log.txt only gets a one-line header saying
// logging is off and every append is skipped; set to 1 (or -DPS2_FILE_LOG=1)
// when hunting a freeze.
#ifndef PS2_FILE_LOG
#define PS2_FILE_LOG 0 // audio-dead-after-load bug resolved (mixer thread priority fix); logging back off for load speed
#endif
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "TodDebug.h"
#include "TodCommon.h"
#include "misc/Debug.h"
#include "../SexyAppFramework/SexyAppBase.h"

using namespace Sexy;

static char gLogFileName[512];
static char gDebugDataFolder[512];

#ifdef PS2_PLATFORM
// Holds every printf from boot until TodAssertInitForApp opens the single
// log.txt at <appdata>/userdata/ — sized for the whole boot+init transcript.
static char gEarlyLogBuffer[16384];
static size_t gEarlyLogBufferLen = 0;
static bool gEarlyLogOverflow = false;

static void TodBufferEarlyLog(const char* theMsg)
{
	if (theMsg == nullptr)
		return;

	size_t aLen = strlen(theMsg);
	size_t aRemaining = sizeof(gEarlyLogBuffer) - gEarlyLogBufferLen;
	size_t aCopyLen = aLen < aRemaining ? aLen : aRemaining;
	if (aCopyLen > 0)
	{
		memcpy(gEarlyLogBuffer + gEarlyLogBufferLen, theMsg, aCopyLen);
		gEarlyLogBufferLen += aCopyLen;
	}
	if (aCopyLen < aLen)
		gEarlyLogOverflow = true;
}
#endif

//0x514EA0
void TodErrorMessageBox(const char* theMessage, const char* theTitle)
{
#ifdef __SWITCH__
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, theTitle, theMessage);
	errorApplicationShow(&c);
#else
	throw std::runtime_error("Error Box\n--" + std::string(theTitle) + "--\n" + theMessage);
#endif
}

void TodTraceMemory()
{
}

void* TodMalloc(int theSize)
{
	TOD_ASSERT(theSize > 0);
	return malloc(theSize);
}

void TodFree(void* theBlock)
{
	if (theBlock != nullptr)
	{
		free(theBlock);
	}
}

void TodAssertFailed(const char* theCondition, const char* theFile, int theLine, const char* theMsg, ...)
{
	char aFormattedMsg[1024];
	va_list argList;
	va_start(argList, theMsg);
	int aCount = TodVsnprintf(aFormattedMsg, sizeof(aFormattedMsg), theMsg, argList);
	va_end(argList);

	if (aCount != 0) {
		if (aFormattedMsg[aCount - 1] != '\n')
		{
			if (aCount + 1 < 1024)
			{
				aFormattedMsg[aCount] = '\n';
				aFormattedMsg[aCount + 1] = '\0';
			}
			else
			{
				aFormattedMsg[aCount - 1] = '\n';
			}
		}
	}

	char aBuffer[1024];
	if (*theCondition != '\0')
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: '%s'\n%s\n", theFile, theLine, theCondition, aFormattedMsg);
	}
	else
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: %s\n", theFile, theLine, aFormattedMsg);
	}
	TodTrace("%s", aBuffer);

	TodErrorMessageBox(aBuffer, "Assertion failed");

	exit(0);
}

void TodLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	TodLogString(aButter);
}

void TodLogString(const char* theMsg)
{
#ifdef PS2_PLATFORM
	TodAppendToDebugLog(theMsg);
	return;
#endif
	FILE* f = fopen(gLogFileName, "a");
	if (f == nullptr)
	{
		fprintf(stderr, __S("Failed to open log file '%s'\n"), gLogFileName);
		return;
	}

	if (fwrite(theMsg, strlen(theMsg), 1, f) != 1)
	{
		fprintf(stderr, __S("Failed to write to log file\n"));
	}

	fclose(f);
}

#ifdef PS2_PLATFORM
extern "C" void TodInitDebugLogForAppData(const char* theAppDataFolder)
{
	if (theAppDataFolder == nullptr)
		theAppDataFolder = "";

	std::string aUserPath = std::string(theAppDataFolder) + "userdata/";
	MkDir(theAppDataFolder);
	MkDir(aUserPath);

	strcpy(gDebugDataFolder, aUserPath.c_str());
	strcpy(gLogFileName, gDebugDataFolder);
	strcpy(gLogFileName + strlen(gLogFileName), "log.txt");
	TOD_ASSERT(strlen(gLogFileName) < 512);

	Ps2IoLockAcquire();
	FILE* aFile = fopen(gLogFileName, "w");
	if (aFile != nullptr)
	{
		// Build stamp so a pulled stick proves at a glance the log is from the
		// binary just deployed, not a stale run.
		static const char kHeader[] =
			"[PS2 DEBUG] log.txt started from boot (build " __DATE__ " " __TIME__ ")\n";
		fwrite(kHeader, 1, sizeof(kHeader) - 1, aFile);
#if !PS2_FILE_LOG
		static const char kDisabled[] =
			"[PS2 DEBUG] file logging disabled (PS2_FILE_LOG=0 in TodDebug.cpp)\n";
		fwrite(kDisabled, 1, sizeof(kDisabled) - 1, aFile);
#endif
		if (gEarlyLogBufferLen > 0)
		{
			static const char kBufferedHeader[] = "[PS2 DEBUG] early printf buffer:\n";
			fwrite(kBufferedHeader, 1, sizeof(kBufferedHeader) - 1, aFile);
			fwrite(gEarlyLogBuffer, 1, gEarlyLogBufferLen, aFile);
			if (gEarlyLogBuffer[gEarlyLogBufferLen - 1] != '\n')
				fwrite("\n", 1, 1, aFile);
		}
		if (gEarlyLogOverflow)
		{
			static const char kOverflow[] = "[PS2 DEBUG] early printf buffer overflow; old output was truncated.\n";
			fwrite(kOverflow, 1, sizeof(kOverflow) - 1, aFile);
		}
		fclose(aFile);
	}
	gEarlyLogBufferLen = 0;
	gEarlyLogOverflow = false;
	// fprintf is not link-wrapped like printf, so this reaches the real EE
	// TTY and PCSX2's console shows where the log landed. Only when running
	// from host: (PCSX2/ps2link) — on a real console the TTY fio RPC can
	// wedge the IOP while the USB stack is busy, freezing boot on a black
	// screen.
	if (strncmp(gLogFileName, "host:", 5) == 0)
		fprintf(stderr, "[PS2 DEBUG] log file '%s' (%s)\n",
			gLogFileName, (aFile != nullptr) ? "open" : "OPEN FAILED");
	Ps2IoLockRelease();
}

extern "C" void TodAppendToDebugLog(const char* theMsg)
{
#if !PS2_FILE_LOG
	(void)theMsg;
	return;
#else
	if (theMsg == nullptr)
		return;

	if (gLogFileName[0] == '\0')
	{
		TodBufferEarlyLog(theMsg);
		return;
	}

	// Open-append-close per line, NOT a persistent handle: on FAT (USB stick)
	// and the memory card, the directory entry's file size is only committed
	// on close. A handle kept open across a freeze leaves log.txt reading as
	// 0KB even though every line was fwritten+fflushed into its clusters —
	// the "log siempre vacío" symptom. Closing per line is slower, but it
	// guarantees the last line survives any freeze, which is the whole point
	// of this log.
	//
	// Freeze forensics (real HW): the border colour is repainted at each step
	// of the write (pure GS register, no IO). When the console freezes, the
	// main-thread heartbeat stops repainting too, so the colour left on the
	// border names the exact wedged syscall:
	//   NAVY   (0,0,64)  = stuck inside fopen(log)
	//   PURPLE (64,0,64) = stuck inside fwrite
	//   OLIVE  (64,64,0) = stuck inside fclose
	//   DARK GREEN (0,64,0) = log write completed; freeze is in game code
	Ps2IoLockAcquire();
	Ps2BootStage(0, 0, 64);
	FILE* aFile = fopen(gLogFileName, "a");
	Ps2BootStage(64, 0, 64);
	if (aFile != nullptr)
	{
		fwrite(theMsg, 1, strlen(theMsg), aFile);
		Ps2BootStage(64, 64, 0);
		fclose(aFile);
	}
	Ps2BootStage(0, 64, 0);
	Ps2IoLockRelease();
#endif // PS2_FILE_LOG
}

// ---------------------------------------------------------------------------
// printf/puts/putchar link wraps (CMakeLists: -Wl,--wrap=printf ...).
//
// The real printf on PS2 is a SIF fio RPC to the IOP TTY; on real hardware
// that traffic competes with USB mass-storage and audsrv and can wedge the
// console. Instead, every console print goes to <save prefix>/userdata/log.txt
// through the persistent handle above — same file the original game used.
// Lines printed before the save prefix is chosen land in the early-log buffer
// and are replayed into log.txt when it opens.
// ---------------------------------------------------------------------------

extern "C" int __wrap_printf(const char* theFormat, ...)
{
	char aBuffer[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = vsnprintf(aBuffer, sizeof(aBuffer), theFormat, argList);
	va_end(argList);

	if (aCount > 0)
		TodAppendToDebugLog(aBuffer);
	return aCount;
}

extern "C" int __wrap_puts(const char* theString)
{
	TodAppendToDebugLog(theString != nullptr ? theString : "(null)");
	TodAppendToDebugLog("\n");
	return 0;
}

extern "C" int __wrap_putchar(int theChar)
{
	char aStr[2] = { (char)theChar, '\0' };
	TodAppendToDebugLog(aStr);
	return theChar;
}
#endif

void TodTrace(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

#ifdef PS2_PLATFORM
	Ps2IoLockAcquire();
	printf("%s", aButter);
	Ps2IoLockRelease();
#else
	printf("%s", aButter);
#endif
}

void TodHesitationTrace(const char* theFormat, ...)
{
#ifdef PS2_PLATFORM
	// PS2 loading diagnostics: print each hesitation point with current heap
	// usage so a stall shows exactly which step it reached (the original build
	// leaves this empty). Also report the last delta so growth is visible.
	static long sLastUsed = 0;
	struct mallinfo mi = mallinfo();
	long aUsed = (long)mi.uordblks;
	long aFree = (long)mi.fordblks;

	char aBuffer[512];
	va_list argList;
	va_start(argList, theFormat);
	TodVsnprintf(aBuffer, sizeof(aBuffer), theFormat, argList);
	va_end(argList);

	Ps2IoLockAcquire();
	printf("[HESITATE used=%.2fMB (%+.2fMB) free=%.2fMB] %s\n",
		(double)aUsed / (1024.0 * 1024.0),
		(double)(aUsed - sLastUsed) / (1024.0 * 1024.0),
		(double)aFree / (1024.0 * 1024.0),
		aBuffer);
	Ps2IoLockRelease();
	sLastUsed = aUsed;
#else
	(void)theFormat;
#endif
}

void TodTraceAndLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	#ifdef PS2_PLATFORM
	Ps2IoLockAcquire();
	printf("%s", aButter);
	Ps2IoLockRelease();
	#else
	printf("%s", aButter);
	TodLogString(aButter);
	#endif
}

void TodTraceWithoutSpamming(const char* theFormat, ...)
{
	static uint64_t gLastTraceTime = 0LL;
	uint64_t aTime = time(NULL);
	if (aTime < gLastTraceTime)
	{
		return;
	}

	gLastTraceTime = aTime;
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

#ifdef PS2_PLATFORM
	Ps2IoLockAcquire();
	printf("%s", aButter);
	Ps2IoLockRelease();
#else
	printf("%s", aButter);
#endif
}

void TodAssertInitForApp()
{
#ifdef PS2_PLATFORM
	// SexyAppBase::Init re-points the app data folder at <cwd>/savedata/,
	// overriding the boot-time save prefix from Ps2PvzServices. A log opened
	// at boot can therefore sit somewhere else entirely (e.g. inside mc0:
	// while saves land on host/USB). Reopen it at the effective location
	// whenever the path changed; TodInitDebugLogForAppData closes the old
	// handle itself.
	if ((GetAppDataFolder() + "userdata/log.txt") != gLogFileName)
	{
		TodInitDebugLogForAppData(GetAppDataFolder().c_str());
	}
	else
	{
		MkDir(GetAppDataFolder());
		MkDir(GetAppDataFolder() + "userdata");
	}
	strcpy(gDebugDataFolder, (GetAppDataFolder() + "userdata/").c_str());
	strcpy(gLogFileName, gDebugDataFolder);
	strcpy(gLogFileName + strlen(gLogFileName), "log.txt");
	TOD_ASSERT(strlen(gLogFileName) < 512);
#else
	MkDir(GetAppDataFolder());
	MkDir(GetAppDataFolder() + "userdata");
	std::string aRelativeUserPath = GetAppDataFolder() + "userdata/";
	strcpy(gDebugDataFolder, aRelativeUserPath.c_str());
	strcpy(gLogFileName, gDebugDataFolder);
	strcpy(gLogFileName + strlen(gLogFileName), "log.txt");
	TOD_ASSERT(strlen(gLogFileName) < 512);
#endif

	TodLog("Started %d\n", (uint64_t)time(NULL));
}
