#ifdef PS2_PLATFORM

#include "Ps2SoundManager.h"
#include "../Ps2PvzServices.h"
#include "../Ps2IoLock.h"
#include "paklib/PakInterface.h"

#include <kernel.h>
#include <audsrv.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Plain stb_vorbis (the copy vendored with SDL-Mixer-X works standalone when
// STB_VORBIS_SDL is not defined). Pull in the implementation here only.
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_MAX_CHANNELS 2
#include "../../../sound/SDL-Mixer-X/src/codecs/stb_vorbis/stb_vorbis.h"

using namespace Sexy;

#define PS2_MIX_RATE     22050
// Samples per mixer iteration. ~46ms of latency, but the extra buffered audio
// rides out IOP stalls (asset reads share the IOP with audsrv), which showed
// up as crackling/choppy sfx whenever assets loaded.
#define PS2_MIX_CHUNK    1024
#define PS2_NUM_VOICES   32
#define PS2_VOL_ONE      4096

// Guard canaries bracket the voice table and the mixer stack bottom. All are
// zero-initialized (.bss) so the linker keeps them adjacent to what they
// guard; they get their fill pattern at manager init and are checked from the
// main thread on every StartVoice. A tripped guard pinpoints who is being
// overwritten (observed in the wild as an EE crash executing s_voices data).
#define PS2_GUARD_SIZE 64
#define PS2_GUARD_FILL 0xC5
static unsigned char s_voicesGuardLo[PS2_GUARD_SIZE];
static Ps2Voice s_voices[PS2_NUM_VOICES];
static unsigned char s_voicesGuardHi[PS2_GUARD_SIZE];
static volatile int s_masterVolFix = PS2_VOL_ONE; // master sfx volume, 0..4096
static volatile int s_mixerRun = 0;
// Pause NESTING COUNT (>0 = paused). The mixer's audsrv_wait/play_audio are
// SIF RPCs to the IOP with no Ps2IoLock; file reads are the other SIF caller.
// Concurrent SIF RPC is not thread-safe on this stack (shared with usbhdfsd),
// so on real hardware the two collide and wedge the IOP — a silent freeze
// whose position wanders with I/O timing, and which never reproduces on PCSX2
// (it serializes SIF). While this is >0 the mixer issues no audsrv RPCs at all
// (the screen goes silent), leaving the IOP entirely to file IO. Raised by the
// loading thread for its whole run, and by every other file-IO site that can
// execute with the mixer live (music OGG loads, lazy sample decodes).
static volatile int s_mixerLoadPause = 0;
// Handshake: 1 while the mixer is sleeping in its pause branch, i.e. outside
// any audsrv RPC. Ps2MixerIoPauseBegin waits for this so a caller never starts
// fio with an audsrv RPC still in flight.
static volatile int s_mixerParked = 0;
// Liveness diagnostics, read by Ps2MixerHealthTick on the main thread. Beat
// increments once per loop pass (park spins included); spot records the phase
// the mixer entered last, so a frozen beat pinpoints WHERE it is stuck.
#define PS2_MIXSPOT_PARKED 1
#define PS2_MIXSPOT_REINIT 2
#define PS2_MIXSPOT_MIX    3
#define PS2_MIXSPOT_WAIT   4
#define PS2_MIXSPOT_PLAY   5
static volatile unsigned int s_mixerBeat = 0;
static volatile int          s_mixerSpot = 0;
static int s_mixerThreadId = -1;
// 32KB proved uncomfortably close to the vorbis-decode high-water mark; an
// overflow lands straight in s_voices (declared just above in the same .bss).
static u8 s_mixerStack[128 * 1024] __attribute__((aligned(16)));

static void Ps2FillAudioGuards()
{
    memset(s_voicesGuardLo, PS2_GUARD_FILL, sizeof(s_voicesGuardLo));
    memset(s_voicesGuardHi, PS2_GUARD_FILL, sizeof(s_voicesGuardHi));
    // Low-watermark canary in the mixer stack's bottom bytes: if the mixer
    // ever gets within 256 bytes of overflowing, this pattern dies first.
    memset(s_mixerStack, PS2_GUARD_FILL, 256);
}

