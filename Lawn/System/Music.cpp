#include "Music.h"
#include "../Board.h"
#include "PlayerInfo.h"
#include "../../LawnApp.h"
#include "paklib/PakInterface.h"
#include "../../Sexy.TodLib/TodDebug.h"
#include "../../Sexy.TodLib/TodCommon.h"
#ifdef PS2_PLATFORM
#include "sound/MusicInterface.h"
#include "platform/ps2/sound/Ps2SoundManager.h"
#include <map>
#include <stdlib.h>
#include <stdio.h>

struct Mix_Music {};
struct SDL_RWops {};
static inline SDL_RWops* SDL_RWFromMem(void*, int) { return 0; }
static inline Mix_Music* Mix_LoadMUS_RW(SDL_RWops*, int) { return 0; }
static inline void Mix_ModMusicStreamSetChannelVolume(Mix_Music*, int, int) {}
static inline void Mix_HaltMusicStream(Mix_Music*) {}
static inline void Mix_PlayMusicStream(Mix_Music*, int) {}
static inline void Mix_ModMusicStreamJumpToOrder(Mix_Music*, int) {}
static inline void Mix_VolumeMusicStream(Mix_Music*, int) {}

struct SDLMusicInfo
{
	Mix_Music* mHMusic;
	bool mStopOnFade;
	double mVolume;
	double mVolumeCap;
	double mVolumeAdd;

	SDLMusicInfo() : mHMusic(0), mStopOnFade(false), mVolume(0.0), mVolumeCap(1.0), mVolumeAdd(0.0) {}
};

typedef std::map<int, SDLMusicInfo> SDLMusicMap;

class SDLMusicInterface : public Sexy::MusicInterface
{
public:
	SDLMusicMap mMusicMap;

	virtual bool LoadMusic(int, const std::string&) { return false; }
	virtual void PlayMusic(int, int, bool) {}
	virtual void StopMusic(int) {}
	virtual void PauseMusic(int) {}
	virtual void ResumeMusic(int) {}
	virtual void StopAllMusic() {}
	virtual void UnloadMusic(int) {}
	virtual void UnloadAllMusic() {}
	virtual void PauseAllMusic() {}
	virtual void ResumeAllMusic() {}
	virtual void FadeIn(int, int, double, bool) {}
	virtual void FadeOut(int, bool, double) {}
	virtual void FadeOutAll(bool, double) {}
	virtual void SetSongVolume(int, double) {}
	virtual void SetSongMaxVolume(int, double) {}
	virtual bool IsPlaying(int) { return false; }
	virtual void SetVolume(double) {}
	virtual void SetMusicAmplify(int, double) {}
	virtual void Update() {}
	virtual int GetMusicOrder(int) { return 0; }
};
// ---------------------------------------------------------------------------
// OGG music mode: the PS2 build has no mod (.mo3) playback. If the user drops
// per-tune OGG files into sounds/ (PVZ soundtrack names, e.g. Grasswalk.ogg),
// every tune plays its own streamed OGG instead of a mainmusic.mo3 order.
// Drum/hihat layers and music bursts don't exist in this mode.
// ---------------------------------------------------------------------------

static bool gPs2OggMusicMode = false;
static bool gPs2OggMusicProbed = false;

// Candidate file names (without "sounds/" or ".ogg") accepted for each tune.
static const char* const* Ps2TuneOggNames(MusicTune theTune)
{
	static const char* const kTitle[]    = { "CrazyDave", "Crazy Dave (Intro Theme)", "Crazy Dave", "crazydave", "MainTheme", "maintheme", 0 };
	static const char* const kDay[]      = { "Grasswalk", "GrassWalk", "grasswalk", 0 };
	static const char* const kNight[]    = { "Moongrains", "MoonGrains", "moongrains", 0 };
	static const char* const kPool[]     = { "WateryGraves", "Watery Graves", "waterygraves", 0 };
	static const char* const kFog[]      = { "RigorMormist", "Rigor Mormist", "rigormormist", 0 };
	static const char* const kRoof[]     = { "GrazeTheRoof", "Graze the Roof", "grazetheroof", 0 };
	static const char* const kChooser[]  = { "ChooseYourSeeds", "Choose Your Seeds", "chooseyourseeds", 0 };
	static const char* const kZen[]      = { "ZenGarden", "Zen Garden", "zengarden", 0 };
	static const char* const kPuzzle[]   = { "Cerebrawl", "cerebrawl", 0 };
	static const char* const kMinigame[] = { "Loonboon", "loonboon", 0 };
	static const char* const kConveyer[] = { "UltimateBattle", "Ultimate Battle", "ultimatebattle", 0 };
	static const char* const kBoss[]     = { "BrainiacManiac", "Brainiac Maniac", "brainiacmaniac", 0 };
	static const char* const kCredits[]  = { "ZombiesOnYourLawn", "Zombies On Your Lawn", "zombiesonyourlawn", 0 };

	switch (theTune)
	{
	case MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME:	return kTitle;
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:				return kDay;
	case MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS:			return kNight;
	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:			return kPool;
	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:			return kFog;
	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:			return kRoof;
	case MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS:			return kChooser;
	case MusicTune::MUSIC_TUNE_ZEN_GARDEN:					return kZen;
	case MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL:			return kPuzzle;
	case MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON:			return kMinigame;
	case MusicTune::MUSIC_TUNE_CONVEYER:					return kConveyer;
	case MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC:	return kBoss;
	case MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN:return kCredits;
	default: return 0;
	}
}

