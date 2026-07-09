#ifndef __DUMMYSOUNDMANAGER_H__
#define __DUMMYSOUNDMANAGER_H__

#include "SoundManager.h"
#include "SoundInstance.h"

namespace Sexy
{

class DummySoundInstance : public SoundInstance
{
public:
	virtual void Release() { delete this; }
	virtual void SetBaseVolume(double) {}
	virtual void SetBasePan(int) {}
	virtual void AdjustPitch(double) {}
	virtual void SetVolume(double) {}
	virtual void SetPan(int) {}
	virtual bool Play(bool, bool) { return false; }
	virtual void Stop() {}
	virtual bool IsPlaying() { return false; }
	virtual bool IsReleased() { return true; }
	virtual double GetVolume() { return 0.0; }
};

class DummySoundManager : public SoundManager
{
private:
	bool mLoaded[MAX_SOURCE_SOUNDS];
	std::string mSourceFileNames[MAX_SOURCE_SOUNDS];
	double mMasterVolume;

public:
	DummySoundManager() : mMasterVolume(1.0)
	{
		for (int i = 0; i < MAX_SOURCE_SOUNDS; ++i)
			mLoaded[i] = false;
	}

	virtual bool Initialized() { return true; }
	virtual bool LoadSound(unsigned int theSfxID, const std::string& theFilename)
	{
		if (theSfxID >= MAX_SOURCE_SOUNDS)
			return false;

		mLoaded[theSfxID] = true;
		mSourceFileNames[theSfxID] = theFilename;
		return true;
	}

	virtual int LoadSound(const std::string& theFilename)
	{
		for (int i = 0; i < MAX_SOURCE_SOUNDS; ++i)
			if (mLoaded[i] && mSourceFileNames[i] == theFilename)
				return i;

		int id = GetFreeSoundId();
		if (id < 0)
			return -1;

		return LoadSound((unsigned int)id, theFilename) ? id : -1;
	}

	virtual void ReleaseSound(unsigned int theSfxID)
	{
		if (theSfxID >= MAX_SOURCE_SOUNDS)
			return;

		mLoaded[theSfxID] = false;
		mSourceFileNames[theSfxID].clear();
	}
	virtual void SetVolume(double) {}
	virtual bool SetBaseVolume(unsigned int, double) { return false; }
	virtual bool SetBasePan(unsigned int, int) { return false; }
	virtual SoundInstance* GetSoundInstance(unsigned int theSfxID)
	{
		if (theSfxID >= MAX_SOURCE_SOUNDS || !mLoaded[theSfxID])
			return NULL;

		return new DummySoundInstance();
	}

	virtual void ReleaseSounds()
	{
		for (int i = 0; i < MAX_SOURCE_SOUNDS; ++i)
		{
			mLoaded[i] = false;
			mSourceFileNames[i].clear();
		}
	}
	virtual void ReleaseChannels() {}
	virtual double GetMasterVolume() { return mMasterVolume; }
	virtual void SetMasterVolume(double theVolume) { mMasterVolume = theVolume; }
	virtual void Flush() {}
	virtual void SetCooperativeWindow(HWND) {}
	virtual void StopAllSounds() {}
	virtual int GetFreeSoundId()
	{
		for (int i = 0; i < MAX_SOURCE_SOUNDS; ++i)
			if (!mLoaded[i])
				return i;

		return -1;
	}

	virtual int GetNumSounds()
	{
		int count = 0;
		for (int i = 0; i < MAX_SOURCE_SOUNDS; ++i)
			if (mLoaded[i])
				++count;

		return count;
	}
};

}

#endif
