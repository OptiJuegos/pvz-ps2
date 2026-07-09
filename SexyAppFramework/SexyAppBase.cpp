//#define SEXY_TRACING_ENABLED
//#define SEXY_PERF_ENABLED
//#define SEXY_MEMTRACE

#include <string>
#include <fstream>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <SDL.h>

#ifdef __SWITCH__
#include <switch.h>
#include <locale>
#include <codecvt>
#elifdef __3DS__
#include <3ds.h>
#endif

#include "SexyAppBase.h"
//#include "misc/SEHCatcher.h"
#include "widget/WidgetManager.h"
#include "widget/Widget.h"
#include "misc/Debug.h"
#include "misc/KeyCodes.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/MemoryImage.h"
//#include "misc/HTTPTransfer.h"
#include "widget/Dialog.h"
#include "imagelib/ImageLib.h"
#ifdef PS2_PLATFORM
#include "sound/DummySoundManager.h"
#include "platform/ps2/sound/Ps2SoundManager.h"
#include "platform/ps2/Ps2IoLock.h"
#include "platform/ps2/Ps2PvzServices.h"
#include "platform/ps2/Ps2Trace.h"
#include <malloc.h>
#include <new>
#else
#include "sound/SDLSoundManager.h"
#include "sound/SDLSoundInstance.h"
#endif
#include "misc/Rect.h"
#include "misc/PropertiesParser.h"
#include "misc/PerfTimer.h"
#include "misc/MTRand.h"
#include "misc/ModVal.h"
//#include "graphics/SysFont.h"
#include "misc/ResourceManager.h"
#ifndef PS2_PLATFORM
#include "sound/SDLMusicInterface.h"
#endif
#include "misc/AutoCrit.h"
#include "misc/Debug.h"
#include "paklib/PakInterface.h"
#ifdef PS2_PLATFORM
#include "../Sexy.TodLib/TodDebug.h"

extern SexyString (*gGetCurrentLevelName)();

static bool Ps2ShouldLogAppUpdate()
{
	static int sAppUpdateLogs = 0;
	if (sAppUpdateLogs >= 8 || gGetCurrentLevelName == NULL || gGetCurrentLevelName() != __S("Game Selector"))
		return false;

	sAppUpdateLogs++;
	return true;
}
#endif
#include "sound/DummyMusicInterface.h"
#include "fcaseopen/fcaseopen.h"

#include "misc/memmgr.h"
#include "misc/RegEmu.h"

using namespace Sexy;

const int DEMO_FILE_ID = 0x42BEEF78;
const int DEMO_VERSION = 2;

SexyAppBase* Sexy::gSexyAppBase = NULL;

//SEHCatcher Sexy::gSEHCatcher;

//HMODULE gDDrawDLL = NULL;
//HMODULE gDSoundDLL = NULL;
//HMODULE gVersionDLL = NULL;

//typedef struct { UINT cbSize; DWORD dwTime; } LASTINPUTINFO;
//typedef BOOL (WINAPI*GetLastInputInfoFunc)(LASTINPUTINFO *plii);
//GetLastInputInfoFunc gGetLastInputInfoFunc = NULL;
static bool gScreenSaverActive = false;

#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif


//HotSpot: 11 4
//Size: 32 32
unsigned char gFingerCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3, 
	0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 
	0xc0, 0x07, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xfc, 0x40, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff, 
	0xfc, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xc0, 
	0x03, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 
	0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1b, 0x60, 0x00, 0x00, 0x1b, 0x68, 0x00, 
	0x00, 0x1b, 0x6c, 0x00, 0x01, 0x9f, 0xec, 0x00, 0x01, 0xdf, 0xfc, 0x00, 0x00, 0xdf, 0xfc, 
	0x00, 0x00, 0x5f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 
	0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 
	0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00
};

//HotSpot: 15 10
//Size: 32 32
unsigned char gDraggingCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xe0, 
	0x01, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 
	0xe0, 0x00, 0xff, 0xfe, 0x60, 0x00, 0xff, 0xfc, 0x20, 0x00, 0xff, 0xfc, 0x00, 0x00, 0xff, 
	0xfe, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x80, 0x00, 
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xf0, 
	0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
	0x80, 0x00, 0x00, 0x01, 0xb0, 0x00, 0x00, 0x0d, 0xb0, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 
	0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 
	0x01, 0x8d, 0xb6, 0x00, 0x01, 0xcf, 0xfe, 0x00, 0x00, 0xef, 0xfe, 0x00, 0x00, 0xff, 0xfe, 
	0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x1f, 
	0xfc, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00, 
	0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00
};
static GLImage* gFPSImage = NULL;

//////////////////////////////////////////////////////////////////////////

/*
typedef HRESULT (WINAPI *SHGetFolderPathFunc)(HWND, int, HANDLE, DWORD, LPTSTR);
void* GetSHGetFolderPath(const char* theDLL, HMODULE* theMod)
{
	HMODULE aMod = LoadLibrary(theDLL);
	SHGetFolderPathFunc aFunc = NULL;

	if (aMod != NULL)
	{
		*((void**)&aFunc) = (void*)GetProcAddress(aMod, "SHGetFolderPathA");
		if (aFunc == NULL)
		{
			FreeLibrary(aMod);
			aMod = NULL;
		}
	}	

	*theMod = aMod;
	return (void *)aFunc;
}
*/

//////////////////////////////////////////////////////////////////////////

SexyAppBase::SexyAppBase()
{
	gSexyAppBase = this;

	SDL_Init(SDL_INIT_TIMER);

	//gVersionDLL = LoadLibraryA("version.dll");
	//gDDrawDLL = LoadLibraryA("ddraw.dll");
	//gDSoundDLL = LoadLibraryA("dsound.dll");
	//gGetLastInputInfoFunc = (GetLastInputInfoFunc) GetProcAddress(GetModuleHandleA("user32.dll"),"GetLastInputInfo");

	//ImageLib::InitJPEG2000();

	//mMutex = NULL;
	mNotifyGameMessage = 0;

#ifdef _DEBUG
	mOnlyAllowOneCopyToRun = false;
#else
	mOnlyAllowOneCopyToRun = true;
#endif

	// Extract product version
	//char aPath[_MAX_PATH];
	//GetModuleFileNameA(NULL, aPath, 256);
	//mProductVersion = GetProductVersion(aPath);	
	//mChangeDirTo = GetFileDir(aPath);

#ifdef __SWITCH__
	mChangeDirTo = "sdmc:/switch/PlantsvsZombies/";
#elifdef __3DS__
	mChangeDirTo = "sdmc:/3ds/PlantsvsZombies/";
#else
	mChangeDirTo = "./";
#endif

	mNoDefer = false;	
	mFullScreenPageFlip = true; // should we page flip in fullscreen?
	mTimeLoaded = SDL_GetTicks();
	mSEHOccured = false;
	mProdName = "Product";
	mTitle = __S("SexyApp");
	mShutdown = false;
	mExitToTop = false;
	mWidth = 640;
	mHeight = 480;
	mFullscreenBits = 16;
	mIsWindowed = true;
	mIsPhysWindowed = true;
	mFullScreenWindow = false;
	mPreferredX = -1;
	mPreferredY = -1;
	mIsScreenSaver = false;
	mAllowMonitorPowersave = true;
	//mHWnd = NULL;
	mGLInterface = NULL;	
	mMusicInterface = NULL;
	//mInvisHWnd = NULL;
	mFrameTime = 10;
	mNonDrawCount = 0;
	mDrawCount = 0;
	mSleepCount = 0;
	mUpdateCount = 0;	
	mUpdateAppState = 0;
	mUpdateAppDepth = 0;
	mPendingUpdatesAcc = 0.0;
	mUpdateFTimeAcc = 0.0;
	mHasPendingDraw = true;
	mIsDrawing = false;
	mLastDrawWasEmpty = false;	
	mLastTimeCheck = 0;
	mUpdateMultiplier = 1;
	mPaused = false;
	mFastForwardToUpdateNum = 0;
	mFastForwardToMarker = false;
	mFastForwardStep = false;
	mSoundManager = NULL;
	mCursorNum = CURSOR_POINTER;		
	mMouseIn = false;
	mRunning = false;
	mActive = true;
	mProcessInTimer = false;
	mMinimized = false;	
	mPhysMinimized = false;
	mIsDisabled = false;
	mLoaded = false;	
	mYieldMainThread = false; 
	mLoadingFailed = false;	
	mLoadingThreadStarted = false;
	mAutoStartLoadingThread = true;
	mLoadingThreadCompleted = false;
	mCursorThreadRunning = false;
	mNumLoadingThreadTasks = 0;
	mCompletedLoadingThreadTasks = 0;	
	mLastDrawTick = SDL_GetTicks();
	mNextDrawTick = SDL_GetTicks();
	mSysCursor = true;	
	mForceFullscreen = false;
	mForceWindowed = false;
	mHasFocus = true;			
	mCustomCursorsEnabled = false;	
	mCustomCursorDirty = false;
	//mOverrideCursor = NULL;
	mIsOpeningURL = false;		
	mInitialized = false;	
	mLastShutdownWasGraceful = true;	
	mReadFromRegistry = false;	
	mCmdLineParsed = false;
	mSkipSignatureChecks = false;	
	mCtrlDown = false;
	mAltDown = false;
	mAllowAltEnter = true;
	mStepMode = 0;
	mCleanupSharedImages = false;
	mStandardWordWrap = true;
	mbAllowExtendedChars = true;
	mEnableMaximizeButton = false;
	mWriteToSexyCache = true;
	mSexyCacheBuffers = false;

	mMusicVolume = 0.85;
	mSfxVolume = 0.85;
	mDemoMusicVolume = mDemoSfxVolume = 0.0;
	mMuteCount = 0;	
	mAutoMuteCount = 0;
	mDemoMute = false;
	mMuteOnLostFocus = false;
	mCurHandleNum = 0;
	mFPSTime = 0;
	mFPSStartTick = SDL_GetTicks();
	mFPSFlipCount = 0;
	mFPSCount = 0;
	mFPSDirtyCount = 0;
	mShowFPS = false;
	mShowFPSMode = FPS_ShowFPS;
	mDrawTime = 0;
	mScreenBltTime = 0;
	mAlphaDisabled = false;	
	mDebugKeysEnabled = false;
	mOldWndProc = 0;
	mNoSoundNeeded = false;
	mWantFMod = false;

	mSyncRefreshRate = 100;
	mVSyncUpdates = false;
	mVSyncBroken = false;
	mVSyncBrokenCount = 0;
	mVSyncBrokenTestStartTick = 0;
	mVSyncBrokenTestUpdates = 0;
	mWaitForVSync = false;
	mSoftVSyncWait = true;
	mUserChanged3DSetting = false;
	mAutoEnable3D = false;
	mTest3D = false;
	mMinVidMemory3D = 6;
	mRecommendedVidMemory3D = 14;
	mRelaxUpdateBacklogCount = 0;
	mWidescreenAware = false;
	mEnableWindowAspect = false;
	mWindowAspect.Set(4, 3);
	mIsWideWindow = false;

	mWindow = 0;
	mContext = 0;
	mSurface = 0;

	int i;

	for (i = 0; i < NUM_CURSORS; i++)
		mCursorImages[i] = NULL;	

	for (i = 0; i < 256; i++)
		mAdd8BitMaxTable[i] = i;

	for (i = 256; i < 512; i++)
		mAdd8BitMaxTable[i] = 255;
	
	// Set default strings.  Init could read in overrides from partner.xml
	SetString("DIALOG_BUTTON_OK",		L"OK");
	SetString("DIALOG_BUTTON_CANCEL",	L"CANCEL");

	SetString("UPDATE_CHECK_TITLE",		L"Update Check");
	SetString("UPDATE_CHECK_BODY",		L"Checking if there are any updates available for this product ...");

	SetString("UP_TO_DATE_TITLE",		L"Up to Date");	
	SetString("UP_TO_DATE_BODY",		L"There are no updates available for this product at this time.");
	SetString("NEW_VERSION_TITLE",		L"New Version");
	SetString("NEW_VERSION_BODY",		L"There is an update available for this product.  Would you like to visit the web site to download it?");

	mDemoPrefix = "sexyapp";
	mDemoFileName = mDemoPrefix + ".dmo";
	mPlayingDemoBuffer = false;
	mManualShutdown = false;
	mRecordingDemoBuffer = false;
	mLastDemoMouseX = 0;
	mLastDemoMouseY = 0;
	mLastDemoUpdateCnt = 0;
	mDemoNeedsCommand = true;
	mDemoLoadingComplete = false;
	mDemoLength = 0;
	mDemoCmdNum = 0;
	mDemoCmdOrder = -1; // Means we haven't processed any demo commands yet
	mDemoCmdBitPos = 0;

	mWidgetManager = new WidgetManager(this);
	mResourceManager = new ResourceManager(this);

	mPrimaryThreadId = 0;

	/*
	if (GetSystemMetrics(86)) // check for tablet pc
	{
		mTabletPC = true;
		mFullScreenPageFlip = false; // so that tablet keyboard can show up
	}
	else*/
	mTabletPC = false;

	//gSEHCatcher.mApp = this;	
	
	//std::wifstream stringsFile(_wfopen(L".\\properties\\fstrings", L"rb"));
	//
	//if(!stringsFile)
	//{
	//	MessageBox(NULL, "file missing: 'install-folder\\properties\\fstrings' Please re-install", "FATAL ERROR", MB_OK);
	//	DoExit(1);
	//}
	//std::getline(stringsFile, mString_HardwareAccelSwitchedOn);
	//std::getline(stringsFile, mString_HardwareAccelConfirm);
	//std::getline(stringsFile, mString_HardwareAccelNotWorking);
	//std::getline(stringsFile, mString_SetColorDepth);
	//std::getline(stringsFile, mString_FailedInitDirectDrawColon);
	//std::getline(stringsFile, mString_UnableOpenProperties);
	//std::getline(stringsFile, mString_SigCheckFailed);
	//std::getline(stringsFile, mString_InvalidCommandLineParam);
	//std::getline(stringsFile, mString_RequiresDirectX);
	//std::getline(stringsFile, mString_YouNeedDirectX);
	//std::getline(stringsFile, mString_FailedInitDirectDraw);
	//std::getline(stringsFile, mString_FatalError);
	//std::getline(stringsFile, mString_UnexpectedErrorOccured);
	//std::getline(stringsFile, mString_PleaseHelpBy);
	//std::getline(stringsFile, mString_FailedConnectPopcap);
	//stringsFile.close();
}

SexyAppBase::~SexyAppBase()
{
	Shutdown();

	/*
	// Check if we should write the current 3d setting
	bool showedMsgBox = false;
	if (mUserChanged3DSetting)
	{
		bool writeToRegistry = true;
		bool is3D = false;
		bool is3DOptionSet = RegistryReadBoolean("Is3D", &is3D);
		if(!is3DOptionSet) // should we write the option?
		{
			if(!Is3DAccelerationRecommended()) // may need to prompt user if he wants to keep 3d acceleration on
			{
				if (Is3DAccelerated())
				{
					showedMsgBox = true;
					int aResult = MessageBox(NULL,
									GetString("HARDWARE_ACCEL_SWITCHED_ON", 
															__S("Hardware Acceleration was switched on during this session.\r\n")
															__S("If this resulted in slower performance, it should be switched off.\r\n")
															__S("Would you like to keep Hardware Acceleration switched on?")).c_str(),
									(StringToSexyString(mCompanyName) + __S(" ") +
									 GetString("HARDWARE_ACCEL_CONFIRMATION", __S("Hardware Acceleration Confirmation"))).c_str(),
									MB_YESNO | MB_ICONQUESTION);

					//mDDInterface->mIs3D = aResult == IDYES ? true : false;
					if (aResult!=IDYES)
						writeToRegistry = false;
				}
				else
					writeToRegistry = false;
			}
		}

		if (writeToRegistry)
			RegistryWriteBoolean("Is3D", mDDInterface->mIs3D);
	}

	if (!showedMsgBox && gD3DInterfacePreDrawError && !IsScreenSaver())
	{
		int aResult = MessageBox(NULL, 
						GetString("HARDWARE_ACCEL_NOT_WORKING", 
									__S("Hardware Acceleration may not have been working correctly during this session.\r\n")
									__S("If you noticed graphics problems, you may want to turn off Hardware Acceleration.\r\n")
									__S("Would you like to keep Hardware Acceleration switched on?")).c_str(),
						(StringToSexyString(mCompanyName) + __S(" ") +
						 GetString("HARDWARE_ACCEL_CONFIRMATION", __S("Hardware Acceleration Confirmation"))).c_str(),
						MB_YESNO | MB_ICONQUESTION);

		if (aResult==IDNO)
			RegistryWriteBoolean("Is3D", false);
	}
	*/


	DialogMap::iterator aDialogItr = mDialogMap.begin();
	while (aDialogItr != mDialogMap.end())
	{
		mWidgetManager->RemoveWidget(aDialogItr->second);
		delete aDialogItr->second;
		++aDialogItr;
	}
	mDialogMap.clear();
	mDialogList.clear();
	
	/*
	if (mInvisHWnd != NULL)
	{
		HWND aWindow = mInvisHWnd;
		mInvisHWnd = NULL;
		SetWindowLongPtr(aWindow, GWLP_USERDATA, 0);
		DestroyWindow(aWindow);
	}
*/	
	
	delete mWidgetManager;	
	delete mResourceManager;
	delete gFPSImage;
	gFPSImage = NULL;
	
	SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
	while (aSharedImageItr != mSharedImageMap.end())
	{
		SharedImage* aSharedImage = &aSharedImageItr->second;
		DBG_ASSERTE(aSharedImage->mRefCount == 0);		
		delete aSharedImage->mImage;
		mSharedImageMap.erase(aSharedImageItr++);		
	}
	
	delete mGLInterface;
	delete mMusicInterface;
	delete mSoundManager;			

	/*
	if (mHWnd != NULL)
	{
		HWND aWindow = mHWnd;
		mHWnd = NULL;

		SetWindowLongPtr(aWindow, GWLP_USERDATA, 0);

		//char aStr[256];
		//sprintf(aStr, "HWND: %d\r\n", aWindow);
		//OutputDebugString(aStr);

		DestroyWindow(aWindow);
	}
	*/
	
	WaitForLoadingThread();	

	//DestroyCursor(mHandCursor);
	//DestroyCursor(mDraggingCursor);			

	gSexyAppBase = NULL;

	WriteDemoBuffer();

	//if (mMutex != NULL)
		//::CloseHandle(mMutex);

	/*
	FreeLibrary(gDDrawDLL);
	FreeLibrary(gDSoundDLL);
	FreeLibrary(gVersionDLL);
	*/
}

/*
static BOOL CALLBACK ChangeDisplayWindowEnumProc(HWND hwnd, LPARAM lParam)
{
	typedef std::map<HWND,RECT> WindowMap;
	static WindowMap aMap;

	if (lParam==0 && aMap.find(hwnd)==aMap.end()) // record
	{
		RECT aRect;
		if (!IsIconic(hwnd) && IsWindowVisible(hwnd))
		{
			if (GetWindowRect(hwnd,&aRect))
			{
//				char aBuf[4096];
//				GetWindowText(hwnd,aBuf,4000);
//				DWORD aProcessId = 0;
//				GetWindowThreadProcessId(hwnd,&aProcessId);
//				SEXY_TRACE(StrFormat("%s %d - %d %d %d %d",aBuf,aProcessId,aRect.left,aRect.top,aRect.right,aRect.bottom).c_str());
				aMap[hwnd] = aRect;
			}
		}
	}
	else 
	{
		WindowMap::iterator anItr = aMap.find(hwnd);
		if (anItr != aMap.end())
		{
			RECT &r = anItr->second;
			MoveWindow(hwnd,r.left,r.top,abs(r.right-r.left),abs(r.bottom-r.top),TRUE);
		}
	}
	return TRUE;
}
*/

void SexyAppBase::ClearUpdateBacklog(bool relaxForASecond)
{
	mLastTimeCheck = SDL_GetTicks();
	mUpdateFTimeAcc = 0.0;

	if (relaxForASecond)
		mRelaxUpdateBacklogCount = 1000;
}

bool SexyAppBase::IsScreenSaver()
{
	return mIsScreenSaver;
}

bool SexyAppBase::AppCanRestore()
{
	return !mIsDisabled;
}

bool SexyAppBase::ReadDemoBuffer(std::string &theError)
{
	FILE* aFP = fopen(mDemoFileName.c_str(), "rb");

	if (aFP == NULL)
	{
		theError = "Demo file not found: " + mDemoFileName;
		return false;
	}

	struct AutoFile { FILE *f; AutoFile(FILE *file) : f(file) {} ~AutoFile() { fclose(f); } };
	AutoFile aCloseFile(aFP);

	uint32_t aFileID;
	fread(&aFileID, 4, 1, aFP);

	DBG_ASSERTE(aFileID == DEMO_FILE_ID);
	if (aFileID != DEMO_FILE_ID)
	{
		theError = "Invalid demo file.";
		return false;
	}


	uint32_t aVersion;
	fread(&aVersion, 4, 1, aFP);
	
	fread(&mRandSeed, 4, 1, aFP);
	SRand(mRandSeed);

	ushort aStrLen = 4;
	fread(&aStrLen, 2, 1, aFP);
	if (aStrLen > 255)
		aStrLen = 255;
	char aStr[256];
	fread(aStr, 1, aStrLen, aFP);
	aStr[aStrLen] = '\0';

	DBG_ASSERTE(mProductVersion == aStr);
	if (mProductVersion != aStr)
	{
		theError = "This demo file appears to be for '" + std::string(aStr) + "'";
		return false;
	}

	int aFilePos = ftell(aFP);
	fseek(aFP, 0, SEEK_END);
	int aBytesLeft = ftell(aFP) - aFilePos;
	fseek(aFP, aFilePos, SEEK_SET);

	uchar* aBuffer;
	// read marker list
	if (aVersion >= 2) 
	{
		int aSize;
		fread(&aSize, 4, 1, aFP);
		aBytesLeft -= 4;

		if (aSize >= aBytesLeft)
		{
			theError = "Invalid demo file.";
			return false;
		}

		Buffer aMarkerBuffer;

		aBuffer = new uchar[aSize];
		fread(aBuffer, 1, aSize, aFP);
		aMarkerBuffer.WriteBytes(aBuffer, aSize);
		aMarkerBuffer.SeekFront();

		int aNumItems = aMarkerBuffer.ReadLong();
		int i;
		for (i=0; i<aNumItems && !aMarkerBuffer.AtEnd(); i++)
		{
			mDemoMarkerList.push_back(DemoMarker());
			DemoMarker &aMarker = mDemoMarkerList.back();
			aMarker.first = aMarkerBuffer.ReadString();
			aMarker.second = aMarkerBuffer.ReadLong();
		}

		if (i!=aNumItems)
		{
			theError = "Invalid demo file.";
			return false;
		}

		aBytesLeft -= aSize;

		delete [] aBuffer;			
	}

	// Read demo commands
	fread(&mDemoLength, 4, 1, aFP);
	aBytesLeft -= 4;
	
	if (aBytesLeft <= 0)
	{
		theError = "Invalid demo file.";
		return false;
	}


	aBuffer = new uchar[aBytesLeft];
	fread(aBuffer, 1, aBytesLeft, aFP);		

	mDemoBuffer.WriteBytes(aBuffer, aBytesLeft);
	mDemoBuffer.SeekFront();

	delete [] aBuffer;
	return true;
}

