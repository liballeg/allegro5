/* Title: Timing routines
 */

#ifndef _al_included_altime_h
#define _al_included_altime_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_TIMEOUT
 */
typedef struct ALLEGRO_TIMEOUT ALLEGRO_TIMEOUT;
struct ALLEGRO_TIMEOUT {
   uint64_t __pad1__;
   uint64_t __pad2__;
};



/* Function: al_current_time
 */
AL_FUNC(double, al_current_time, (void));



/* Function: al_rest
 */
AL_FUNC(void, al_rest, (double seconds));



/* Function: al_init_timeout
 */
AL_FUNC(void, al_init_timeout, (ALLEGRO_TIMEOUT *timeout, double seconds));



#ifdef __cplusplus
   }
#endif

#endif
