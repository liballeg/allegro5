#ifndef		__DEMO_FRAMEWORK_H__
#define		__DEMO_FRAMEWORK_H__

#include <allegro5/allegro.h>
#include "defines.h"

extern A5O_EVENT_QUEUE *event_queue;

/*
   Initializes Allegro, loads configuration settings, installs all
   required Allegro submodules, creates all game state objects, loads
   all data and does all other framework initialization tasks. Each
   successfull call to this function must be paired with a call to
   shutdown_framework()!

   Parameters:
      none

   Returns:
      Error code: DEMO_OK if initialization was successfull, otherwise
      the code of the error that caused the function to fail. See
      defines.h for a list of possible error codes.
*/
extern int init_framework(void);

/*
   Runs the framework main loop. The framework consists of user definable
   game state objects, each representing one screen or page. The main
   loop simply switches between these states according to internal and
   user specified inputs. Note: init_framework() must have been successfully
   called before this function may be used.

   Parameters:
      none

   Returns:
      nothing
*/
extern void run_framework(void);

/*
   Destroys all game state object and other framework modules that were
   initialized in init_framework().

   Parameters:
      none

   Returns:
      nothing
*/
extern void shutdown_framework(void);

extern bool key_down(int k);

#endif				/* __DEMO_FRAMEWORK_H__ */
