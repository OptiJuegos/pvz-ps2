#ifndef __SDLMUSICINTERFACE_H__
#define __SDLMUSICINTERFACE_H__

#include "MusicInterface.h"

#include <SDL.h>
#include <SDL_mixer_ext/SDL_mixer_ext.h>

namespace Sexy
{

class SexyAppBase;

class SDLMusicInfo
{
public:
	Mix_Music*				mHMusic;
	double					mVolume;
	double					mVolumeAdd;
	double					mVolumeCap;
	bool					mStopOnFade;

public:
	SDLMusicInfo();

	Mix_Music* GetHandle() { return mHMusic; }
};

typedef std::map<int, SDLMusicInfo> SDLMusicMap;

class SDLMusicInterface : public MusicInterface
{
public:
	SDLMusicMap				mMusicMap;
	int						mGlobalVolume;
	int						mMusicLoadFlags;

public:
	SDLMusicInterface();
	virtual ~SDLMusicInterface();

	virtual bool			LoadMusic(int theSongId, const std::string& theFileName);
	virtual void			PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);
	virtual void			StopMusic(int theSongId);
	virtual void			PauseMusic(int theSongId);
	virtual void			ResumeMusic(int theSongId);
	virtual void			StopAllMusic();

	virtual void			UnloadMusic(int theSongId);
	virtual void			UnloadAllMusic();
	virtual void			PauseAllMusic();
	virtual void			ResumeAllMusic();
	
	virtual void			FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void			FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void			FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void			SetSongVolume(int theSongId, double theVolume);
	virtual void			SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool			IsPlaying(int theSongId);
	
	virtual void			SetVolume(double theVolume);
	virtual void			SetMusicAmplify(int theSongId, double theAmp);
	virtual void			Update();

	// functions for dealing with MODs
	int						GetMusicOrder(int theSongId);
};
}

#endif //__SDLMUSICINTERFACE_H__
