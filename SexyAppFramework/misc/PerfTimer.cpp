#include "PerfTimer.h"
#include <map>
#include <SDL.h>

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline int QueryCounters(int64_t *lpPerformanceCount)
{
	(void)lpPerformanceCount;
	// Argh fuck it just hope it never happens
	unreachable();
	// returns TSC only
	/*
	asm (
		"mov ebx, dword ptr [lpPerformanceCount]"
		"rdtsc"
		"	mov dword ptr [ebx], eax"
		"	mov dword ptr [ebx+4], edx"
	);
	return 1;
	*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline int DeltaCounters(int64_t *lpPerformanceCount)
{
	(void)lpPerformanceCount;
	// Argh fuck it just hope it never happens
	unreachable();
	/*
	asm (
		"mov ebx, dword ptr [lpPerformanceCount]"
		"rdtsc"
		"	sub eax, dword ptr [ebx]"
		"	sbb edx, dword ptr [ebx+4]"
		"	mov dword ptr [ebx],   eax"
		"		mov dword ptr [ebx+4], edx"
	);
	return 1;
	*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int64_t CalcCPUSpeed()
{
	/*
	int aPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
	int64_t	goal, current, period;
	int64_t Ticks;

	if( !QueryPerformanceFrequency( &period ) ) return 0;

	QueryPerformanceCounter(&goal);
	goal.QuadPart+=period.QuadPart/100;
	QueryCounters( &Ticks );
	do
	{
		QueryPerformanceCounter(&current);
	} while(current.QuadPart<goal.QuadPart);
	DeltaCounters( &Ticks );

	SetThreadPriority(GetCurrentThread(),aPriority);
	return( Ticks * 100 );		// Hz
	*/
	return 0;
}

