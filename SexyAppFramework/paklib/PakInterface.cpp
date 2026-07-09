#include <unistd.h>
#include "Common.h"
#include "PakInterface.h"
#include "fcaseopen/fcaseopen.h"
#ifdef PS2_PLATFORM
#include "Ps2IoLock.h"
#include "Ps2AssetIndex.h"
#include <cstdlib> // malloc/free for per-record buffering
#include <map>     // shared pak read-handle cache
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

enum
{
	FILEFLAGS_END = 0x80
};

PakInterface* gPakInterface = new PakInterface();

#ifdef PS2_PLATFORM
// Read in bounded chunks instead of one giant fread. Two reasons, both
// real-hardware only: the legacy usbhdfsd driver is not trusted with single
// large transfers (log writes are already capped at 2KB for the same reason),
// and a multi-MB fread over USB 1.1 takes seconds — holding Ps2IoLock for that
// whole stretch starves the main thread's pad poll.
//
// theDropLockBetweenChunks: pass true only when theFP is PRIVATE to the
// calling PFILE (loose files, per-PFILE streaming fallback) — releasing the
// lock between chunks keeps the app responsive and another thread cannot
// disturb this handle's cursor. Pass false for the SHARED pak handle: its
// seek+read must stay atomic under Ps2IoLock or a concurrent reader moves the
// cursor mid-record.
static size_t Ps2ChunkedFRead(void* thePtr, size_t theSizeBytes, FILE* theFP,
                              bool theDropLockBetweenChunks)
{
	const size_t kChunk = 128 * 1024;
	size_t aTotal = 0;
	if (!theDropLockBetweenChunks)
		Ps2IoLockAcquire();
	while (aTotal < theSizeBytes)
	{
		size_t aWant = theSizeBytes - aTotal;
		if (aWant > kChunk)
			aWant = kChunk;
		if (theDropLockBetweenChunks)
			Ps2IoLockAcquire();
		size_t aGot = fread((unsigned char*)thePtr + aTotal, 1, aWant, theFP);
		if (theDropLockBetweenChunks)
			Ps2IoLockRelease();
		aTotal += aGot;
		if (aGot != aWant)
			break;
	}
	if (!theDropLockBetweenChunks)
		Ps2IoLockRelease();
	return aTotal;
}

// One persistent read handle per pak collection, opened on first use and kept
// for the life of the app. The previous code fcaseopen'd + fclose'd main.pak
// for EVERY record it served — thousands of open/close cycles over usbhdfsd
// (each one an IOP open RPC plus a FAT directory walk), which is exactly the
// fd/IO churn the legacy driver wedges under on real hardware. Callers must
// hold Ps2IoLock across their whole seek+read of this handle and must NOT
// fclose it.
static FILE* Ps2GetSharedPakHandle(PakCollection* theCollection, const char* thePath)
{
	static std::map<PakCollection*, FILE*> sPakHandles;
	Ps2IoLockAcquire();
	FILE*& aFP = sPakHandles[theCollection];
	if (aFP == NULL)
		aFP = fcaseopen(thePath, "rb");
	FILE* aResult = aFP;
	Ps2IoLockRelease();
	return aResult;
}
#endif

static std::string StringToUpper(const std::string& theString)
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += toupper(theString[i]);

	return aString;
}

PakInterface::PakInterface()
{
	//if (GetPakPtr() == NULL)
		//*gPakInterfaceP = this;
}

PakInterface::~PakInterface()
{
}

