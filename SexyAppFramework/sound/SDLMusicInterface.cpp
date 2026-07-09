#include "SDLMusicInterface.h"
#include "paklib/PakInterface.h"

using namespace Sexy;

SDLMusicInfo::SDLMusicInfo()
{
	mVolume = 0.0;
	mVolumeAdd = 0.0;
	mVolumeCap = 1.0;
	mStopOnFade = false;
	mHMusic = 0;
}

SDLMusicInterface::SDLMusicInterface()
{
	mGlobalVolume = MIX_MAX_VOLUME;
}

SDLMusicInterface::~SDLMusicInterface()
{
	Mix_HaltMusic();
}

bool SDLMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	Mix_Music* aHMusic = 0;
	
	std::string anExt;
	size_t aDotPos = theFileName.find_last_of('.');
	if (aDotPos!=std::string::npos)
		anExt = StringToLower(theFileName.substr(aDotPos+1));

	/*
	if (anExt=="wav" || anExt=="ogg" || anExt=="mp3")
		aStream = gBass->BASS_StreamCreateFile(FALSE, (void*) theFileName.c_str(), 0, 0, 0);
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");
		if (aFP == NULL)
			return false;

		p_fseek(aFP, 0, SEEK_END);
		int aSize = p_ftell(aFP);
		p_fseek(aFP, 0, SEEK_SET);

		uchar* aData = new uchar[aSize];
		p_fread(aData, 1, aSize, aFP);
		p_fclose(aFP);

		aHMusic = gBass->BASS_MusicLoad(FALSE, (void*) theFileName.c_str(), 0, 0, BASS_MUSIC_LOOP | BASS2_MUSIC_RAMP, 0);

		delete[] aData;
	}
	*/
	aHMusic = Mix_LoadMUS(theFileName.c_str());

	if (aHMusic==0)
		return false;
	
	SDLMusicInfo aMusicInfo;	
	aMusicInfo.mHMusic = aHMusic;
	mMusicMap.insert(SDLMusicMap::value_type(theSongId, aMusicInfo));

	return true;
}

void SDLMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap;
		aMusicInfo->mVolumeAdd = 0.0;
		aMusicInfo->mStopOnFade = noLoop;

		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		Mix_PlayMusicStream(aMusicInfo->mHMusic, (noLoop) ? 0 : -1);
		if (theOffset > 0)
			Mix_ModMusicStreamJumpToOrder(aMusicInfo->mHMusic, theOffset);
		/*
		gBass->BASS_ChannelSetAttribute(aMusicInfo->GetHandle(), BASS_ATTRIB_VOL, (int) (aMusicInfo->mVolume));

		gBass->BASS_ChannelStop(aMusicInfo->GetHandle());
		if (aMusicInfo->mHMusic)
		{
			if (gBass->mVersion2)
				gBass->BASS_MusicPlayEx(aMusicInfo->mHMusic, theOffset, BASS_MUSIC_POSRESET | BASS2_MUSIC_RAMP | (noLoop ? 0 : BASS_MUSIC_LOOP), TRUE);
			else
				gBass->BASS_MusicPlayEx(aMusicInfo->mHMusic, theOffset, noLoop ? 0 : BASS_MUSIC_LOOP, TRUE);
		}
		else
		{
			BOOL flush = theOffset == -1 ? FALSE : TRUE;
			gBass->BASS_StreamPlay(aMusicInfo->mHStream, flush, noLoop ? 0 : BASS_MUSIC_LOOP);
			if (theOffset > 0)
				gBass->BASS_ChannelSetPosition(aMusicInfo->mHStream, theOffset, BASS_POS_BYTE);
		}
		*/
	}
}

void SDLMusicInterface::StopMusic(int theSongId)
{
	Mix_HaltMusic();

	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = 0.0;
		Mix_HaltMusicStream(aMusicInfo->mHMusic);
	}
}

void SDLMusicInterface::PauseMusic(int theSongId)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		Mix_PauseMusicStream(aMusicInfo->mHMusic);
	}
}

void SDLMusicInterface::ResumeMusic(int theSongId)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		//gBass->BASS_ChannelResume(aMusicInfo->GetHandle());
		Mix_ResumeMusicStream(aMusicInfo->mHMusic);
	}
}

void SDLMusicInterface::StopAllMusic()
{
	SDLMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = 0.0;
		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		++anItr;
	}
}

void SDLMusicInterface::UnloadMusic(int theSongId)
{
	StopMusic(theSongId);
	
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		Mix_FreeMusic(aMusicInfo->mHMusic);

		mMusicMap.erase(anItr);
	}
}

void SDLMusicInterface::UnloadAllMusic()
{
	StopAllMusic();
	for (SDLMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		Mix_FreeMusic(aMusicInfo->mHMusic);
	}
	mMusicMap.clear();
}

void SDLMusicInterface::PauseAllMusic()
{
	for (SDLMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		if (Mix_PlayingMusicStream(aMusicInfo->mHMusic) && !Mix_PausedMusicStream(aMusicInfo->mHMusic))
			Mix_PauseMusicStream(aMusicInfo->mHMusic);
	}
}