static int64_t gCPUSpeed = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PerfTimer::PerfTimer()
{
	mDuration = 0;
	mStart = 0;
	mRunning = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::CalcDuration()
{
	int64_t anEnd = SDL_GetPerformanceCounter();
	int64_t aFreq = SDL_GetPerformanceFrequency();
	mDuration = ((anEnd-mStart)*1000)/(double)aFreq;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::Start()
{
	mRunning = true;
	mStart = SDL_GetPerformanceCounter();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PerfTimer::Stop()
{
	if(mRunning)
	{
		CalcDuration();
		mRunning = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
double PerfTimer::GetDuration()
{
	if(mRunning)
		CalcDuration();

	return mDuration;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int64_t PerfTimer::GetCPUSpeed()
{
	if(gCPUSpeed<=0)
	{
		gCPUSpeed = CalcCPUSpeed();
		if (gCPUSpeed<=0)
			gCPUSpeed = 1;
	}

	return gCPUSpeed;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int PerfTimer::GetCPUSpeedMHz()
{
	return (int)(gCPUSpeed/1000000);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct PerfInfo
{
	const char *mPerfName;
	mutable int64_t mStartTime;
	mutable int64_t mDuration;
	mutable double mMillisecondDuration;
	mutable double mLongestCall;
	mutable int mStartCount;
	mutable int mCallCount;

	PerfInfo(const char *theName) : mPerfName(theName), mStartTime(0), mDuration(0), mLongestCall(0), mStartCount(0), mCallCount(0) { }

	bool operator<(const PerfInfo &theInfo) const { return strcasecmp(mPerfName,theInfo.mPerfName)<0; }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef std::set<PerfInfo> PerfInfoSet;
static PerfInfoSet gPerfInfoSet;
static bool gPerfOn = false;
static int64_t gStartTime;
static int64_t gCollateTime;
double gDuration = 0;
int gStartCount = 0;
int gPerfRecordTop = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct PerfRecord
{
	const char *mName;
	int64_t mTime;
	bool mStart;

	PerfRecord() { }
	PerfRecord(const char *theName, bool start) : mName(theName), mStart(start) { QueryCounters(&mTime); }
};
typedef std::vector<PerfRecord> PerfRecordVector;
PerfRecordVector gPerfRecordVector;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void InsertPerfRecord(PerfRecord &theRecord)
{
	if(theRecord.mStart)
	{
		PerfInfoSet::iterator anItr = gPerfInfoSet.insert(PerfInfo(theRecord.mName)).first;
		anItr->mCallCount++;

		if ( ++anItr->mStartCount == 1)
			anItr->mStartTime = theRecord.mTime;
	}
	else
	{
		PerfInfoSet::iterator anItr = gPerfInfoSet.find(theRecord.mName);
		if(anItr != gPerfInfoSet.end())
		{
			if( --anItr->mStartCount == 0)
			{
				int64_t aDuration = theRecord.mTime - anItr->mStartTime;
				anItr->mDuration += aDuration;

				if (aDuration > anItr->mLongestCall)
					anItr->mLongestCall = (double)aDuration;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void CollatePerfRecords()
{
	int64_t aTime1,aTime2;
	QueryCounters(&aTime1);

	for(int i=0; i<gPerfRecordTop; i++)
		InsertPerfRecord(gPerfRecordVector[i]);

	gPerfRecordTop = 0;
	QueryCounters(&aTime2);

	gCollateTime += aTime2-aTime1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void PushPerfRecord(PerfRecord theRecord)
{
	if(gPerfRecordTop >= (int)gPerfRecordVector.size())
		gPerfRecordVector.push_back(theRecord);
	else
		gPerfRecordVector[gPerfRecordTop] = theRecord;

	++gPerfRecordTop;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool SexyPerf::IsPerfOn()
{
	return gPerfOn;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::BeginPerf(bool measurePerfOverhead)
{
	gPerfInfoSet.clear();
	gPerfRecordTop = 0;
	gStartCount = 0;
	gCollateTime = 0;

	if(!measurePerfOverhead)
		gPerfOn = true;
	
	QueryCounters(&gStartTime);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::EndPerf()
{
	int64_t anEndTime;
	QueryCounters(&anEndTime);

	CollatePerfRecords();

	gPerfOn = false;

	int64_t aFreq = PerfTimer::GetCPUSpeed();

	gDuration = ((double)(anEndTime - gStartTime - gCollateTime))*1000/aFreq;

	for (PerfInfoSet::iterator anItr = gPerfInfoSet.begin(); anItr != gPerfInfoSet.end(); ++anItr)
	{
		const PerfInfo &anInfo = *anItr;
		anInfo.mMillisecondDuration = (double)anInfo.mDuration*1000/aFreq;
		anInfo.mLongestCall = anInfo.mLongestCall*1000/aFreq;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::StartTiming(const char *theName)
{
	if(gPerfOn)
	{
		++gStartCount;
		PushPerfRecord(PerfRecord(theName,true));
	}
}

	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SexyPerf::StopTiming(const char *theName)
{
	if(gPerfOn)
	{
		PushPerfRecord(PerfRecord(theName,false));
		if(--gStartCount==0)
			CollatePerfRecords();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
std::string SexyPerf::GetResults()
{
	std::string aResult;
	char aBuf[512];

	sprintf(aBuf,"Total Time: %.2f\n",gDuration);
	aResult += aBuf;
	for (PerfInfoSet::iterator anItr = gPerfInfoSet.begin(); anItr != gPerfInfoSet.end(); ++anItr)
	{
		const PerfInfo &anInfo = *anItr;
		sprintf(aBuf,"%s (%d calls, %%%.2f time): %.2f (%.2f avg, %.2f longest)\n",anInfo.mPerfName,anInfo.mCallCount,anInfo.mMillisecondDuration/gDuration*100,anInfo.mMillisecondDuration,anInfo.mMillisecondDuration/anInfo.mCallCount,anInfo.mLongestCall);
		aResult += aBuf;
	}


	return aResult;
}