static PFILE* Ps2OpenTuneOgg(MusicTune theTune, std::string& thePathOut)
{
	const char* const* aNames = Ps2TuneOggNames(theTune);
	if (aNames == 0)
		return 0;

	for (int i = 0; aNames[i] != 0; i++)
	{
		thePathOut = std::string("sounds/") + aNames[i] + ".ogg";
		PFILE* aFile = p_fopen(thePathOut.c_str(), "rb");
		if (aFile != 0)
			return aFile;
	}
	return 0;
}

// One-time detection: OGG music mode kicks in if the title theme (or, failing
// that, Grasswalk) is present in sounds/.
static bool Ps2OggMusicAvailable()
{
	if (!gPs2OggMusicProbed)
	{
		// The probe fopens run on the main thread at title time, with the
		// mixer live; park it so its audsrv RPCs can't collide with the fio
		// on real hardware.
		Sexy::Ps2MixerIoPauseScope aMixerPause;
		gPs2OggMusicProbed = true;
		std::string aPath;
		PFILE* aFile = Ps2OpenTuneOgg(MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME, aPath);
		if (aFile == 0)
			aFile = Ps2OpenTuneOgg(MusicTune::MUSIC_TUNE_DAY_GRASSWALK, aPath);
		if (aFile != 0)
		{
			p_fclose(aFile);
			gPs2OggMusicMode = true;
			printf("[PS2] music: OGG soundtrack detected ('%s')\n", aPath.c_str());
		}
		else
			printf("[PS2] music: no OGG soundtrack found, falling back to mo3 path\n");
	}
	return gPs2OggMusicMode;
}

// Reads the whole compressed OGG into RAM and hands it to the streamer.
static bool Ps2PlayTuneOgg(MusicTune theTune)
{
	// This is a multi-MB read (seconds over USB 1.1) that runs with the mixer
	// live — at the title, at the menu, and on every in-game tune change. Park
	// the mixer for the whole load: on real hardware its unserialized audsrv
	// RPCs colliding with such a long stretch of fio RPCs wedges the IOP.
	// (Also keeps Ps2MusicStop below safe: parked mixer => direct teardown.)
	Sexy::Ps2MixerIoPauseScope aMixerPause;

	std::string aPath;
	PFILE* aFile = Ps2OpenTuneOgg(theTune, aPath);
	if (aFile == 0 && theTune != MusicTune::MUSIC_TUNE_DAY_GRASSWALK)
	{
		// Missing track: fall back to the day theme rather than going silent.
		printf("[PS2] music: no OGG for tune %d, trying Grasswalk\n", (int)theTune);
		aFile = Ps2OpenTuneOgg(MusicTune::MUSIC_TUNE_DAY_GRASSWALK, aPath);
	}
	if (aFile == 0)
	{
		printf("[PS2] music: no OGG for tune %d\n", (int)theTune);
		Sexy::Ps2MusicStop();
		return false;
	}

	p_fseek(aFile, 0, SEEK_END);
	int aSize = p_ftell(aFile);
	p_fseek(aFile, 0, SEEK_SET);
	unsigned char* aData = (unsigned char*)malloc(aSize);
	if (aData == 0)
	{
		p_fclose(aFile);
		printf("[PS2] music: out of RAM for '%s' (%d bytes)\n", aPath.c_str(), aSize);
		return false;
	}
	p_fread(aData, 1, aSize, aFile);
	p_fclose(aFile);

	bool aNoLoop = theTune == MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN;
	return Sexy::Ps2MusicPlay(aData, aSize, !aNoLoop);  // takes ownership of aData
}
#else
#include "sound/SDLMusicInterface.h"
#endif

using namespace Sexy;

//0x45A260
Music::Music()
{
	mApp = (LawnApp*)gSexyAppBase;
	mMusicInterface = gSexyAppBase->mMusicInterface;
	mCurMusicTune = MusicTune::MUSIC_TUNE_NONE;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
	mBurstOverride = -1;
	mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
	mQueuedDrumTrackPackedOrder = -1;
	mBaseBPM = 155;
	mBaseModSpeed = 3;
	mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
	mPauseOffset = 0;
	mPauseOffsetDrums = 0;
	mPaused = false;
	mMusicDisabled = false;
	mFadeOutCounter = 0;
	mFadeOutDuration = 0;
}

MusicFileData gMusicFileData[MusicFile::NUM_MUSIC_FILES];  //0x6A9ED0