//0x5D84D0
static void FixFileName(const char* theFileName, char* theUpperName)
{
	// 检测路径是否为从盘符开始的绝对路径
	if ((theFileName[0] != 0) && (theFileName[1] == ':'))
	{
		char aDir[256];
		getcwd(aDir, 256);  // 取得当前工作路径
		int aLen = strlen(aDir);
		aDir[aLen++] = '/';
		aDir[aLen] = 0;

		// 判断 theFileName 文件是否位于当前目录下
		if (strncasecmp(aDir, theFileName, aLen) == 0)
			theFileName += aLen;  // 若是，则跳过从盘符到当前目录的部分，转化为相对路径
	}

	bool lastSlash = false;
	const char* aSrc = theFileName;
	char* aDest = theUpperName;

	for (;;)
	{
		char c = *(aSrc++);

		if ((c == '\\') || (c == '/'))
		{
			// 统一转为右斜杠，且多个斜杠的情况下只保留一个
			if (!lastSlash)
				*(aDest++) = '/';
			lastSlash = true;
		}
		else if ((c == '.') && (lastSlash) && (*aSrc == '.'))
		{
			// We have a '/..' on our hands
			aDest--;
			while ((aDest > theUpperName + 1) && (*(aDest-1) != '\\'))  // 回退到上一层目录
				--aDest;
			aSrc++;
			// 此处将形如“a\b\..\c”的路径简化为“a\c”
		}
		else
		{
			*(aDest++) = toupper((uchar) c);
			if (c == 0)
				break;
			lastSlash = false;				
		}
	}
}

