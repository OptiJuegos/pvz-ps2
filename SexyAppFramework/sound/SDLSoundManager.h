#ifndef __SDLSOUNDMANAGER_H__
#define __SDLSOUNDMANAGER_H__

#include "SoundManager.h"
#include <SDL.h>
#include <SDL_mixer_ext/SDL_mixer_ext.h>

namespace Sexy
{

class SDLSoundInstance;

class SDLSoundManager : public SoundManager
{
	friend class SDLSoundInstance;

protected:
	bool					mInitializedMixer;
	Mix_Chunk*				mSourceSounds[MAX_SOURCE_SOUNDS];
	std::string				mSourceFileNames[MAX_SOURCE_SOUNDS];
	double					mBaseVolumes[MAX_SOURCE_SOUNDS];
	int						mBasePans[MAX_SOURCE_SOUNDS];
	SDLSoundInstance*		mPlayingSounds[MAX_CHANNELS];
	double					mMasterVolume;
	uint64_t				mLastReleaseTick;
	int						mMixerFreq;
	uint16_t				mMixerFormat;
	int						mMixerChannels;

protected:
	int						FindFreeChannel();
	bool					LoadAUSound(unsigned int theSfxID, const std::string& theFilename);
	void					ReleaseFreeChannels();

public:
	SDLSoundManager();
	virtual ~SDLSoundManager();

	virtual bool			Initialized();

	virtual bool			LoadSound(unsigned int theSfxID, const std::string& theFilename);
	virtual int				LoadSound(const std::string& theFilename);
	virtual void			ReleaseSound(unsigned int theSfxID);

	virtual void			SetVolume(double theVolume);
	virtual bool			SetBaseVolume(unsigned int theSfxID, double theBaseVolume);
	virtual bool			SetBasePan(unsigned int theSfxID, int theBasePan);

	virtual SoundInstance*	GetSoundInstance(unsigned int theSfxID);

	virtual void			ReleaseSounds();
	virtual void			ReleaseChannels();

	virtual double			GetMasterVolume();
	virtual void			SetMasterVolume(double theVolume);

	virtual void			Flush();
	virtual void			SetCooperativeWindow(HWND theHWnd);
	virtual void			StopAllSounds();
	virtual int				GetFreeSoundId();
	virtual int				GetNumSounds();
};

}

#endif //__SDLSOUNDMANAGER_H__