//0x45A2C0
bool Music::TodLoadMusic(MusicFile theMusicFile, const std::string& theFileName)
{
	Mix_Music* aHMusic = 0;
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface;
	std::string anExt;

	size_t aDot = theFileName.rfind('.');
	if (aDot != std::string::npos)  // 文件名中不含“.”（文件无扩展名）
		anExt = StringToLower(theFileName.substr(aDot + 1));  // 取得小写的文件扩展名

	PFILE* pFile = p_fopen(theFileName.c_str(), "rb");
	if (pFile == nullptr)
		return false;

	p_fseek(pFile, 0, SEEK_END);  // 指针调整至文件末尾
	int aSize = p_ftell(pFile);  // 当前位置即为文件长度
	p_fseek(pFile, 0, SEEK_SET);  // 指针调回文件开头
	void* aData = operator new[](aSize);
	p_fread(aData, sizeof(char), aSize, pFile);  // 按字节读取数据
	p_fclose(pFile);  // 关闭文件流

	aHMusic = Mix_LoadMUS_RW(SDL_RWFromMem(aData, aSize), 1);
	delete[] (char *)aData;

	if (aHMusic == 0)
		return false;

	/*
	if (anExt.compare("wav") && anExt.compare("ogg") && anExt.compare("mp3"))  // 如果不是这三种拓展名
	{
		aHMusic = gBass->BASS_MusicLoad(true, aData, 0, aSize, aBass->mMusicLoadFlags, 0);
		delete[] (char *)aData;

		if (aHMusic == 0)
			return false;
	}
	else
	{
		aStream = gBass->BASS_StreamCreateFile(true, aData, 0, aSize, 0);
		TOD_ASSERT(gMusicFileData[theMusicFile].mFileData == nullptr);
		gMusicFileData[theMusicFile].mFileData = (unsigned int*)aData;

		if (aStream == 0)
			return false;
	}
	*/
	
	SDLMusicInfo aMusicInfo;
	aMusicInfo.mHMusic = aHMusic;
	anSDL->mMusicMap.insert(SDLMusicMap::value_type(theMusicFile, aMusicInfo));  // 将目标音乐文件编号和音乐信息的对组加入音乐数据容器
	return true;
}

//0x45A6C0
void Music::SetupMusicFileForTune(MusicFile theMusicFile, MusicTune theMusicTune)
{
	int aTrackCount = 0;
	int aTrackStart1 = -1, aTrackEnd1 = -1, aTrackStart2 = -1, aTrackEnd2 = -1;

	switch (theMusicTune)
	{
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:
		switch (theMusicFile) {
		case MusicFile::MUSIC_FILE_MAIN_MUSIC:		aTrackCount = 29;	aTrackStart1 = 0;	aTrackEnd1 = 23;											break;
		case MusicFile::MUSIC_FILE_HIHATS:			aTrackCount = 29;	aTrackStart1 = 27;	aTrackEnd1 = 27;											break;
		case MusicFile::MUSIC_FILE_DRUMS:			aTrackCount = 29;	aTrackStart1 = 24;	aTrackEnd1 = 26;											break;
		default: break;
		} break;
	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:
		switch (theMusicFile) {
		case MusicFile::MUSIC_FILE_MAIN_MUSIC:		aTrackCount = 29;	aTrackStart1 = 0;	aTrackEnd1 = 17;											break;
		case MusicFile::MUSIC_FILE_HIHATS:			aTrackCount = 29;	aTrackStart1 = 18;	aTrackEnd1 = 24;	aTrackStart2 = 29;	aTrackEnd2 = 29;	break;
		case MusicFile::MUSIC_FILE_DRUMS:			aTrackCount = 29;	aTrackStart1 = 25;	aTrackEnd1 = 28;											break;
		default: break;
		} break;
	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:
		switch (theMusicFile) {
		case MusicFile::MUSIC_FILE_MAIN_MUSIC:		aTrackCount = 29;	aTrackStart1 = 0;	aTrackEnd1 = 15;											break;
		case MusicFile::MUSIC_FILE_HIHATS:			aTrackCount = 29;	aTrackStart1 = 23;	aTrackEnd1 = 23;											break;
		case MusicFile::MUSIC_FILE_DRUMS:			aTrackCount = 29;	aTrackStart1 = 16;	aTrackEnd1 = 22;											break;
		default: break;
		} break;
	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:
		switch (theMusicFile) {
		case MusicFile::MUSIC_FILE_MAIN_MUSIC:		aTrackCount = 29;	aTrackStart1 = 0;	aTrackEnd1 = 17;											break;
		case MusicFile::MUSIC_FILE_HIHATS:			aTrackCount = 29;	aTrackStart1 = 21;	aTrackEnd1 = 21;											break;
		case MusicFile::MUSIC_FILE_DRUMS:			aTrackCount = 29;	aTrackStart1 = 18;	aTrackEnd1 = 20;											break;
		default: break;
		} break;
	default:
		if (theMusicFile == MusicFile::MUSIC_FILE_MAIN_MUSIC || theMusicFile == MusicFile::MUSIC_FILE_DRUMS)
		{
			aTrackCount = 29;
			aTrackStart1 = 0;
			aTrackEnd1 = 29;
		}
		break;
	}

	Mix_Music* aHMusic = GetMusicHandle(theMusicFile);
	for (int aTrack = 0; aTrack < aTrackCount; aTrack++)
	{
		float aVolume;
		if (aTrack >= aTrackStart1 && aTrack <= aTrackEnd1)
			aVolume = 1;
		else if (aTrack >= aTrackStart2 && aTrack <= aTrackEnd2)
			aVolume = 1;
		else
			aVolume = 0;

		Mix_ModMusicStreamSetChannelVolume(aHMusic, aTrack, (int)(aVolume*128));
		//gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_VOL_CHAN + aTrack, aVolume);  // 设置音乐每条轨道的音量属性（静音与否）
	}
}