bool PakInterface::AddPakFile(const std::string& theFileName)
{
#ifdef PS2_PLATFORM
	// PS2: stream from disk instead of loading 44 MB into 32 MB RAM.
	// Parse the directory header now, then keep the pak path so each
	// FOpen() can open a private file handle for concurrent-safe reads.
	FILE* f = fcaseopen(theFileName.c_str(), "rb");
	if (!f) return false;

	// Helper: read + XOR-decrypt a block from the file.
	auto xread = [f](void* dst, size_t sz) -> bool {
		if (fread(dst, 1, sz, f) != sz) return false;
		for (size_t i = 0; i < sz; i++)
			((uint8_t*)dst)[i] ^= 0xF7;
		return true;
	};

	uint32_t aMagic = 0;
	if (!xread(&aMagic, 4) || aMagic != 0xBAC04AC0) { fclose(f); return false; }
	uint32_t aVersion = 0;
	if (!xread(&aVersion, 4) || aVersion > 0)        { fclose(f); return false; }

	mPakCollectionList.emplace_back(0);
	PakCollection* aPakCollection = &mPakCollectionList.back();
	aPakCollection->mPakPath = theFileName;

	int aPos = 0;
	int aRecordCount = 0;
	for (;;)
	{
		uchar aFlags = 0;
		if (fread(&aFlags, 1, 1, f) != 1) { printf("[PvZ PS2] pak dir: fread flags failed at entry %d\n", aRecordCount); break; }
		aFlags ^= 0xF7;
		if (aFlags & FILEFLAGS_END) { printf("[PvZ PS2] pak dir: END flag at entry %d\n", aRecordCount); break; }

		uchar aNameWidth = 0;
		char aName[256];
		if (fread(&aNameWidth, 1, 1, f) != 1) break;
		aNameWidth ^= 0xF7;
		if ((size_t)fread(aName, 1, aNameWidth, f) != (size_t)aNameWidth) break;
		for (int i = 0; i < aNameWidth; i++) aName[i] ^= 0xF7;
		aName[aNameWidth] = 0;

		int aSrcSize = 0;
		int64_t aFileTime = 0;
		if (!xread(&aSrcSize, sizeof(int))) break;
		if (!xread(&aFileTime, sizeof(int64_t))) break;

		for (int i = 0; i < aNameWidth; i++)
			if (aName[i] == '\\') aName[i] = '/';

		char anUpperName[256];
		FixFileName(aName, anUpperName);

		PakRecordMap::iterator aRecordItr = mPakRecordMap.insert(
			PakRecordMap::value_type(StringToUpper(aName), PakRecord())).first;
		PakRecord* aPakRecord = &(aRecordItr->second);
		aPakRecord->mCollection = aPakCollection;
		aPakRecord->mFileName   = anUpperName;
		aPakRecord->mStartPos   = aPos;
		aPakRecord->mSize       = aSrcSize;
		aPakRecord->mFileTime   = aFileTime;
		aPos += aSrcSize;
		++aRecordCount;
	}

	int anOffset = (int)ftell(f);
	fclose(f);

	// Adjust all start positions to absolute offsets inside the pak file.
	for (auto& kv : mPakRecordMap)
	{
		if (kv.second.mCollection == aPakCollection)
			kv.second.mStartPos += anOffset;
	}

	printf("[PvZ PS2] AddPakFile '%s': %d records, data_start=%d\n",
		theFileName.c_str(), aRecordCount, anOffset);
	// Print a few sample keys so we can verify the format
	{
		int n = 0;
		for (auto& kv : mPakRecordMap) {
			if (n++ >= 4) { printf("  ...\n"); break; }
			printf("  key[%d]='%s' start=%d size=%d\n",
				n, kv.first.c_str(), kv.second.mStartPos, kv.second.mSize);
		}
	}

	return true;

#else // !PS2_PLATFORM

	/*
	HANDLE aFileHandle = CreateFile(theFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (aFileHandle == INVALID_HANDLE_VALUE)
		return false;

	int aFileSize = GetFileSize(aFileHandle, 0);

	HANDLE aFileMapping = CreateFileMapping(aFileHandle, NULL, PAGE_READONLY, 0, aFileSize, NULL);
	if (aFileMapping == NULL)
	{
		CloseHandle(aFileHandle);
		return false;
	}

	void* aPtr = MapViewOfFile(aFileMapping, FILE_MAP_READ, 0, 0, aFileSize);
	if (aPtr == NULL)
	{
		CloseHandle(aFileMapping);
		CloseHandle(aFileHandle);
		return false;
	}
	*/

	FILE *aFileHandle = fcaseopen(theFileName.c_str(), "rb");
    if (!aFileHandle) return false;

    fseek(aFileHandle, 0, SEEK_END);
    size_t aFileSize = ftell(aFileHandle);
    fseek(aFileHandle, 0, SEEK_SET);

	mPakCollectionList.emplace_back(aFileSize);
	PakCollection* aPakCollection = &mPakCollectionList.back();
	/*
	aPakCollection->mFileHandle = aFileHandle;
	aPakCollection->mMappingHandle = aFileMapping;
	aPakCollection->mDataPtr = aPtr;
	*/

	if (fread(aPakCollection->mDataPtr, 1, aFileSize, aFileHandle) != aFileSize) {
        fclose(aFileHandle);
        return false;
    }
    fclose(aFileHandle);

    {
        auto *aDataPtr = static_cast<uint8_t *>(aPakCollection->mDataPtr);
        for (size_t i = 0; i < aFileSize; i++)
            *aDataPtr++ ^= 0xF7;
    }

	PakRecordMap::iterator aRecordItr = mPakRecordMap.insert(PakRecordMap::value_type(StringToUpper(theFileName), PakRecord())).first;
	PakRecord* aPakRecord = &(aRecordItr->second);
	aPakRecord->mCollection = aPakCollection;
	aPakRecord->mFileName = theFileName;
	aPakRecord->mStartPos = 0;
	aPakRecord->mSize = aFileSize;

	PFILE* aFP = FOpen(theFileName.c_str(), "rb");
	if (aFP == NULL)
		return false;

	uint32_t aMagic = 0;
	FRead(&aMagic, sizeof(uint32_t), 1, aFP);
	if (aMagic != 0xBAC04AC0)
	{
		FClose(aFP);
		return false;
	}

	uint32_t aVersion = 0;
	FRead(&aVersion, sizeof(uint32_t), 1, aFP);
	if (aVersion > 0)
	{
		FClose(aFP);
		return false;
	}

	int aPos = 0;

	for (;;)
	{
		uchar aFlags = 0;
		int aCount = FRead(&aFlags, 1, 1, aFP);
		if ((aFlags & FILEFLAGS_END) || (aCount == 0))
			break;

		uchar aNameWidth = 0;
		char aName[256];
		FRead(&aNameWidth, 1, 1, aFP);
		FRead(aName, 1, aNameWidth, aFP);
		aName[aNameWidth] = 0;
		int aSrcSize = 0;
		FRead(&aSrcSize, sizeof(int), 1, aFP);
		int64_t aFileTime;
		FRead(&aFileTime, sizeof(int64_t), 1, aFP);

		for (int i=0; i<aNameWidth; i++)
		{
			if (aName[i] == '\\')
				aName[i] = '/'; // lol
		}

		char anUpperName[256];
		FixFileName(aName, anUpperName);

		PakRecordMap::iterator aRecordItr = mPakRecordMap.insert(PakRecordMap::value_type(StringToUpper(aName), PakRecord())).first;
		PakRecord* aPakRecord = &(aRecordItr->second);
		aPakRecord->mCollection = aPakCollection;
		aPakRecord->mFileName = anUpperName;
		aPakRecord->mStartPos = aPos;
		aPakRecord->mSize = aSrcSize;
		aPakRecord->mFileTime = aFileTime;

		aPos += aSrcSize;
	}

	int anOffset = FTell(aFP);

	// Now fix file starts
	aRecordItr = mPakRecordMap.begin();
	while (aRecordItr != mPakRecordMap.end())
	{
		PakRecord* aPakRecord = &(aRecordItr->second);
		if (aPakRecord->mCollection == aPakCollection)
			aPakRecord->mStartPos += anOffset;
		++aRecordItr;
	}

	FClose(aFP);

	return true;
#endif // PS2_PLATFORM
}

