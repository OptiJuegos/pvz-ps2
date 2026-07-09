#ifndef __PS2SOUNDMANAGER_H__
#define __PS2SOUNDMANAGER_H__
#ifdef PS2_PLATFORM

// Real sound for the PS2 build. Sound effects (.ogg via stb_vorbis, .wav PCM)
// are decoded at load time into 22050Hz mono s16 PCM in EE RAM; a dedicated
// EE thread software-mixes up to 32 voices (volume/pan/pitch, looping) into a
// stereo stream fed to the SPU2 through audsrv.
//
// Threading model: the main thread only writes voice slots (activating a
// voice writes its fields before the `active` flag); the mixer thread only
// clears `active` when a voice ends. SoundInstance objects are deleted
// exclusively on the main thread (dead instances are reaped on the next
// manager call), so the mixer never allocates or frees.

#include "sound/SoundManager.h"
#include "sound/SoundInstance.h"
#include <list>

namespace Sexy
{

class Ps2SoundManager;

struct Ps2Sample
{
	short*	mPCM;		// 22050Hz mono
	int		mNumSamples;
	Ps2Sample() : mPCM(0), mNumSamples(0) {}
};

struct Ps2Voice
{
	volatile int	mActive;		// set last by the starter; cleared by the mixer
	volatile int	mStopRequest;
	const short*	mData;
	int				mNumSamples;
	// 48.16 fixed point sample cursor. Must be 64-bit: a 32-bit 16.16 cursor
	// wraps at 65535 samples (~3s at 22050Hz), which truncated long sounds
	// and corrupted looping ones.
	unsigned long long	mPos;
	unsigned int	mStep;			// 16.16 playback step (pitch)
	volatile int	mVolL;			// 0..4096
	volatile int	mVolR;			// 0..4096
	int				mLooping;
	int				mSerial;		// increments per activation (stale-handle guard)
};

class Ps2SoundInstance : public SoundInstance
{
public:
	Ps2SoundManager*	mManager;
	int					mSfxID;
	int					mVoice;			// -1 when not playing
	int					mVoiceSerial;
	double				mBaseVolume;
	double				mVolume;
	int					mBasePan;
	int					mPan;
	double				mPitchRatio;
	bool				mReleased;
	bool				mAutoRelease;

	Ps2SoundInstance(Ps2SoundManager* theManager, int theSfxID);

	bool				VoiceAlive();
	void				ApplyVolume();

	virtual void		Release();
	virtual void		SetBaseVolume(double theBaseVolume);
	virtual void		SetBasePan(int theBasePan);
	virtual void		AdjustPitch(double theNumSteps);
	virtual void		SetVolume(double theVolume);
	virtual void		SetPan(int thePosition);
	virtual bool		Play(bool looping, bool autoRelease);
	virtual void		Stop();
	virtual bool		IsPlaying();
	virtual bool		IsReleased();
	virtual double		GetVolume();
};

class Ps2SoundManager : public SoundManager
{
public:
	bool		mInitialized;
	Ps2Sample	mSamples[MAX_SOURCE_SOUNDS];
	std::string	mSourceFileNames[MAX_SOURCE_SOUNDS];
	double		mBaseVolumes[MAX_SOURCE_SOUNDS];
	int			mBasePans[MAX_SOURCE_SOUNDS];
	double		mMasterVolume;

	std::list<Ps2SoundInstance*> mInstances;

	Ps2SoundManager();
	virtual ~Ps2SoundManager();

	void				ReapDeadInstances();
	bool				EnsureSampleLoaded(unsigned int theSfxID);
	bool				EnsureSampleLoadedLocked(unsigned int theSfxID);
	int					StartVoice(const Ps2Sample& theSample, unsigned int theStep, int theVolL, int theVolR, bool looping, int& theSerialOut);

	virtual bool		Initialized();
	virtual bool		LoadSound(unsigned int theSfxID, const std::string& theFilename);
	virtual int			LoadSound(const std::string& theFilename);
	virtual void		ReleaseSound(unsigned int theSfxID);
	virtual void		SetVolume(double theVolume);
	virtual bool		SetBaseVolume(unsigned int theSfxID, double theBaseVolume);
	virtual bool		SetBasePan(unsigned int theSfxID, int theBasePan);
	virtual SoundInstance* GetSoundInstance(unsigned int theSfxID);
	virtual void		ReleaseSounds();
	virtual void		ReleaseChannels();
	virtual double		GetMasterVolume();
	virtual void		SetMasterVolume(double theVolume);
	virtual void		Flush();
	virtual void		SetCooperativeWindow(HWND theHWnd);
	virtual void		StopAllSounds();
	virtual int			GetFreeSoundId();
	virtual int			GetNumSounds();
	virtual void		PurgeSounds();
	virtual void		PreloadSound(unsigned int theSfxID);
};

// ---------------------------------------------------------------------------
// Streamed music (OGG). The whole compressed file lives in RAM and is decoded
// incrementally by the mixer thread, so no disc/SIF traffic happens outside
// the main thread. All functions below must be called from the main (or
// loading) thread only.
// ---------------------------------------------------------------------------

// Takes ownership of theOggData (malloc'd); frees it on stop/failure.
bool Ps2MusicPlay(unsigned char* theOggData, int theSize, bool theLoop);
void Ps2MusicStop();
void Ps2MusicSetPaused(bool thePaused);
bool Ps2MusicIsPlaying();
void Ps2MusicSetVolume(double theVolume);		// per-song volume (fades), 0..1
void Ps2MusicSetMasterVolume(double theVolume);	// options slider, 0..1

// Silence the mixer (no audsrv SIF RPCs) while the loading thread owns the IOP,
// so file I/O and audio don't collide on the shared SIF link and freeze real
// hardware. Called by LawnApp::LoadingThreadProc around its work.
void Ps2MixerSetLoadPause(bool thePaused);

// Nestable variant with a park handshake: Begin raises the pause AND waits
// (bounded) until the mixer has parked outside audsrv_wait/play, so no audsrv
// RPC is in flight when the caller's file IO starts. Setting the flag alone is
// not enough — the mixer may already be past its pause check, inside a ~46ms
// RPC window. Wrap EVERY file read that can run while the mixer is live
// (music OGG loads, lazy sample decodes); the loading thread's whole-load
// pause nests on the same counter.
void Ps2MixerIoPauseBegin();
void Ps2MixerIoPauseEnd();

// RAII scope for Ps2MixerIoPauseBegin/End.
struct Ps2MixerIoPauseScope
{
	Ps2MixerIoPauseScope()  { Ps2MixerIoPauseBegin(); }
	~Ps2MixerIoPauseScope() { Ps2MixerIoPauseEnd(); }
};

// Mixer liveness report, main thread only, call ~1/s (DoMainLoop). Prints on
// state changes (alive / PAUSED / STALLED@spot) plus a slow heartbeat —
// diagnoses "audio dead after load" by showing where the mixer is stuck.
void Ps2MixerHealthTick();

}

#endif // PS2_PLATFORM
#endif // __PS2SOUNDMANAGER_H__