void Music::LoadSong(MusicFile theMusicFile, const std::string& theFileName)
{
	TodHesitationTrace("preloadsong");
	if (!TodLoadMusic(theMusicFile, theFileName))
	{
		TodTrace("music failed to load\n");
		mMusicDisabled = true;
	}
	else
	{
		//gBass->BASS_ChannelSetAttribute(GetBassMusicHandle(theMusicFile), BASS_ATTRIB_MUSIC_PSCALER, 4);  // 设置音乐定位精确度属性
		TodHesitationTrace("song '%s'", theFileName.c_str());
	}
}

//0x45A8A0
void Music::MusicTitleScreenInit()
{
#ifdef PS2_PLATFORM
	// Same rule as MusicInit below: on PS2 NEVER fall through to the
	// LoadSong(".mo3") path. This runs on the MAIN thread at the first title
	// draw — before the loading thread even starts — so a Mix_LoadMUS_RW hang
	// here freezes the console right as the LOADING bar appears. No OGG
	// soundtrack on the media => the title simply runs silent.
	if (Ps2OggMusicAvailable())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME);
	return;
#endif
	LoadSong(MusicFile::MUSIC_FILE_MAIN_MUSIC, "sounds/mainmusic.mo3");
	MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME);
}

//0x45A980
void Music::MusicInit()
{
#ifdef _DEBUG
	int aNumLoadingTasks = mApp->mCompletedLoadingThreadTasks + GetNumLoadingTasks();
#endif

#ifdef PS2_PLATFORM
	// The PS2 build has NO .mo3 (mod) playback: music is per-tune OGG streaming
	// only. When the OGG soundtrack is present it streams on demand; when it is
	// absent we run SILENT. Either way we return here and MUST NOT fall through
	// to the LoadSong(".mo3") path below.
	//
	// That mo3 path (LoadSong -> TodLoadMusic -> Mix_LoadMUS_RW) is unsupported
	// on PS2 and HANGS the loading thread on real hardware — the console freezes
	// at the static green LOAD_MUSICINIT border. It only appeared to "work" under
	// PCSX2 because the OGGs resolve via host: there, so this fallback was never
	// reached; on the real console the OGGs aren't on the media, OGG mode stays
	// off, and the game wedged loading the mod.
	if (!Ps2OggMusicAvailable())
	{
		printf("[PS2] music: no OGG soundtrack found; running without music "
		       "(mo3 playback is not supported on PS2)\n");
		mMusicDisabled = true;
	}
	mApp->mCompletedLoadingThreadTasks += 3500 * 2;  // keep the loading bar accounting
	return;
#endif
	LoadSong(MusicFile::MUSIC_FILE_DRUMS, "sounds/mainmusic.mo3");
	mApp->mCompletedLoadingThreadTasks += /*原版*/3500;///*内测版*/800;
	LoadSong(MusicFile::MUSIC_FILE_HIHATS, "sounds/mainmusic_hihats.mo3");
	mApp->mCompletedLoadingThreadTasks += /*原版*/3500;///*内测版*/800;

#ifdef _DEBUG
	LoadSong(MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN, "sounds/ZombiesOnYourLawn.ogg");
	mApp->mCompletedLoadingThreadTasks += /*原版*/3500;///*内测版*/800;
	if (mApp->mCompletedLoadingThreadTasks != aNumLoadingTasks)
		TodTrace("Didn't calculate loading task count correctly!!!!");
#endif
}

//0x45AAC0
void Music::MusicCreditScreenInit()
{
#ifdef PS2_PLATFORM
	// OGG mode streams the credits tune via PlayMusic; without OGGs there is
	// nothing to play anyway (the Mix_* music API is stubbed out on PS2, see
	// the top of this file), and the LoadSong probe below would be unguarded
	// main-thread fio with the mixer live. Never fall through.
	return;
#endif
#ifndef _DEBUG
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface;
	if (anSDL->mMusicMap.find((int)MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN) == anSDL->mMusicMap.end())  // 如果尚未加载
		LoadSong(MusicFile::MUSIC_FILE_MAIN_MUSIC, "sounds/ZombiesOnYourLawn.ogg");
#endif
}