void SexyAppBase::WriteDemoBuffer()
{
	if (mRecordingDemoBuffer)
	{
		FILE* aFP = fopen(mDemoFileName.c_str(), "w+b");

		if (aFP != NULL)
		{
			uint32_t aFileID = DEMO_FILE_ID;
			fwrite(&aFileID, 4, 1, aFP);		

			uint32_t aVersion = DEMO_VERSION;
			fwrite(&aVersion, 4, 1, aFP);
			
			fwrite(&mRandSeed, 4, 1, aFP);

			ushort aStrLen = mProductVersion.length();
			fwrite(&aStrLen, 2, 1, aFP);		
			fwrite(mProductVersion.c_str(), 1, mProductVersion.length(), aFP);

			Buffer aMarkerBuffer;
			aMarkerBuffer.WriteLong(mDemoMarkerList.size());
			for (DemoMarkerList::iterator aMarkerItr = mDemoMarkerList.begin(); aMarkerItr != mDemoMarkerList.end(); ++aMarkerItr)
			{
				aMarkerBuffer.WriteString(aMarkerItr->first);
				aMarkerBuffer.WriteLong(aMarkerItr->second);
			}
			int aMarkerBufferSize = aMarkerBuffer.GetDataLen();
			fwrite(&aMarkerBufferSize, 4, 1, aFP);
			fwrite(aMarkerBuffer.GetDataPtr(), aMarkerBufferSize, 1, aFP);

			uint32_t aDemoLength = mUpdateCount;
			fwrite(&aDemoLength, 4, 1, aFP);

			fwrite(mDemoBuffer.GetDataPtr(), 1, mDemoBuffer.GetDataLen(), aFP);
			fclose(aFP);
		}		
	}
}

void SexyAppBase::DemoSyncBuffer(Buffer* theBuffer)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_SYNC);

		uint32_t aLen = mDemoBuffer.ReadLong();
		
		theBuffer->Clear();
		for (int i = 0; i < (int) aLen; i++)
			theBuffer->WriteByte(mDemoBuffer.ReadByte());		
	}
	else if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_SYNC, 5);		
		mDemoBuffer.WriteLong(theBuffer->GetDataLen());
		mDemoBuffer.WriteBytes((uchar*) theBuffer->GetDataPtr(), theBuffer->GetDataLen());
	}
}

void SexyAppBase::DemoSyncString(std::string* theString)
{
	Buffer aBuffer;
	aBuffer.WriteString(*theString);
	DemoSyncBuffer(&aBuffer);
	*theString = aBuffer.ReadString();
}

void SexyAppBase::DemoSyncInt(int* theInt)
{
	Buffer aBuffer;
	aBuffer.WriteLong(*theInt);
	DemoSyncBuffer(&aBuffer);
	*theInt = aBuffer.ReadLong();
}

void SexyAppBase::DemoSyncBool(bool* theBool)
{
	Buffer aBuffer;
	aBuffer.WriteBoolean(*theBool);
	DemoSyncBuffer(&aBuffer);
	*theBool = aBuffer.ReadBoolean();
}

void SexyAppBase::DemoAssertStringEqual(const std::string& theString)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_ASSERT_STRING_EQUAL);

		std::string aString = mDemoBuffer.ReadString();
		DBG_ASSERTE(aString == theString);
	}
	else if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_ASSERT_STRING_EQUAL, 5);				
		mDemoBuffer.WriteString(theString);
	}
}

void SexyAppBase::DemoAddMarker(const std::string& theString)
{
	if (mPlayingDemoBuffer)
	{
		mFastForwardToMarker = false;
	}
	else if (mRecordingDemoBuffer)
	{
		mDemoMarkerList.push_back(DemoMarker(theString,mUpdateCount));
	}
}

void SexyAppBase::DemoRegisterHandle(HANDLE theHandle)
{
	if ((mRecordingDemoBuffer) || (mPlayingDemoBuffer))
	{
		// Insert the handle into a map with an auto-incrementing number so
		//  we can match up the auto-incrementing numbers with the handle
		//  later on, as handles may not be the same between executions
		std::pair<HandleToIntMap::iterator, bool> aPair = mHandleToIntMap.insert(HandleToIntMap::value_type(theHandle, mCurHandleNum));
		(void)aPair; // unused in Release mode
		DBG_ASSERT(aPair.second);
		mCurHandleNum++;
	}
}

/*
void SexyAppBase::DemoWaitForHandle(HANDLE theHandle)
{
	WaitForSingleObject(theHandle, INFINITE);
	
	if ((mRecordingDemoBuffer) || (mPlayingDemoBuffer))
	{
		// Remove the handle from our waiting map
		HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);					
		DBG_ASSERT(anItr != mHandleToIntMap.end());
		mHandleToIntMap.erase(anItr);
	}
}

bool SexyAppBase::DemoCheckHandle(HANDLE theHandle)
{
	if (mPlayingDemoBuffer)
	{	
		// We only need to try to get the result if we think we are waiting for one	
		if (gSexyAppBase->PrepareDemoCommand(false))
		{
			if ((!gSexyAppBase->mDemoIsShortCmd) && (gSexyAppBase->mDemoCmdNum == DEMO_HANDLE_COMPLETE))
			{
				// Find auto-incrementing handle num from handle
				HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);					
				DBG_ASSERT(anItr != mHandleToIntMap.end());

				int anOldBufferPos = gSexyAppBase->mDemoBuffer.mReadBitPos;

				// Since we don't require a demo result entry to be here, we must verify
				//  that this is referring to us
				int aDemoHandleNum = gSexyAppBase->mDemoBuffer.ReadLong();
				
				if (aDemoHandleNum == anItr->second)
				{
					// Alright, this was the handle we were waiting for!
					gSexyAppBase->mDemoNeedsCommand = true;

					// Actually wait for our local buddy to complete
					WaitForSingleObject(theHandle, INFINITE);
					mHandleToIntMap.erase(anItr);

					return true;
				}
				else
				{
					// Not us, go back
					gSexyAppBase->mDemoBuffer.mReadBitPos = anOldBufferPos;
				}
			}
		}

		return false;	
	}
	else 
	{
		if (WaitForSingleObject(theHandle, 0) == WAIT_OBJECT_0)
		{
			if (mRecordingDemoBuffer)
			{
				// Find auto-incrementing handle num from handle
				HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);					
				DBG_ASSERT(anItr != mHandleToIntMap.end());

				gSexyAppBase->WriteDemoTimingBlock();
				gSexyAppBase->mDemoBuffer.WriteNumBits(0, 1);
				gSexyAppBase->mDemoBuffer.WriteNumBits(DEMO_HANDLE_COMPLETE, 5);
				gSexyAppBase->mDemoBuffer.WriteLong(anItr->second);

				mHandleToIntMap.erase(anItr);
			}

			return true;
		}

		return false;
	}
}
*/

void SexyAppBase::DemoAssertIntEqual(int theInt)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_ASSERT_INT_EQUAL);

		int anInt = mDemoBuffer.ReadLong();
		(void)anInt; // unused in Release mode
		DBG_ASSERTE(anInt == theInt);
	}
	else if (mRecordingDemoBuffer)
	{		
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_ASSERT_INT_EQUAL, 5);				
		mDemoBuffer.WriteLong(theInt);
	}
}

Dialog* SexyAppBase::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{	
	Dialog* aDialog = new Dialog(NULL, NULL, theDialogId, isModal, theDialogHeader,	theDialogLines, theDialogFooter, theButtonMode);		
	return aDialog;
}

Dialog* SexyAppBase::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	KillDialog(theDialogId);

	Dialog* aDialog = NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);		

	AddDialog(theDialogId, aDialog);

	return aDialog;
}


Dialog*	SexyAppBase::GetDialog(int theDialogId)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())	
		return anItr->second;

	return NULL;
}

bool SexyAppBase::KillDialog(int theDialogId, bool removeWidget, bool deleteWidget)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
	{
		Dialog* aDialog = anItr->second;

		// set the result to something else so DoMainLoop knows that the dialog is gone 
		// in case nobody else sets mResult		
		if (aDialog->mResult == -1) 
			aDialog->mResult = 0;
		
		DialogList::iterator aListItr = std::find(mDialogList.begin(),mDialogList.end(),aDialog);
		if (aListItr != mDialogList.end())
			mDialogList.erase(aListItr);
		
		mDialogMap.erase(anItr);

		if (removeWidget || deleteWidget)
		mWidgetManager->RemoveWidget(aDialog);

		if (aDialog->IsModal())
		{			
			ModalClose();
			mWidgetManager->RemoveBaseModal(aDialog);
		}				

		if (deleteWidget)
		SafeDeleteWidget(aDialog);
		
		return true;
	}

	return false;
}

bool SexyAppBase::KillDialog(int theDialogId)
{
	return KillDialog(theDialogId,true,true);
}

bool SexyAppBase::KillDialog(Dialog* theDialog)
{
	return KillDialog(theDialog->mId);
}

int SexyAppBase::GetDialogCount()
{
	return mDialogMap.size();
}

void SexyAppBase::AddDialog(int theDialogId, Dialog* theDialog)
{
	KillDialog(theDialogId);

	if (theDialog->mWidth == 0)
	{
		// Set the dialog position ourselves
		int aWidth = mWidth/2;
		theDialog->Resize((mWidth - aWidth)/2, mHeight / 5, aWidth, theDialog->GetPreferredHeight(aWidth));
	}

	mDialogMap.insert(DialogMap::value_type(theDialogId, theDialog));
	mDialogList.push_back(theDialog);

	mWidgetManager->AddWidget(theDialog);
	if (theDialog->IsModal())
	{
		mWidgetManager->AddBaseModal(theDialog);
		ModalOpen();
	}
}

void SexyAppBase::AddDialog(Dialog* theDialog)
{
	AddDialog(theDialog->mId, theDialog);
}

void SexyAppBase::ModalOpen()
{
}

void SexyAppBase::ModalClose()
{
}

void SexyAppBase::DialogButtonPress(int theDialogId, int theButtonId)
{	
	if (theButtonId == Dialog::ID_YES)
		ButtonPress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonPress(3000 + theDialogId);	
}

void SexyAppBase::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonDepress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonDepress(3000 + theDialogId);
}

void SexyAppBase::GotFocus()
{	
}

void SexyAppBase::LostFocus()
{	
}

void SexyAppBase::URLOpenFailed(const std::string& theURL)
{
	(void)theURL;
	mIsOpeningURL = false;
}

void SexyAppBase::URLOpenSucceeded(const std::string& theURL)
{	
	(void)theURL;
	mIsOpeningURL = false;

	if (mShutdownOnURLOpen)
		Shutdown();	
}

bool SexyAppBase::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	if ((!mIsOpeningURL) || (theURL != mOpeningURL))
	{
		mShutdownOnURLOpen = shutdownOnOpen;
		mIsOpeningURL = true;
		mOpeningURL = theURL;
		mOpeningURLTime = SDL_GetTicks();

        /*
		if ((intptr_t) ShellExecuteA(NULL, "open", theURL.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32)
		{
			return true;
		}
		else
		{
			URLOpenFailed(theURL);
			return false;
		}
        */
	}

	return true;
}

std::string SexyAppBase::GetProductVersion(const std::string& thePath)
{
	return "0";
	/*
	// Dynamically Load Version.dll
	typedef DWORD (APIENTRY *GetFileVersionInfoSizeFunc)(LPSTR lptstrFilename, LPDWORD lpdwHandle);
	typedef BOOL (APIENTRY *GetFileVersionInfoFunc)(LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
	typedef BOOL (APIENTRY *VerQueryValueFunc)(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen);

	static GetFileVersionInfoSizeFunc aGetFileVersionInfoSizeFunc = NULL;
	static GetFileVersionInfoFunc aGetFileVersionInfoFunc = NULL;
	static VerQueryValueFunc aVerQueryValueFunc = NULL;

	if (aGetFileVersionInfoSizeFunc==NULL)
	{
		aGetFileVersionInfoSizeFunc = (GetFileVersionInfoSizeFunc)GetProcAddress(gVersionDLL,"GetFileVersionInfoSizeA");
		aGetFileVersionInfoFunc = (GetFileVersionInfoFunc)GetProcAddress(gVersionDLL,"GetFileVersionInfoA");
		aVerQueryValueFunc = (VerQueryValueFunc)GetProcAddress(gVersionDLL,"VerQueryValueA");
	}

	// Get Product Version
	std::string aProductVersion;
	
	uint aSize = aGetFileVersionInfoSizeFunc((char*) thePath.c_str(), 0);
	if (aSize > 0)		
	{
		uchar* aVersionBuffer = new uchar[aSize];
		aGetFileVersionInfoFunc((char*) thePath.c_str(), 0, aSize, aVersionBuffer);	
		char* aBuffer;
		if (aVerQueryValueFunc(aVersionBuffer, 
				  (char*)"\\StringFileInfo\\040904B0\\ProductVersion",
				  (void**) &aBuffer, 
				  &aSize))
		{
			aProductVersion = aBuffer;
		}
		else if (aVerQueryValueFunc(aVersionBuffer, 
				  (char*)"\\StringFileInfo\\040904E4\\ProductVersion", 
				  (void**) &aBuffer, 
				  &aSize))
		{
			aProductVersion = aBuffer;
		}

		delete[] aVersionBuffer;	
	}

	return aProductVersion;
	*/
}

void SexyAppBase::WaitForLoadingThread()
{
	int ms = 20;

	timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	while ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		nanosleep(&ts, &ts);
}

void SexyAppBase::SetCursorImage(int theCursorNum, Image* theImage)
{
	if ((theCursorNum >= 0) && (theCursorNum < NUM_CURSORS))
	{
		mCursorImages[theCursorNum] = theImage;
		EnforceCursor();
	}
}

void SexyAppBase::TakeScreenshot()
{
	if (mGLInterface==NULL)
		return;

	/*
	// Get free image name
	std::string anImageDir = GetAppDataFolder() + "_screenshots";
	MkDir(anImageDir);
	anImageDir += "/";

	WIN32_FIND_DATAA aData;
	int aMaxId = 0;
	std::string anImagePrefix = "image";
	HANDLE aHandle = FindFirstFileA((anImageDir + "*.png").c_str(), &aData);
	if (aHandle!=INVALID_HANDLE_VALUE)
	{
		do {
			int aNum = 0;
			if (sscanf(aData.cFileName,(anImagePrefix + "%d.png").c_str(), &aNum)==1)
			{
				if (aNum>aMaxId)
					aMaxId = aNum;
			}

		} 
		while(FindNextFileA(aHandle,&aData));
		FindClose(aHandle);
	}
	std::string anImageName = anImageDir + anImagePrefix + StrFormat("%d.png",aMaxId+1);

	// Capture screen
	LPDIRECTDRAWSURFACE aSurface = mDDInterface->mDrawSurface;
	
	// Temporarily set the mDrawSurface to NULL so DDImage::Check3D 
	// returns false so we can lock the surface.
	mDDInterface->mDrawSurface = NULL; 
	
	DDImage anImage(mDDInterface);
	anImage.SetSurface(aSurface);
	anImage.GetBits();
	anImage.DeleteDDSurface();
	mDDInterface->mDrawSurface = aSurface; 

	if (anImage.mBits==NULL)
		return;
		
	// Write image
	ImageLib::Image aSaveImage;
	aSaveImage.mBits = anImage.mBits;
	aSaveImage.mWidth = anImage.mWidth;
	aSaveImage.mHeight = anImage.mHeight;
	ImageLib::WritePNGImage(anImageName, &aSaveImage);
	aSaveImage.mBits = NULL;
	*/
		

/*
	keybd_event(VK_MENU,0,0,0);
    keybd_event(VK_SNAPSHOT,0,0,0);
    keybd_event(VK_MENU,0,KEYEVENTF_KEYUP,0);
	if (OpenClipboard(mHWnd))
	{
		HBITMAP aBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
		if (aBitmap!=NULL)
		{
			BITMAP anObject;
			ZeroMemory(&anObject,sizeof(anObject));
			GetObject(aBitmap,sizeof(anObject),&anObject);

			BITMAPINFO anInfo;
			ZeroMemory(&anInfo,sizeof(anInfo));
			BITMAPINFOHEADER &aHeader = anInfo.bmiHeader;
			aHeader.biBitCount = 32;
			aHeader.biPlanes = 1;
			aHeader.biHeight = -abs(anObject.bmHeight);
			aHeader.biWidth = abs(anObject.bmWidth);
			aHeader.biSize = sizeof(aHeader);
			aHeader.biSizeImage = aHeader.biHeight*aHeader.biWidth*4;
			ImageLib::Image aSaveImage;
			aSaveImage.mBits = new DWORD[abs(anObject.bmWidth*anObject.bmHeight)];
			aSaveImage.mWidth = abs(anObject.bmWidth);
			aSaveImage.mHeight = abs(anObject.bmHeight);

			HDC aDC = GetDC(NULL);
			if (GetDIBits(aDC,aBitmap,0,aSaveImage.mHeight,aSaveImage.mBits,&anInfo,DIB_RGB_COLORS))
				ImageLib::WritePNGImage(anImageName, &aSaveImage);

			ReleaseDC(NULL,aDC);
		}
		CloseClipboard();
	}*/

	ClearUpdateBacklog();
}

void SexyAppBase::DumpProgramInfo()
{
	/*
	Deltree(GetAppDataFolder() + "_dump");

	for (;;)
	{
		if (mkdir((GetAppDataFolder() + "_dump").c_str()))
			break;
		Sleep(100);
	}

	std::fstream aDumpStream((GetAppDataFolder() + "_dump/imagelist.html").c_str(), std::ios::out);

	time_t aTime;
	time(&aTime);
	tm* aTM = localtime(&aTime);

	aDumpStream << "<HTML><BODY BGCOLOR=EEEEFF><CENTER><FONT SIZE=+2><B>" << asctime(aTM) << "</B></FONT><BR>" << std::endl;

	int anImgNum = 0;

	int aThumbWidth = 64;
	int aThumbHeight = 64;

	ImageLib::Image anImageLibImage;
	anImageLibImage.mWidth = aThumbWidth;
	anImageLibImage.mHeight = aThumbHeight;
	anImageLibImage.mBits = new unsigned long[aThumbWidth*aThumbHeight];	

	typedef std::multimap<int, MemoryImage*, std::greater<int> > SortedImageMap;

	int aTotalMemory = 0;

	SortedImageMap aSortedImageMap;
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;				

		int aNumPixels = aMemoryImage->mWidth*aMemoryImage->mHeight;

		DDImage* aDDImage = dynamic_cast<DDImage*>(aMemoryImage);

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;

		int aMemorySize = 0;
		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256*4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256*4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;
		if (aMemoryImage->mD3DData != NULL)
			aTextureMemory += ((TextureData*)aMemoryImage->mD3DData)->mTexMemSize;

		aMemorySize = aBitsMemory + aSurfaceMemory + aPalletizedMemory + aNativeAlphaMemory + aRLAlphaMemory + aRLAdditiveMemory + aTextureMemory;
		aTotalMemory += aMemorySize;

		aSortedImageMap.insert(SortedImageMap::value_type(aMemorySize, aMemoryImage));

		++anItr;
	}

	aDumpStream << "Total Image Allocation: " << CommaSeperate(aTotalMemory).c_str() << " bytes<BR>";
	aDumpStream << "<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=4>";

	int aTotalMemorySize = 0;
	int aTotalBitsMemory = 0;
	int aTotalSurfaceMemory = 0;
	int aTotalPalletizedMemory = 0;
	int aTotalNativeAlphaMemory = 0;
	int aTotalRLAlphaMemory = 0;
	int aTotalRLAdditiveMemory = 0;
	int aTotalTextureMemory = 0;

	SortedImageMap::iterator aSortedItr = aSortedImageMap.begin();
	while (aSortedItr != aSortedImageMap.end())
	{
		MemoryImage* aMemoryImage = aSortedItr->second;				

		char anImageName[256];
		sprintf(anImageName, "img%04d.png", anImgNum);

		char aThumbName[256];
		sprintf(aThumbName, "thumb%04d.jpg", anImgNum);
		
		aDumpStream << "<TR>" << std::endl;

		aDumpStream << "<TD><A HREF=" << anImageName << "><IMG SRC=" << aThumbName << " WIDTH=" << aThumbWidth << " HEIGHT=" << aThumbHeight << "></A></TD>" << std::endl;
		
		int aNumPixels = aMemoryImage->mWidth*aMemoryImage->mHeight;

		DDImage* aDDImage = dynamic_cast<DDImage*>(aMemoryImage);

		int aMemorySize = aSortedItr->first;

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;
		std::string aTextureFormatName;
		
		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256*4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256*4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;		
		if (aMemoryImage->mD3DData != NULL)
		{
			aTextureMemory += ((TextureData*)aMemoryImage->mD3DData)->mTexMemSize;

			switch (((TextureData*)aMemoryImage->mD3DData)->mPixelFormat)
			{				
			case PixelFormat_A8R8G8B8: aTextureFormatName = "A8R8G8B8"; break;
			case PixelFormat_A4R4G4B4: aTextureFormatName = "A4R4G4B4"; break;
			case PixelFormat_R5G6B5: aTextureFormatName = "R5G6B5"; break;
			case PixelFormat_Palette8: aTextureFormatName = "Palette8"; break;
			case PixelFormat_Unknown: break;
			}			
		}

		aTotalMemorySize		+= aMemorySize;
		aTotalBitsMemory		+= aBitsMemory;
		aTotalTextureMemory		+= aTextureMemory;
		aTotalSurfaceMemory		+= aSurfaceMemory;
		aTotalPalletizedMemory	+= aPalletizedMemory;
		aTotalNativeAlphaMemory	+= aNativeAlphaMemory;
		aTotalRLAlphaMemory		+= aRLAlphaMemory;
		aTotalRLAdditiveMemory	+= aRLAdditiveMemory;

				

		char aStr[256];
		sprintf(aStr, "%d x %d<BR>%s bytes", aMemoryImage->mWidth, aMemoryImage->mHeight, CommaSeperate(aMemorySize).c_str());
		aDumpStream << "<TD ALIGN=RIGHT>" << aStr << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aBitsMemory != 0) ? __S("mBits<BR>") + CommaSeperate(aBitsMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;	
		aDumpStream << "<TD>" << SexyStringToString(((aPalletizedMemory != 0) ? __S("Palletized<BR>") + CommaSeperate(aPalletizedMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;				
		aDumpStream << "<TD>" << SexyStringToString(((aSurfaceMemory != 0) ? __S("DDSurface<BR>") + CommaSeperate(aSurfaceMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;		
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mD3DData!=NULL) ? __S("Texture<BR>") + StringToSexyString(aTextureFormatName) + __S("<BR>") + CommaSeperate(aTextureMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mIsVolatile) ? __S("Volatile") : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mForcedMode) ? __S("Forced") : __S("&nbsp;"))) << "</TD>" << std::endl;		
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasAlpha) ? __S("HasAlpha") : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasTrans) ? __S("HasTrans") : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aNativeAlphaMemory != 0) ? __S("NativeAlpha<BR>") + CommaSeperate(aNativeAlphaMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAlphaMemory != 0) ? __S("RLAlpha<BR>") + CommaSeperate(aRLAlphaMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAdditiveMemory != 0) ? __S("RLAdditive<BR>") + CommaSeperate(aRLAdditiveMemory) : __S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << (aMemoryImage->mFilePath.empty()? "&nbsp;":aMemoryImage->mFilePath) << "</TD>" << std::endl;

		aDumpStream << "</TR>" << std::endl;
		
		// Write thumb

		MemoryImage aCopiedImage(*aMemoryImage);

		uint32_t* aBits = aCopiedImage.GetBits();

		uint32_t* aThumbBitsPtr = anImageLibImage.mBits;

		for (int aThumbY = 0; aThumbY < aThumbHeight; aThumbY++)
			for (int aThumbX = 0; aThumbX < aThumbWidth; aThumbX++)
			{
				int aSrcX = (int) (aCopiedImage.mWidth  * (aThumbX + 0.5)) / aThumbWidth;
				int aSrcY = (int) (aCopiedImage.mHeight * (aThumbY + 0.5)) / aThumbHeight;
				
				*(aThumbBitsPtr++) = aBits[aSrcX + (aSrcY*aCopiedImage.mWidth)];
			}

		ImageLib::WriteJPEGImage((GetAppDataFolder() + std::string("_dump/") + aThumbName).c_str(), &anImageLibImage);

		// Write high resolution image

		ImageLib::Image anFullImage;
		anFullImage.mBits = aCopiedImage.GetBits();
		anFullImage.mWidth = aCopiedImage.GetWidth();
		anFullImage.mHeight = aCopiedImage.GetHeight();

		ImageLib::WritePNGImage((GetAppDataFolder() + std::string("_dump/") + anImageName).c_str(), &anFullImage);

		anFullImage.mBits = NULL;

		anImgNum++;

		aSortedItr++;		
	}

	aDumpStream << "<TD>Totals</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalMemorySize)) << "</TD>" << std::endl;	
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalBitsMemory)) << "</TD>" << std::endl;	
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalPalletizedMemory)) << "</TD>" << std::endl;				
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalSurfaceMemory)) << "</TD>" << std::endl;		
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalTextureMemory)) << "</TD>" << std::endl;		
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalNativeAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAdditiveMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;

	aDumpStream << "</TABLE></CENTER></BODY></HTML>" << std::endl;
	*/
}