// Main-thread only (printf is a fio RPC; never call this from the mixer).
static void Ps2CheckAudioGuards(const char* theWhere)
{
    static bool aReported[3];
    struct { const unsigned char* mPtr; int mLen; int mIdx; const char* mName; } aChecks[] = {
        { s_voicesGuardLo, PS2_GUARD_SIZE, 0, "voices-lo" },
        { s_voicesGuardHi, PS2_GUARD_SIZE, 1, "voices-hi" },
        { s_mixerStack,    256,            2, "mixer-stack-bottom" },
    };
    for (int c = 0; c < 3; c++)
    {
        if (aReported[aChecks[c].mIdx])
            continue;
        for (int i = 0; i < aChecks[c].mLen; i++)
        {
            if (aChecks[c].mPtr[i] != PS2_GUARD_FILL)
            {
                aReported[aChecks[c].mIdx] = true;
                Ps2IoLockAcquire();
                printf("[PS2] AUDIO GUARD '%s' corrupted at +%d (%02X) — %s\n",
                    aChecks[c].mName, i, aChecks[c].mPtr[i], theWhere);
                Ps2IoLockRelease();
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Streamed music state. The compressed OGG lives entirely in EE RAM; the
// mixer thread pulls PCM out of stb_vorbis a small buffer at a time and
// resamples it into the mix. The main thread never touches the decoder while
// s_musActive is set: to stop, it raises s_musStopReq and waits for the mixer
// to drop s_musActive (same handshake as voice teardown), then frees.
// ---------------------------------------------------------------------------

#define PS2_MUS_BUF 1024 // decoded frames per refill

static stb_vorbis*       s_musVorbis = NULL;
static unsigned char*    s_musData = NULL;
static volatile int      s_musActive = 0;    // owned by the mixer once set
static volatile int      s_musStopReq = 0;
static volatile int      s_musPaused = 0;
static volatile int      s_musLoop = 1;
static volatile int      s_musVolFix = PS2_VOL_ONE;    // per-song volume (fades)
static volatile int      s_musMasterFix = PS2_VOL_ONE; // options music slider
static int               s_musChannels = 2;
static unsigned int      s_musStep = 65536;  // 16.16: srcRate / PS2_MIX_RATE
static short             s_musBuf[PS2_MUS_BUF * 2] __attribute__((aligned(64)));
static int               s_musBufFrames = 0;
static unsigned int      s_musBufPos = 0;    // 16.16 cursor inside s_musBuf

// Runs on the mixer thread. Decodes/resamples the current music stream into
// the accumulator. No allocation happens here: stb_vorbis pull-API decoding
// reuses buffers set up at open time.
static void Ps2MusicMixInto(int* acc)
{
    if (!s_musActive)
        return;
    if (s_musStopReq)
    {
        s_musActive = 0; // main thread may now free the decoder
        return;
    }
    if (s_musPaused)
        return;

    // Pairs with the publish barrier in Ps2MusicPlay.
    __asm__ __volatile__("" ::: "memory");

    const int vol = (s_musVolFix * s_musMasterFix) >> 12;
    for (int i = 0; i < PS2_MIX_CHUNK; i++)
    {
        // Refill the rolling decode buffer when the cursor runs off its end.
        while ((int)(s_musBufPos >> 16) >= s_musBufFrames)
        {
            s_musBufPos -= (unsigned int)s_musBufFrames << 16;
            int n = stb_vorbis_get_samples_short_interleaved(
                s_musVorbis, s_musChannels, s_musBuf, PS2_MUS_BUF * s_musChannels);
            if (n <= 0)
            {
                if (s_musLoop)
                {
                    stb_vorbis_seek_start(s_musVorbis);
                    n = stb_vorbis_get_samples_short_interleaved(
                        s_musVorbis, s_musChannels, s_musBuf, PS2_MUS_BUF * s_musChannels);
                }
                if (n <= 0)
                {
                    s_musActive = 0;
                    return;
                }
            }
            s_musBufFrames = n;
        }

        unsigned int idx = s_musBufPos >> 16;
        int frac = (s_musBufPos >> 8) & 0xFF;
        int l, r;
        if (s_musChannels == 1)
        {
            int s0 = s_musBuf[idx];
            int s1 = (idx + 1 < (unsigned int)s_musBufFrames) ? s_musBuf[idx + 1] : s0;
            l = r = s0 + (((s1 - s0) * frac) >> 8);
        }
        else
        {
            int l0 = s_musBuf[idx * 2],     r0 = s_musBuf[idx * 2 + 1];
            int l1 = l0, r1 = r0;
            if (idx + 1 < (unsigned int)s_musBufFrames)
            {
                l1 = s_musBuf[(idx + 1) * 2];
                r1 = s_musBuf[(idx + 1) * 2 + 1];
            }
            l = l0 + (((l1 - l0) * frac) >> 8);
            r = r0 + (((r1 - r0) * frac) >> 8);
        }

        acc[i * 2 + 0] += (l * vol) >> 12;
        acc[i * 2 + 1] += (r * vol) >> 12;
        s_musBufPos += s_musStep;
    }
}

// ---------------------------------------------------------------------------
// Mixer thread
// ---------------------------------------------------------------------------

static void Ps2MixerThread(void*)
{
    static short s_out[PS2_MIX_CHUNK * 2] __attribute__((aligned(64)));
    static int   s_acc[PS2_MIX_CHUNK * 2];

    while (s_mixerRun)
    {
        s_mixerBeat++;

        // While the loading thread owns the IOP, issue no audsrv SIF RPCs. Sleep
        // ~one chunk's worth so we yield instead of busy-spinning (EE threads are
        // cooperative; a tight loop would starve the loading thread we are trying
        // to protect). The SPU2 ring drains to silence and refills once resumed.
        if (s_mixerLoadPause)
        {
            s_mixerSpot = PS2_MIXSPOT_PARKED;
            s_mixerParked = 1;  // no audsrv RPC can be in flight from here on
            usleep(PS2_MIX_CHUNK * 1000000 / PS2_MIX_RATE);
            continue;
        }
        if (s_mixerParked)
        {
            s_mixerSpot = PS2_MIXSPOT_REINIT;
            // Coming back from a park. After a multi-second starvation with the
            // SPU2 already streaming, audsrv wedges and the next
            // audsrv_wait_audio never returns (audio stays dead for the rest of
            // the session). Re-arm it — stop + format + volume, same sequence
            // as boot init — before touching the ring again. s_mixerParked must
            // be cleared BEFORE these RPCs: a concurrent Ps2MixerIoPauseBegin
            // waits on it, and seeing it still set would let fio RPCs collide
            // with this re-init on real hardware. No printf here: fio RPCs are
            // unsafe from the mixer thread.
            s_mixerParked = 0;
            audsrv_stop_audio();
            audsrv_fmt_t fmt;
            fmt.freq = PS2_MIX_RATE;
            fmt.bits = 16;
            fmt.channels = 2;
            audsrv_set_format(&fmt);
            audsrv_set_volume(MAX_VOLUME);
        }

        s_mixerSpot = PS2_MIXSPOT_MIX;
        memset(s_acc, 0, sizeof(s_acc));

        int master = s_masterVolFix;
        for (int v = 0; v < PS2_NUM_VOICES; v++)
        {
            Ps2Voice& voice = s_voices[v];
            if (!voice.mActive)
                continue;
            if (voice.mStopRequest)
            {
                voice.mActive = 0;
                continue;
            }

            // Don't let the compiler hoist the field reads above the volatile
            // mActive check (pairs with the publish barrier in StartVoice).
            __asm__ __volatile__("" ::: "memory");
            const short* data = voice.mData;
            const unsigned long long end = (unsigned long long)voice.mNumSamples << 16;
            unsigned long long pos = voice.mPos;
            const unsigned int step = voice.mStep;
            const int volL = (voice.mVolL * master) >> 12;
            const int volR = (voice.mVolR * master) >> 12;

            for (int i = 0; i < PS2_MIX_CHUNK; i++)
            {
                if (pos >= end)
                {
                    if (!voice.mLooping || voice.mNumSamples <= 0)
                    {
                        voice.mActive = 0;
                        break;
                    }
                    pos -= end;
                }
                // Linear interpolation between neighbouring samples.
                unsigned int idx = (unsigned int)(pos >> 16);
                int frac = (pos >> 8) & 0xFF;
                int s0 = data[idx];
                int s1 = (idx + 1 < (unsigned int)voice.mNumSamples) ? data[idx + 1] : s0;
                int s = s0 + (((s1 - s0) * frac) >> 8);

                s_acc[i * 2 + 0] += (s * volL) >> 12;
                s_acc[i * 2 + 1] += (s * volR) >> 12;
                pos += step;
            }
            voice.mPos = pos;
        }

        Ps2MusicMixInto(s_acc);

        for (int i = 0; i < PS2_MIX_CHUNK * 2; i++)
        {
            int s = s_acc[i];
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            s_out[i] = (short)s;
        }

        // audsrv_wait_audio blocks until the SPU2 has drained enough of its ring
        // to accept another chunk — this is what paces the mixer to real time.
        // (Do NOT replace it with a fillbuf/semaphore scheme: without this block
        // the mixer free-runs at CPU speed and audio plays sped-up/choppy.)
        s_mixerSpot = PS2_MIXSPOT_WAIT;
        audsrv_wait_audio(sizeof(s_out));
        s_mixerSpot = PS2_MIXSPOT_PLAY;
        audsrv_play_audio((const char*)s_out, sizeof(s_out));
    }

    ExitThread();
}

// Ask every voice reading thePCM to stop and wait until the mixer drops it.
// The wait must block (usleep) instead of spinning: EE threads have no time
// slicing, so a tight loop on the main thread starves the mixer and mActive
// never clears (observed as a hard freeze in the first PurgeSounds call).
// Bounded so a wedged mixer can't hang the game; returns false in that case
// and the caller must leak the buffer instead of freeing it under the mixer.
static bool Ps2StopVoicesReading(const short* thePCM)
{
    if (s_mixerLoadPause && s_mixerParked)
    {
        // Parked mixer isn't reading any voice and our caller keeps the pause
        // raised, so it can't resume mid-loop: clear directly instead of
        // waiting on a handshake the parked mixer would never answer.
        for (int v = 0; v < PS2_NUM_VOICES; v++)
            if (s_voices[v].mData == thePCM)
                s_voices[v].mActive = 0;
        return true;
    }

    bool anyActive = false;
    for (int v = 0; v < PS2_NUM_VOICES; v++)
    {
        if (s_voices[v].mData == thePCM && s_voices[v].mActive)
        {
            s_voices[v].mStopRequest = 1;
            anyActive = true;
        }
    }
    if (!anyActive)
        return true;

    for (int aTries = 0; aTries < 100; aTries++)  // ~200ms >> one 23ms mixer chunk
    {
        bool stillActive = false;
        for (int v = 0; v < PS2_NUM_VOICES; v++)
            if (s_voices[v].mData == thePCM && s_voices[v].mActive)
                stillActive = true;
        if (!stillActive)
            return true;
        usleep(2000);
    }
    printf("[PS2] sound: voice stop timed out, leaking sample\n");
    return false;
}

// Raise the pause count, then wait (bounded) until the mixer is parked at the
// top of its loop. Without the wait the mixer can already be past its pause
// check, inside audsrv_wait/play — a ~46ms window in which the caller's first
// fio RPC would collide with the in-flight audsrv RPC on real hardware. The
// counter is mutated under Ps2IoLock (recursive, so callers that already hold
// it — e.g. EnsureSampleLoadedLocked — nest fine); the mixer itself never
// takes that lock, so waiting while holding it cannot deadlock.
static int s_pauseBegins = 0; // lifetime Begin/End totals for the health log:
static int s_pauseEnds = 0;   // a lasting imbalance = leaked pause scope

void Sexy::Ps2MixerIoPauseBegin()
{
    Ps2IoLockAcquire();
    s_mixerLoadPause = s_mixerLoadPause + 1;
    s_pauseBegins++;
    Ps2IoLockRelease();

    if (s_mixerRun && s_mixerThreadId >= 0)
    {
        // Typical wait is one chunk (~46ms); bounded so a wedged mixer can't
        // hang the caller forever.
        for (int aTries = 0; aTries < 100 && !s_mixerParked; aTries++)
            usleep(2000);
    }
}

void Sexy::Ps2MixerIoPauseEnd()
{
    Ps2IoLockAcquire();
    if (s_mixerLoadPause > 0)
        s_mixerLoadPause = s_mixerLoadPause - 1;
    s_pauseEnds++;
    Ps2IoLockRelease();
}

// Called by the loading thread around its work (LawnApp::LoadingThreadProc) so
// the mixer stops issuing audsrv SIF RPCs that would race the file I/O. Kept
// as the bool-flavoured wrapper over the nestable pause above.
void Sexy::Ps2MixerSetLoadPause(bool thePaused)
{
    if (thePaused)
        Ps2MixerIoPauseBegin();
    else
        Ps2MixerIoPauseEnd();
}

// Main-thread mixer liveness report (printf = fio RPC, never call from the
// mixer). Call ~1/s: prints on every state change plus a slow heartbeat, so
// it cannot flood the log. States: OFF (thread never ran / shut down), PAUSED
// (pause count held), STALLED@n (beat frozen — the mixer is stuck inside the
// phase named by spot n, see PS2_MIXSPOT_*), alive.
void Sexy::Ps2MixerHealthTick()
{
    static unsigned int aLastBeat = 0;
    static int aLastState = -1;
    static int aCalls = 0;

    unsigned int aBeat = s_mixerBeat;
    bool aMoved = (aBeat != aLastBeat);
    aLastBeat = aBeat;

    int aState;
    if (!s_mixerRun)
        aState = 0;
    else if (!aMoved)
        aState = 100 + s_mixerSpot; // stalled, one state per stuck-spot
    else if (s_mixerLoadPause > 0)
        aState = 1;
    else
        aState = 2;

    aCalls++;
    if (aState == aLastState && (aCalls % 30) != 0)
        return;
    aLastState = aState;

    int aVoices = 0;
    for (int v = 0; v < PS2_NUM_VOICES; v++)
        if (s_voices[v].mActive)
            aVoices++;

    const char* aLabel = (aState == 0) ? "OFF"
                       : (aState == 1) ? "PAUSED"
                       : (aState == 2) ? "alive"
                       : "STALLED";
    Ps2IoLockAcquire();
    printf("[PS2] mixer %s: beat=%u spot=%d pause=%d(b%d/e%d) parked=%d mus=%d/%d voices=%d vol=%d\n",
        aLabel, aBeat, (int)s_mixerSpot, (int)s_mixerLoadPause, s_pauseBegins, s_pauseEnds,
        (int)s_mixerParked, (int)s_musActive, (int)s_musPaused, aVoices, (int)s_masterVolFix);
    Ps2IoLockRelease();
}

// ---------------------------------------------------------------------------
// Streamed music API (main/loading thread only)
// ---------------------------------------------------------------------------

// Blocks (bounded) until the mixer is no longer touching the music stream,
// then tears the decoder down.
void Sexy::Ps2MusicStop()
{
    if (s_musActive)
    {
        if (s_mixerLoadPause && s_mixerParked)
        {
            // Mixer is parked (holds no reference into the decoder), and our
            // caller keeps the pause raised for the whole operation, so it
            // stays parked: drop the stream directly. The handshake below
            // would otherwise time out (the parked mixer never processes
            // s_musStopReq) and leak the multi-MB OGG buffer.
            s_musActive = 0;
        }
        else
        {
            s_musStopReq = 1;
            for (int aTries = 0; aTries < 100 && s_musActive; aTries++)
                usleep(2000);
        }
    }

    if (s_musActive)
    {
        // Wedged mixer: leak instead of freeing under it.
        printf("[PS2] music: stop timed out, leaking stream\n");
        s_musVorbis = NULL;
        s_musData = NULL;
        return;
    }

    if (s_musVorbis)
    {
        stb_vorbis_close(s_musVorbis);
        s_musVorbis = NULL;
    }
    if (s_musData)
    {
        free(s_musData);
        s_musData = NULL;
    }
    s_musStopReq = 0;
}

bool Sexy::Ps2MusicPlay(unsigned char* theOggData, int theSize, bool theLoop)
{
    if (!s_mixerRun || theOggData == NULL || theSize <= 0)
    {
        free(theOggData);
        return false;
    }

    Ps2MusicStop();

    int err = 0;
    stb_vorbis* v = stb_vorbis_open_memory(theOggData, theSize, &err, NULL);
    if (v == NULL)
    {
        printf("[PS2] music: vorbis open failed (%d)\n", err);
        free(theOggData);
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(v);
    s_musVorbis = v;
    s_musData = theOggData;
    s_musChannels = (info.channels >= 2) ? 2 : 1;
    s_musStep = (unsigned int)(((unsigned long long)info.sample_rate << 16) / PS2_MIX_RATE);
    s_musBufFrames = 0;
    s_musBufPos = 0;
    s_musLoop = theLoop ? 1 : 0;
    s_musPaused = 0;
    s_musVolFix = PS2_VOL_ONE;
    s_musStopReq = 0;
    // Order the decoder-state stores before the volatile publish.
    __asm__ __volatile__("" ::: "memory");
    s_musActive = 1; // publish last
    printf("[PS2] music: playing %d bytes, %luHz %dch loop=%d\n",
        theSize, (unsigned long)info.sample_rate, s_musChannels, s_musLoop);
    return true;
}

void Sexy::Ps2MusicSetPaused(bool thePaused)
{
    s_musPaused = thePaused ? 1 : 0;
}

bool Sexy::Ps2MusicIsPlaying()
{
    return s_musActive && !s_musPaused;
}

void Sexy::Ps2MusicSetVolume(double theVolume)
{
    int fix = (int)(theVolume * PS2_VOL_ONE);
    if (fix < 0) fix = 0;
    if (fix > PS2_VOL_ONE) fix = PS2_VOL_ONE;
    s_musVolFix = fix;
}

void Sexy::Ps2MusicSetMasterVolume(double theVolume)
{
    int fix = (int)(theVolume * PS2_VOL_ONE);
    if (fix < 0) fix = 0;
    if (fix > PS2_VOL_ONE) fix = PS2_VOL_ONE;
    s_musMasterFix = fix;
}

// ---------------------------------------------------------------------------
// Decoding helpers
// ---------------------------------------------------------------------------

// Convert interleaved s16 PCM at (rate, channels) to 22050Hz mono. Returns a
// malloc'd buffer and sample count.
static short* Ps2ResampleToMix(const short* src, int srcSamples, int channels, int rate, int* outSamples)
{
    if (srcSamples <= 0 || channels <= 0 || rate <= 0)
        return NULL;

    int dstSamples = (int)((long long)srcSamples * PS2_MIX_RATE / rate);
    if (dstSamples < 1)
        dstSamples = 1;

    short* dst = (short*)malloc(dstSamples * sizeof(short));
    if (!dst)
        return NULL;

    for (int i = 0; i < dstSamples; i++)
    {
        // 16.16 source cursor with linear interpolation.
        long long srcPosFix = (long long)i * rate * 65536 / PS2_MIX_RATE;
        int idx = (int)(srcPosFix >> 16);
        int frac = (int)((srcPosFix >> 8) & 0xFF);
        if (idx >= srcSamples - 1)
        {
            idx = srcSamples - 1;
            frac = 0;
        }

        int a, b;
        if (channels == 1)
        {
            a = src[idx];
            b = (idx + 1 < srcSamples) ? src[idx + 1] : a;
        }
        else // downmix stereo
        {
            a = ((int)src[idx * 2] + src[idx * 2 + 1]) >> 1;
            b = (idx + 1 < srcSamples)
                ? (((int)src[(idx + 1) * 2] + src[(idx + 1) * 2 + 1]) >> 1) : a;
        }
        dst[i] = (short)(a + (((b - a) * frac) >> 8));
    }

    *outSamples = dstSamples;
    return dst;
}

// Minimal RIFF/WAVE reader: PCM (format 1), 8 or 16 bit, mono/stereo.
static short* Ps2DecodeWav(const unsigned char* buf, int len, int* outSamples)
{
    if (len < 44 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0)
        return NULL;

    int fmtTag = 0, channels = 0, rate = 0, bits = 0;
    const unsigned char* data = NULL;
    int dataLen = 0;

    int pos = 12;
    while (pos + 8 <= len)
    {
        const unsigned char* chunk = buf + pos;
        int chunkLen = chunk[4] | (chunk[5] << 8) | (chunk[6] << 16) | (chunk[7] << 24);
        const unsigned char* body = chunk + 8;
        if (memcmp(chunk, "fmt ", 4) == 0 && chunkLen >= 16)
        {
            fmtTag   = body[0] | (body[1] << 8);
            channels = body[2] | (body[3] << 8);
            rate     = body[4] | (body[5] << 8) | (body[6] << 16) | (body[7] << 24);
            bits     = body[14] | (body[15] << 8);
        }
        else if (memcmp(chunk, "data", 4) == 0)
        {
            data = body;
            dataLen = chunkLen;
        }
        pos += 8 + chunkLen + (chunkLen & 1);
    }

    if (fmtTag != 1 || !data || dataLen <= 0 || (bits != 8 && bits != 16) ||
        (channels != 1 && channels != 2) || rate <= 0)
        return NULL;

    if (dataLen > len - (int)(data - buf))
        dataLen = len - (int)(data - buf);

    int frames = dataLen / (channels * (bits / 8));
    short* pcm16;
    short* tmp = NULL;
    if (bits == 16)
        pcm16 = (short*)data;
    else
    {
        tmp = (short*)malloc(frames * channels * sizeof(short));
        if (!tmp)
            return NULL;
        for (int i = 0; i < frames * channels; i++)
            tmp[i] = (short)((data[i] - 128) << 8);
        pcm16 = tmp;
    }

    short* out = Ps2ResampleToMix(pcm16, frames, channels, rate, outSamples);
    free(tmp);
    return out;
}

// ---------------------------------------------------------------------------
// Ps2SoundManager
// ---------------------------------------------------------------------------

Ps2SoundManager::Ps2SoundManager()
{
    mInitialized = false;
    mMasterVolume = 1.0;
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
    {
        mBaseVolumes[i] = 1.0;
        mBasePans[i] = 0;
    }
    memset(s_voices, 0, sizeof(s_voices));
    Ps2FillAudioGuards();

    if (!Ps2AudioAvailable())
    {
        printf("[PS2] sound: audsrv module missing, staying silent\n");
        return;
    }

    int ret = audsrv_init();
    if (ret != 0)
    {
        printf("[PS2] audsrv_init failed: %s\n", audsrv_get_error_string());
        return;
    }

    audsrv_fmt_t fmt;
    fmt.freq = PS2_MIX_RATE;
    fmt.bits = 16;
    fmt.channels = 2;
    if (audsrv_set_format(&fmt) != 0)
    {
        printf("[PS2] audsrv_set_format failed: %s\n", audsrv_get_error_string());
        return;
    }
    audsrv_set_volume(MAX_VOLUME);

    s_mixerRun = 1;
    ee_thread_t th;
    memset(&th, 0, sizeof(th));
    th.func = (void*)Ps2MixerThread;
    th.stack = s_mixerStack;
    th.stack_size = sizeof(s_mixerStack);
    th.gp_reg = (void*)&_gp;
    th.initial_priority = 20; // higher priority than the game loop
    s_mixerThreadId = CreateThread(&th);
    if (s_mixerThreadId >= 0)
    {
        StartThread(s_mixerThreadId, NULL);
        mInitialized = true;
        printf("[PS2] sound: audsrv mixer running (%dHz, %d voices)\n", PS2_MIX_RATE, PS2_NUM_VOICES);
    }
    else
    {
        s_mixerRun = 0;
        printf("[PS2] sound: CreateThread failed (%d)\n", s_mixerThreadId);
    }
}

Ps2SoundManager::~Ps2SoundManager()
{
    StopAllSounds();
    s_mixerRun = 0;
    ReleaseSounds();
    for (std::list<Ps2SoundInstance*>::iterator it = mInstances.begin(); it != mInstances.end(); ++it)
        delete *it;
    mInstances.clear();
}

void Ps2SoundManager::ReapDeadInstances()
{
    for (std::list<Ps2SoundInstance*>::iterator it = mInstances.begin(); it != mInstances.end();)
    {
        Ps2SoundInstance* inst = *it;
        if ((inst->mReleased || inst->mAutoRelease) && !inst->VoiceAlive())
        {
            delete inst;
            it = mInstances.erase(it);
        }
        else
            ++it;
    }
}

int Ps2SoundManager::StartVoice(const Ps2Sample& theSample, unsigned int theStep, int theVolL, int theVolR, bool looping, int& theSerialOut)
{
    if (!mInitialized || theSample.mPCM == NULL || theSample.mNumSamples <= 0)
        return -1;

    Ps2CheckAudioGuards("StartVoice");

    for (int v = 0; v < PS2_NUM_VOICES; v++)
    {
        Ps2Voice& voice = s_voices[v];
        if (voice.mActive)
            continue;

        voice.mStopRequest = 0;
        voice.mData = theSample.mPCM;
        voice.mNumSamples = theSample.mNumSamples;
        voice.mPos = 0;
        voice.mStep = theStep;
        voice.mVolL = theVolL;
        voice.mVolR = theVolR;
        voice.mLooping = looping ? 1 : 0;
        voice.mSerial++;
        theSerialOut = voice.mSerial;
        // The non-volatile field stores above must be emitted before the
        // volatile publish below (volatile only orders volatile accesses).
        __asm__ __volatile__("" ::: "memory");
        voice.mActive = 1; // publish last
        return v;
    }
    return -1;
}

bool Ps2SoundManager::Initialized()
{
    return mInitialized;
}

// The resource loading pthread (LoadingSounds group, foley setup) and the
// main thread (PlaySample — including the title screen's loading-bar sprout
// sounds, which fire while LoadingSounds is still being loaded) both enter
// the manager. mSourceFileNames/mSamples/mInstances are not thread-safe, so
// every mutating entry point serializes on the recursive SIF/IO lock.
bool Ps2SoundManager::LoadSound(unsigned int theSfxID, const std::string& theFilename)
{
    if (theSfxID >= MAX_SOURCE_SOUNDS)
        return false;

    Ps2IoLockAcquire();
    ReleaseSound(theSfxID);
    // Lazy: only remember the filename; the PCM is decoded on the first
    // GetSoundInstance() and can be purged again at screen transitions.
    mSourceFileNames[theSfxID] = theFilename;
    Ps2IoLockRelease();
    return true;
}

bool Ps2SoundManager::EnsureSampleLoaded(unsigned int theSfxID)
{
    if (theSfxID >= MAX_SOURCE_SOUNDS)
        return false;

    Ps2IoLockAcquire();
    bool aLoaded = EnsureSampleLoadedLocked(theSfxID);
    Ps2IoLockRelease();
    return aLoaded;
}

bool Ps2SoundManager::EnsureSampleLoadedLocked(unsigned int theSfxID)
{
    if (mSamples[theSfxID].mPCM != NULL)
        return true;
    if (!mInitialized || mSourceFileNames[theSfxID].empty())
        return false;

    // The decode below reads the sample file (fio RPCs). First play of a sound
    // mid-game lands here on the main thread with the mixer live; park it so
    // its audsrv RPCs can't collide with ours on real hardware. Nests under
    // the loading thread's whole-load pause.
    Ps2MixerIoPauseScope aMixerPause;

    const std::string& theFilename = mSourceFileNames[theSfxID];

    static const char* kFormats[] = { ".ogg", ".wav" };
    for (int f = 0; f < 2; f++)
    {
        std::string aFilename = theFilename + kFormats[f];
        PFILE* fp = p_fopen(aFilename.c_str(), "rb");
        if (!fp)
            continue;

        p_fseek(fp, 0, SEEK_END);
        int aSize = p_ftell(fp);
        p_fseek(fp, 0, SEEK_SET);
        unsigned char* aData = new unsigned char[aSize];
        p_fread(aData, 1, aSize, fp);
        p_fclose(fp);

        short* pcm = NULL;
        int numSamples = 0;

        if (f == 0)
        {
            int channels = 0, rate = 0;
            short* decoded = NULL;
            int frames = stb_vorbis_decode_memory(aData, aSize, &channels, &rate, &decoded);
            if (frames > 0 && decoded)
            {
                pcm = Ps2ResampleToMix(decoded, frames, channels, rate, &numSamples);
                free(decoded);
            }
        }
        else
            pcm = Ps2DecodeWav(aData, aSize, &numSamples);

        delete[] aData;

        if (pcm)
        {
            mSamples[theSfxID].mPCM = pcm;
            mSamples[theSfxID].mNumSamples = numSamples;
            return true;
        }
        printf("[PS2] sound: failed to decode %s\n", aFilename.c_str());
    }

    // Missing/undecodable sound: stay silent for this id. Clear the name so
    // we don't retry the decode on every play attempt.
    mSourceFileNames[theSfxID].clear();
    return false;
}

int Ps2SoundManager::LoadSound(const std::string& theFilename)
{
    Ps2IoLockAcquire();
    int aFound = -1;
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
        if (!mSourceFileNames[i].empty() && mSourceFileNames[i] == theFilename)
        {
            aFound = i;
            break;
        }

    if (aFound < 0)
    {
        int id = GetFreeSoundId();
        if (id >= 0 && LoadSound((unsigned int)id, theFilename))
            aFound = id;
    }
    Ps2IoLockRelease();
    return aFound;
}

void Ps2SoundManager::ReleaseSound(unsigned int theSfxID)
{
    if (theSfxID >= MAX_SOURCE_SOUNDS)
        return;

    Ps2IoLockAcquire();
    if (mSamples[theSfxID].mPCM != NULL)
    {
        // Kill any voice still reading this buffer before freeing it.
        if (Ps2StopVoicesReading(mSamples[theSfxID].mPCM))
            free(mSamples[theSfxID].mPCM);
        mSamples[theSfxID].mPCM = NULL;
        mSamples[theSfxID].mNumSamples = 0;
    }
    mSourceFileNames[theSfxID].clear();
    Ps2IoLockRelease();
}

void Ps2SoundManager::SetVolume(double theVolume)
{
    mMasterVolume = theVolume;
    int fix = (int)(theVolume * PS2_VOL_ONE);
    if (fix < 0) fix = 0;
    if (fix > PS2_VOL_ONE) fix = PS2_VOL_ONE;
    s_masterVolFix = fix;
}

bool Ps2SoundManager::SetBaseVolume(unsigned int theSfxID, double theBaseVolume)
{
    if (theSfxID >= MAX_SOURCE_SOUNDS)
        return false;
    mBaseVolumes[theSfxID] = theBaseVolume;
    return true;
}

bool Ps2SoundManager::SetBasePan(unsigned int theSfxID, int theBasePan)
{
    if (theSfxID >= MAX_SOURCE_SOUNDS)
        return false;
    mBasePans[theSfxID] = theBasePan;
    return true;
}

SoundInstance* Ps2SoundManager::GetSoundInstance(unsigned int theSfxID)
{
    Ps2IoLockAcquire();
    ReapDeadInstances();

    if (theSfxID >= MAX_SOURCE_SOUNDS || !EnsureSampleLoadedLocked(theSfxID))
    {
        Ps2IoLockRelease();
        return NULL;
    }

    Ps2SoundInstance* inst = new Ps2SoundInstance(this, (int)theSfxID);
    inst->mBaseVolume = mBaseVolumes[theSfxID];
    inst->mBasePan = mBasePans[theSfxID];
    mInstances.push_back(inst);
    Ps2IoLockRelease();
    return inst;
}

// Decode a sample ahead of time (e.g. at level start, where the cost hides
// inside the loading hitch) instead of on its first mid-gameplay play.
void Ps2SoundManager::PreloadSound(unsigned int theSfxID)
{
    if (theSfxID < MAX_SOURCE_SOUNDS)
        EnsureSampleLoaded(theSfxID);
}

void Ps2SoundManager::ReleaseSounds()
{
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
        ReleaseSound(i);
}

void Ps2SoundManager::PurgeSounds()
{
    Ps2IoLockAcquire();

    // Two-phase: ask every playing voice to stop first, then wait once for
    // the mixer to drop them all. The old per-sample Ps2StopVoicesReading
    // serialized one mixer-chunk wait per active sample, which added up to a
    // visible stall at screen transitions.
    for (int v = 0; v < PS2_NUM_VOICES; v++)
        if (s_voices[v].mActive)
            s_voices[v].mStopRequest = 1;

    int aPurged = 0;
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
    {
        if (mSamples[i].mPCM == NULL)
            continue;

        // Voices were already asked to stop above; this just waits (usually
        // zero or one mixer chunk in total across the whole purge).
        if (Ps2StopVoicesReading(mSamples[i].mPCM))
            free(mSamples[i].mPCM);
        mSamples[i].mPCM = NULL;
        mSamples[i].mNumSamples = 0;
        // mSourceFileNames stays: the sample re-decodes on the next play.
        aPurged++;
    }
    Ps2IoLockRelease();
    printf("[LAZY] purged %d sounds\n", aPurged);
}

void Ps2SoundManager::ReleaseChannels()
{
    StopAllSounds();
}

double Ps2SoundManager::GetMasterVolume()
{
    return mMasterVolume;
}

void Ps2SoundManager::SetMasterVolume(double theVolume)
{
    SetVolume(theVolume);
}

void Ps2SoundManager::Flush()
{
}

void Ps2SoundManager::SetCooperativeWindow(HWND)
{
}

void Ps2SoundManager::StopAllSounds()
{
    for (int v = 0; v < PS2_NUM_VOICES; v++)
        if (s_voices[v].mActive)
            s_voices[v].mStopRequest = 1;
    Ps2IoLockAcquire();
    ReapDeadInstances();
    Ps2IoLockRelease();
}

int Ps2SoundManager::GetFreeSoundId()
{
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
        if (mSamples[i].mPCM == NULL && mSourceFileNames[i].empty())
            return i;
    return -1;
}

int Ps2SoundManager::GetNumSounds()
{
    int count = 0;
    for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
        if (mSamples[i].mPCM != NULL)
            count++;
    return count;
}

// ---------------------------------------------------------------------------
// Ps2SoundInstance
// ---------------------------------------------------------------------------

Ps2SoundInstance::Ps2SoundInstance(Ps2SoundManager* theManager, int theSfxID)
{
    mManager = theManager;
    mSfxID = theSfxID;
    mVoice = -1;
    mVoiceSerial = 0;
    mBaseVolume = 1.0;
    mVolume = 1.0;
    mBasePan = 0;
    mPan = 0;
    mPitchRatio = 1.0;
    mReleased = false;
    mAutoRelease = false;
}

bool Ps2SoundInstance::VoiceAlive()
{
    return mVoice >= 0 && mVoice < PS2_NUM_VOICES &&
        s_voices[mVoice].mSerial == mVoiceSerial &&
        s_voices[mVoice].mActive;
}

void Ps2SoundInstance::ApplyVolume()
{
    double vol = mBaseVolume * mVolume;
    if (vol < 0.0) vol = 0.0;
    if (vol > 1.0) vol = 1.0;

    // Pan is in hundredths of a dB (negative = left). Attenuate the far side.
    double volL = vol, volR = vol;
    int pan = mBasePan + mPan;
    if (pan < 0)
        volR *= pow(10.0, pan / 2000.0);
    else if (pan > 0)
        volL *= pow(10.0, -pan / 2000.0);

    if (VoiceAlive())
    {
        s_voices[mVoice].mVolL = (int)(volL * PS2_VOL_ONE);
        s_voices[mVoice].mVolR = (int)(volR * PS2_VOL_ONE);
    }
}

void Ps2SoundInstance::Release()
{
    // Must halt playback, matching SDLSoundInstance::Release(). TodFoley stops
    // LOOPING foleys (sod-roll digger, rain, ...) via Release() alone — with a
    // mark-only release the looping voice never ends, plays forever, and its
    // slot is never reaped, so voices leak until the mixer saturates.
    Stop();
    mReleased = true; // reaped by the manager once the voice finishes
}

void Ps2SoundInstance::SetBaseVolume(double theBaseVolume)
{
    mBaseVolume = theBaseVolume;
    ApplyVolume();
}

void Ps2SoundInstance::SetBasePan(int theBasePan)
{
    mBasePan = theBasePan;
    ApplyVolume();
}

void Ps2SoundInstance::AdjustPitch(double theNumSteps)
{
    mPitchRatio = pow(2.0, theNumSteps / 12.0);
    if (VoiceAlive())
        s_voices[mVoice].mStep = (unsigned int)(65536.0 * mPitchRatio);
}

void Ps2SoundInstance::SetVolume(double theVolume)
{
    mVolume = theVolume;
    ApplyVolume();
}

void Ps2SoundInstance::SetPan(int thePosition)
{
    mPan = thePosition;
    ApplyVolume();
}

bool Ps2SoundInstance::Play(bool looping, bool autoRelease)
{
    Stop();
    mAutoRelease = autoRelease;

    // Long-lived instances (e.g. TodFoley's cached loops) can outlive a
    // PurgeSounds(): re-decode the sample here, not only in GetSoundInstance,
    // or this instance stays silent forever after a purge.
    if (!mManager->EnsureSampleLoaded(mSfxID))
        return false;

    double vol = mBaseVolume * mVolume;
    if (vol < 0.0) vol = 0.0;
    if (vol > 1.0) vol = 1.0;
    double volL = vol, volR = vol;
    int pan = mBasePan + mPan;
    if (pan < 0)
        volR *= pow(10.0, pan / 2000.0);
    else if (pan > 0)
        volL *= pow(10.0, -pan / 2000.0);

    unsigned int step = (unsigned int)(65536.0 * mPitchRatio);
    mVoice = mManager->StartVoice(mManager->mSamples[mSfxID], step,
        (int)(volL * PS2_VOL_ONE), (int)(volR * PS2_VOL_ONE), looping, mVoiceSerial);
    return mVoice >= 0;
}

void Ps2SoundInstance::Stop()
{
    if (VoiceAlive())
        s_voices[mVoice].mStopRequest = 1;
    mVoice = -1;
}

bool Ps2SoundInstance::IsPlaying()
{
    return VoiceAlive();
}

bool Ps2SoundInstance::IsReleased()
{
    return mReleased;
}

double Ps2SoundInstance::GetVolume()
{
    return mBaseVolume * mVolume;
}

#endif // PS2_PLATFORM