//0x45ABB0
void Music::StopAllMusic()
{
#ifdef PS2_PLATFORM
	if (gPs2OggMusicMode)
		Ps2MusicStop();
#endif
	if (mMusicInterface != nullptr)
	{
		if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
			mMusicInterface->StopMusic(mCurMusicFileMain);
		if (mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
			mMusicInterface->StopMusic(mCurMusicFileDrums);
		if (mCurMusicFileHihats != MusicFile::MUSIC_FILE_NONE)
			mMusicInterface->StopMusic(mCurMusicFileHihats);
	}

	mCurMusicTune = MusicTune::MUSIC_TUNE_NONE;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
	mQueuedDrumTrackPackedOrder = -1;
	mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
	mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
	mPauseOffset = 0;
	mPauseOffsetDrums = 0;
	mPaused = false;
	mFadeOutCounter = 0;
}

//0x45AC20
Mix_Music* Music::GetMusicHandle(MusicFile theMusicFile)
{
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface;
	auto anItr = anSDL->mMusicMap.find((int)theMusicFile);
	TOD_ASSERT(anItr != anSDL->mMusicMap.end());
	return anItr->second.mHMusic;
}

//0x45AC70
void Music::PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume)
{
#ifdef PS2_PLATFORM
	if (gPs2OggMusicMode)
	{
		// Only the credit screen calls this directly; the offset (a mod order)
		// has no OGG equivalent, so restart the song from the top.
		if (theMusicFile == MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN)
			Ps2PlayTuneOgg(MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN);
		return;
	}
#endif
	SDLMusicInterface* anSDL = (SDLMusicInterface*)mApp->mMusicInterface;
	auto anItr = anSDL->mMusicMap.find((int)theMusicFile);
	TOD_ASSERT(anItr != anSDL->mMusicMap.end());
	SDLMusicInfo* aMusicInfo = &anItr->second;

	/*
	if (aMusicInfo->mHStream)
	{
		bool aNoLoop = theMusicFile == MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN;  // MV 音乐不循环
		mMusicInterface->PlayMusic(theMusicFile, theOffset, aNoLoop);
	}
	else
	*/
	{
		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		aMusicInfo->mStopOnFade = false;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap * theVolume;
		aMusicInfo->mVolumeAdd = 0.0;
		Mix_PlayMusicStream(aMusicInfo->mHMusic, -1);
		Mix_ModMusicStreamJumpToOrder(aMusicInfo->mHMusic, theOffset);
		Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
		SetupMusicFileForTune(theMusicFile, mCurMusicTune);  // 调整每条轨道的静音与否
		/*
		gBass->BASS_ChannelStop(aMusicInfo->mHMusic);  // 先停止正在播放的音乐
		SetupMusicFileForTune(theMusicFile, mCurMusicTune);  // 调整每条轨道的静音与否
		aMusicInfo->mStopOnFade = false;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap * theVolume;
		aMusicInfo->mVolumeAdd = 0.0;
		//gBass->BASS_ChannelSetAttribute(aMusicInfo->mHMusic, -1, aMusicInfo->mVolume * 100.0, -101);  // 调整音乐音量
		gBass->BASS_ChannelSetAttribute(aMusicInfo->mHMusic, BASS_ATTRIB_VOL, aMusicInfo->mVolume);
		gBass->BASS_ChannelFlags(aMusicInfo->mHMusic, BASS_MUSIC_POSRESET | BASS_MUSIC_RAMP | BASS_MUSIC_LOOP, -1);
		gBass->BASS_ChannelSetPosition(aMusicInfo->mHMusic, MAKELONG(theOffset, 0), BASS_POS_MUSIC_ORDER);  // 设置偏移位置
		gBass->BASS_ChannelPlay(aMusicInfo->mHMusic, false);  // 重新开始播放
		*/
	}
}

//0x45ADB0
void Music::PlayMusic(MusicTune theMusicTune, int theOffset, int theDrumsOffset)
{
#ifdef PS2_PLATFORM
	if (gPs2OggMusicMode)
	{
		// One streamed OGG per tune; offsets/drum layers don't apply.
		mCurMusicTune = theMusicTune;
		mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
		mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
		Ps2PlayTuneOgg(theMusicTune);
		return;
	}
#endif
	if (mMusicDisabled)
		return;

	mCurMusicTune = theMusicTune;
	mCurMusicFileMain = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileDrums = MusicFile::MUSIC_FILE_NONE;
	mCurMusicFileHihats = MusicFile::MUSIC_FILE_NONE;
	bool aRestartingSong = theOffset != -1;

	switch (theMusicTune)
	{
	case MusicTune::MUSIC_TUNE_DAY_GRASSWALK:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		mCurMusicFileHihats = MusicFile::MUSIC_FILE_HIHATS;
		if (theOffset == -1)
			theOffset = 0;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theOffset, 0.0);
		PlayFromOffset(mCurMusicFileHihats, theOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		if (theOffset == -1)
		{
			theOffset = 0x30;
			theDrumsOffset = 0x5C;
		}
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theDrumsOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		mCurMusicFileHihats = MusicFile::MUSIC_FILE_HIHATS;
		if (theOffset == -1)
			theOffset = 0x5E;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theOffset, 0.0);
		PlayFromOffset(mCurMusicFileHihats, theOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		mCurMusicFileHihats = MusicFile::MUSIC_FILE_HIHATS;
		if (theOffset == -1)
			theOffset = 0x7D;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theOffset, 0.0);
		PlayFromOffset(mCurMusicFileHihats, theOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		mCurMusicFileDrums = MusicFile::MUSIC_FILE_DRUMS;
		mCurMusicFileHihats = MusicFile::MUSIC_FILE_HIHATS;
		if (theOffset == -1)
			theOffset = 0xB8;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		PlayFromOffset(mCurMusicFileDrums, theOffset, 0.0);
		PlayFromOffset(mCurMusicFileHihats, theOffset, 0.0);
		break;

	case MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x7A;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x98;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_ZEN_GARDEN:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xDD;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xB1;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xA6;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_CONVEYER:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0xD4;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_MAIN_MUSIC;
		if (theOffset == -1)
			theOffset = 0x9E;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	case MusicTune::MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN:
		mCurMusicFileMain = MusicFile::MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN;
		if (theOffset == -1)
			theOffset = 0;
		PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
		break;

	default:
		TOD_ASSERT(false);
		break;
	}

	if (aRestartingSong)
	{
		/*
		if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
		{
			Mix_Music* aHMusic = GetMusicHandle(mCurMusicFileMain);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_BPM, mBaseBPM);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_SPEED, mBaseModSpeed);
		}
		if (mCurMusicFileDrums != -1)
		{
			HMUSIC aHMusic = GetBassMusicHandle(mCurMusicFileDrums);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_BPM, mBaseBPM);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_SPEED, mBaseModSpeed);
		}
		if (mCurMusicFileHihats != -1)
		{
			HMUSIC aHMusic = GetBassMusicHandle(mCurMusicFileHihats);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_BPM, mBaseBPM);
			gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_SPEED, mBaseModSpeed);
		}
		*/
	}
	else
	{
		Mix_Music* aHMusic = GetMusicHandle(mCurMusicFileMain);
		/*
		gBass->BASS_ChannelGetAttribute(aHMusic, BASS_ATTRIB_MUSIC_BPM, &mBaseBPM);
		gBass->BASS_ChannelGetAttribute(aHMusic, BASS_ATTRIB_MUSIC_SPEED, &mBaseModSpeed);
		*/
	}
}