double SexyAppBase::GetLoadingThreadProgress()
{
	if (mLoaded)
		return 1.0;
	if (!mLoadingThreadStarted)
		return 0.0;
	if (mNumLoadingThreadTasks == 0)
		return 0.0;
	return std::min(mCompletedLoadingThreadTasks / (double) mNumLoadingThreadTasks, 1.0);
}

bool SexyAppBase::RegistryWrite(const std::string& theValueName, uint32_t theType, const uchar* theValue, uint32_t theLength)
{
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_WRITE);

		return mDemoBuffer.ReadNumBits(1, false) != 0;		
	}

	//HKEY aGameKey;
	
	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
	std::string aValueName;

	int aSlashPos = (int) theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	/*
	int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_WRITE, &aGameKey);
	if (aResult != ERROR_SUCCESS)
	{
		uint32_t aDisp;
		aResult = RegCreateKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, (char *)"Key", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL, &aGameKey, &aDisp);
	}

	if (aResult != ERROR_SUCCESS)
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_WRITE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}
		
	RegSetValueExA(aGameKey, aValueName.c_str(), 0, theType, theValue, theLength);
	RegCloseKey(aGameKey);
	*/

	if (!regemu::RegistryWrite(aKeyName, aValueName, theType, theValue, theLength))
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_WRITE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_REGISTRY_WRITE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	return true;
}

bool SexyAppBase::RegistryWriteString(const std::string& theValueName, const std::string& theString)
{
	return RegistryWrite(theValueName, regemu::REGEMU_SZ, (uchar*) theString.c_str(), theString.length());
}

bool SexyAppBase::RegistryWriteInteger(const std::string& theValueName, int theValue)
{
	return RegistryWrite(theValueName, regemu::REGEMU_DWORD, (uchar*) &theValue, sizeof(int));
}

bool SexyAppBase::RegistryWriteBoolean(const std::string& theValueName, bool theValue)
{
	int aValue = theValue ? 1 : 0;
	return RegistryWrite(theValueName, regemu::REGEMU_DWORD, (uchar*) &aValue, sizeof(int));
}

bool SexyAppBase::RegistryWriteData(const std::string& theValueName, const uchar* theValue, uint32_t theLength)
{
	return RegistryWrite(theValueName, regemu::REGEMU_BINARY, (uchar*) theValue, theLength);
}

void SexyAppBase::WriteToRegistry()
{	
	RegistryWriteInteger("MusicVolume", (int) (mMusicVolume * 100));
	RegistryWriteInteger("SfxVolume", (int) (mSfxVolume * 100));
	RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount > 0) ? 1 : 0);
	RegistryWriteInteger("ScreenMode", mIsWindowed ? 0 : 1);
	RegistryWriteInteger("PreferredX", mPreferredX);
	RegistryWriteInteger("PreferredY", mPreferredY);
	RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);		
	RegistryWriteInteger("InProgress", 0);
	RegistryWriteBoolean("WaitForVSync", mWaitForVSync);	
}

bool SexyAppBase::RegistryEraseKey(const SexyString& _theKeyName)
{
	std::string theKeyName = SexyStringToStringFast(_theKeyName);
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_ERASE);

		return mDemoBuffer.ReadNumBits(1, false) != 0;		
	}	
	
	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey) + "\\" + theKeyName;

	/*
	int aResult = RegDeleteKeyA(HKEY_CURRENT_USER, aKeyName.c_str());
	if (aResult != ERROR_SUCCESS)
	*/
	if (!regemu::RegistryEraseKey(aKeyName))
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_ERASE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_REGISTRY_ERASE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	return true;
}

void SexyAppBase::RegistryEraseValue(const SexyString& _theValueName)
{
	std::string theValueName = SexyStringToStringFast(_theValueName);
	if (mRegKey.length() == 0)
		return;

	//HKEY aGameKey;
	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
	std::string aValueName;

	int aSlashPos = (int) theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	/*
	int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_WRITE, &aGameKey);
	if (aResult == ERROR_SUCCESS)
	{		
		RegDeleteValueA(aGameKey, aValueName.c_str());
		RegCloseKey(aGameKey);
	}
	*/

	regemu::RegistryEraseValue(aKeyName, aValueName);
}

/*
bool SexyAppBase::RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys)
{
	theSubKeys->clear();

	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_GETSUBKEYS);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		int aNumKeys = mDemoBuffer.ReadLong();

		for (int i = 0; i < aNumKeys; i++)
			theSubKeys->push_back(mDemoBuffer.ReadString());

		return true;
	}
	else
	{
		HKEY aKey;
		
		std::string aKeyName = RemoveTrailingSlash(RemoveTrailingSlash("SOFTWARE\\" + mRegKey) + "\\" + theKeyName);	
		int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_READ, &aKey);
		
		if (aResult == ERROR_SUCCESS)
		{		
			for (int anIdx = 0; ; anIdx++)
			{
				char aStr[1024];

				aResult = RegEnumKeyA(aKey, anIdx, aStr, 1024);
				if (aResult != ERROR_SUCCESS)
					break;

				theSubKeys->push_back(aStr);
			}

			RegCloseKey(aKey);

			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_REGISTRY_GETSUBKEYS, 5);
				mDemoBuffer.WriteNumBits(1, 1); // success
				mDemoBuffer.WriteLong(theSubKeys->size());

				for (int i = 0; i < (int) theSubKeys->size(); i++)
					mDemoBuffer.WriteString((*theSubKeys)[i]);				
			}

			return true;
		}
		else
		{
			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_REGISTRY_GETSUBKEYS, 5);
				mDemoBuffer.WriteNumBits(0, 1); // failure
			}

			return false;	
		}
	}
}
*/

bool SexyAppBase::RegistryRead(const std::string& theValueName, uint32_t* theType, uchar* theValue, uint32_t* theLength)
{
	return RegistryReadKey(theValueName, theType, theValue, theLength);
}

bool SexyAppBase::RegistryReadKey(const std::string& theValueName, uint32_t* theType, uchar* theValue, uint32_t* theLength)
{
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return false;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
				
		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_READ);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		*theType = mDemoBuffer.ReadLong();

		uint32_t aLen = mDemoBuffer.ReadLong();
		*theLength = aLen;
		
		if (*theLength >= aLen)
		{			
			mDemoBuffer.ReadBytes(theValue, aLen);
			return true;
		}
		else
		{
			for (int i = 0; i < (int) aLen; i++)
				mDemoBuffer.ReadByte();
			return false;
		}		
	}
	else
	{		
		//HKEY aGameKey;

		std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
		std::string aValueName;

		int aSlashPos = (int) theValueName.rfind('\\');
		if (aSlashPos != -1)
		{
			aKeyName += "\\" + theValueName.substr(0, aSlashPos);
			aValueName = theValueName.substr(aSlashPos + 1);
		}
		else
		{
			aValueName = theValueName;
		}		

		/*
		if (RegOpenKeyExA(theKey, aKeyName.c_str(), 0, KEY_READ, &aGameKey) == ERROR_SUCCESS)
		{
			if (RegQueryValueExA(aGameKey, aValueName.c_str(), 0, theType, (uchar*) theValue, theLength) == ERROR_SUCCESS)
			{
				if (mRecordingDemoBuffer)
				{
					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_REGISTRY_READ, 5);
					mDemoBuffer.WriteNumBits(1, 1); // success
					mDemoBuffer.WriteLong(*theType);
					mDemoBuffer.WriteLong(*theLength);
					mDemoBuffer.WriteBytes(theValue, *theLength);
				}

				RegCloseKey(aGameKey);
				return true;	
			}

			RegCloseKey(aGameKey);
		}
		*/

		if (regemu::RegistryRead(aKeyName, aValueName, (uint32_t*)theType, theValue, (uint32_t*)theLength))
		{
			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_REGISTRY_READ, 5);
				mDemoBuffer.WriteNumBits(1, 1); // success
				mDemoBuffer.WriteLong(*theType);
				mDemoBuffer.WriteLong(*theLength);
				mDemoBuffer.WriteBytes(theValue, *theLength);
			}

			return true;
		}

		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_READ, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}
		
		return false;
	}
}

// aStr isn't initialised lmao
bool SexyAppBase::RegistryReadString(const std::string& theKey, std::string* theString)
{
	char aStr[1024];
	
	uint32_t aType;	
	uint32_t aLen = sizeof(aStr) - 1;
	if (!RegistryRead(theKey, &aType, (uchar*) aStr, &aLen))
		return false;

	if (aType != regemu::REGEMU_SZ)
		return false;

	aStr[aLen] = 0;

	*theString = aStr;
	return true;
}

bool SexyAppBase::RegistryReadInteger(const std::string& theKey, int* theValue)
{
	uint32_t aType;
	uint32_t aLong;
	uint32_t aLen = 4;
	if (!RegistryRead(theKey, &aType, (uchar*) &aLong, &aLen))
		return false;

	if (aType != regemu::REGEMU_DWORD)
		return false;

	*theValue = aLong;
	return true;
}

bool SexyAppBase::RegistryReadBoolean(const std::string& theKey, bool* theValue)
{
	int aValue;
	if (!RegistryReadInteger(theKey, &aValue))
		return false;
	
	*theValue = aValue != 0;
	return true;
}

bool SexyAppBase::RegistryReadData(const std::string& theKey, uchar* theValue, uint32_t* theLength)
{		
	uint32_t aType;
	if (!RegistryRead(theKey, &aType, (uchar*) theValue, theLength))
		return false;

	if (aType != regemu::REGEMU_BINARY)
		return false;
	
	return true;
}

void SexyAppBase::ReadFromRegistry()
{
	mReadFromRegistry = true;
	mRegKey = SexyStringToString(GetString("RegistryKey", StringToSexyString(mRegKey)));

	if (mRegKey.length() == 0)
		return;

#ifdef PS2_PLATFORM
	// Persist the emulated registry next to the save data (mass:/mc0:/cwd).
	regemu::SetRegFile(GetAppDataFolder() + "registry.regemu");
#else
	regemu::SetRegFile("registry.regemu");
#endif

	int anInt;
	if (RegistryReadInteger("MusicVolume", &anInt))
		mMusicVolume = anInt / 100.0;
	
	if (RegistryReadInteger("SfxVolume", &anInt))
		mSfxVolume = anInt / 100.0;

	if (RegistryReadInteger("Muted", &anInt))
		mMuteCount = anInt;

	if (RegistryReadInteger("ScreenMode", &anInt))
		mIsWindowed = anInt == 0;

	RegistryReadInteger("PreferredX", &mPreferredX);
	RegistryReadInteger("PreferredY", &mPreferredY);	

	if (RegistryReadInteger("CustomCursors", &anInt))
		EnableCustomCursors(anInt != 0);	
			
	RegistryReadBoolean("WaitForVSync", &mWaitForVSync);	

	if (RegistryReadInteger("InProgress", &anInt))
		mLastShutdownWasGraceful = anInt == 0;
		
	if (!IsScreenSaver())
		RegistryWriteInteger("InProgress", 1);	
}

bool SexyAppBase::WriteBytesToFile(const std::string& theFileName, const void *theData, unsigned long theDataLen)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_WRITE);		

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		return true;
	}	

	MkDir(GetFileDir(theFileName));
	FILE* aFP = fcaseopen(theFileName.c_str(), "w+b");

	if (aFP == NULL)
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_WRITE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure				
		}

		return false;
	}

	fwrite(theData, 1, theDataLen, aFP);
	fclose(aFP);

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_FILE_WRITE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	if (mSexyCacheBuffers && mWriteToSexyCache)
	{
		GetFullPath(theFileName);
		/*
		unsigned char* aSetData = (unsigned char*)gSexyCache->AllocSetData(GetFullPath(theFileName), "Buffer", theDataLen + 1);
		if (aSetData)
		{
			*aSetData = 1;
			memcpy(aSetData, theData, theDataLen);
			gSexyCache->SetData(aSetData);
			gSexyCache->FreeSetData(aSetData);
			gSexyCache->SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
		}
		*/
	}

	return true;
}

bool SexyAppBase::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer)
{
	return WriteBytesToFile(theFileName,theBuffer->GetDataPtr(),theBuffer->GetDataLen());
}


bool SexyAppBase::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo)
{
	if ((mPlayingDemoBuffer) && (!dontWriteToDemo))
	{
		if (mManualShutdown)
			return false;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_READ);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		uint32_t aLen = mDemoBuffer.ReadLong();		
				
		theBuffer->Clear();
		for (int i = 0; i < (int) aLen; i++)
			theBuffer->WriteByte(mDemoBuffer.ReadByte());

		return true;		
	}
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");

		if (aFP == NULL)
		{
			if ((mRecordingDemoBuffer) && (!dontWriteToDemo))
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_FILE_READ, 5);
				mDemoBuffer.WriteNumBits(0, 1); // failure				
			}

			return false;
		}
		
		p_fseek(aFP, 0, SEEK_END);
		int aFileSize = p_ftell(aFP);
		p_fseek(aFP, 0, SEEK_SET);
		
		uchar* aData = new uchar[aFileSize];

		p_fread(aData, 1, aFileSize, aFP);
		p_fclose(aFP);

		theBuffer->Clear();
		theBuffer->SetData(aData, aFileSize);

		if ((mRecordingDemoBuffer) && (!dontWriteToDemo))
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_READ, 5);
			mDemoBuffer.WriteNumBits(1, 1); // success			
			mDemoBuffer.WriteLong(aFileSize);
			mDemoBuffer.WriteBytes(aData, aFileSize);
		}

		delete [] aData;

		return true;
	}
}

bool SexyAppBase::FileExists(const std::string& theFileName)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);		
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_EXISTS);		

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;		
		return success;
	}
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");

		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_EXISTS, 5);
			mDemoBuffer.WriteNumBits((aFP != NULL) ? 1 : 0, 1); 
		}

		if (aFP == NULL)		
			return false;		
		
		p_fclose(aFP);
		return true;
	}
}

bool SexyAppBase::EraseFile(const std::string& theFileName)
{
	if (mPlayingDemoBuffer)
		return true;

	return remove(theFileName.c_str()) == 0;
}

void SexyAppBase::SEHOccured()
{
	SetMusicVolume(0);
	//::ShowWindow(mHWnd, SW_HIDE);
	mSEHOccured = true;
	EnforceCursor();
}

std::string SexyAppBase::GetGameSEHInfo()
{
	int aSecLoaded = (SDL_GetTicks() - mTimeLoaded) / 1000;

	char aTimeStr[16];
	sprintf(aTimeStr, "%02d:%02d:%02d", (aSecLoaded/60/60), (aSecLoaded/60)%60, aSecLoaded%60);
	
	char aThreadIdStr[16];
	sprintf(aThreadIdStr, "%lX", mPrimaryThreadId);

	std::string anInfoString = 
		"Product: " + mProdName + "\r\n" +		
		"Version: " + mProductVersion + "\r\n";			

	anInfoString +=
		"Time Loaded: " + std::string(aTimeStr) + "\r\n"
		"Fullscreen: " + (mIsWindowed ? std::string("No") : std::string("Yes")) + "\r\n"
		"Primary ThreadId: " + aThreadIdStr + "\r\n";	

	return anInfoString;						
}


void SexyAppBase::GetSEHWebParams(DefinesMap*){}

void SexyAppBase::ShutdownHook()
{
}

void SexyAppBase::Shutdown()
{
	if ((mPrimaryThreadId != 0) && ((void*)pthread_self() != mPrimaryThreadId))
	{
		mLoadingFailed = true;
	}
	else if (!mShutdown)
	{
		mExitToTop = true;
		mShutdown = true;
		ShutdownHook();

		if (mPlayingDemoBuffer)
		{
			//if the music/sfx volume is 0, then it means that in playback
			//someone pressed the "S" key to mute sounds (or that the 
			//sound volume was set to 0 in the first place). Out of politeness,
			//return the system sound volume to what it last was in the game.
			SetMusicVolume(mDemoMusicVolume);
			SetSfxVolume(mDemoSfxVolume);
		}

		// Blah
		/*
		while (mCursorThreadRunning)
		{
			Sleep(10);
		}
		*/
		
		if (mMusicInterface != NULL)
			mMusicInterface->StopAllMusic();		
		
		/*
		if ((!mIsPhysWindowed) && (mGLInterface != NULL) && (mDDInterface->mDD != NULL))
		{
			mDDInterface->mDD->RestoreDisplayMode();
		}

		if (mHWnd != NULL) 
		{
			ShowWindow(mHWnd, SW_HIDE);			
		}
		*/

		RestoreScreenResolution();

		if (mReadFromRegistry)
			WriteToRegistry();

		//ImageLib::CloseJPEG2000();
	}
}

void SexyAppBase::RestoreScreenResolution()
{
    /*
	if (mFullScreenWindow)
	{
		EnumWindows(ChangeDisplayWindowEnumProc,0); // get any windows that appeared while we were running
		ChangeDisplaySettings(NULL,0);
		EnumWindows(ChangeDisplayWindowEnumProc,1); // restore window pos
		mFullScreenWindow = false;
	}
    */
}
	
void SexyAppBase::DoExit(int theCode)
{
	RestoreScreenResolution();
	exit(theCode);
}

void SexyAppBase::UpdateFrames()
{
#ifdef PS2_PLATFORM
	bool aLogUpdateFrame = Ps2ShouldLogAppUpdate();
	if (aLogUpdateFrame) TodHesitationTrace("APPUPDATE begin");
#endif
	mUpdateCount++;	

	if (!mMinimized)
	{		
		if (mWidgetManager->UpdateFrame())
			++mFPSDirtyCount;
	}

#ifdef PS2_PLATFORM
	if (aLogUpdateFrame) TodHesitationTrace("APPUPDATE music");
#endif
	mMusicInterface->Update();	
#ifdef PS2_PLATFORM
	if (aLogUpdateFrame) TodHesitationTrace("APPUPDATE clean");
#endif
	CleanSharedImages();
#ifdef PS2_PLATFORM
	if (aLogUpdateFrame) TodHesitationTrace("APPUPDATE end");
#endif
}

void SexyAppBase::DoUpdateFramesF(float theFrac)
{
	if ((mVSyncUpdates) && (!mMinimized))
		mWidgetManager->UpdateFrameF(theFrac);	
}

bool SexyAppBase::DoUpdateFrames()
{
	SEXY_AUTO_PERF("SexyAppBase::DoUpdateFrames");

	if (gScreenSaverActive)
		return false;

	if (mPlayingDemoBuffer)
	{
		if ((mLoadingThreadCompleted) && (!mLoaded) && (mDemoLoadingComplete))
		{			
			mLoaded = true;
			//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
			mYieldMainThread = false;
			LoadingThreadCompleted();
		}

		// Hrrm not sure why we check (mUpdateCount != mLastDemoUpdateCnt) here
		if ((mLoaded == mDemoLoadingComplete) && (mUpdateCount != mLastDemoUpdateCnt))		
		{
			UpdateFrames();		
			return true;
		}

		return false;
	}
	else
	{
		if ((mLoadingThreadCompleted) && (!mLoaded))
		{
			//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
			mLoaded = true;
			mYieldMainThread = false;
			LoadingThreadCompleted();

			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_LOADING_COMPLETE, 5);	
			}
		}
				
		UpdateFrames();		
		return true;
	}
}

