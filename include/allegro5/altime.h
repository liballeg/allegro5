/* Title: Timing routines
 */

#ifndef _al_included_altime_h
#define _al_included_altime_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_TIMEOUT
 *  Represent a timeout value.  The size of the structure is known so can be
 *  statically allocated.  The contents are private.
 */
typedef struct ALLEGRO_TIMEOUT ALLEGRO_TIMEOUT;
struct ALLEGRO_TIMEOUT {
   uint64_t __pad1__;
   uint64_t __pad2__;
};



/* Function: al_current_time
 *  Return the number of seconds since the Allegro library was
 *  initialised. The return value is undefined if Allegro is uninitialised.
 *  The resolution depends on the used driver, but typically can be in the
 *  order of microseconds.
 */
AL_FUNC(double, al_current_time, (void));



/* Function: al_rest
 *  Waits for the specified number seconds. This tells the system to pause
 *  the current thread for the given amount of time. With some operating
 *  systems, the accuracy can be in the order of 10ms. That is, even
 *  > al_rest(0.000001)
 *  might pause for something like 10ms. Also see the section <Timer routines>
 *  on easier ways to time your program without using up all CPU.
 */
AL_FUNC(void, al_rest, (double seconds));



/* Function: al_init_timeout
 *  Set timeout value of some number of seconds after the function call.
 */
AL_FUNC(void, al_init_timeout, (ALLEGRO_TIMEOUT *timeout, double seconds));



#ifdef __cplusplus
   }
#endif

#endif