//0x5D85C0
PFILE* PakInterface::FOpen(const char* theFileName, const char* anAccess)
{
	if ((strcasecmp(anAccess, "r") == 0) || (strcasecmp(anAccess, "rb") == 0) || (strcasecmp(anAccess, "rt") == 0))
	{
		char anUpperName[256];
		FixFileName(theFileName, anUpperName);

#ifdef PS2_PLATFORM
		static int sFOpenDbg = 0;
		if (sFOpenDbg < 8) {
			printf("[PvZ PS2] FOpen #%d map=%zu key='%s'\n",
				sFOpenDbg, mPakRecordMap.size(), anUpperName);
			++sFOpenDbg;
		}
#endif

		PakRecordMap::iterator anItr = mPakRecordMap.find(anUpperName);
		if (anItr == mPakRecordMap.end())
			anItr = mPakRecordMap.find(theFileName);

		if (anItr != mPakRecordMap.end())
		{
			PFILE* aPFP = new PFILE;
			aPFP->mRecord = &anItr->second;
			aPFP->mPos = 0;
#ifdef PS2_PLATFORM
			// Read the whole (decrypted) record into memory once, then serve all
			// reads from it. This avoids a per-call fseek+fread SIF RPC — the XML
			// parser reads records one byte at a time, which otherwise turns each
			// small file into hundreds of thousands of SIF RPC round-trips.
			aPFP->mFP = NULL;
			aPFP->mBuffer = NULL;
			int aSize = anItr->second.mSize;
			Ps2IoLockAcquire(); // printf is a fio RPC too — serialize with pak IO
			printf("[PAKREAD] '%s' size=%d start=%d ...\n",
				anItr->second.mFileName.c_str(), aSize, anItr->second.mStartPos);
			fflush(stdout); // ensure the record name is visible even if the read hangs
			Ps2IoLockRelease();
			unsigned char* aBuffer = (unsigned char*)malloc(aSize > 0 ? aSize : 1);
			if (aBuffer != NULL)
			{
				// Shared persistent handle: one open for the app's lifetime
				// instead of an open/close pair per record. Seek+read stay
				// atomic under Ps2IoLock (false below) because the handle is
				// shared between the loading and main threads.
				FILE* f = Ps2GetSharedPakHandle(anItr->second.mCollection,
					anItr->second.mCollection->mPakPath.c_str());
				if (f != NULL)
				{
					Ps2IoLockAcquire();
					fseek(f, anItr->second.mStartPos, SEEK_SET);
					size_t aRead = Ps2ChunkedFRead(aBuffer, (size_t)aSize, f, false);
					for (size_t i = 0; i < aRead; i++)
						aBuffer[i] ^= 0xF7;
					aPFP->mBuffer = aBuffer;
					printf("[PAKREAD] '%s' done (%u bytes)\n",
						anItr->second.mFileName.c_str(), (unsigned)aRead);
					Ps2IoLockRelease();
				}
				else
				{
					free(aBuffer);
					Ps2IoLockAcquire();
					printf("[PAKREAD] '%s' open FAILED\n", anItr->second.mFileName.c_str());
					Ps2IoLockRelease();
				}
			}
			else
			{
				Ps2IoLockAcquire();
				printf("[PAKREAD] '%s' malloc(%d) FAILED\n",
					anItr->second.mFileName.c_str(), aSize);
				Ps2IoLockRelease();
			}
			// Fallback: if buffering failed, keep a streaming handle open.
			if (aPFP->mBuffer == NULL)
				aPFP->mFP = fcaseopen(anItr->second.mCollection->mPakPath.c_str(), "rb");
#else
			aPFP->mFP = NULL;
#endif
			return aPFP;
		}
	}

#ifdef PS2_PLATFORM
	// The pak lookup already missed; for read-only opens consult the startup
	// asset index before paying a fio open round-trip. Image loading probes
	// dozens of candidate paths per asset, and each miss was a full RPC.
	bool aWrites = (strchr(anAccess, 'w') != NULL || strchr(anAccess, 'a') != NULL || strchr(anAccess, '+') != NULL);
	if (!aWrites && !Ps2AssetExists(theFileName))
		return NULL;
#endif

	FILE* aFP = fcaseopen(theFileName, anAccess);
	if (aFP == NULL)
		return NULL;
#ifdef PS2_PLATFORM
	if (aWrites)
		Ps2AssetNoteCreated(theFileName);
#endif
	PFILE* aPFP = new PFILE;
	aPFP->mRecord = NULL;
	aPFP->mPos = 0;
	aPFP->mFP = aFP;
	return aPFP;
}

