/* Title: Timing routines
 */

#ifndef _al_included_altime_h
#define _al_included_altime_h

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/* Function: al_current_time
 *  Return the number of milliseconds since the Allegro library was
 *  initialised.  The return value is undefined if Allegro is uninitialised.
 */
AL_FUNC(unsigned long, al_current_time, (void));



/* Function: al_rest
 *  Waits for the specified number of milliseconds.
 */
AL_FUNC(void, al_rest, (long msecs));



AL_END_EXTERN_C

#endif
