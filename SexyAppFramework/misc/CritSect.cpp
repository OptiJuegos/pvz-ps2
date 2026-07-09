#pragma warning( disable : 4786 )

#include "CritSect.h"

using namespace Sexy;

////////////////////////////////////////////////////////////////////////////////

CritSect::CritSect(void)
{
	pthread_mutexattr_t anAttr;
	pthread_mutexattr_init(&anAttr);
	// PTHREAD_MUTEX_RECURSIVE is an enum (not a #define) on some platforms
	// (e.g. the PS2 toolchain's pthreads-embedded), so it is invisible to the
	// preprocessor and an #if defined() guard silently produces a NORMAL
	// (non-recursive) mutex. The framework relies on recursive locking
	// (e.g. CleanSharedImages -> ~GLImage -> RemoveGLImage on the same
	// CritSect), which self-deadlocks on a NORMAL mutex.
	pthread_mutexattr_settype(&anAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mCriticalSection, &anAttr);
	pthread_mutexattr_destroy(&anAttr);
}

////////////////////////////////////////////////////////////////////////////////

CritSect::~CritSect(void)
{
	pthread_mutex_destroy(&mCriticalSection);
}