//0x5D8780
int PakInterface::FClose(PFILE* theFile)
{
#ifdef PS2_PLATFORM
	// Free the in-memory record buffer (normal pak-record path) or close the
	// streaming handle (fallback / non-record files).
	if (theFile->mBuffer != NULL)
		free(theFile->mBuffer);
	if (theFile->mFP != NULL)
	{
		Ps2IoLockAcquire();
		fclose(theFile->mFP);
		Ps2IoLockRelease();
	}
#else
	if (theFile->mRecord == NULL)
		fclose(theFile->mFP);
#endif
	delete theFile;
	return 0;
}

//0x5D87B0
int PakInterface::FSeek(PFILE* theFile, long theOffset, int theOrigin)
{
	if (theFile->mRecord != NULL)
	{
		if (theOrigin == SEEK_SET)
			theFile->mPos = theOffset;
		else if (theOrigin == SEEK_END)
			theFile->mPos = theFile->mRecord->mSize - theOffset;
		else if (theOrigin == SEEK_CUR)
			theFile->mPos += theOffset;

		// 当前指针位置不能超过整个文件的大小，且不能小于 0
		theFile->mPos = std::max(std::min(theFile->mPos, theFile->mRecord->mSize), 0);
		return 0;
	}
	else
	{
#ifdef PS2_PLATFORM
		Ps2IoLockAcquire();
		int aResult = fseek(theFile->mFP, theOffset, theOrigin);
		Ps2IoLockRelease();
		return aResult;
#else
		return fseek(theFile->mFP, theOffset, theOrigin);
#endif
	}
}

//0x5D8830
int PakInterface::FTell(PFILE* theFile)
{
	if (theFile->mRecord != NULL)
		return theFile->mPos;
	else
	{
#ifdef PS2_PLATFORM
		Ps2IoLockAcquire();
		int aResult = ftell(theFile->mFP);
		Ps2IoLockRelease();
		return aResult;
#else
		return ftell(theFile->mFP);
#endif
	}
}

