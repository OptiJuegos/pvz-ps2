#include "Ps2IoLock.h"

#ifdef PS2_PLATFORM

#include <pthread.h>

/*
 * Recursive mutex so a single thread can nest calls (e.g. FOpen -> fcaseopen,
 * both of which take the lock) without self-deadlock. Initialized lazily via
 * pthread_once because ps2sdk's static PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
 * is not reliably available.
 */
static pthread_mutex_t sIoMutex;
static pthread_once_t  sIoOnce = PTHREAD_ONCE_INIT;

static void Ps2IoLockInit(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sIoMutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

void Ps2IoLockAcquire(void)
{
    pthread_once(&sIoOnce, Ps2IoLockInit);
    pthread_mutex_lock(&sIoMutex);
}

void Ps2IoLockRelease(void)
{
    pthread_mutex_unlock(&sIoMutex);
}

int Ps2IoLockTryAcquire(void)
{
    pthread_once(&sIoOnce, Ps2IoLockInit);
    /* pthread_mutex_trylock returns 0 when the lock was acquired. */
    return pthread_mutex_trylock(&sIoMutex) == 0;
}

#else /* !PS2_PLATFORM */

void Ps2IoLockAcquire(void) {}
void Ps2IoLockRelease(void) {}
int  Ps2IoLockTryAcquire(void) { return 1; }

#endif