bool gIsFailing = false;

void SexyAppBase::Redraw(Rect* theClipRect)
{
	SEXY_AUTO_PERF("SexyAppBase::Redraw");

	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if ((mIsDrawing) || (mShutdown))
		return;

	if (gScreenSaverActive)
		return;

	static DWORD aRetryTick = 0;
	mGLInterface->Redraw(theClipRect);
	/*
	if (!mGLInterface->Redraw(theClipRect))
	{
		//gD3DInterfacePreDrawError = false; // this predraw error happens naturally when ddraw is failing
		if (!gIsFailing)
		{
			//gDebugStream << GetTickCount() << " Redraw failed!" << std::endl;
			gIsFailing = true;
		}

		WINDOWPLACEMENT aWindowPlacement;
		ZeroMemory(&aWindowPlacement, sizeof(aWindowPlacement));
		aWindowPlacement.length = sizeof(aWindowPlacement);
		::GetWindowPlacement(mHWnd, &aWindowPlacement);

		DWORD aTick = SDL_GetTicks();
		//if ((mActive || (aTick-aRetryTick>1000 && mIsPhysWindowed)) && (aWindowPlacement.showCmd != SW_SHOWMINIMIZED) && (!mMinimized))
		if ((mActive || (aTick-aRetryTick>1000 && mIsPhysWindowed)) && (!mMinimized))
		{
			aRetryTick = aTick;

			mWidgetManager->mImage = NULL;

			// Re-check resolution at this point, because we hit here when you change your resolution.
		
			if (((mWidth >= GetSystemMetrics(SM_CXFULLSCREEN)) || (mHeight >= GetSystemMetrics(SM_CYFULLSCREEN))) && (mIsWindowed))
			{
				if (mForceWindowed)
				{
					Popup(GetString("PLEASE_SET_COLOR_DEPTH", __S("Please set your desktop color depth to 16 bit."))); 
					Shutdown();
					return;
				}
				mForceFullscreen = true;

				SwitchScreenMode(false);
				return;
			}


			int aResult = InitGLInterface();

			//gDebugStream << GetTickCount() << " ReInit..." << std::endl;

			if ((mIsWindowed) && (aResult == DDInterface::RESULT_INVALID_COLORDEPTH))
			{
				//gDebugStream << GetTickCount() << "ReInit Invalid Colordepth" << std::endl;
				if (!mActive) // don't switch to full screen if not active app
					return;

				SwitchScreenMode(false);
				mForceFullscreen = true;
				return;
			}
			else if (aResult == DDInterface::RESULT_3D_FAIL)
			{
				Set3DAcclerated(false);
				return;
			}
			else if (aResult != DDInterface::RESULT_OK)
			{
				//gDebugStream << GetTickCount() << " ReInit Failed" << std::endl;
				//Fail("Failed to initialize DirectDraw");
				//Sleep(1000);				
				
				return;
			}
			if (!aResult) return;

			ReInitImages();

			mWidgetManager->mImage = mGLInterface->GetScreenImage();
			mWidgetManager->MarkAllDirty();

			mLastTime = SDL_GetTicks();
		}
	}
	else
	{
		if (gIsFailing)
		{
			//gDebugStream << GetTickCount() << " Redraw succeeded" << std::endl;
			gIsFailing = false;
			aRetryTick = 0;
		}
	}
	*/

	mFPSFlipCount++;
}

///////////////////////////// FPS Stuff
static PerfTimer gFPSTimer;
static int gFrameCount;
static int gFPSDisplay;
static bool gForceDisplay = false;
static void CalculateFPS()
{
	gFrameCount++;

	/*
	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	if (gFPSImage==NULL)
	{
		gFPSImage = new GLImage(gSexyAppBase->mGLInterface);
		gFPSImage->Create(50,aFont.GetHeight()+4);
		gFPSImage->SetImageMode(false,false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		//gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	if (gFPSTimer.GetDuration() >= 1000 || gForceDisplay)
	{
		gFPSTimer.Stop();
		if (!gForceDisplay)
			gFPSDisplay = (int)(gFrameCount*1000/gFPSTimer.GetDuration() + 0.5f);
		else
		{
			gForceDisplay = false;
			gFPSDisplay = 0;
		}

		gFPSTimer.Start();
		gFrameCount = 0;

		Graphics aDrawG(gFPSImage);
		aDrawG.SetFont(&aFont);
		SexyString aFPS = StrFormat(__S("FPS: %d"), gFPSDisplay);
		aDrawG.SetColor(0x000000);
		aDrawG.FillRect(0,0,gFPSImage->GetWidth(),gFPSImage->GetHeight());
		aDrawG.SetColor(0xFFFFFF);
		aDrawG.DrawString(aFPS,2,aFont.GetAscent());
		//gFPSImage->mKeepBits = false;
		//gFPSImage->GenerateDDSurface();
		gFPSImage->mBitsChangedCount++;
	}
	*/
}

///////////////////////////// FPS Stuff to draw mouse coords
static void FPSDrawCoords(int theX, int theY)
{
	/*
	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	if (gFPSImage==NULL)
	{
		gFPSImage = new GLImage(gSexyAppBase->mGLInterface);
		gFPSImage->Create(50,aFont.GetHeight()+4);
		gFPSImage->SetImageMode(false,false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		//gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	Graphics aDrawG(gFPSImage);
	aDrawG.SetFont(&aFont);
	SexyString aFPS = StrFormat(__S("%d,%d"),theX,theY);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0,0,gFPSImage->GetWidth(),gFPSImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS,2,aFont.GetAscent());	
	gFPSImage->mBitsChangedCount++;
	*/
}

///////////////////////////// Demo TimeLeft Stuff
static GLImage* gDemoTimeLeftImage = NULL;
static void CalculateDemoTimeLeft()
{
	/*
	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	static DWORD aLastTick = 0;

	if (gDemoTimeLeftImage==NULL)
	{
		gDemoTimeLeftImage = new GLImage(gSexyAppBase->mGLInterface);
		gDemoTimeLeftImage->Create(50,aFont.GetHeight()+4);
		gDemoTimeLeftImage->SetImageMode(false,false);
		gDemoTimeLeftImage->SetVolatile(true);
		gDemoTimeLeftImage->mPurgeBits = false;
		//gDemoTimeLeftImage->mWantDDSurface = true;
		gDemoTimeLeftImage->PurgeBits();
	}

	DWORD aTick = GetTickCount();
	if (aTick - aLastTick < 1000/gSexyAppBase->mUpdateMultiplier)
		return;

	aLastTick = aTick;

	int aNumUpdatesLeft = gSexyAppBase->mDemoLength - gSexyAppBase->mUpdateCount;
	Graphics aDrawG(gDemoTimeLeftImage);
	aDrawG.SetFont(&aFont);

	int aTotalSeconds = aNumUpdatesLeft*gSexyAppBase->mFrameTime/1000;
	int aSeconds = aTotalSeconds%60;
	int aMinutes = (aTotalSeconds/60)%60;
	int anHours = (aTotalSeconds/3600);

	SexyString aFPS = StrFormat(__S("%02d:%02d:%02d"), anHours,aMinutes,aSeconds);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0,0,gDemoTimeLeftImage->GetWidth(),gDemoTimeLeftImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS,2,aFont.GetAscent());	
	gDemoTimeLeftImage->mBitsChangedCount++;
	*/
}

static void UpdateScreenSaverInfo(DWORD theTick)
{
	/*
	if (gSexyAppBase->IsScreenSaver() || !gSexyAppBase->mIsPhysWindowed)
		return;

	// Get screen saver timeout		
	static DWORD aPeriodicTick = 0;
	static DWORD aScreenSaverTimeout = 60000;
	static BOOL aScreenSaverEnabled = TRUE;

	if (theTick-aPeriodicTick > 10000)
	{
		aPeriodicTick = theTick;

		int aTimeout = 0;

		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT,0,&aTimeout,0);
		SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,&aScreenSaverEnabled,0);
		aTimeout-=2;

		if (aTimeout < 1)
			aTimeout = 1;

		aScreenSaverTimeout = aTimeout*1000;

		if (!aScreenSaverEnabled)
			gScreenSaverActive = false;
	}

	// Get more accurate last user input time
	if (gGetLastInputInfoFunc)
	{
		LASTINPUTINFO anInfo;
		anInfo.cbSize = sizeof(anInfo);
		if (gGetLastInputInfoFunc(&anInfo))
		{
			if (anInfo.dwTime > theTick)
				anInfo.dwTime = theTick;

			gSexyAppBase->mLastUserInputTick = anInfo.dwTime;
		}
	}

	if (!aScreenSaverEnabled)
		return;

	DWORD anIdleTime = theTick - gSexyAppBase->mLastUserInputTick;
	if (gScreenSaverActive)
	{
		BOOL aBool = FALSE;
		if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &aBool, 0))
		{
			if (aBool) // screen saver not off yet
				return;
		}

		if (anIdleTime < aScreenSaverTimeout)
		{
			gScreenSaverActive = false;
			gSexyAppBase->mWidgetManager->MarkAllDirty();
		}
	}
	else if (anIdleTime > aScreenSaverTimeout)
		gScreenSaverActive = true;
	*/
}

bool SexyAppBase::DrawDirtyStuff()
{
	SEXY_AUTO_PERF("SexyAppBase::DrawDirtyStuff");
	MTAutoDisallowRand aDisallowRand;
#ifdef PS2_PLATFORM
	static int sPs2FirstDrawStage = 0;
#endif

	if (gIsFailing) // just try to reinit
	{
		Redraw(NULL);
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;		
		return false;
	}	

	if (mShowFPS)
	{
		switch(mShowFPSMode)
		{
			case FPS_ShowFPS: CalculateFPS(); break;
			case FPS_ShowCoords:
				if (mWidgetManager!=NULL)
					FPSDrawCoords(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY); 
				break;
		}

		if (mPlayingDemoBuffer)
			CalculateDemoTimeLeft();
	}

	DWORD aStartTime = SDL_GetTicks();

	// Update user input and screen saver info
	static DWORD aPeriodicTick = 0;
	if (aStartTime-aPeriodicTick > 1000)
	{
		aPeriodicTick = aStartTime;
		UpdateScreenSaverInfo(aStartTime);
	}

	if (gScreenSaverActive)
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;		
		return false;
	}

	mIsDrawing = true;
#ifdef PS2_PLATFORM
	if (sPs2FirstDrawStage == 0)
	{
		sPs2FirstDrawStage = 1;
		Ps2Trace(PS2_TRACE_FRAME_DRAW);
	}
#endif
	bool drewScreen = mWidgetManager->DrawScreen();
	mIsDrawing = false;
#ifdef PS2_PLATFORM
	if (sPs2FirstDrawStage == 1)
	{
		sPs2FirstDrawStage = 2;
		Ps2Trace(PS2_TRACE_FRAME_DRAWDONE);
	}
#endif

	if ((drewScreen || (aStartTime - mLastDrawTick >= 1000) || (mCustomCursorDirty)) &&
		((int) (aStartTime - mNextDrawTick) >= 0))
	{
		mLastDrawWasEmpty = false;

		mDrawCount++;		

		DWORD aMidTime = SDL_GetTicks();

		mFPSCount++;
		mFPSTime += aMidTime - aStartTime;

		mDrawTime += aMidTime - aStartTime;

		if (mShowFPS)
		{
			Graphics g(mGLInterface->GetScreenImage());
			g.DrawImage(gFPSImage,mWidth-gFPSImage->GetWidth()-10,mHeight-gFPSImage->GetHeight()-10);
		
			if (mPlayingDemoBuffer)
				g.DrawImage(gDemoTimeLeftImage,mWidth-gDemoTimeLeftImage->GetWidth()-10,mHeight-gFPSImage->GetHeight()-gDemoTimeLeftImage->GetHeight()-15);
		}

		/*
		if (mWaitForVSync && mIsPhysWindowed && mSoftVSyncWait)
		{
			DWORD aTick = SDL_GetTicks();
			if (aTick-mLastDrawTick < mGLInterface->mMillisecondsPerFrame)
				Sleep(mDDInterface->mMillisecondsPerFrame - (aTick-mLastDrawTick));
		}
		*/

		DWORD aPreScreenBltTime = SDL_GetTicks();
		mLastDrawTick = aPreScreenBltTime;

#ifdef PS2_PLATFORM
		if (sPs2FirstDrawStage == 2)
			Ps2Trace(PS2_TRACE_FRAME_REDRAW);
#endif
		Redraw(NULL);
#ifdef PS2_PLATFORM
		if (sPs2FirstDrawStage == 2)
		{
			sPs2FirstDrawStage = 3;
			Ps2Trace(PS2_TRACE_FRAME_GS);
		}
#endif

		// This is our one UpdateFTimeAcc if we are vsynched
		UpdateFTimeAcc(); 

		DWORD aEndTime = SDL_GetTicks();

		mScreenBltTime = aEndTime - aPreScreenBltTime;

#ifdef _DEBUG
		/*if (mFPSTime >= 5000) // Show FPS about every 5 seconds
		{
			uint32_t aTickNow = GetTickCount();

			OutputDebugString(StrFormat(__S("Theoretical FPS: %d\r\n"), (int) (mFPSCount * 1000 / mFPSTime)).c_str());
			OutputDebugString(StrFormat(__S("Actual      FPS: %d\r\n"), (mFPSFlipCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());
			OutputDebugString(StrFormat(__S("Dirty Rate     : %d\r\n"), (mFPSDirtyCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());

			mFPSTime = 0;
			mFPSCount = 0;
			mFPSFlipCount = 0;
			mFPSStartTick = aTickNow;
			mFPSDirtyCount = 0;
		}*/
#endif

		if ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		{
			int aTotalTime = aEndTime - aStartTime;

			mNextDrawTick += 35 + std::max(aTotalTime, 15);

			if ((int) (aEndTime - mNextDrawTick) >= 0)			
				mNextDrawTick = aEndTime;			

			/*char aStr[256];
			sprintf(aStr, "Next Draw Time: %d\r\n", mNextDrawTick);
			OutputDebugString(aStr);*/
		}
		else
			mNextDrawTick = aEndTime;

		mHasPendingDraw = false;		
		mCustomCursorDirty = false;

		return true;
	}
	else
	{		
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;		
		return false;
	}
}

void SexyAppBase::LogScreenSaverError(const std::string &theError)
{
	/*
	static bool firstTime = true;
	char aBuf[512];

	const char *aFlag = firstTime?"w":"a+";
	firstTime = false;

	FILE *aFile = fopen("ScrError.txt",aFlag);
	if (aFile != NULL)
	{
		fprintf(aFile,"%s %s %lu\n",theError.c_str(),_strtime(aBuf),GetTickCount());
		fclose(aFile);
	}
	*/
}

void SexyAppBase::BeginPopup()
{
	/*
	if (!mIsPhysWindowed)
	{
		if (mDDInterface && mDDInterface->mDD)
		{
			mDDInterface->mDD->FlipToGDISurface();
			mNoDefer = true;
		}
	}
	*/
}

void SexyAppBase::EndPopup()
{
	if (!mIsPhysWindowed)
		mNoDefer = false;

	ClearUpdateBacklog();
	ClearKeysDown();

	if (mWidgetManager->mDownButtons)
	{
		mWidgetManager->DoMouseUps();
		//ReleaseCapture();
	}
}

int SexyAppBase::MsgBox(const std::string& theText, const std::string& theTitle, int theFlags)
{
//	if (mDDInterface && mDDInterface->mDD)
//		mDDInterface->mDD->FlipToGDISurface();
	/*
	if (IsScreenSaver())
	{
		LogScreenSaverError(theText);
		return IDOK;
	}
	*/

	BeginPopup();
	//int aResult = MessageBoxA(mHWnd, theText.c_str(), theTitle.c_str(), theFlags);
	printf("%s\n===\n%s\n", theTitle.c_str(), theText.c_str());

#ifdef __SWITCH__
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, theTitle.c_str(), theText.c_str());
	errorApplicationShow(&c);
#endif

	EndPopup();

	return 0;
}

int SexyAppBase::MsgBox(const std::wstring& theText, const std::wstring& theTitle, int theFlags)
{
//	if (mDDInterface && mDDInterface->mDD)
//		mDDInterface->mDD->FlipToGDISurface();
	/*
	if (IsScreenSaver())
	{
		LogScreenSaverError(WStringToString(theText));
		return IDOK;
	}
	*/

	BeginPopup();
	//int aResult = MessageBoxW(mHWnd, theText.c_str(), theTitle.c_str(), theFlags);
	wprintf(L"%s\n===\n%s\n", theTitle.c_str(), theText.c_str());

#ifdef __SWITCH__
	std::wstring_convert<std::codecvt_utf8<wchar_t> > cv;
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, cv.to_bytes(theTitle).c_str(), cv.to_bytes(theText).c_str());
	errorApplicationShow(&c);
#endif

	EndPopup();

	return 0;
}

void SexyAppBase::Popup(const std::string& theString)
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(theString);
		return;
	}

	BeginPopup();
	if (!mShutdown)
		printf("FATAL ERROR\n===\n%s\n", theString.c_str());

#ifdef __SWITCH__
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, "Fatal error", theString.c_str());
	errorApplicationShow(&c);
#endif

	EndPopup();
}

void SexyAppBase::Popup(const std::wstring& theString)
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(WStringToString(theString));
		return;
	}

	BeginPopup();
	if (!mShutdown)
		wprintf(L"FATAL ERROR\n===\n%s\n", theString.c_str());

#ifdef __SWITCH__
	std::wstring_convert<std::codecvt_utf8<wchar_t> > cv;
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, "Fatal error", cv.to_bytes(theString).c_str());
	errorApplicationShow(&c);
#endif

	EndPopup();
}

void SexyAppBase::SafeDeleteWidget(Widget* theWidget)
{
	WidgetSafeDeleteInfo aWidgetSafeDeleteInfo;
	aWidgetSafeDeleteInfo.mUpdateAppDepth = mUpdateAppDepth;
	aWidgetSafeDeleteInfo.mWidget = theWidget;
	mSafeDeleteList.push_back(aWidgetSafeDeleteInfo);
}

/*
BOOL CALLBACK EnumCloseThing2(HWND hwnd, LPARAM lParam)
{
	//CloseWindow(hwnd);
	char aClassName[256];
	if (GetClassNameA(hwnd, aClassName, 256) != 0)
	{
		if (strcmp(aClassName, "Internet Explorer_Server") == 0)
		{
			DestroyWindow(hwnd);			
		}
		else
		{
			EnumChildWindows(hwnd, EnumCloseThing2, lParam);
		}
	}

	return TRUE;
}

BOOL CALLBACK EnumCloseThing(HWND hwnd, LPARAM lParam)
{
	//CloseWindow(hwnd);
	char aClassName[256];
	if (GetClassNameA(hwnd, aClassName, 256) != 0)
	{
		if (strcmp(aClassName, "AmWBC_WClass") == 0)
		{
			EnumChildWindows(hwnd, EnumCloseThing2, lParam);
		}
	}

	return TRUE;
}

static INT_PTR CALLBACK MarkerListDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND aListBox = GetDlgItem(hwnd,100);

			DWORD       dwExtent = 0;
			HDC         hDCListBox;
			HFONT       hFontOld, hFontNew;
			TEXTMETRIC  tm; 
			RECT aRect;
			SIZE aSize;

			hDCListBox = GetDC(aListBox); 
			hFontNew = (HFONT)SendMessage(aListBox, WM_GETFONT, 0, 0); 
			hFontOld = (HFONT)SelectObject(hDCListBox, hFontNew); 
			GetTextMetrics(hDCListBox, (LPTEXTMETRIC)&tm); 
			GetClientRect(hwnd, &aRect);
			MoveWindow(aListBox,10,10,aRect.right-aRect.left-20,aRect.bottom-aRect.top-20,FALSE);
			for (SexyAppBase::DemoMarkerList::iterator anItr = gSexyAppBase->mDemoMarkerList.begin(); anItr != gSexyAppBase->mDemoMarkerList.end(); ++anItr)
			{
				if (anItr->second <= gSexyAppBase->mUpdateCount)
					continue;

				int aTotalSeconds = (gSexyAppBase->mDemoLength - anItr->second)*gSexyAppBase->mFrameTime/1000;
				int aSeconds = aTotalSeconds%60;
				int aMinutes = (aTotalSeconds/60)%60;
				int anHours = (aTotalSeconds/3600);

				SexyString aStr = StrFormat(__S("%s (%02d:%02d:%02d)"), anItr->first.c_str(),anHours,aMinutes,aSeconds);				
				GetTextExtentPoint32(hDCListBox, aStr.c_str(), aStr.length(), &aSize);
				dwExtent = std::max (aSize.cx + tm.tmAveCharWidth, (LONG)dwExtent);
				SendMessage(aListBox, LB_SETHORIZONTALEXTENT, dwExtent, 0);
				LRESULT anIndex = SendMessage(aListBox, LB_ADDSTRING, 0, (LPARAM)aStr.c_str());
				SendMessage(aListBox, LB_SETITEMDATA, anIndex, anItr->second);
			}
	
			SelectObject(hDCListBox, hFontOld);
			ReleaseDC(aListBox, hDCListBox); 

			return TRUE;
		}

		case WM_CLOSE:
			EndDialog(hwnd,0);
			return TRUE;

		case WM_COMMAND:
			if (HIWORD(wParam)==LBN_DBLCLK)
			{
				HWND aListBox = GetDlgItem(hwnd,100);

				int anIndex = SendMessage(aListBox,LB_GETCURSEL,0,0);
				if (anIndex >= 0)
				{
					int anUpdateTime = SendMessage(aListBox,LB_GETITEMDATA,anIndex,0);
					if (anUpdateTime > gSexyAppBase->mUpdateCount)
					{
						gSexyAppBase->mFastForwardToUpdateNum = anUpdateTime;
						EndDialog(hwnd,0);
					}
				}
				return TRUE;
			}
			break;

	}

	return FALSE;
}

static LPWORD lpdwAlign ( LPWORD lpIn)
{
    uint32_t ul;

    ul = (uint32_t)(intptr_t)lpIn;
    ul +=3;
    ul >>=2;
    ul <<=2;
    return (LPWORD)(intptr_t)ul;
}
*/