//0x5D8850
size_t PakInterface::FRead(void* thePtr, int theElemSize, int theCount, PFILE* theFile)
{
	if (theFile->mRecord != NULL)
	{
		int aSizeBytes = std::min(theElemSize*theCount, theFile->mRecord->mSize - theFile->mPos);
#ifdef PS2_PLATFORM
		if (theFile->mBuffer != NULL)
		{
			// Fast path: serve from the pre-read, pre-decrypted record buffer.
			memcpy(thePtr, theFile->mBuffer + theFile->mPos, aSizeBytes);
			theFile->mPos += aSizeBytes;
			return aSizeBytes / theElemSize;
		}
		// Fallback streaming: seek to the record's position and read + decrypt.
		FILE* f = theFile->mFP;
		if (f == NULL) return 0;
		Ps2IoLockAcquire();
		fseek(f, theFile->mRecord->mStartPos + theFile->mPos, SEEK_SET);
		Ps2IoLockRelease();
		size_t bytesRead = Ps2ChunkedFRead(thePtr, (size_t)aSizeBytes, f, true);
		uchar* p = (uchar*)thePtr;
		for (size_t i = 0; i < bytesRead; i++) p[i] ^= 0xF7;
		theFile->mPos += (int)bytesRead;
		return bytesRead / (size_t)theElemSize;
#else
		uchar* src = (uchar*) theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos;
		uchar* dest = (uchar*) thePtr;
		memcpy(dest, src, aSizeBytes);
		theFile->mPos += aSizeBytes;
		return aSizeBytes / theElemSize;
#endif
	}

#ifdef PS2_PLATFORM
	// Loose files (e.g. the OGG soundtrack) can be multi-MB single reads too.
	size_t aBytes = Ps2ChunkedFRead(thePtr, (size_t)theElemSize * (size_t)theCount, theFile->mFP, true);
	return (theElemSize > 0) ? aBytes / (size_t)theElemSize : 0;
#else
	return fread(thePtr, theElemSize, theCount, theFile->mFP);
#endif
}

int PakInterface::FGetC(PFILE* theFile)
{
	if (theFile->mRecord != NULL)
	{
		for (;;)
		{
			if (theFile->mPos >= theFile->mRecord->mSize)
				return EOF;
#ifdef PS2_PLATFORM
			uchar aChar = 0;
			if (FRead(&aChar, 1, 1, theFile) != 1) return EOF;
			if (aChar != '\r') return (uchar)aChar;
#else
			char aChar = *((char*) theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos++);
			if (aChar != '\r')
				return (uchar) aChar;
#endif
		}
	}

	return fgetc(theFile->mFP);
}

int PakInterface::UnGetC(int theChar, PFILE* theFile)
{
	if (theFile->mRecord != NULL)
	{
		// This won't work if we're not pushing the same chars back in the stream
		theFile->mPos = std::max(theFile->mPos - 1, 0);
		return theChar;
	}

	return ungetc(theChar, theFile->mFP);
}

char* PakInterface::FGetS(char* thePtr, int theSize, PFILE* theFile)
{
	if (theFile->mRecord != NULL)
	{
		int anIdx = 0;
		while (anIdx < theSize)
		{
			if (theFile->mPos >= theFile->mRecord->mSize)
			{
				if (anIdx == 0)
					return NULL;
				break;
			}
#ifdef PS2_PLATFORM
			uchar aChar = 0;
			if (FRead(&aChar, 1, 1, theFile) != 1) break;
#else
			char aChar = *((char*) theFile->mRecord->mCollection->mDataPtr + theFile->mRecord->mStartPos + theFile->mPos++);
#endif
			if (aChar != '\r')
				thePtr[anIdx++] = (char)aChar;
			if (aChar == '\n')
				break;
		}
		thePtr[anIdx] = 0;
		return thePtr;
	}

	return fgets(thePtr, theSize, theFile->mFP);
}

