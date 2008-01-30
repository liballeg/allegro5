/* Title: Timing routines
 */

#ifndef _al_included_altime_h
#define _al_included_altime_h

#include "allegro5/base.h"

AL_BEGIN_EXTERN_C



/* Function: al_current_time
 *  Return the number of seconds since the Allegro library was
 *  initialised.  The return value is undefined if Allegro is uninitialised.
 *  microsecond resolution.
 */
AL_FUNC(double, al_current_time, (void));



/* Function: al_rest
 *  Waits for the specified number seconds.
 */
AL_FUNC(void, al_rest, (double seconds));



AL_END_EXTERN_C

#endif
