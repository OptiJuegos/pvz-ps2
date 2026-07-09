#ifndef SEXY_PS2_IOLOCK_H
#define SEXY_PS2_IOLOCK_H

/*
 * Global SIF RPC serialization lock.
 *
 * On the PS2 all `host:`/pak file I/O and libpad polling ultimately go through
 * SIF RPC (EE<->IOP), which is NOT thread-safe in ps2sdk. The resource loading
 * pthread (LawnApp::LoadingThreadProc) and the main thread's per-frame pad poll
 * (SexyAppBase::ProcessDeferredMessages) can otherwise issue overlapping RPC
 * requests, corrupting the in-flight packet and crashing in _request_end
 * (sifrpc.c). This recursive lock serializes both sides.
 *
 * C linkage so the C file fcaseopen.c can call it too. No-op when not building
 * for the PS2 platform.
 */

#ifdef __cplusplus
extern "C" {
#endif

void Ps2IoLockAcquire(void);
void Ps2IoLockRelease(void);

/* Non-blocking acquire. Returns non-zero if the lock was taken (caller must
 * Release), zero if it was already held by someone else. Used by the audio
 * mixer thread: it must serialize its audsrv SIF RPC against file I/O, but it
 * can never block on the lock (that would let a resource teardown that waits
 * for the mixer deadlock it), so it drops an audio chunk instead. Always
 * returns non-zero (a no-op "success") when not building for the PS2. */
int Ps2IoLockTryAcquire(void);

#ifdef __cplusplus
}
#endif

#endif /* SEXY_PS2_IOLOCK_H */
