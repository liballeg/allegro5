#ifndef __al_included_allegro_aintern_wjoydxnu_h
#define __al_included_allegro_aintern_wjoydxnu_h


/** Part of the Windows joystick wrapper driver 
 * types are shared here for use by the haptic susbystem. */
 
typedef struct A5O_JOYSTICK_WINDOWS_ALL {
   A5O_JOYSTICK            parent;          /* must be first */
   bool                        active;
   int                         index;
   A5O_JOYSTICK          * handle;
   A5O_JOYSTICK_DRIVER   * driver;
} A5O_JOYSTICK_WINDOWS_ALL;

#endif

/* vim: set sts=3 sw=3 et: */