static int ListDemoMarkers()
{
	/*
	HGLOBAL hgbl;
    LPDLGTEMPLATE lpdt;
    LPDLGITEMTEMPLATE lpdit;
    LPWORD lpw;
    LPWSTR lpwsz;
    LRESULT ret;
    int nchar;

    hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
    if (!hgbl)
        return -1;
 
    lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
 
    // Define a dialog box. 
    lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
    lpdt->cdit = 1;  // number of controls
    lpdt->x  = 10;  lpdt->y  = 10;
    lpdt->cx = 200; lpdt->cy = 200;

    lpw = (LPWORD) (lpdt + 1);
    *lpw++ = 0;   // no menu
    *lpw++ = 0;   // predefined dialog box class (by default)

    lpwsz = (LPWSTR) lpw;
    nchar = MultiByteToWideChar (CP_ACP, 0, "Marker List", -1, lpwsz, 50);
    lpw   += nchar;
	*lpw++ = 8;
	lpwsz = (LPWSTR) lpw;
    nchar = MultiByteToWideChar (CP_ACP, 0, "Tahoma", -1, lpwsz, 50);
	lpw += nchar;

	// Define Listbox
    lpw = lpdwAlign (lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE) lpw;
    lpdit->x  = 5; lpdit->y  = 5;
    lpdit->cx = 190; lpdit->cy = 195;
    lpdit->id = 100;  
	lpdit->style = WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY;
	lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
    lpw = (LPWORD) (lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0083;    // listbox class
	*lpw++ = 0;			// no window text
    *lpw++ = 0;			// no creation data


    GlobalUnlock(hgbl); 
    ret = DialogBoxIndirect(gHInstance, (LPDLGTEMPLATE) hgbl, gSexyAppBase->mHWnd, (DLGPROC) MarkerListDialogProc); 
    GlobalFree(hgbl); 
	*/

	gSexyAppBase->mLastTime = SDL_GetTicks();

    return 0; 
}

/*
static INT_PTR CALLBACK JumpToTimeDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND anEdit = GetDlgItem(hwnd,100);
			HKEY aGameKey;
			std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + gSexyAppBase->mRegKey);
			if (RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_READ | KEY_WRITE, &aGameKey) == ERROR_SUCCESS)
			{
				char aBuf[1024];
				DWORD aLength = 1000;
				DWORD aType = REG_SZ;
				if (RegQueryValueExA(aGameKey, "DemoJumpTime", 0, &aType, (uchar*) aBuf, &aLength) == ERROR_SUCCESS)
				{
					aBuf[aLength] = 0;
					SetWindowTextA(anEdit,aBuf);
					SendMessage(anEdit,EM_SETSEL,0,-1);
				}
				RegCloseKey(aGameKey);
			}
			return TRUE;
		}
		break;

		case WM_CLOSE:
			EndDialog(hwnd,0);
			return TRUE;

		case WM_COMMAND:
			if (HIWORD(wParam)==BN_CLICKED)
			{
				if (LOWORD(wParam)==IDOK)
				{
					char aBuf[512];
					HWND anEdit = GetDlgItem(hwnd,100);
					GetWindowTextA(anEdit,aBuf,500);

					HKEY aGameKey;
					std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + gSexyAppBase->mRegKey);
					if (RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_READ | KEY_WRITE, &aGameKey) == ERROR_SUCCESS)
					{
						RegSetValueExA(aGameKey, "DemoJumpTime", 0, REG_SZ, (const BYTE*)aBuf, strlen(aBuf)+1);
						RegCloseKey(aGameKey);
					}

					int aTime = 0;
					char *aPtr = strtok(aBuf,":");
					while (aPtr != NULL)
					{
						aTime *= 60;
						aTime += atoi(aPtr);
						aPtr = strtok(NULL,":");
					}
					aTime++;

					int aNumFrames = aTime*1000/gSexyAppBase->mFrameTime;
					gSexyAppBase->mFastForwardToUpdateNum = gSexyAppBase->mDemoLength - aNumFrames;					


				}
					
				EndDialog(hwnd,0);
				return TRUE;
			}
			break;
	}

	return FALSE;
}
*/

static int DemoJumpToTime()
{
	/*
	HGLOBAL hgbl;
    LPDLGTEMPLATE lpdt;
    LPDLGITEMTEMPLATE lpdit;
    LPWORD lpw;
    LPWSTR lpwsz;
    LRESULT ret;
    int nchar;

    hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
    if (!hgbl)
        return -1;
 
    lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
 
    // Define a dialog box. 
    lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
    lpdt->cdit = 3;  // number of controls
    lpdt->x  = 10;  lpdt->y  = 10;
    lpdt->cx = 200; lpdt->cy = 50;

    lpw = (LPWORD) (lpdt + 1);
    *lpw++ = 0;   // no menu
    *lpw++ = 0;   // predefined dialog box class (by default)

    lpwsz = (LPWSTR) lpw;
    nchar = MultiByteToWideChar (CP_ACP, 0, "Jump To Time", -1, lpwsz, 50);
    lpw   += nchar;
	*lpw++ = 8;
	lpwsz = (LPWSTR) lpw;
    nchar = MultiByteToWideChar (CP_ACP, 0, "Tahoma", -1, lpwsz, 50);
	lpw += nchar;

	// Define Edit
    lpw = lpdwAlign (lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE) lpw;
    lpdit->x  = 5; lpdit->y  = 5;
    lpdit->cx = 190; lpdit->cy = 15;
    lpdit->id = 100;  
	lpdit->style = WS_VISIBLE | WS_CHILD;
	lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
    lpw = (LPWORD) (lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0081;    // edit class
	*lpw++ = 0;			// no window text
    *lpw++ = 0;			// no creation data

	// Define Button
    lpw = lpdwAlign (lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE) lpw;
    lpdit->x  = 30; lpdit->y  = 25;
    lpdit->cx = 60; lpdit->cy = 15;
    lpdit->id = IDOK;  
	lpdit->style = WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON;
//	lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
    lpw = (LPWORD) (lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0080;    // button class
	lpwsz = (LPWSTR) lpw;
	nchar = MultiByteToWideChar (CP_ACP, 0, "Ok", -1, lpwsz, 50);
    lpw   += nchar;
    lpw = lpdwAlign (lpw); // align creation data on DWORD boundary
    *lpw++ = 0;			// no creation data


	// Define Button
    lpw = lpdwAlign (lpw); // align DLGITEMTEMPLATE on DWORD boundary
    lpdit = (LPDLGITEMTEMPLATE) lpw;
    lpdit->x  = 100; lpdit->y  = 25;
    lpdit->cx = 60; lpdit->cy = 15;
    lpdit->id = IDCANCEL;  
	lpdit->style = WS_VISIBLE | WS_CHILD;
//	lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
    lpw = (LPWORD) (lpdit + 1);
    *lpw++ = 0xFFFF;
    *lpw++ = 0x0080;    // button class
	lpwsz = (LPWSTR) lpw;
	nchar = MultiByteToWideChar (CP_ACP, 0, "Cancel", -1, lpwsz, 50);
    lpw   += nchar;
    lpw = lpdwAlign (lpw); // align creation data on DWORD boundary
    *lpw++ = 0;			// no creation data




    GlobalUnlock(hgbl); 
    ret = DialogBoxIndirect(gHInstance, (LPDLGTEMPLATE) hgbl, gSexyAppBase->mHWnd, (DLGPROC) JumpToTimeDialogProc); 
    GlobalFree(hgbl); 
	*/

	gSexyAppBase->mLastTime = SDL_GetTicks();

    return 0; 
}

static void ToggleDemoSoundVolume()
{
	if (gSexyAppBase->GetMusicVolume() == 0.0)
		gSexyAppBase->SetMusicVolume(gSexyAppBase->mDemoMusicVolume);
	else
	{
		gSexyAppBase->mDemoMusicVolume = gSexyAppBase->mMusicVolume;
		gSexyAppBase->SetMusicVolume(0.0);
	}

	if (gSexyAppBase->GetSfxVolume() == 0.0)
		gSexyAppBase->SetSfxVolume(gSexyAppBase->mDemoSfxVolume);
	else
	{
		gSexyAppBase->mDemoSfxVolume = gSexyAppBase->mSfxVolume;
		gSexyAppBase->SetSfxVolume(0.0);
	}
}

static DWORD gPowerSaveTick = 0;

void SexyAppBase::HandleNotifyGameMessage(int theType)
{
	/*
	if (theType==0) // bring to front message
	{
		WINDOWPLACEMENT aWindowPlacement;
		aWindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(mHWnd, &aWindowPlacement);		

		if (aWindowPlacement.showCmd == SW_SHOWMINIMIZED)
			ShowWindow(mHWnd, SW_RESTORE);

		::SetForegroundWindow(mHWnd);
	}
	*/
}

void SexyAppBase::RehupFocus()
{
	bool wantHasFocus = mActive && !mMinimized;

	if (wantHasFocus != mHasFocus)
	{					
		mHasFocus = wantHasFocus;

		if (mHasFocus)
		{
			if (mMuteOnLostFocus)
				Unmute(true);
	
			mWidgetManager->GotFocus();
			GotFocus();
		}
		else
		{
			if (mMuteOnLostFocus)
				Mute(true);
	
			mWidgetManager->LostFocus();
			LostFocus();

			//ReleaseCapture();
			mWidgetManager->DoMouseUps();
		}
	}
}

void SexyAppBase::ClearKeysDown()
{
	if (mWidgetManager != NULL) // fix stuck alt-key problem
	{
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
			mWidgetManager->mKeyDown[aKeyNum] = false;
	}
	mCtrlDown = false;
	mAltDown = false;
}

void SexyAppBase::WriteDemoTimingBlock()
{
	// Demo writing functions can only be called from the main thread and after SexyAppBase::Init
	DBG_ASSERTE((void*)pthread_self() == mPrimaryThreadId);

	while (mUpdateCount - mLastDemoUpdateCnt > 15)
	{
		mDemoBuffer.WriteNumBits(15, 4);
		mLastDemoUpdateCnt += 15;

		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_IDLE, 5);
		mDemoCmdOrder++;
	}
	
	mDemoBuffer.WriteNumBits(mUpdateCount - mLastDemoUpdateCnt, 4);
	mLastDemoUpdateCnt = mUpdateCount;
	mDemoCmdOrder++;
}

int aNumBigMoveMessages = 0;
int aNumSmallMoveMessages = 0;
int aNumTimerMessages = 0;

// Required unused in release mode
bool SexyAppBase::PrepareDemoCommand([[maybe_unused]] bool required)
{
	if (mDemoNeedsCommand)
	{
		mDemoCmdBitPos = mDemoBuffer.mReadBitPos;

		mLastDemoUpdateCnt += mDemoBuffer.ReadNumBits(4, false);

		mDemoIsShortCmd = mDemoBuffer.ReadNumBits(1, false) == 1;

		if (mDemoIsShortCmd)		
			mDemoCmdNum = mDemoBuffer.ReadNumBits(1, false);
		else
			mDemoCmdNum = mDemoBuffer.ReadNumBits(5, false);

		mDemoNeedsCommand = false;

		mDemoCmdOrder++;
	}

	DBG_ASSERTE((mUpdateCount == mLastDemoUpdateCnt) || (!required));

	return mUpdateCount == mLastDemoUpdateCnt;
}

void SexyAppBase::ProcessDemo()
{
	if (mPlayingDemoBuffer)
	{
		// At end of demo buffer?  How dare you!
		DBG_ASSERTE(!mDemoBuffer.AtEnd());

		while ((!mShutdown) && (mUpdateCount == mLastDemoUpdateCnt) && (!mDemoBuffer.AtEnd()))
		{
			if (PrepareDemoCommand(false))
			{
				mDemoNeedsCommand = true;
				
				if (mDemoIsShortCmd)
				{
					switch (mDemoCmdNum)
					{
					case 0:
						{
							int aDeltaX = mDemoBuffer.ReadNumBits(6, true);
							int aDeltaY = mDemoBuffer.ReadNumBits(6, true);
							mLastDemoMouseX += aDeltaX;
							mLastDemoMouseY += aDeltaY;

							mWidgetManager->MouseMove(mLastDemoMouseX, mLastDemoMouseY);
						}
						break;
					case 1:
						{
							bool down = mDemoBuffer.ReadNumBits(1, false) != 0;
							int aBtnCount = mDemoBuffer.ReadNumBits(3, true);							
		
							if (down)
								mWidgetManager->MouseDown(mLastDemoMouseX, mLastDemoMouseY, aBtnCount);
							else
								mWidgetManager->MouseUp(mLastDemoMouseX, mLastDemoMouseY, aBtnCount);
						}
						break;
					}
				}
				else
				{				
					switch (mDemoCmdNum)
					{
					case DEMO_MOUSE_POSITION:
						{
							mLastDemoMouseX = mDemoBuffer.ReadNumBits(12, false);
							mLastDemoMouseY = mDemoBuffer.ReadNumBits(12, false);

							mWidgetManager->MouseMove(mLastDemoMouseX, mLastDemoMouseY);						
						}
						break;
					case DEMO_ACTIVATE_APP:
						{
							mActive = mDemoBuffer.ReadNumBits(1, false) != 0;

							RehupFocus();
							
							if ((mActive) && (!mIsWindowed))
								mWidgetManager->MarkAllDirty();

							if ((mIsOpeningURL) && (!mActive))
								URLOpenSucceeded(mOpeningURL);
						}
						break;
					case DEMO_SIZE:
						{
							bool isMinimized = mDemoBuffer.ReadBoolean();

							if ((!mShutdown) && (isMinimized != mMinimized))
							{
								mMinimized = isMinimized;

								// We don't want any sounds (or music) playing while its minimized
								if (mMinimized)
									Mute(true);
								else
								{
									Unmute(true);
									mWidgetManager->MarkAllDirty();
								}
							}

							RehupFocus();
						}
						break;
					case DEMO_MOUSE_WHEEL:
						{
							int aScroll = mDemoBuffer.ReadNumBits(8, true);
							mWidgetManager->MouseWheel(aScroll);
						}
						break;
					case DEMO_KEY_DOWN:
						{
							KeyCode aKeyCode = (KeyCode) mDemoBuffer.ReadNumBits(8, false);
							mWidgetManager->KeyDown(aKeyCode);
						}
						break;
					case DEMO_KEY_UP:
						{
							KeyCode aKeyCode = (KeyCode) mDemoBuffer.ReadNumBits(8, false);
							mWidgetManager->KeyUp(aKeyCode);
						}
						break;
					case DEMO_KEY_CHAR:
						{
							int sizeMult = (int)mDemoBuffer.ReadNumBits(1, false) + 1; // will be 1 for single, 2 for double
							SexyChar aChar = (SexyChar) mDemoBuffer.ReadNumBits(8*sizeMult, false);
							mWidgetManager->KeyChar(aChar);
						}
						break;
					case DEMO_CLOSE:
						Shutdown();
						break;
					case DEMO_MOUSE_ENTER:
						mMouseIn = true;
						EnforceCursor();
						break;
					case DEMO_MOUSE_EXIT:
						mWidgetManager->MouseExit(mLastDemoMouseX, mLastDemoMouseY);
						mMouseIn = false;
						EnforceCursor();
						break;
					case DEMO_LOADING_COMPLETE:
						mDemoLoadingComplete = true;
						break;
					case DEMO_VIDEO_DATA:
						mIsWindowed = mDemoBuffer.ReadBoolean();
						mSyncRefreshRate = mDemoBuffer.ReadByte();
						break;
					case DEMO_IDLE:
						break;
					default:
						DBG_ASSERTE("Invalid Demo Command" == 0);
						break;
					}
				}
			}
		}
	}
}

void SexyAppBase::ShowMemoryUsage()
{
	/*
	DWORD aTotal = 0;
	DWORD aFree = 0;

	if (mDDInterface->mDD7 != NULL)
	{
		DDSCAPS2 aCaps;
		ZeroMemory(&aCaps,sizeof(aCaps));
		aCaps.dwCaps = DDSCAPS_VIDEOMEMORY;	
		mDDInterface->mDD7->GetAvailableVidMem(&aCaps,&aTotal,&aFree);
	}

	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	typedef std::pair<int,int> FormatUsage;
	typedef std::map<PixelFormat,FormatUsage> FormatMap;
	FormatMap aFormatMap;
	int aTextureMemory = 0;
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;				
		if (aMemoryImage->mD3DData != NULL)
		{
			TextureData *aData = (TextureData*)aMemoryImage->mD3DData;
			aTextureMemory += aData->mTexMemSize;

			FormatUsage &aUsage = aFormatMap[aData->mPixelFormat];
			aUsage.first++;
			aUsage.second += aData->mTexMemSize;
		}

		++anItr;
	}

	std::string aStr;

	const char *aDesc;
	if (Is3DAccelerationRecommended())
		aDesc = "Recommended";
	else if (Is3DAccelerationSupported())
		aDesc = "Supported";
	else
		aDesc = "Unsupported";

	aStr += StrFormat("3D-Mode is %s (3D is %s on this system)\r\n\r\n",Is3DAccelerated()?"On":"Off",aDesc);

	aStr += StrFormat("Num Images: %d\r\n",(int)mMemoryImageSet.size());
	aStr += StrFormat("Num Sounds: %d\r\n",mSoundManager->GetNumSounds());
	aStr += StrFormat("Video Memory: %s/%s KB\r\n", SexyStringToString(CommaSeperate((aTotal-aFree)/1024)).c_str(), SexyStringToString(CommaSeperate(aTotal/1024)).c_str());
	aStr += StrFormat("Texture Memory: %s KB\r\n",CommaSeperate(aTextureMemory/1024).c_str());

	FormatUsage aUsage = aFormatMap[PixelFormat_A8R8G8B8];
	aStr += StrFormat("A8R8G8B8: %d - %s KB\r\n",aUsage.first,SexyStringToString(CommaSeperate(aUsage.second/1024)).c_str());
	aUsage = aFormatMap[PixelFormat_A4R4G4B4];
	aStr += StrFormat("A4R4G4B4: %d - %s KB\r\n",aUsage.first,SexyStringToString(CommaSeperate(aUsage.second/1024)).c_str());
	aUsage = aFormatMap[PixelFormat_R5G6B5];
	aStr += StrFormat("R5G6B5: %d - %s KB\r\n",aUsage.first,SexyStringToString(CommaSeperate(aUsage.second/1024)).c_str());
	aUsage = aFormatMap[PixelFormat_Palette8];
	aStr += StrFormat("Palette8: %d - %s KB\r\n",aUsage.first,SexyStringToString(CommaSeperate(aUsage.second/1024)).c_str());
	
	MsgBox(aStr,"Video Stats",MB_OK);
	mLastTime = SDL_GetTicks();
	*/
}

bool SexyAppBase::DebugKeyDown(int theKey)
{
	if ((theKey == 'R') && (mWidgetManager->mKeyDown[KEYCODE_MENU]))
	{	
/*
#ifndef RELEASEFINAL
		if (ReparseModValues())
			PlaySoundA("c:\\windows\\media\\Windows XP Menu Command.wav", NULL, SND_ASYNC);				
		else
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mWidgetManager->mKeyDown[aKeyNum] = false;
		}
#endif
*/
	}
	/*
	else if (theKey == VK_F3)
	{
		if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mShowFPS = true;
			if (++mShowFPSMode >= Num_FPS_Types)
				mShowFPSMode = 0;
		}
		else
			mShowFPS = !mShowFPS;

		mWidgetManager->MarkAllDirty();

		if (mShowFPS)
		{
			gFPSTimer.Start();
			gFrameCount = 0;
			gFPSDisplay = 0;
			gForceDisplay = true;
		}
	}
	else if (theKey == VK_F8)
	{
		if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			Set3DAcclerated(!Is3DAccelerated());

			char aBuf[512];
			sprintf(aBuf,"3D-Mode: %s",Is3DAccelerated()?"ON":"OFF");
			MsgBox(aBuf,"Mode Switch",MB_OK);
			mLastTime = SDL_GetTicks();
		}
		else
			ShowMemoryUsage();

		return true;
	}
	else if (theKey == VK_F10)
	{
#ifndef RELEASEFINAL
		if (mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			if (mUpdateMultiplier==0.25)
				mUpdateMultiplier = 1.0;
			else
				mUpdateMultiplier = 0.25;
		}
		else if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mStepMode = 0;
			ClearUpdateBacklog();
		}
		else
			mStepMode = 1;
#endif

		return true;
	}
	else if (theKey == VK_F11)
	{
		if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
			DumpProgramInfo();
		else
			TakeScreenshot();

		return true;
	}
	else if (theKey == VK_F2)
	{
		bool isPerfOn = !SexyPerf::IsPerfOn();
		if (isPerfOn)
		{
//			MsgBox("Perf Monitoring: ON", "Perf Monitoring", MB_OK);
			ClearUpdateBacklog();
			SexyPerf::BeginPerf();
		}
		else
		{
			SexyPerf::EndPerf();
			MsgBox(SexyPerf::GetResults().c_str(), "Perf Results", MB_OK);
			ClearUpdateBacklog();
		}
	}
	else
		return false;
	*/

	return false;
}

/*
bool SexyAppBase::DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown)
{
	return false;
}
*/

void SexyAppBase::CloseRequestAsync()
{
}

void SexyAppBase::Done3dTesting()
{
}

// return file name that you want to upload
std::string	SexyAppBase::NotifyCrashHook()
{
	return "";
}

void SexyAppBase::DeleteNativeImageData()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;		
		aMemoryImage->DeleteNativeData();			
		++anItr;
	}
}

void SexyAppBase::DeleteExtraImageData()
{
	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->DeleteExtraBuffers();
		++anItr;
	}
}

void SexyAppBase::ReInitImages()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;				
		aMemoryImage->ReInit();
		++anItr;
	}
}


void SexyAppBase::LoadingThreadProc()
{
}

void SexyAppBase::LoadingThreadCompleted()
{
}

void* SexyAppBase::LoadingThreadProcStub(void *theArg)
{
	SexyAppBase* aSexyApp = (SexyAppBase*) theArg;
	
	aSexyApp->LoadingThreadProc();		

	printf("Resource Loading Time: %d\r\n", (SDL_GetTicks() - aSexyApp->mTimeLoaded));

	aSexyApp->mLoadingThreadCompleted = true;
	return 0;
}

