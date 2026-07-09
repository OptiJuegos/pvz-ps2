#ifndef _H_CritSect
#define _H_CritSect

#include <pthread.h>
#include "Common.h"

class CritSync;

namespace Sexy
{

class CritSect 
{
private:
	pthread_mutex_t mCriticalSection;
	friend class AutoCrit;

public:
	CritSect(void);
	~CritSect(void);
};

}

#endif // _H_CritSect