void SDLMusicInterface::ResumeAllMusic()
{
	for (SDLMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
		if (Mix_PausedMusicStream(aMusicInfo->mHMusic))
			Mix_ResumeMusicStream(aMusicInfo->mHMusic);
	}
}

void SDLMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
				
		aMusicInfo->mVolumeAdd = theSpeed;
		aMusicInfo->mStopOnFade = noLoop;

		Mix_HaltMusicStream(aMusicInfo->mHMusic);
		Mix_PlayMusicStream(aMusicInfo->mHMusic, (noLoop) ? 0 : -1);
		if (theOffset > 0)
			Mix_ModMusicStreamJumpToOrder(aMusicInfo->mHMusic, theOffset);

		/*
		gBass->BASS_ChannelStop(aMusicInfo->GetHandle());
		gBass->BASS_ChannelSetAttribute(aMusicInfo->GetHandle(), BASS_ATTRIB_VOL, (int) (aMusicInfo->mVolume));
		if (aMusicInfo->mHMusic)
		{
			if (theOffset == -1)
				gBass->BASS_MusicPlay(aMusicInfo->mHMusic);
			else
			{
				gBass->BASS_MusicPlayEx(aMusicInfo->mHMusic, theOffset, BASS2_MUSIC_RAMP | (noLoop ? 0 : BASS_MUSIC_LOOP), TRUE);
			}
		}
		else
		{
			BOOL flush = theOffset == -1 ? FALSE : TRUE;
			gBass->BASS_StreamPlay(aMusicInfo->mHStream, flush, noLoop ? 0 : BASS_MUSIC_LOOP);
			if (theOffset > 0)
				gBass->BASS_ChannelSetPosition(aMusicInfo->mHStream, theOffset, BASS_POS_BYTE);
		}
		*/

	}
}

void SDLMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		SDLMusicInfo* aMusicInfo = &anItr->second;
		
		if (aMusicInfo->mVolume != 0.0)
		{
			aMusicInfo->mVolumeAdd = -theSpeed;			
		}

		aMusicInfo->mStopOnFade = stopSong;
	}
}

void SDLMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	SDLMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;
				
		aMusicInfo->mVolumeAdd = -theSpeed;
		aMusicInfo->mStopOnFade = stopSong;

		++anItr;
	}
}

void SDLMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		SDLMusicInfo* aMusicInfo = &anItr->second;

		aMusicInfo->mVolume = theVolume;
		//gBass->BASS_ChannelSetAttribute(aMusicInfo->GetHandle(), BASS_ATTRIB_VOL, (int) (aMusicInfo->mVolume));
		Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
	}
}

void SDLMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		SDLMusicInfo* aMusicInfo = &anItr->second;

		aMusicInfo->mVolumeCap = theMaxVolume;
		aMusicInfo->mVolume = std::min(aMusicInfo->mVolume, theMaxVolume);
		Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
	}
}

bool SDLMusicInterface::IsPlaying(int theSongId)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		SDLMusicInfo* aMusicInfo = &anItr->second;
		return Mix_PlayingMusicStream(aMusicInfo->mHMusic);
	}

	return false;
}

void SDLMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = (int)(theVolume*80);
}

void SDLMusicInterface::SetMusicAmplify(int theSongId, double theAmp)
{
	
}

void SDLMusicInterface::Update()
{
	Mix_VolumeMusic(mGlobalVolume);
	Mix_VolumeMusicGeneral(mGlobalVolume);

	SDLMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		SDLMusicInfo* aMusicInfo = &anItr->second;

		if (aMusicInfo->mVolumeAdd != 0.0)
		{
			aMusicInfo->mVolume += aMusicInfo->mVolumeAdd;
			
			if (aMusicInfo->mVolume > aMusicInfo->mVolumeCap)
			{
				aMusicInfo->mVolume = aMusicInfo->mVolumeCap;
				aMusicInfo->mVolumeAdd = 0.0;
			}
			else if (aMusicInfo->mVolume < 0.0)
			{
				aMusicInfo->mVolume = 0.0;
				aMusicInfo->mVolumeAdd = 0.0;

				if (aMusicInfo->mStopOnFade)
					Mix_HaltMusicStream(aMusicInfo->mHMusic);
			}

			//gBass->BASS_ChannelSetAttribute(aMusicInfo->GetHandle(), BASS_ATTRIB_VOL, (int) (aMusicInfo->mVolume));
			Mix_VolumeMusicStream(aMusicInfo->mHMusic, (int)(aMusicInfo->mVolume*128));
		}

		++anItr;
	}
}

// functions for dealing with MODs
int SDLMusicInterface::GetMusicOrder(int theSongId)
{
	SDLMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		SDLMusicInfo* aMusicInfo = &anItr->second;
		int aPosition = -1;
		Mix_ModMusicStreamGetOrder(aMusicInfo->mHMusic, &aPosition);
		return aPosition;
	}
	return -1;
}