int PakInterface::FEof(PFILE* theFile)
{
	if (theFile->mRecord != NULL)
		return theFile->mPos >= theFile->mRecord->mSize;
	else
		return feof(theFile->mFP);
}

/*
bool PakInterface::PFindNext(PFindData* theFindData, LPWIN32_FIND_DATA lpFindFileData)
{
	PakRecordMap::iterator anItr;
	if (theFindData->mLastFind.size() == 0)
		anItr = mPakRecordMap.begin();
	else
	{
		anItr = mPakRecordMap.find(theFindData->mLastFind);
		if (anItr != mPakRecordMap.end())
			anItr++;
	}

	while (anItr != mPakRecordMap.end())
	{
		const char* aFileName = anItr->first.c_str();
		PakRecord* aPakRecord = &anItr->second;

		int aStarPos = (int) theFindData->mFindCriteria.find('*');
		if (aStarPos != -1)
		{
			if (strncmp(theFindData->mFindCriteria.c_str(), aFileName, aStarPos) == 0)
			{				
				// First part matches
				const char* anEndData = theFindData->mFindCriteria.c_str() + aStarPos + 1;
				if ((*anEndData == 0) || (strcmp(anEndData, ".*") == 0) ||								
					(strcmp(theFindData->mFindCriteria.c_str() + aStarPos + 1, 
					aFileName + strlen(aFileName) - (theFindData->mFindCriteria.length() - aStarPos) + 1) == 0))
				{
					// Matches before and after star
					memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAA));
					
					int aLastSlashPos = (int) anItr->second.mFileName.rfind('/');
					if (aLastSlashPos == -1)
						strcpy(lpFindFileData->cFileName, anItr->second.mFileName.c_str());
					else
						strcpy(lpFindFileData->cFileName, anItr->second.mFileName.c_str() + aLastSlashPos + 1);

					const char* aEndStr = aFileName + strlen(aFileName) - (theFindData->mFindCriteria.length() - aStarPos) + 1;
					if (strchr(aEndStr, '/') != NULL)
						lpFindFileData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

					lpFindFileData->nFileSizeLow = aPakRecord->mSize;
					lpFindFileData->ftCreationTime = aPakRecord->mFileTime;
					lpFindFileData->ftLastWriteTime = aPakRecord->mFileTime;
					lpFindFileData->ftLastAccessTime = aPakRecord->mFileTime;
					theFindData->mLastFind = aFileName;

					return true;
				}
			}
		}

		++anItr;
	}

	return false;
}

HANDLE PakInterface::FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	PFindData* aFindData = new PFindData;

	char anUpperName[256];
	FixFileName(lpFileName, anUpperName);
	aFindData->mFindCriteria = anUpperName;
	aFindData->mWHandle = INVALID_HANDLE_VALUE;

	if (PFindNext(aFindData, lpFindFileData))
		return (HANDLE) aFindData;

	aFindData->mWHandle = ::FindFirstFile(aFindData->mFindCriteria.c_str(), lpFindFileData);
	if (aFindData->mWHandle != INVALID_HANDLE_VALUE)
		return (HANDLE) aFindData;

	delete aFindData;
	return INVALID_HANDLE_VALUE;
}

BOOL PakInterface::FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	PFindData* aFindData = (PFindData*) hFindFile;

	if (aFindData->mWHandle == INVALID_HANDLE_VALUE)
	{
		if (PFindNext(aFindData, lpFindFileData))
			return TRUE;

		aFindData->mWHandle = ::FindFirstFile(aFindData->mFindCriteria.c_str(), lpFindFileData);
		return (aFindData->mWHandle != INVALID_HANDLE_VALUE);			
	}
	
	return ::FindNextFile(aFindData->mWHandle, lpFindFileData);
}

BOOL PakInterface::FindClose(HANDLE hFindFile)
{
	PFindData* aFindData = (PFindData*) hFindFile;

	if (aFindData->mWHandle != INVALID_HANDLE_VALUE)
		::FindClose(aFindData->mWHandle);

	delete aFindData;
	return TRUE;
}
*/