unsigned long Music::GetMusicOrder(MusicFile theMusicFile)
{
	TOD_ASSERT(theMusicFile != MusicFile::MUSIC_FILE_NONE);
	return ((SDLMusicInterface*)mApp->mMusicInterface)->GetMusicOrder((int)theMusicFile);
}

//0x45B1B0
void Music::MusicResyncChannel(MusicFile theMusicFileToMatch, MusicFile theMusicFileToSync)
{
	unsigned int aPosToMatch = GetMusicOrder(theMusicFileToMatch);
	unsigned int aPosToSync = GetMusicOrder(theMusicFileToSync);
	int aDiff = (aPosToSync >> 16) - (aPosToMatch >> 16);  // 待同步的音乐与目标音乐的乐曲序号之差
	if (abs(aDiff) <= 128)  // 当前进行的乐曲序号之差超过 128 时，开摆（
	{
		//HMUSIC aHMusic = GetBassMusicHandle(theMusicFileToSync);

		int aBPM = mBaseBPM;
		if (aDiff > 2)
			aBPM -= 2;
		else if (aDiff > 0)
			aBPM -= 1;
		else if (aDiff < -2)
			aBPM += 2;
		else if (aDiff < 0)
			aBPM -= 1;

		//gBass->BASS_ChannelSetAttribute(aHMusic, BASS_ATTRIB_MUSIC_BPM, aBPM);  // 适当调整待同步音乐的速率以缩小差距
	}
}

void Music::MusicResync()
{
	if (mCurMusicFileMain != MusicFile::MUSIC_FILE_NONE)
	{
		if (mCurMusicFileDrums != MusicFile::MUSIC_FILE_NONE)
			MusicResyncChannel(mCurMusicFileMain, mCurMusicFileDrums);
		if (mCurMusicFileHihats != MusicFile::MUSIC_FILE_NONE)
			MusicResyncChannel(mCurMusicFileMain, mCurMusicFileHihats);
	}
}

//0x45B240
void Music::StartBurst()
{ 
	if (mMusicBurstState == MusicBurstState::MUSIC_BURST_OFF)
	{ 
		mMusicBurstState = MusicBurstState::MUSIC_BURST_STARTING;
		mBurstStateCounter = 400;
	}
}

