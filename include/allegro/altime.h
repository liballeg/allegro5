#ifndef _al_included_altime_h
#define _al_included_altime_h

#include "allegro/base.h"

AL_BEGIN_EXTERN_C

AL_FUNC(unsigned long, al_current_time, (void));
AL_FUNC(void, al_rest, (long msecs));

AL_END_EXTERN_C

#endif