void SexyAppBase::StartLoadingThread()
{	
	if (!mLoadingThreadStarted)
	{
		mYieldMainThread = true; 
		//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);		
		mLoadingThreadStarted = true;
		//_beginthread(LoadingThreadProcStub, 0, this);
		pthread_t id;
#ifdef PS2_PLATFORM
		// PTE's default pthread stack is only 32KB (malloc'd from the heap).
		// The loading thread runs libpng/JPEG/XML/vorbis decoding; overflowing
		// that stack corrupts adjacent heap blocks and detonates much later.
		pthread_attr_t anAttr;
		pthread_attr_init(&anAttr);
		pthread_attr_setstacksize(&anAttr, 1024 * 1024);
		pthread_create(&id, &anAttr, LoadingThreadProcStub, this);
		pthread_attr_destroy(&anAttr);
#else
		pthread_create(&id, NULL, LoadingThreadProcStub, this);
#endif
		pthread_detach(id);
	}
}
void SexyAppBase::CursorThreadProc()
{
	/*
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	POINT aLastCursorPos = {0, 0};
	int aLastDrawCount = 0;

	while (!mShutdown)
	{
//		if (mProcessInTimer)
//			PostMessage(mHWnd,WM_TIMER,101,0);

		POINT aCursorPos;

		if (mPlayingDemoBuffer)
		{
			aCursorPos.x = mLastDemoMouseX;
			aCursorPos.y = mLastDemoMouseY;
		}
		else
		{
			::GetCursorPos(&aCursorPos);
			::ScreenToClient(mHWnd, &aCursorPos);
		}

		if (aLastDrawCount != mDrawCount)
		{
			// We did a draw so we may have committed a pending mNextCursorX/Y 
			aLastCursorPos.x = mGLInterface->mCursorX;
			aLastCursorPos.y = mGLInterface->mCursorY;
		}

		if ((aCursorPos.x != aLastCursorPos.x) ||
			(aCursorPos.y != aLastCursorPos.y))
		{	
			DWORD aTimeNow = SDL_GetTicks();
			if (aTimeNow - mNextDrawTick > mGLInterface->mMillisecondsPerFrame + 5)
			{
				// Do the special drawing if we are rendering at less than full framerate				
				mGLInterface->SetCursorPos(aCursorPos.x, aCursorPos.y);
				aLastCursorPos = aCursorPos;
			}
			else
			{
				// Set them up to get assigned in the next screen redraw
				mGLInterface->mNextCursorX = aCursorPos.x;
				mGLInterface->mNextCursorY = aCursorPos.y;
			}			
		}		

		Sleep(10);
	}
	
	mCursorThreadRunning = false;
	*/
}

void SexyAppBase::CursorThreadProcStub(void *theArg)
{
	/*
	CoInitialize(NULL);
	SexyAppBase* aSexyApp = (SexyAppBase*) theArg;
	aSexyApp->CursorThreadProc();
	*/
}

void SexyAppBase::StartCursorThread()
{
	/*
	if (!mCursorThreadRunning)
	{
		mCursorThreadRunning = true;
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		_beginthread(CursorThreadProcStub, 0, this);
	}
	*/
}

void SexyAppBase::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mForceFullscreen)
		wantWindowed = false;

	if (mIsWindowed == wantWindowed && !force)
	{
		Set3DAcclerated(is3d);
		return;
	}

	// Set 3d acceleration preference
	Set3DAcclerated(is3d,false);

	// Always make the app windowed when playing demos, in order to
	//  make it easier to track down bugs.  We place this after the
	//  sanity check just so things get re-initialized and stuff
	//if (mPlayingDemoBuffer)
	//	wantWindowed = true;

	mIsWindowed = wantWindowed;	

	MakeWindow();
	
	// We need to do this check to allow IE to get focus instead of
	//  stealing it away for ourselves
	if (!mIsOpeningURL)
	{
		//::ShowWindow(mHWnd, SW_NORMAL);
		//::SetForegroundWindow(mHWnd);
	}
	else
	{
		// Show it but don't activate it
		//::ShowWindow(mHWnd, SW_SHOWNOACTIVATE);
	}

	if (mSoundManager!=NULL)
	{
		//mSoundManager->SetCooperativeWindow(mHWnd);
	}	

	mLastTime = SDL_GetTicks();
}

void SexyAppBase::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

void SexyAppBase::SwitchScreenMode()
{
	SwitchScreenMode(mIsWindowed, Is3DAccelerated(), true);
}

void SexyAppBase::SetAlphaDisabled(bool isDisabled)
{
	if (mAlphaDisabled != isDisabled)
	{
		mAlphaDisabled = isDisabled;
		mGLInterface->SetVideoOnlyDraw(mAlphaDisabled);
		mWidgetManager->mImage = mGLInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
}

void SexyAppBase::EnforceCursor()
{
	/*
	bool wantSysCursor = true;

	if (mDDInterface == NULL)
		return;

	if ((mSEHOccured) || (!mMouseIn))
	{
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));	
		if (mDDInterface->SetCursorImage(NULL))
			mCustomCursorDirty = true;
	}
	else
	{
		if ((mCursorImages[mCursorNum] == NULL) || 
			((!mPlayingDemoBuffer) && (!mCustomCursorsEnabled) && (mCursorNum != CURSOR_CUSTOM)))
		{
			if (mOverrideCursor != NULL)
				::SetCursor(mOverrideCursor);
			else if (mCursorNum == CURSOR_POINTER)
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));
			else if (mCursorNum == CURSOR_HAND)
				::SetCursor(mHandCursor);
			else if (mCursorNum == CURSOR_TEXT)
				::SetCursor(::LoadCursor(NULL, IDC_IBEAM));
			else if (mCursorNum == CURSOR_DRAGGING)
				::SetCursor(mDraggingCursor);
			else if (mCursorNum == CURSOR_CIRCLE_SLASH)
				::SetCursor(::LoadCursor(NULL, IDC_NO));
			else if (mCursorNum == CURSOR_SIZEALL)
				::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
			else if (mCursorNum == CURSOR_SIZENESW)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENESW));
			else if (mCursorNum == CURSOR_SIZENS)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
			else if (mCursorNum == CURSOR_SIZENWSE)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENWSE));
			else if (mCursorNum == CURSOR_SIZEWE)
				::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			else if (mCursorNum == CURSOR_WAIT)
				::SetCursor(::LoadCursor(NULL, IDC_WAIT));
			else if (mCursorNum == CURSOR_CUSTOM) 
				::SetCursor(NULL); // Default to not showing anything
			else if (mCursorNum == CURSOR_NONE)
				::SetCursor(NULL);			
			else
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));

			if (mDDInterface->SetCursorImage(NULL))
				mCustomCursorDirty = true;
		}
		else
		{
			if (mDDInterface->SetCursorImage(mCursorImages[mCursorNum]))
				mCustomCursorDirty = true;

			if (!mPlayingDemoBuffer)
			{
				::SetCursor(NULL);
			}
			else
			{
				// Give the NO cursor in the client area and an arrow on the title bar

				POINT aULCorner = {0, 0};
				::ClientToScreen(mHWnd, &aULCorner);

				POINT aBRCorner = {mWidth, mHeight};
				::ClientToScreen(mHWnd, &aBRCorner);

				POINT aPoint;
				::GetCursorPos(&aPoint);			
									
				if ((aPoint.x >= aULCorner.x) && (aPoint.y >= aULCorner.y) &&
					(aPoint.x < aBRCorner.x) && (aPoint.y < aBRCorner.y))
				{
					::SetCursor(::LoadCursor(NULL, IDC_NO));
				}
				else
				{
					::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				}
			}

			wantSysCursor = false;
		}
	}

	if (wantSysCursor != mSysCursor)
	{
		mSysCursor = wantSysCursor;

		// Don't hide the hardware cursor when playing back a demo buffer
//		if (!mPlayingDemoBuffer)
//			::ShowCursor(mSysCursor);
	}
	*/
}

void SexyAppBase::ProcessSafeDeleteList()
{
	MTAutoDisallowRand aDisallowRand;

	WidgetSafeDeleteList::iterator anItr = mSafeDeleteList.begin();
	while (anItr != mSafeDeleteList.end())
	{
		WidgetSafeDeleteInfo* aWidgetSafeDeleteInfo = &(*anItr);
		if (mUpdateAppDepth <= aWidgetSafeDeleteInfo->mUpdateAppDepth)
		{
			delete aWidgetSafeDeleteInfo->mWidget;
			anItr = mSafeDeleteList.erase(anItr);
		}
		else
			++anItr;
	}	
}

void SexyAppBase::UpdateFTimeAcc()
{
	DWORD aCurTime = SDL_GetTicks();

	if (mLastTimeCheck != 0)
	{				
		int aDeltaTime = aCurTime - mLastTimeCheck;		

		mUpdateFTimeAcc = std::min(mUpdateFTimeAcc + aDeltaTime, 200.0);

		if (mRelaxUpdateBacklogCount > 0)				
			mRelaxUpdateBacklogCount = std::max(mRelaxUpdateBacklogCount - aDeltaTime, 0);				
	}

	mLastTimeCheck = aCurTime;
}

//int aNumCalls = 0;
//DWORD aLastCheck = 0;

bool SexyAppBase::Process(bool allowSleep)
{
	/*DWORD aTimeNow = GetTickCount();
	if (aTimeNow - aLastCheck >= 10000)
	{
		OutputDebugString(StrFormat(__S("FUpdates: %d\n"), aNumCalls).c_str());
		aLastCheck = aTimeNow;
		aNumCalls = 0;
	}*/

	if (mLoadingFailed)
		Shutdown();
	
	bool isVSynched = (!mPlayingDemoBuffer) && (mVSyncUpdates) && (!mLastDrawWasEmpty) && (!mVSyncBroken) &&
		((!mIsPhysWindowed) || (mIsPhysWindowed && mWaitForVSync && !mSoftVSyncWait));
	double aFrameFTime;
	double anUpdatesPerUpdateF;

	if (mVSyncUpdates)
	{
		aFrameFTime = (1000.0 / mSyncRefreshRate) / mUpdateMultiplier;
		anUpdatesPerUpdateF = (float) (1000.0 / (mFrameTime * mSyncRefreshRate));
	}
	else
	{
		aFrameFTime = mFrameTime / mUpdateMultiplier;
		anUpdatesPerUpdateF = 1.0;
	}

	// Do we need to fast forward?
	if (mPlayingDemoBuffer)
	{
		if (mUpdateCount < mFastForwardToUpdateNum || mFastForwardToMarker)
		{
			if (!mDemoMute && !mFastForwardStep)
			{
				mDemoMute = true;
				Mute(true);
			}

			static DWORD aTick = SDL_GetTicks();
			while (mUpdateCount < mFastForwardToUpdateNum || mFastForwardToMarker)
			{
				ClearUpdateBacklog();
				int aLastUpdateCount = mUpdateCount;
								
				// Actual updating code below
				//////////////////////////////////////////////////////////////////////////
				
				bool hadRealUpdate = DoUpdateFrames();

				if (hadRealUpdate)
				{					
					mPendingUpdatesAcc += anUpdatesPerUpdateF;
					mPendingUpdatesAcc -= 1.0;
					ProcessSafeDeleteList();

					// Process any extra updates
					while (mPendingUpdatesAcc >= 1.0)
					{						
						// These should just be IDLE commands we have to clear out
						ProcessDemo();												

						bool hasRealUpdate = DoUpdateFrames();
						DBG_ASSERTE(hasRealUpdate);

						if (!hasRealUpdate)
							break;
												
						ProcessSafeDeleteList();
						mPendingUpdatesAcc -= 1.0;
					}
										
					DoUpdateFramesF((float) anUpdatesPerUpdateF);					
					ProcessSafeDeleteList();
				}

				//////////////////////////////////////////////////////////////////////////				

				// If the update count doesn't change, its because we are
				//  playing back a demo and need to read more
				if (aLastUpdateCount == mUpdateCount)
					return true;

				DWORD aNewTick = SDL_GetTicks();
				if (aNewTick - aTick >= 1000 || mFastForwardStep) // let the app draw some
				{
					mFastForwardStep = false;
					aTick = SDL_GetTicks();
					DrawDirtyStuff();			
					return true;
				}
			}
		}

		if (mDemoMute)
		{
			mDemoMute = false;
			mSoundManager->StopAllSounds();
			Unmute(true);
		}
	}

	// Make sure we're not paused
	if ((!mPaused) && (mUpdateMultiplier > 0))
	{
		uint32_t aStartTime = SDL_GetTicks();
		
		// uint32_t aCurTime = aStartTime; // Unused
		int aCumSleepTime = 0;
		
		// When we are VSynching, only calculate this FTimeAcc right after drawing
		
		if (!isVSynched)		
			UpdateFTimeAcc();					

		// mNonDrawCount is used to make sure we draw the screen at least
		// 10 times per second, even if it means we have to slow down
		// the updates to make it draw 10 times per second in "game time"
		
		bool didUpdate = false;
		
		if (mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			if ((++mNonDrawCount < (int) ceil(10*mUpdateMultiplier)) || (!mLoaded))
			{
				bool doUpdate = false;
				
				if (isVSynched)
				{
					// Synch'ed to vertical refresh, so update as soon as possible after draw
					doUpdate = (!mHasPendingDraw) || (mUpdateFTimeAcc >= (int) (aFrameFTime * 0.75));
				}
				else if (mUpdateFTimeAcc >= aFrameFTime)
				{
					doUpdate = true;
				}

				if (doUpdate)
				{
					// Do VSyncBroken test.  This test fails if we're in fullscreen and
					// "don't vsync" has been forced in Advanced settings up Display Properties
					if ((!mPlayingDemoBuffer) && (mUpdateMultiplier == 1.0))
					{
						mVSyncBrokenTestUpdates++;
						if (mVSyncBrokenTestUpdates >= (DWORD) ((1000+mFrameTime-1)/mFrameTime))
						{
							// It has to be running 33% fast to be "broken" (25% = 1/0.800)
							if (aStartTime - mVSyncBrokenTestStartTick <= 800)
							{
								// The test has to fail 3 times in a row before we decide that
								//  vsync is broken overall
								mVSyncBrokenCount++;
								if (mVSyncBrokenCount >= 3)
									mVSyncBroken = true;
							}
							else
								mVSyncBrokenCount = 0;

							mVSyncBrokenTestStartTick = aStartTime;
							mVSyncBrokenTestUpdates = 0;
						}
					}
					
					bool hadRealUpdate = DoUpdateFrames();
					if (hadRealUpdate)
						mUpdateAppState = UPDATESTATE_PROCESS_2;					

					mHasPendingDraw = true;
					didUpdate = true;
				}
			}
		}
		else if (mUpdateAppState == UPDATESTATE_PROCESS_2)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;
			
			mPendingUpdatesAcc += anUpdatesPerUpdateF;
			mPendingUpdatesAcc -= 1.0;
			ProcessSafeDeleteList();

			// Process any extra updates
			while (mPendingUpdatesAcc >= 1.0)
			{	
				// These should just be IDLE commands we have to clear out
				ProcessDemo();												

				++mNonDrawCount;
				bool hasRealUpdate = DoUpdateFrames();
				DBG_ASSERTE(hasRealUpdate);

				if (!hasRealUpdate)
					break;
										
				ProcessSafeDeleteList();
				mPendingUpdatesAcc -= 1.0;
			}					

			//aNumCalls++;
			DoUpdateFramesF((float) anUpdatesPerUpdateF);
			ProcessSafeDeleteList();		
		
			// Don't let mUpdateFTimeAcc dip below 0
			//  Subtract an extra 0.2ms, because sometimes refresh rates have some
			//  fractional component that gets truncated, and it's better to take off
			//  too much to keep our timing tending toward occuring right after 
			//  redraws
			if (isVSynched)
				mUpdateFTimeAcc = std::max(mUpdateFTimeAcc - aFrameFTime - 0.2f, 0.0);
			else
				mUpdateFTimeAcc -= aFrameFTime;

			if (mRelaxUpdateBacklogCount > 0)
				mUpdateFTimeAcc = 0;

			didUpdate = true;
		}
		
		if (!didUpdate)
		{			
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mNonDrawCount = 0;
			
			if (mHasPendingDraw)
			{
				DrawDirtyStuff();
			}
			else
			{
				// Let us take into account the time it took to draw dirty stuff			
				int aTimeToNextFrame = (int) (aFrameFTime - mUpdateFTimeAcc);
				if (aTimeToNextFrame > 0)
				{
					if (!allowSleep)
						return false;

					// Wait till next processing cycle
					++mSleepCount;

					timespec ts;
					ts.tv_sec = aTimeToNextFrame / 1000;
					ts.tv_nsec = (aTimeToNextFrame % 1000) * 1000000;
					nanosleep(&ts, &ts);

					aCumSleepTime += aTimeToNextFrame;					
				}
			}
		}

		if (mYieldMainThread)
		{
			// This is to make sure that the title screen doesn't take up any more than 
			// 1/3 of the processor time

			uint32_t anEndTime = SDL_GetTicks();
			int anElapsedTime = (anEndTime - aStartTime) - aCumSleepTime;
			int aLoadingYieldSleepTime = std::min(250, (anElapsedTime * 2) - aCumSleepTime);

			if (aLoadingYieldSleepTime >= 0)
			{
				if (!allowSleep)
					return false;

				timespec ts;
				ts.tv_sec = aLoadingYieldSleepTime / 1000;
				ts.tv_nsec = (aLoadingYieldSleepTime % 1000) * 1000000;
				nanosleep(&ts, &ts);
			}
		}
	}

	ProcessSafeDeleteList();	
	return true;
}

/*void SexyAppBase::DoMainLoop()
{
	Dialog* aDialog = NULL;
	if (theModalDialogId != -1)
	{
		aDialog = GetDialog(theModalDialogId);
		DBG_ASSERTE(aDialog != NULL);
		if (aDialog == NULL)
			return;
	}

	while (!mShutdown)
	{		
		MSG msg;
		while ((PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) && (!mShutdown))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ProcessDemo();		
		ProcessDeferredMessages();

		if ((aDialog != NULL) && (aDialog->mResult != -1))
			return;

		if (!mShutdown)
		{
			//++aCount;
			Process();
		}		
	}
}*/

#ifdef PS2_PLATFORM
namespace Sexy { void Ps2MixerHealthTick(); }
#endif

void SexyAppBase::DoMainLoop()
{
#ifdef PS2_PLATFORM
	int aPs2LoopFrame = 0;
	Ps2Trace(PS2_TRACE_MAINLOOP);
#endif
	while (!mShutdown)
	{
		if (mExitToTop)
			mExitToTop = false;
#ifdef PS2_PLATFORM
		if (aPs2LoopFrame == 0)
			Ps2Trace(PS2_TRACE_FIRST_UPDATE);
#endif
		UpdateApp();
#ifdef PS2_PLATFORM
		if (aPs2LoopFrame == 0 && mDrawCount == 0)
			Ps2Trace(PS2_TRACE_FIRST_UPDATE_DONE);
		// Liveness tick: blinks the border's blue channel ~1Hz while the main
		// thread runs, so a frozen screen tells us WHICH side died (see
		// Ps2Trace.h). Pure GS register write, no fio/SIF.
		Ps2TraceMainBeat();
		// Mixer liveness: prints only on state changes, so no log flood.
		if ((aPs2LoopFrame % 100) == 0)
			Sexy::Ps2MixerHealthTick();
		++aPs2LoopFrame;
#endif
	}
}

bool SexyAppBase::UpdateAppStep(bool* updated)
{
	if (updated != NULL)
		*updated = false;

	if (mExitToTop)
		return false;

	if (mUpdateAppState == UPDATESTATE_PROCESS_DONE)
		mUpdateAppState = UPDATESTATE_MESSAGES;

	mUpdateAppDepth++;

	// We update in two stages to avoid doing a Process if our loop termination
	//  condition has already been met by processing windows messages		
	if (mUpdateAppState == UPDATESTATE_MESSAGES)
	{
		/*
		MSG msg;
		while ((PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) && (!mShutdown))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		*/

		ProcessDemo();
		if (!ProcessDeferredMessages(true))
		{			
			mUpdateAppState = UPDATESTATE_PROCESS_1;
		}
	}
	else
	{
		// Process changes state by itself
		if (mStepMode)
		{
			if (mStepMode==2)
			{
				timespec ts;
				ts.tv_sec = mFrameTime / 1000;
				ts.tv_nsec = (mFrameTime % 1000) * 1000000;
				nanosleep(&ts, &ts);

				mUpdateAppState = UPDATESTATE_PROCESS_DONE; // skip actual update until next step
			}
			else
			{
				mStepMode = 2;
				DoUpdateFrames();
				DoUpdateFramesF(1.0f);
				DrawDirtyStuff();
			}
		}
		else
		{
			int anOldUpdateCnt = mUpdateCount;
			Process();		
			if (updated != NULL)
				*updated = mUpdateCount != anOldUpdateCnt;			
		}
	}

	mUpdateAppDepth--;

	return true;
}

bool SexyAppBase::UpdateApp()
{
	bool updated;
	for (;;)
	{
		if (!UpdateAppStep(&updated))
			return false;
		if (updated)
			return true;
	}
}

int SexyAppBase::InitGLInterface()
{
	PreGLInterfaceInitHook();
	DeleteNativeImageData();
	int aResult = mGLInterface->Init(mIsPhysWindowed);
	DemoSyncRefreshRate();
	if (aResult)
	{
		mScreenBounds.mX = ( mWidth - mGLInterface->mWidth ) / 2;
		mScreenBounds.mY = ( mHeight - mGLInterface->mHeight ) / 2;
		mScreenBounds.mWidth = mGLInterface->mWidth;
		mScreenBounds.mHeight = mGLInterface->mHeight;
		mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
		PostGLInterfaceInitHook();
	}
	return aResult;
}

void SexyAppBase::PreTerminate()
{
}

void SexyAppBase::Start()
{
	if (mShutdown)
		return;

	StartCursorThread();

	if (mAutoStartLoadingThread)
		StartLoadingThread();

	//::ShowWindow(mHWnd, SW_SHOW);
	//::SetFocus(mHWnd);

	//timeBeginPeriod(1);

	//int aCount = 0; // unused
	//int aSleepCount = 0; // unused

	DWORD aStartTime = SDL_GetTicks();

	mRunning = true;
	mLastTime = aStartTime;
	mLastUserInputTick = aStartTime;
	mLastTimerTime = aStartTime;

	DoMainLoop();
	ProcessSafeDeleteList();

	mRunning = false;

	WaitForLoadingThread();

	printf("Seconds       = %g\r\n", (SDL_GetTicks() - aStartTime) / 1000.0);
	//printf("Count         = %d\r\n", aCount);
	printf("Sleep Count   = %d\r\n", mSleepCount);
	printf("Update Count  = %d\r\n", mUpdateCount);
	printf("Draw Count    = %d\r\n", mDrawCount);
	printf("Draw Time     = %d\r\n", mDrawTime);
	printf("Screen Blt    = %d\r\n", mScreenBltTime);
	if (mDrawTime+mScreenBltTime > 0)
	{
		printf("Avg FPS       = %d\r\n", (mDrawCount*1000)/(mDrawTime+mScreenBltTime));
	}

	//timeEndPeriod(1);	

	PreTerminate();

	WriteToRegistry();
}