void Music::FadeOut(int theFadeOutDuration)
{ 
	if (mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
	{
		mFadeOutCounter = theFadeOutDuration;
		mFadeOutDuration = theFadeOutDuration;
	}
}

//0x45B260
void Music::UpdateMusicBurst()
{
	if (mApp->mBoard == nullptr)
		return;

	int aBurstScheme;
	if (mCurMusicTune == MusicTune::MUSIC_TUNE_DAY_GRASSWALK || mCurMusicTune == MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES ||
		mCurMusicTune == MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST || mCurMusicTune == MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF)
		aBurstScheme = 1;
	else if (mCurMusicTune == MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS)
		aBurstScheme = 2;
	else
		return;

	int aPackedOrderMain = GetMusicOrder(mCurMusicFileMain);
	if (mBurstStateCounter > 0)
		mBurstStateCounter--;
	if (mDrumsStateCounter > 0)
		mDrumsStateCounter--;

	float aFadeTrackVolume = 0.0f;
	float aDrumsVolume = 0.0f;
	float aMainTrackVolume = 1.0f;
	switch (mMusicBurstState)
	{
		case MusicBurstState::MUSIC_BURST_OFF:
			if (mApp->mBoard->CountZombiesOnScreen() >= 10 || mBurstOverride == 1)
				StartBurst();
			break;
		case MusicBurstState::MUSIC_BURST_STARTING:
			if (aBurstScheme == 1)
			{
				aFadeTrackVolume = TodAnimateCurveFloat(400, 0, mBurstStateCounter, 0.0f, 1.0f, TodCurves::CURVE_LINEAR);
				if (mBurstStateCounter == 100)
				{
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
				}
				else if (mBurstStateCounter == 0)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_ON;
					mBurstStateCounter = 800;
				}
			}
			else if (aBurstScheme == 2)
			{
				if (mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_OFF)
				{
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
					mBurstStateCounter = 400;
				}
				else if (mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_ON_QUEUED)
					mBurstStateCounter = 400;
				else
				{
					aMainTrackVolume = TodAnimateCurveFloat(400, 0, mBurstStateCounter, 1.0f, 0.0f, TodCurves::CURVE_LINEAR);
					if (mBurstStateCounter == 0)
					{
						mMusicBurstState = MusicBurstState::MUSIC_BURST_ON;
						mBurstStateCounter = 800;
					}
				}
			}
			break;
		case MusicBurstState::MUSIC_BURST_ON:
			aFadeTrackVolume = 1.0f;
			if (aBurstScheme == 2)
				aMainTrackVolume = 0.0f;
			if (mBurstStateCounter == 0 && ((mApp->mBoard->CountZombiesOnScreen() < 4 && mBurstOverride == -1) || mBurstOverride == 2))
			{
				if (aBurstScheme == 1)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_FINISHING;
					mBurstStateCounter = 800;
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF_QUEUED;
					mQueuedDrumTrackPackedOrder = aPackedOrderMain;
				}
				else if (aBurstScheme == 2)
				{
					mMusicBurstState = MusicBurstState::MUSIC_BURST_FINISHING;
					mBurstStateCounter = 1100;
					mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_FADING;
					mDrumsStateCounter = 800;
				}
			}
			break;
		case MusicBurstState::MUSIC_BURST_FINISHING:
			if (aBurstScheme == 1)
				aFadeTrackVolume = TodAnimateCurveFloat(800, 0, mBurstStateCounter, 1.0f, 0.0f, TodCurves::CURVE_LINEAR);
			else
				aMainTrackVolume = TodAnimateCurveFloat(400, 0, mBurstStateCounter, 0.0f, 1.0f, TodCurves::CURVE_LINEAR);
			if (mBurstStateCounter == 0 && mMusicDrumsState == MusicDrumsState::MUSIC_DRUMS_OFF)
				mMusicBurstState = MusicBurstState::MUSIC_BURST_OFF;
			break;
	}

	int aDrumsJumpOrder = -1;
	int aOrderMain = 0, aOrderDrum = 0;
	if (aBurstScheme == 1)
	{
		//aOrderMain = HIWORD(aPackedOrderMain) / 128;
		//aOrderDrum = HIWORD(mQueuedDrumTrackPackedOrder) / 128;
		aOrderMain = aPackedOrderMain;
		aOrderDrum = mQueuedDrumTrackPackedOrder;
	}
	else if (aBurstScheme == 2)
	{
		/*
		aOrderMain = LOWORD(aPackedOrderMain);
		aOrderDrum = LOWORD(mQueuedDrumTrackPackedOrder);
		if (HIWORD(aPackedOrderMain) > 252)
			aOrderMain++;
		if (HIWORD(mQueuedDrumTrackPackedOrder) > 252)
			aOrderDrum++;
		*/
		aOrderMain = aPackedOrderMain;
		aOrderDrum = mQueuedDrumTrackPackedOrder;
	}

	switch (mMusicDrumsState)
	{
		case MusicDrumsState::MUSIC_DRUMS_ON_QUEUED:
			if (aOrderMain != aOrderDrum)
			{
				aDrumsVolume = 1.0f;
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_ON;
				if (aBurstScheme == 2)
					aDrumsJumpOrder = (aOrderMain % 2 == 0) ? 76 : 77;
			}
			break;
		case MusicDrumsState::MUSIC_DRUMS_ON:
			aDrumsVolume = 1.0f;
			break;
		case MusicDrumsState::MUSIC_DRUMS_OFF_QUEUED:
			aDrumsVolume = 1.0f;
			if (aOrderMain != aOrderDrum && aBurstScheme == 1)
			{
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_FADING;
				mDrumsStateCounter = 50;
			}
			break;
		case MusicDrumsState::MUSIC_DRUMS_FADING:
			if (aBurstScheme == 2)
				aDrumsVolume = TodAnimateCurveFloat(800, 0, mDrumsStateCounter, 1.0f, 0.0f, TodCurves::CURVE_LINEAR);
			else
				aDrumsVolume = TodAnimateCurveFloat(50, 0, mDrumsStateCounter, 1.0f, 0.0f, TodCurves::CURVE_LINEAR);
			if (mDrumsStateCounter == 0)
				mMusicDrumsState = MusicDrumsState::MUSIC_DRUMS_OFF;
			break;
		case MusicDrumsState::MUSIC_DRUMS_OFF:
			break;
	}

	if (aBurstScheme == 1)
	{
		mMusicInterface->SetSongVolume(mCurMusicFileHihats, aFadeTrackVolume);
		mMusicInterface->SetSongVolume(mCurMusicFileDrums, aDrumsVolume);
	}
	else if (aBurstScheme == 2)
	{
		mMusicInterface->SetSongVolume(mCurMusicFileMain, aMainTrackVolume);
		mMusicInterface->SetSongVolume(mCurMusicFileDrums, aDrumsVolume);
		if (aDrumsJumpOrder != -1)
			Mix_ModMusicStreamJumpToOrder(GetMusicHandle(mCurMusicFileDrums), aDrumsJumpOrder);
			//gBass->BASS_ChannelSetPosition(GetBassMusicHandle(mCurMusicFileDrums), MAKELONG(aDrumsJumpOrder,0) /*| 0x80000000*/, BASS_POS_MUSIC_ORDER);
	}
}

//0x45B670
void Music::MusicUpdate()
{
	if (mFadeOutCounter > 0)
	{
		mFadeOutCounter--;
		if (mFadeOutCounter == 0)
			StopAllMusic();
		else
		{
			float aFadeLevel = TodAnimateCurveFloat(mFadeOutDuration, 0, mFadeOutCounter, 1.0f, 0.0f, TodCurves::CURVE_LINEAR);
#ifdef PS2_PLATFORM
			if (gPs2OggMusicMode)
				Ps2MusicSetVolume(aFadeLevel);
			else
#endif
			mMusicInterface->SetSongVolume(mCurMusicFileMain, aFadeLevel);
		}
	}

#ifdef PS2_PLATFORM
	// Burst/resync poke at mod-music orders that don't exist in OGG mode.
	if (gPs2OggMusicMode)
		return;
#endif
	if (mApp->mBoard == nullptr || !mApp->mBoard->mPaused)
	{
		UpdateMusicBurst();
		MusicResync();
	}
}

//0x45B750
// GOTY @Patoke: 0x45EFA0
void Music::MakeSureMusicIsPlaying(MusicTune theMusicTune)
{
	if (mCurMusicTune != theMusicTune)
	{
		StopAllMusic();
		PlayMusic(theMusicTune, -1, -1);
	}
}

//0x45B770
void Music::StartGameMusic()
{
	TOD_ASSERT(mApp->mBoard);

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_ZEN_GARDEN);
	else if (mApp->IsFinalBossLevel())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC);
	else if (mApp->IsWallnutBowlingLevel() || mApp->IsWhackAZombieLevel() || mApp->IsLittleTroubleLevel() || mApp->IsBungeeBlitzLevel() ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SPEED)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_MINIGAME_LOONBOON);
	else if ((mApp->IsAdventureMode() && (mApp->mPlayerInfo->GetLevel() == 10 || mApp->mPlayerInfo->GetLevel() == 20 || mApp->mPlayerInfo->GetLevel() == 30)) ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN)
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_CONVEYER);
	else if (mApp->IsStormyNightLevel())
		StopAllMusic();
	else if (mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_PUZZLE_CEREBRAWL);
	else if (mApp->mBoard->StageHasFog())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST);
	else if (mApp->mBoard->StageIsNight())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS);
	else if (mApp->mBoard->StageHas6Rows())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES);
	else if (mApp->mBoard->StageHasRoof())
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF);
	else
		MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_DAY_GRASSWALK);
}

//0x45B930
void Music::GameMusicPause(bool thePause)
{
#ifdef PS2_PLATFORM
	if (gPs2OggMusicMode)
	{
		if (thePause && !mPaused && mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
		{
			Ps2MusicSetPaused(true);
			mPaused = true;
		}
		else if (!thePause && mPaused)
		{
			Ps2MusicSetPaused(false);
			mPaused = false;
		}
		return;
	}
#endif
	if (thePause)
	{
		if (!mPaused && mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
		{
			SDLMusicInterface* anSDL = (SDLMusicInterface*)mMusicInterface;
			auto anItr = anSDL->mMusicMap.find(mCurMusicFileMain);
			TOD_ASSERT(anItr != anSDL->mMusicMap.end());
			SDLMusicInfo* aMusicInfo = &anItr->second;

			/*
			if (aMusicInfo->mHStream)
			{
				mPauseOffset = gBass->BASS_ChannelGetPosition(aMusicInfo->mHStream, BASS_POS_MUSIC_ORDER);
				mMusicInterface->StopMusic(mCurMusicFileMain);
			}
			else
			*/
			{
				mPauseOffset = GetMusicOrder(mCurMusicFileMain);
				mMusicInterface->StopMusic(mCurMusicFileMain);

				if (mCurMusicTune == MusicTune::MUSIC_TUNE_DAY_GRASSWALK || mCurMusicTune == MusicTune::MUSIC_TUNE_POOL_WATERYGRAVES ||
					mCurMusicTune == MusicTune::MUSIC_TUNE_FOG_RIGORMORMIST || mCurMusicTune == MusicTune::MUSIC_TUNE_ROOF_GRAZETHEROOF)
				{
					mMusicInterface->StopMusic(mCurMusicFileDrums);
					mMusicInterface->StopMusic(mCurMusicFileHihats);
				}
				else if (mCurMusicTune == MusicTune::MUSIC_TUNE_NIGHT_MOONGRAINS)
				{
					mPauseOffsetDrums = GetMusicOrder(mCurMusicFileDrums);
					mMusicInterface->StopMusic(mCurMusicFileDrums);
				}
			}
			mPaused = true;
		}
	}
	else if (mPaused)
	{
		if (mCurMusicTune != MusicTune::MUSIC_TUNE_NONE)
			PlayMusic(mCurMusicTune, mPauseOffset, mPauseOffsetDrums);
		mPaused = false;
	}
}

int Music::GetNumLoadingTasks()
{
	//return 800 * 3;  // 内测版
	return 3500 * 2;  // 原版
}