/*
bool SexyAppBase::CheckSignature(const Buffer& theBuffer, const std::string& theFileName)
{
	(void)theBuffer;(void)theFileName;
	// Add your own signature checking code here
	return false;
}
*/

bool SexyAppBase::LoadProperties(const std::string& theFileName, bool required, bool checkSig)
{
	Buffer aBuffer;
	if (!ReadBufferFromFile(theFileName, &aBuffer))
	{
		if (!required)
			return true;
		else
		{
			Popup(GetString("UNABLE_OPEN_PROPERTIES", __S("Unable to open properties file ")) + StringToSexyString(theFileName));
			return false;
		}
	}
	if (checkSig)
	{
		//if (!CheckSignature(aBuffer, theFileName))
		//{
			//Popup(GetString("PROPERTIES_SIG_FAILED", __S("Signature check failed on ")) + StringToSexyString(theFileName + "'"));
			//return false;
		//}
	}

	PropertiesParser aPropertiesParser(this);

	// Load required language-file properties
		if (!aPropertiesParser.ParsePropertiesBuffer(aBuffer))
		{
			Popup(aPropertiesParser.GetErrorText());		
			return false;
		}
		else
			return true;
}

bool SexyAppBase::LoadProperties()
{
	// Load required language-file properties
	return LoadProperties("properties/default.xml", true, false);
}

void SexyAppBase::LoadResourceManifest()
{
	if (!mResourceManager->ParseResourcesFile("properties/resources.xml"))
		ShowResourceError(true);
}

void SexyAppBase::ShowResourceError(bool doExit)
{
	Popup(mResourceManager->GetErrorText());	
	if (doExit)
		DoExit(0);
}

bool SexyAppBase::GetBoolean(const std::string& theId)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);
	DBG_ASSERTE(anItr != mBoolProperties.end());
	
	if (anItr != mBoolProperties.end())	
		return anItr->second;
	else
		return false;
}

bool SexyAppBase::GetBoolean(const std::string& theId, bool theDefault)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);	
	
	if (anItr != mBoolProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

int SexyAppBase::GetInteger(const std::string& theId)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);
	DBG_ASSERTE(anItr != mIntProperties.end());
	
	if (anItr != mIntProperties.end())	
		return anItr->second;
	else
		return false;
}

int SexyAppBase::GetInteger(const std::string& theId, int theDefault)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);	
	
	if (anItr != mIntProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

double SexyAppBase::GetDouble(const std::string& theId)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);
	DBG_ASSERTE(anItr != mDoubleProperties.end());
	
	if (anItr != mDoubleProperties.end())	
		return anItr->second;
	else
		return false;
}

double SexyAppBase::GetDouble(const std::string& theId, double theDefault)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);	
	
	if (anItr != mDoubleProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

SexyString SexyAppBase::GetString(const std::string& theId)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);
	DBG_ASSERTE(anItr != mStringProperties.end());
	
	if (anItr != mStringProperties.end())	
		return WStringToSexyString(anItr->second);
	else
		return __S("");
}

SexyString SexyAppBase::GetString(const std::string& theId, const SexyString& theDefault)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);	
	
	if (anItr != mStringProperties.end())	
		return WStringToSexyString(anItr->second);
	else
		return theDefault;	
}

StringVector SexyAppBase::GetStringVector(const std::string& theId)
{
	StringStringVectorMap::iterator anItr = mStringVectorProperties.find(theId);
	DBG_ASSERTE(anItr != mStringVectorProperties.end());
	
	if (anItr != mStringVectorProperties.end())	
		return anItr->second;
	else
		return StringVector();
}

void SexyAppBase::SetString(const std::string& theId, const std::wstring& theValue)
{
	std::pair<StringWStringMap::iterator, bool> aPair = mStringProperties.insert(StringWStringMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}


void SexyAppBase::SetBoolean(const std::string& theId, bool theValue)
{
	std::pair<StringBoolMap::iterator, bool> aPair = mBoolProperties.insert(StringBoolMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetInteger(const std::string& theId, int theValue)
{
	std::pair<StringIntMap::iterator, bool> aPair = mIntProperties.insert(StringIntMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetDouble(const std::string& theId, double theValue)
{
	std::pair<StringDoubleMap::iterator, bool> aPair = mDoubleProperties.insert(StringDoubleMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::DoParseCmdLine()
{
	/*
	char* aCmdLine = GetCommandLineA();	
	char* aCmdLinePtr = aCmdLine;
	if (aCmdLinePtr[0] == '"')
	{
		aCmdLinePtr = strchr(aCmdLinePtr + 1, '"');
		if (aCmdLinePtr != NULL)
			aCmdLinePtr++;
	}
	
	if (aCmdLinePtr != NULL)
	{
		aCmdLinePtr = strchr(aCmdLinePtr, ' ');
		if (aCmdLinePtr != NULL)
			ParseCmdLine(aCmdLinePtr+1);	
	}
	*/

	mCmdLineParsed = true;
}

void SexyAppBase::ParseCmdLine(const std::string& theCmdLine)
{
	// Command line example:  -play -demofile="game demo.dmo"
	// Results in HandleCmdLineParam("-play", ""); HandleCmdLineParam("-demofile", "game demo.dmo");
	std::string aCurParamName;
	std::string aCurParamValue;

	//int aSpacePos = 0; // Unused
	bool inQuote = false;
	bool onValue = false;

	for (size_t i = 0; i < theCmdLine.length(); i++)
	{
		char c = theCmdLine[i];
		bool atEnd = false;
		
		if (c == '"')
		{
			inQuote = !inQuote;

			if (!inQuote)
				atEnd = true;
		}
		else if ((c == ' ') && (!inQuote))
			atEnd = true;
		else if (c == '=')
			onValue = true;
		else if (onValue)
			aCurParamValue += c;
		else
			aCurParamName += c;
		
		if (i == theCmdLine.length() - 1)
			atEnd = true;
		
		if (atEnd && !aCurParamName.empty())
		{
			HandleCmdLineParam(aCurParamName, aCurParamValue);
			aCurParamName = "";
			aCurParamValue = "";
			onValue = false;
		}	
	}
}

static int GetMaxDemoFileNum(const std::string& theDemoPrefix, int theMaxToKeep, bool doErase)
{
	/*
	WIN32_FIND_DATAA aData;
	HANDLE aHandle = FindFirstFileA((theDemoPrefix + "*.dmo").c_str(), &aData);
	if (aHandle==INVALID_HANDLE_VALUE)
		return 0;

	typedef std::set<int> IntSet;
	IntSet aSet;

	do {
		int aNum = 0;
		if (sscanf(aData.cFileName,(theDemoPrefix + "%d.dmo").c_str(), &aNum)==1)
			aSet.insert(aNum);

	} while(FindNextFileA(aHandle,&aData));
	FindClose(aHandle);

	IntSet::iterator anItr = aSet.begin();
	if ((int)aSet.size()>theMaxToKeep-1 && doErase)
		DeleteFile(StrFormat((theDemoPrefix + "%d.dmo").c_str(),*anItr).c_str());

	if (aSet.empty())
		return 0;

	anItr = aSet.end();
	--anItr;
	return (*anItr);
	*/
	return 0;
}

void SexyAppBase::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-play")
	{
		mPlayingDemoBuffer = true;
		mRecordingDemoBuffer = false;
	}
	else if (theParamName == "-recnum")
	{
		int aNum = atoi(theParamValue.c_str());
		if (aNum<=0)
			aNum=5;

		int aDemoFileNum = GetMaxDemoFileNum(mDemoPrefix, aNum, true) + 1;
		mDemoFileName = SexyStringToString(StrFormat(StringToSexyString(mDemoPrefix + "%d.dmo").c_str(),aDemoFileNum));
		if (mDemoFileName.length() < 2)
		{
			mDemoFileName = GetAppDataFolder() + mDemoFileName;
		}
		mRecordingDemoBuffer = true;
		mPlayingDemoBuffer = false;
	}
	else if (theParamName == "-playnum")
	{
		int aNum = atoi(theParamValue.c_str())-1;
		if (aNum<0)
			aNum=0;

		int aDemoFileNum = GetMaxDemoFileNum(mDemoPrefix, aNum, false)-aNum;
		mDemoFileName = SexyStringToString(StrFormat(StringToSexyString(mDemoPrefix + "%d.dmo").c_str(),aDemoFileNum));
		mRecordingDemoBuffer = false;
		mPlayingDemoBuffer = true;
	}
	else if (theParamName == "-record")
	{
		mRecordingDemoBuffer = true;
		mPlayingDemoBuffer = false;
	}
	else if (theParamName == "-demofile")
	{
		mDemoFileName = theParamValue;
		if (mDemoFileName.length() < 2)
		{
			mDemoFileName = GetAppDataFolder() + mDemoFileName;
		}
	}	
	else if (theParamName == "-crash")
	{
		// Try to access NULL
		char* a = 0;
		*a = '!';		
	}
	else if (theParamName == "-screensaver")
	{
		mIsScreenSaver = true;
	}
	else if (theParamName == "-changedir")
	{
		mChangeDirTo = theParamValue;
	}
	else
	{
		Popup(GetString("INVALID_COMMANDLINE_PARAM", __S("Invalid command line parameter: ")) + StringToSexyString(theParamName));
		DoExit(0);
	}
}

void SexyAppBase::PreDisplayHook()
{
}

void SexyAppBase::PreGLInterfaceInitHook()
{
}

void SexyAppBase::PostGLInterfaceInitHook()
{
}

bool SexyAppBase::ChangeDirHook(const char *theIntendedPath)
{
	(void)theIntendedPath;
	return false;
}

MusicInterface* SexyAppBase::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new DummyMusicInterface();
#ifdef PS2_PLATFORM
	return new DummyMusicInterface();
#else
	return new SDLMusicInterface();
#endif
}

void SexyAppBase::InitPropertiesHook()
{
}

void SexyAppBase::InitHook()
{
}

void SexyAppBase::Init()
{
	mPrimaryThreadId = (void*)pthread_self();	
	
	if (mShutdown)
		return;

	/*
	if (gDDrawDLL==NULL || gDSoundDLL==NULL)
	{
		MessageBox(NULL, 
						GetString("APP_REQUIRES_DIRECTX", __S("This application requires DirectX to run.  You can get DirectX at http://www.microsoft.com/directx")).c_str(),
						GetString("YOU_NEED_DIRECTX", __S("You need DirectX")).c_str(), 
						MB_OK | MB_ICONERROR);
		DoExit(0);
	}
	*/

	InitPropertiesHook();
	ReadFromRegistry();	

	/*
	if (CheckForVista())
	{
		HMODULE aMod;
		SHGetFolderPathFunc aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath("shell32.dll", &aMod);
		if (aFunc == NULL || aMod == NULL)
			// SHGetFolderPathFunc is shadowed!
			// SHGetFolderPathFunc aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath("shfolder.dll", &aMod);
			aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath("shfolder.dll", &aMod);

		if (aMod != NULL)
		{
			char aPath[MAX_PATH];
			aFunc(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, aPath);

			std::string aDataPath = RemoveTrailingSlash(aPath) + "/" + mFullCompanyName + "/" + mProdName;
			SetAppDataFolder(aDataPath + "/");
			//MkDir(aDataPath);
			//AllowAllAccess(aDataPath);
			if (mDemoFileName.length() < 2)
			{
				mDemoFileName = GetAppDataFolder() + mDemoFileName;
			}

			FreeLibrary(aMod);
		}
	}
	*/
	
	if (!mCmdLineParsed)
		mCmdLineParsed = true;

	if (IsScreenSaver())	
		mOnlyAllowOneCopyToRun = false;	


	//if(gHInstance==NULL)
		//gHInstance = (HINSTANCE)GetModuleHandle(NULL);

	// Change directory
	if (!ChangeDirHook(mChangeDirTo.c_str()))
		chdir(mChangeDirTo.c_str());

	char aPath[512];
#ifdef PS2_PLATFORM
	// App data = the ELF's own folder, so everything lands in
	// <elf dir>/userdata/ (log.txt, saves) exactly like the original PC game —
	// no extra savedata/ level. On a real console getcwd can fail or return a
	// path with no device prefix (newlib's cwd depends on how the launcher
	// passed argv[0]); blindly strcat'ing onto the uninitialized buffer used
	// to poison every save path with stack garbage. Keep the boot-time save
	// prefix from Ps2PvzServices when there is no usable cwd.
	aPath[0] = '\0';
	// On a CD/DVD boot cwd is the read-only disc (cdrom0:): using it as the app
	// data folder sends log.txt and every save to media that cannot be written.
	// Ps2PvzServices already chose a writable save prefix (mass: -> mc0:) and
	// main() applied it; keep it. Only adopt cwd when we are NOT booting from a
	// read-only disc (host:/USB, where cwd is the writable ELF folder).
	if (Ps2GetResourcePrefix()[0] == '\0' &&
	    getcwd(aPath, sizeof(aPath) - 16) != NULL && strchr(aPath, ':') != NULL)
	{
		strcat(aPath, "/");
		SetAppDataFolder(aPath);
	}
#else
	getcwd(aPath, 512);
	strcat(aPath, "/savedata/");
	SetAppDataFolder(aPath);
#endif

	gPakInterface->AddPakFile("main.pak");

	// Create a message we can use to talk to ourselves inter-process
	//mNotifyGameMessage = RegisterWindowMessage((__S("Notify") + StringToSexyString(mProdName)).c_str());

	// Create a globally unique mutex
	/*
	mMutex = CreateMutex(NULL, TRUE, (StringToSexyString(mProdName) + __S("Mutex")).c_str());
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		HandleGameAlreadyRunning();
	*/

	mRandSeed = SDL_GetTicks();
	SRand(mRandSeed);

	// Set up demo recording stuff
	if (mPlayingDemoBuffer)
	{
		std::string anError;
		if (!ReadDemoBuffer(anError))
		{
			mPlayingDemoBuffer = false;
			Popup(anError);
			DoExit(0);
		}
	}

	srand(SDL_GetTicks());

	mIsWideWindow = sizeof(SexyChar) == sizeof(wchar_t);

	/*
	WNDCLASS wc;
	wc.style = CS_DBLCLKS;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = NULL;
	wc.hCursor = NULL;
	wc.hIcon = ::LoadIconA(gHInstance, "IDI_MAIN_ICON");
	wc.hInstance = gHInstance;
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = __S("MainWindow");
	wc.lpszMenuName = NULL;	
	bool success = RegisterClass(&wc) != 0;
	(void)success; // success unused in release mode
	DBG_ASSERTE(success);

	wc.style = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = NULL;
	wc.hCursor = NULL;
	wc.hIcon = NULL;
	wc.hInstance = gHInstance;
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = __S("InvisWindow");
	wc.lpszMenuName = NULL;	
	success = RegisterClass(&wc) != 0;
	DBG_ASSERTE(success);
	*/

	/*
	mInvisHWnd = CreateWindowEx(
			0,
			__S("InvisWindow"),
			mTitle.c_str(),
			0,
			0,
			0,
			0,
			0,
			NULL,
			NULL,
			gHInstance,
			0);	
	SetWindowLongPtr(mInvisHWnd, GWLP_USERDATA, (intptr_t) this);
	*/
		
	//mHandCursor = CreateCursor(gHInstance, 11, 4, 32, 32, gFingerCursorData, gFingerCursorData+sizeof(gFingerCursorData)/2); 
	//mDraggingCursor = CreateCursor(gHInstance, 15, 10, 32, 32, gDraggingCursorData, gDraggingCursorData+sizeof(gDraggingCursorData)/2); 
		
	// Let app do something before showing window, or switching to fullscreen mode
	// NOTE: Moved call to PreDisplayHook above mIsWindowed and GetSystemsMetrics
	// checks because the checks below use values that could change in PreDisplayHook.
	// PreDisplayHook must call mWidgetManager->Resize if it changes mWidth or mHeight.
	PreDisplayHook();

	mWidgetManager->Resize(Rect(0, 0, mWidth, mHeight), Rect(0, 0, mWidth, mHeight));

	/*
	// Check to see if we CAN run windowed or not...
	if (mIsWindowed && !mFullScreenWindow)
	{
		// How can we be windowed if our screen isn't even big enough?
		if ((mWidth >= GetSystemMetrics(SM_CXFULLSCREEN)) ||
			(mHeight >= GetSystemMetrics(SM_CYFULLSCREEN)))
		{
			mIsWindowed = false;
			mForceFullscreen = true;
		}
	}

	if (mFullScreenWindow) // change resoultion using ChangeDisplaySettings
	{
		EnumWindows(ChangeDisplayWindowEnumProc,0); // record window pos
		DEVMODE dm;
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm );

		// Switch resolutions
		if (dm.dmPelsWidth!=(unsigned int)mWidth || dm.dmPelsHeight!=(unsigned int)mHeight || (dm.dmBitsPerPel!=16 && dm.dmBitsPerPel!=32))
		{
			dm.dmPelsWidth = mWidth;
			dm.dmPelsHeight = mHeight;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

			if (dm.dmBitsPerPel!=16 && dm.dmBitsPerPel!=32) // handle 24-bit/256 color case
			{
				dm.dmBitsPerPel = 16;
				dm.dmFields |= DM_BITSPERPEL;
			}

			if (ChangeDisplaySettings(&dm,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
			{
				mFullScreenWindow = false;
				mIsWindowed = false;
			}
		}
	}
	*/

	MakeWindow();
		
	if (mPlayingDemoBuffer)
	{
		// Get video data
		
		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;
		
		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_VIDEO_DATA);

		mIsWindowed = mDemoBuffer.ReadBoolean();
		mSyncRefreshRate = mDemoBuffer.ReadByte();
	}

	if (mSoundManager == NULL)
#ifdef PS2_PLATFORM
		mSoundManager = new Ps2SoundManager();
#else
		mSoundManager = new SDLSoundManager();
#endif

	SetSfxVolume(mSfxVolume);
	
	mMusicInterface = CreateMusicInterface();	

	SetMusicVolume(mMusicVolume);	

	if (IsScreenSaver())
	{
		SetCursor(CURSOR_NONE);
	}

	InitHook();

	InitInput();

	mInitialized = true;
}

void SexyAppBase::HandleGameAlreadyRunning()
{
	if(mOnlyAllowOneCopyToRun)
	{
		/*
		// Notify the other window and then shut ourselves down
		if (mNotifyGameMessage != 0)
			PostMessage(HWND_BROADCAST, mNotifyGameMessage, 0, 0);
		*/

		DoExit(0);
	}
}

void SexyAppBase::CopyToClipboard(const std::string& theString)
{
	if (mPlayingDemoBuffer)
		return;

	/*
	HGLOBAL				aGlobalHandle;
	char*				theData;	
	WCHAR*				theWData;

	if (OpenClipboard(mHWnd))
	{
		EmptyClipboard();

		aGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, theString.length()+1);
		theData = (char*) GlobalLock(aGlobalHandle);
		strcpy(theData, theString.c_str());
		GlobalUnlock(aGlobalHandle);
		SetClipboardData(CF_TEXT, aGlobalHandle);
		SetClipboardData(CF_OEMTEXT, aGlobalHandle);		
		SetClipboardData(CF_LOCALE, aGlobalHandle);

		int aSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, theString.c_str(), theString.length(), NULL, 0);
		aGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (aSize + 1) * sizeof(WCHAR));
		theWData = (WCHAR*) GlobalLock(aGlobalHandle);
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, theString.c_str(), theString.length(), theWData, aSize);
		theWData[aSize] = '\0';
		GlobalUnlock(aGlobalHandle);
		SetClipboardData(CF_UNICODETEXT, aGlobalHandle);
		
		CloseClipboard();
	}
	*/
}

std::string	SexyAppBase::GetClipboard()
{
	/*
	HGLOBAL				aGlobalHandle;	
	std::string			aString;

	if (!mPlayingDemoBuffer)
	{
		if (OpenClipboard(mHWnd))
		{
			aGlobalHandle = GetClipboardData(CF_TEXT);	
			if (aGlobalHandle != NULL)
			{
				char* theData = (char*) GlobalLock(aGlobalHandle);
				if (theData != NULL)
				{
					aString = theData;		
					GlobalUnlock(aGlobalHandle);
				}
			}
			
			CloseClipboard();
		}
	}

	DemoSyncString(&aString);

	return aString;
	*/
	return "";
}

void SexyAppBase::SetCursor(int theCursorNum)
{
	mCursorNum = theCursorNum;
	EnforceCursor();
}

int SexyAppBase::GetCursor()
{
	return mCursorNum;
}

void SexyAppBase::EnableCustomCursors(bool enabled)
{
	mCustomCursorsEnabled = enabled;
	EnforceCursor();
}

Sexy::GLImage* SexyAppBase::GetImage(const std::string& theFileName, bool commitBits)
{
#ifdef SEXY_LAZY_IMAGES
	// Create a dimension-only stub; pixels decode from the pak on the first
	// GetBits() (software blit or texture upload) and can be purged again at
	// screen transitions. Falls through to an eager load if the header can't
	// be parsed.
	int aLazyWidth, aLazyHeight;
	Ps2IoLockAcquire();
	bool aLazyDimsOk = ImageLib::GetImageDims(theFileName, aLazyWidth, aLazyHeight);
	Ps2IoLockRelease();
	if (aLazyDimsOk)
	{
		GLImage* aStubImage = new GLImage(mGLInterface);
		aStubImage->mFilePath = theFileName;
		aStubImage->mWidth = aLazyWidth;
		aStubImage->mHeight = aLazyHeight;
		// Conservative until decoded; CommitBits() fixes these after decode.
		aStubImage->mHasTrans = true;
		aStubImage->mHasAlpha = true;
		aStubImage->mLazyUnloaded = true;
		return aStubImage;
	}
#endif

	ImageLib::Image* aLoadedImage = ImageLib::GetImage(theFileName, true);

	if (aLoadedImage == NULL)
		return NULL;

	GLImage* anImage = new GLImage(mGLInterface);
	anImage->mFilePath = theFileName;
#ifdef PS2_PLATFORM
	if (aLoadedImage->mBits == NULL &&
		(aLoadedImage->mColorTable != NULL || aLoadedImage->mRGBBits != NULL))
	{
		anImage->AdoptCompactBits(aLoadedImage->mColorTable, aLoadedImage->mColorIndices,
			aLoadedImage->mRGBBits, aLoadedImage->GetWidth(), aLoadedImage->GetHeight(),
			aLoadedImage->mHasTrans, aLoadedImage->mHasAlpha, commitBits);
		anImage->mFilePath = theFileName;
		aLoadedImage->mColorTable = NULL;
		aLoadedImage->mColorIndices = NULL;
		aLoadedImage->mRGBBits = NULL;
		delete aLoadedImage;
		return anImage;
	}
#endif
	anImage->SetBits(aLoadedImage->GetBits(), aLoadedImage->GetWidth(), aLoadedImage->GetHeight(), commitBits);
	anImage->mFilePath = theFileName;
	delete aLoadedImage;

	return anImage;
}

#ifdef SEXY_LAZY_IMAGES
// Lazy decodes are serialized with the global SIF RPC / pak IO lock: the
// loading thread and the main (render) thread can both fault in images, and
// neither the pak reader nor SIF RPC is thread-safe. The lock is recursive.
bool SexyAppBase::LoadLazyImageBits(MemoryImage* theImage)
{
	// Before pulling more pixels in, shed other images' decoded bits if the
	// heap is close to the ceiling. Must run before taking Ps2IoLock: the
	// purge takes mCritSect then Ps2IoLock, and taking them in the opposite
	// order here could deadlock against a concurrent purge.
	if (theImage->mLazyUnloaded && IsHeapUnderPressure())
		PurgeLazyImageBits();

	// Canonical lock order is mCritSect -> Ps2IoLock (same as the purges).
	// The decode below can hit Ps2AllocImageBits' OOM fallback, which purges
	// (mCritSect) from inside the IO lock; without holding mCritSect first
	// that inverts the order and deadlocks against a concurrent purge or a
	// draw-thread fault. Taking it here keeps every path in the same order.
	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	Ps2IoLockAcquire();

	if (!theImage->mLazyUnloaded)
	{
		Ps2IoLockRelease();
		return theImage->mBits != NULL;
	}

	ImageLib::Image* aLoadedImage = ImageLib::GetImage(theImage->mFilePath, true);
	if (aLoadedImage == NULL)
	{
		printf("[LAZY] decode FAILED '%s'\n", theImage->mFilePath.c_str());
		theImage->mLazyUnloaded = false;
		Ps2IoLockRelease();
		return false;
	}

	int aWidth = aLoadedImage->GetWidth();
	int aHeight = aLoadedImage->GetHeight();
	printf("[LAZY] decoded '%s' %dx%d\n", theImage->mFilePath.c_str(), aWidth, aHeight);

#ifdef PS2_PLATFORM
	if (aLoadedImage->mBits == NULL &&
		(aLoadedImage->mColorTable != NULL || aLoadedImage->mRGBBits != NULL))
	{
		theImage->AdoptCompactBits(aLoadedImage->mColorTable, aLoadedImage->mColorIndices,
			aLoadedImage->mRGBBits, aWidth, aHeight,
			aLoadedImage->mHasTrans, aLoadedImage->mHasAlpha, false);
		aLoadedImage->mColorTable = NULL;
		aLoadedImage->mColorIndices = NULL;
		aLoadedImage->mRGBBits = NULL;
	}
	else
#endif
	{
		int aSize = aWidth * aHeight;
		// Not SetBits(): with mBits==NULL and matching dimensions SetBits would
		// memcpy into a null buffer, and it would double-buffer the copy.
		delete [] theImage->mBits;
		theImage->mBits = new (std::nothrow) uint32_t[aSize + 1];
		if (theImage->mBits == NULL)
		{
			printf("[LAZY] OOM decoding '%s' (%dx%d) — dropping decode\n",
				theImage->mFilePath.c_str(), aWidth, aHeight);
			delete aLoadedImage;
			Ps2IoLockRelease();
			PurgeLazyImageBits(true);
			return false;
		}
		memcpy(theImage->mBits, aLoadedImage->GetBits(), aSize * sizeof(uint32_t));
		theImage->mBits[aSize] = MEMORYCHECK_ID;
		theImage->mWidth = aWidth;
		theImage->mHeight = aHeight;
		theImage->BitsChanged();
		theImage->CommitBits();
		// Images that arrive 32-bit here were expanded by the alpha-companion
		// compose in ImageLib::GetImage even when the source PNG was indexed.
		// Re-quantizing recovers the 8-bit+CLUT representation (1B/px in EE
		// RAM, PSMT8 in GS VRAM); fails cleanly past 256 unique RGBA values.
		theImage->Palletize();
	}
	delete aLoadedImage;

	theImage->mLazyUnloaded = false;
	theImage->mLazyCleanCount = theImage->mBitsChangedCount;
	printf("[LAZY] committed '%s'\n", theImage->mFilePath.c_str());

	Ps2IoLockRelease();
	return true;
}

// Frees the decoded bits of one lazy image (and optionally its texture); it
// re-decodes from the pak on the next use. theKeepTexture leaves the GS
// texture and the GL shim's CPU copy alive, which is correct while the image
// keeps being drawn: the shim re-uploads from its own copy after VRAM rewinds
// and only a BitsChanged() forces a texture rebuild.
bool SexyAppBase::PurgeLazyImage(MemoryImage* theImage, bool theKeepTexture)
{
	if (theImage->mFilePath.empty() || theImage->mLazyPinned)
		return false;
	if (theImage->mLazyUnloaded)
	{
		if (!theKeepTexture && theImage->mD3DData != NULL)
		{
			Remove3DData(theImage);
			theImage->mD3DFlags |= theImage->mLazyRestoreD3DFlags;
			return true;
		}
		return false;
	}
	if (theImage->mBitsChangedCount != theImage->mLazyCleanCount)
		return false;  // never decoded lazily, or modified at runtime
	if (theImage->mBits == NULL && theImage->mColorIndices == NULL
#ifdef PS2_PLATFORM
		&& theImage->mRGBBits == NULL
#endif
		)
		return false;

	delete [] theImage->mBits;
	theImage->mBits = NULL;
#ifdef PS2_PLATFORM
	delete [] theImage->mRGBBits;
	theImage->mRGBBits = NULL;
#endif
	delete [] theImage->mColorIndices;
	theImage->mColorIndices = NULL;
	delete [] theImage->mColorTable;
	theImage->mColorTable = NULL;
	delete [] theImage->mNativeAlphaData;
	theImage->mNativeAlphaData = NULL;
	delete [] theImage->mRLAlphaData;
	theImage->mRLAlphaData = NULL;
	delete [] theImage->mRLAdditiveData;
	theImage->mRLAdditiveData = NULL;

	if (!theKeepTexture)
	{
		Remove3DData(theImage);  // releases texture pieces -> GS VRAM + shim copies

		// Re-arm one-shot post-processing (e.g. alpha-edge sanding) for the
		// next decode+upload. Only when the texture died too: with a live
		// texture a mD3DFlags change would make CheckCreateTextures rebuild
		// it on every draw.
		theImage->mD3DFlags |= theImage->mLazyRestoreD3DFlags;
	}
	theImage->mLazyUnloaded = true;
	return true;
}

void SexyAppBase::PurgeLazyImages()
{
	// Texture teardown mutates the GL shim's texture table and the VRAM
	// arena, which the primary thread touches without mCritSect while it
	// draws (CheckCreateTextures runs mid-Blt). Off-thread callers — the
	// loading thread's OOM fallbacks in Ps2AllocImageBits/ReanimAtlas — only
	// get the CPU-bits purge, which is safe from any thread.
	if ((mPrimaryThreadId != 0) && ((void*)pthread_self() != mPrimaryThreadId))
	{
		printf("[LAZY] full purge requested off-thread; purging bits only\n");
		PurgeLazyImageBits(true);
		return;
	}

	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	Ps2IoLockAcquire();

	int aPurged = 0;
	for (MemoryImageSet::iterator anItr = mMemoryImageSet.begin(); anItr != mMemoryImageSet.end(); ++anItr)
	{
		if (PurgeLazyImage(*anItr, false))
			aPurged++;
	}

	Ps2IoLockRelease();
	printf("[LAZY] purged %d images\n", aPurged);
}

// EE heap tops out around 26MB (32MB minus load address, ELF image and
// stack); past ~23MB in use the next big decode can fail and a failed
// allocation here is a hard freeze, not an error dialog.
bool SexyAppBase::IsHeapUnderPressure()
{
	struct mallinfo mi = mallinfo();
	// 21MB, not 23: atlas creation needs ~1MB contiguous on top of the decode
	// in flight, and by 23MB in use those allocations were already failing.
	return (long)mi.uordblks > (21L << 20);
}

// Pressure valve: drop only the decoded CPU bits of lazy images but keep
// their GS textures and shim copies, so anything currently on screen keeps
// drawing with no redecode thrash. Software blits fault the bits back in.
int SexyAppBase::PurgeLazyImageBits(bool theForce)
{
	if (!theForce && !IsHeapUnderPressure())
		return 0;

	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	Ps2IoLockAcquire();

	int aPurged = 0;
	for (MemoryImageSet::iterator anItr = mMemoryImageSet.begin(); anItr != mMemoryImageSet.end(); ++anItr)
	{
		if (PurgeLazyImage(*anItr, true))
			aPurged++;
	}

	Ps2IoLockRelease();

	struct mallinfo mi = mallinfo();
	printf("[LAZY] pressure purge: %d image bits freed, used=%.2fMB\n",
		aPurged, (double)mi.uordblks / (1024.0 * 1024.0));
	return aPurged;
}
#else
bool SexyAppBase::LoadLazyImageBits(MemoryImage*) { return false; }
bool SexyAppBase::PurgeLazyImage(MemoryImage*, bool) { return false; }
void SexyAppBase::PurgeLazyImages() { }
bool SexyAppBase::IsHeapUnderPressure() { return false; }
int SexyAppBase::PurgeLazyImageBits(bool) { return 0; }
#endif

Sexy::GLImage* SexyAppBase::CreateCrossfadeImage(Sexy::Image* theImage1, const Rect& theRect1, Sexy::Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	MemoryImage* aMemoryImage1 = dynamic_cast<MemoryImage*>(theImage1);
	MemoryImage* aMemoryImage2 = dynamic_cast<MemoryImage*>(theImage2);

	if ((aMemoryImage1 == NULL) || (aMemoryImage2 == NULL))
		return NULL;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) || 
		(theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect1 out of bounds");
		return NULL;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) || 
		(theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect2 out of bounds");
		return NULL;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	GLImage* anImage = new GLImage(mGLInterface);
	anImage->Create(aWidth, aHeight);

	uint32_t* aDestBits = anImage->GetBits();
	uint32_t* aSrcBits1 = aMemoryImage1->GetBits();
	uint32_t* aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	uint32_t aMult = (int) (theFadeFactor*256);
	uint32_t aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		uint32_t* s1 = &aSrcBits1[(y+theRect1.mY)*aSrc1Width+theRect1.mX];
		uint32_t* s2 = &aSrcBits2[(y+theRect2.mY)*aSrc2Width+theRect2.mX];
		uint32_t* d = &aDestBits[y*aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			uint32_t p1 = *s1++;
			uint32_t p2 = *s2++;

			//p1 = 0;
			//p2 = 0xFFFFFFFF;

			*d++ = 
				((((p1 & 0x000000FF)*aOMM + (p2 & 0x000000FF)*aMult)>>8) & 0x000000FF) |
				((((p1 & 0x0000FF00)*aOMM + (p2 & 0x0000FF00)*aMult)>>8) & 0x0000FF00) |
				((((p1 & 0x00FF0000)*aOMM + (p2 & 0x00FF0000)*aMult)>>8) & 0x00FF0000) |
				((((p1 >> 24)*aOMM + (p2 >> 24)*aMult)<<16) & 0xFF000000);
		}
	}

	anImage->BitsChanged();
	
	return anImage;
}

void SexyAppBase::ColorizeImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return;

	uint32_t* aBits;	
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aBits = aSrcMemoryImage->GetBits();		
		aNumColors = theImage->GetWidth()*theImage->GetHeight();				
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;		
		aNumColors = 256;				
	}
						
	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && 
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aBits[i];

			aBits[i] = 
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}	

	aSrcMemoryImage->BitsChanged();
}

GLImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return NULL;

	GLImage* anImage = new GLImage(mGLInterface);
	
	anImage->Create(theImage->GetWidth(), theImage->GetHeight());
	
	uint32_t* aSrcBits;
	uint32_t* aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();				
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new uint32_t[256];
		aNumColors = 256;
		
		anImage->mColorIndices = new uchar[anImage->mWidth*theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth*theImage->mHeight);
	}
						
	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && 
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aSrcBits[i];

			aDestBits[i] = 
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}	

	anImage->BitsChanged();

	return anImage;
}

GLImage* SexyAppBase::CopyImage(Image* theImage, const Rect& theRect)
{
	GLImage* anImage = new GLImage(mGLInterface);

	anImage->Create(theRect.mWidth, theRect.mHeight);
	
	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

GLImage* SexyAppBase::CopyImage(Image* theImage)
{
	return CopyImage(theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

void SexyAppBase::MirrorImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);	

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		uint32_t* aLeftBits = aSrcBits + (y * aPhysSrcWidth);		
		uint32_t* aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			uint32_t aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();	
}

void SexyAppBase::FlipImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		uint32_t* aTopBits    = aSrcBits + x;
		uint32_t* aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			uint32_t aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();	
}

void SexyAppBase::RotateImageHue(Sexy::MemoryImage *theImage, int theDelta)
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	uint32_t *aPtr = theImage->GetBits();
	for (int i=0; i<aSize; i++)
	{
		uint32_t aPixel = *aPtr;
		int alpha = aPixel&0xff000000;
		int r = (aPixel>>16)&0xff;
		int g = (aPixel>>8) &0xff;
		int b = aPixel&0xff;

		int maxval = std::max(r, std::max(g, b));
		int minval = std::min(r, std::min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval+maxval)/2;
		int delta = maxval - minval;

		if (delta != 0)
		{			
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));
			
			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));
			
			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v= (l < 128) ? (l * (255+s))/255 :
				(l+s-l*s/255);
		
		int y = (int) (2*l-v);

		int aColorDiv = (6 * h) / 256;
		int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (x > 255)
			x = 255;

		int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (z < 0)
			z = 0;
		
		switch (aColorDiv)
		{
			case 0: r = (int) v; g = x; b = y; break;
			case 1: r = z; g= (int) v; b = y; break;
			case 2: r = y; g= (int) v; b = x; break;
			case 3: r = y; g = z; b = (int) v; break;
			case 4: r = x; g = y; b = (int) v; break;
			case 5: r = (int) v; g = y; b = z; break;
			default: r = (int) v; g = x; b = y; break;
		}

		*aPtr++ = alpha | (r<<16) | (g << 8) | (b);	 

	}

	theImage->BitsChanged();
}

uint32_t SexyAppBase::HSLToRGB(int h, int s, int l)
{
	int r;
	int g;
	int b;

	double v= (l < 128) ? (l * (255+s))/255 :
			(l+s-l*s/255);
	
	int y = (int) (2*l-v);

	int aColorDiv = (6 * h) / 256;
	int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (x > 255)
		x = 255;

	int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (z < 0)
		z = 0;
	
	switch (aColorDiv)
	{
		case 0: r = (int) v; g = x; b = y; break;
		case 1: r = z; g= (int) v; b = y; break;
		case 2: r = y; g= (int) v; b = x; break;
		case 3: r = y; g = z; b = (int) v; break;
		case 4: r = x; g = y; b = (int) v; break;
		case 5: r = (int) v; g = y; b = z; break;
		default: r = (int) v; g = x; b = y; break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

uint32_t SexyAppBase::RGBToHSL(int r, int g, int b)
{					
	int maxval = std::max(r, std::max(g, b));
	int minval = std::min(r, std::min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval+maxval)/2;
	int delta = maxval - minval;

	if (delta != 0)
	{			
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));
		
		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));
		
		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);	 
}

void SexyAppBase::HSLToRGB(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void SexyAppBase::RGBToHSL(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}

void SexyAppBase::PrecacheAdditive(MemoryImage* theImage)
{
	theImage->GetRLAdditiveData(mGLInterface);
}

void SexyAppBase::PrecacheAlpha(MemoryImage* theImage)
{
	theImage->GetRLAlphaData();
}

void SexyAppBase::PrecacheNative(MemoryImage* theImage)
{
	theImage->GetNativeAlphaData(mGLInterface);
}


void SexyAppBase::PlaySample(int theSoundNum)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->Play(false, true);
	}
}


void SexyAppBase::PlaySample(int theSoundNum, int thePan)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->SetPan(thePan);
		aSoundInstance->Play(false, true);
	}
}

bool SexyAppBase::IsMuted()
{
	return mMuteCount > 0;
}

void SexyAppBase::Mute(bool autoMute)
{	
	mMuteCount++;
	if (autoMute)
		mAutoMuteCount++;

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

void SexyAppBase::Unmute(bool autoMute)
{	
	if (mMuteCount > 0)
	{
		mMuteCount--;
		if (autoMute)
			mAutoMuteCount--;
	}

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}


double SexyAppBase::GetMusicVolume()
{
	return mMusicVolume;
}

void SexyAppBase::SetMusicVolume(double theVolume)
{
	mMusicVolume = theVolume;

	if (mMusicInterface != NULL)
		mMusicInterface->SetVolume((mMuteCount > 0) ? 0.0 : mMusicVolume);

#ifdef PS2_PLATFORM
	// The PS2 music path streams OGGs outside the MusicInterface.
	Ps2MusicSetMasterVolume((mMuteCount > 0) ? 0.0 : mMusicVolume);
#endif
}

double SexyAppBase::GetSfxVolume()
{
	return mSfxVolume;
}

void SexyAppBase::SetSfxVolume(double theVolume)
{
	mSfxVolume = theVolume;

	if (mSoundManager != NULL)
		mSoundManager->SetVolume((mMuteCount > 0) ? 0.0 : mSfxVolume);
}

double SexyAppBase::GetMasterVolume()
{
	return mSoundManager->GetMasterVolume();
}

void SexyAppBase::SetMasterVolume(double theMasterVolume)
{
	mSfxVolume = theMasterVolume;
	mSoundManager->SetMasterVolume(mSfxVolume);
}

void SexyAppBase::AddMemoryImage(MemoryImage* theMemoryImage)
{
	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	mMemoryImageSet.insert(theMemoryImage);
}

void SexyAppBase::RemoveMemoryImage(MemoryImage* theMemoryImage)
{
	AutoCrit anAutoCrit(mGLInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.find(theMemoryImage);
	if (anItr != mMemoryImageSet.end())
		mMemoryImageSet.erase(anItr);

	Remove3DData(theMemoryImage);
}

void SexyAppBase::Remove3DData(MemoryImage* theMemoryImage)
{
	if (mGLInterface)
		mGLInterface->Remove3DData(theMemoryImage);
}


bool SexyAppBase::Is3DAccelerated()
{
	//return mDDInterface->mIs3D;
	return true;
}

bool SexyAppBase::Is3DAccelerationSupported()
{
	/*
	if (mDDInterface->mD3DTester)
		return mDDInterface->mD3DTester->Is3DSupported();
	else
		return false;
	*/
	return true;
}

bool SexyAppBase::Is3DAccelerationRecommended()
{
	/*
	if (mDDInterface->mD3DTester)
		return mDDInterface->mD3DTester->Is3DRecommended();
	else
		return false;
	*/
	return true;
}

void SexyAppBase::DemoSyncRefreshRate()
{
	mSyncRefreshRate = mGLInterface->mRefreshRate;

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_VIDEO_DATA, 5);
		mDemoBuffer.WriteBoolean(mIsWindowed);
		uchar aByte = (uchar) mSyncRefreshRate;
		mDemoBuffer.WriteByte(aByte);		
	}
}

void SexyAppBase::Set3DAcclerated(bool is3D, bool reinit)
{
	/*
	if (mDDInterface->mIs3D == is3D)
		return;

	mUserChanged3DSetting = true;
	mDDInterface->mIs3D = is3D;
	
	if (reinit)
	{
		int aResult = InitGLInterface();

		if (is3D && aResult != DDInterface::RESULT_OK)
		{
			Set3DAcclerated(false, reinit);
			return;
		}
		else if (aResult != DDInterface::RESULT_OK)
		{
			Popup(GetString("FAILED_INIT_DIRECTDRAW", __S("Failed to initialize DirectDraw: ")) + StringToSexyString(DDInterface::ResultToString(aResult) + " " + mDDInterface->mErrorString));
			DoExit(1);
		}

		ReInitImages();

		mWidgetManager->mImage = mDDInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
	*/
}

SharedImageRef SexyAppBase::SetSharedImage(const std::string& theFileName, const std::string& theVariant, GLImage* theImage, bool* isNew)
{
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;
	
	{
		AutoCrit anAutoCrit(mGLInterface->mCritSect);
		aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
	}

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		aSharedImageRef.mSharedImage->mImage = theImage;
	}

	return aSharedImageRef;
}

SharedImageRef SexyAppBase::GetSharedImage(const std::string& theFileName, const std::string& theVariant, bool* isNew)
{	
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mGLInterface->mCritSect);	
		aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
	}

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		// Pass in a '!' as the first char of the file name to create a new image
		if ((theFileName.length() > 0) && (theFileName[0] == '!'))
			aSharedImageRef.mSharedImage->mImage = new GLImage(mGLInterface);
		else
			aSharedImageRef.mSharedImage->mImage = GetImage(theFileName,false);
	}

	return aSharedImageRef;
}

void SexyAppBase::CleanSharedImages()
{
	AutoCrit anAutoCrit(mGLInterface->mCritSect);

	if (mCleanupSharedImages)
	{
		// Delete shared images with reference counts of 0
		// This doesn't occur in ~SharedImageRef because sometimes we can not only access the image
		//  through the SharedImageRef returned by GetSharedImage, but also by calling GetSharedImage
		//  again with the same params -- so we can have instances where we do the 'final' deref on
		//  an image but immediately re-request it via GetSharedImage
		SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
		while (aSharedImageItr != mSharedImageMap.end())
		{
			SharedImage* aSharedImage = &aSharedImageItr->second;
			if (aSharedImage->mRefCount == 0)
			{
				delete aSharedImage->mImage;
				mSharedImageMap.erase(aSharedImageItr++);
			}
			else
				++aSharedImageItr;
		}

		mCleanupSharedImages = false;
	}
}
