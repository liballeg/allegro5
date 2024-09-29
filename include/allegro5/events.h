#ifndef __al_included_allegro5_events_h
#define __al_included_allegro5_events_h

#include "allegro5/altime.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_EVENT_TYPE
 */
typedef unsigned int A5O_EVENT_TYPE;

enum
{
   A5O_EVENT_JOYSTICK_AXIS               =  1,
   A5O_EVENT_JOYSTICK_BUTTON_DOWN        =  2,
   A5O_EVENT_JOYSTICK_BUTTON_UP          =  3,
   A5O_EVENT_JOYSTICK_CONFIGURATION      =  4,

   A5O_EVENT_KEY_DOWN                    = 10,
   A5O_EVENT_KEY_CHAR                    = 11,
   A5O_EVENT_KEY_UP                      = 12,

   A5O_EVENT_MOUSE_AXES                  = 20,
   A5O_EVENT_MOUSE_BUTTON_DOWN           = 21,
   A5O_EVENT_MOUSE_BUTTON_UP             = 22,
   A5O_EVENT_MOUSE_ENTER_DISPLAY         = 23,
   A5O_EVENT_MOUSE_LEAVE_DISPLAY         = 24,
   A5O_EVENT_MOUSE_WARPED                = 25,

   A5O_EVENT_TIMER                       = 30,

   A5O_EVENT_DISPLAY_EXPOSE              = 40,
   A5O_EVENT_DISPLAY_RESIZE              = 41,
   A5O_EVENT_DISPLAY_CLOSE               = 42,
   A5O_EVENT_DISPLAY_LOST                = 43,
   A5O_EVENT_DISPLAY_FOUND               = 44,
   A5O_EVENT_DISPLAY_SWITCH_IN           = 45,
   A5O_EVENT_DISPLAY_SWITCH_OUT          = 46,
   A5O_EVENT_DISPLAY_ORIENTATION         = 47,
   A5O_EVENT_DISPLAY_HALT_DRAWING        = 48,
   A5O_EVENT_DISPLAY_RESUME_DRAWING      = 49,

   A5O_EVENT_TOUCH_BEGIN                 = 50,
   A5O_EVENT_TOUCH_END                   = 51,
   A5O_EVENT_TOUCH_MOVE                  = 52,
   A5O_EVENT_TOUCH_CANCEL                = 53,
   
   A5O_EVENT_DISPLAY_CONNECTED           = 60,
   A5O_EVENT_DISPLAY_DISCONNECTED        = 61,

   A5O_EVENT_DROP                        = 62,
};


/* Function: A5O_EVENT_TYPE_IS_USER
 *
 *    1 <= n < 512  - builtin events
 *  512 <= n < 1024 - reserved user events (for addons)
 * 1024 <= n        - unreserved user events
 */
#define A5O_EVENT_TYPE_IS_USER(t)        ((t) >= 512)


/* Function: A5O_GET_EVENT_TYPE
 */
#define A5O_GET_EVENT_TYPE(a, b, c, d)   AL_ID(a, b, c, d)


/* Type: A5O_EVENT_SOURCE
 */
typedef struct A5O_EVENT_SOURCE A5O_EVENT_SOURCE;

struct A5O_EVENT_SOURCE
{
   int __pad[32];
};



/*
 * Event structures
 *
 * All event types have the following fields in common.
 *
 *  type      -- the type of event this is
 *  timestamp -- when this event was generated
 *  source    -- which event source generated this event
 *
 * For people writing event sources: The common fields must be at the
 * very start of each event structure, i.e. put _AL_EVENT_HEADER at the
 * front.
 */

#define _AL_EVENT_HEADER(srctype)                    \
   A5O_EVENT_TYPE type;                          \
   srctype *source;                                  \
   double timestamp;


typedef struct A5O_ANY_EVENT
{
   _AL_EVENT_HEADER(A5O_EVENT_SOURCE)
} A5O_ANY_EVENT;


typedef struct A5O_DISPLAY_EVENT
{
   _AL_EVENT_HEADER(struct A5O_DISPLAY)
   int x, y;
   int width, height;
   int orientation;
} A5O_DISPLAY_EVENT;


typedef struct A5O_JOYSTICK_EVENT
{
   _AL_EVENT_HEADER(struct A5O_JOYSTICK)
   struct A5O_JOYSTICK *id;
   int stick;
   int axis;
   float pos;
   int button;
} A5O_JOYSTICK_EVENT;



typedef struct A5O_KEYBOARD_EVENT
{
   _AL_EVENT_HEADER(struct A5O_KEYBOARD)
   struct A5O_DISPLAY *display; /* the window the key was pressed in */
   int keycode;                 /* the physical key pressed */
   int unichar;                 /* unicode character or negative */
   unsigned int modifiers;      /* bitfield */
   bool repeat;                 /* auto-repeated or not */
} A5O_KEYBOARD_EVENT;



typedef struct A5O_MOUSE_EVENT
{
   _AL_EVENT_HEADER(struct A5O_MOUSE)
   struct A5O_DISPLAY *display;
   /* (display) Window the event originate from
    * (x, y) Primary mouse position
    * (z) Mouse wheel position (1D 'wheel'), or,
    * (w, z) Mouse wheel position (2D 'ball')
    * (pressure) The pressure applied, for stylus (0 or 1 for normal mouse)
    */
   int x,  y,  z, w;
   int dx, dy, dz, dw;
   unsigned int button;
   float pressure;
} A5O_MOUSE_EVENT;



typedef struct A5O_TIMER_EVENT
{
   _AL_EVENT_HEADER(struct A5O_TIMER)
   int64_t count;
   double error;
} A5O_TIMER_EVENT;



typedef struct A5O_TOUCH_EVENT
{
   _AL_EVENT_HEADER(struct A5O_TOUCH_INPUT)
   struct A5O_DISPLAY *display;
   /* (id) Identifier of the event, always positive number.
    * (x, y) Touch position on the screen in 1:1 resolution.
    * (dx, dy) Relative touch position.
    * (primary) True, if touch is a primary one (usually first one).
    */
   int id;
   float x, y;
   float dx, dy;
   bool primary;
} A5O_TOUCH_EVENT;



/* Type: A5O_USER_EVENT
 */
typedef struct A5O_USER_EVENT A5O_USER_EVENT;

struct A5O_USER_EVENT
{
   _AL_EVENT_HEADER(struct A5O_EVENT_SOURCE)
   struct A5O_USER_EVENT_DESCRIPTOR *__internal__descr;
   intptr_t data1;
   intptr_t data2;
   intptr_t data3;
   intptr_t data4;
};



typedef struct A5O_DROP_EVENT
{
   _AL_EVENT_HEADER(struct A5O_DISPLAY)
   int x, y;
   int row;
   bool is_file;
   char *text;
   bool is_complete;
} A5O_DROP_EVENT;



/* Type: A5O_EVENT
 */
typedef union A5O_EVENT A5O_EVENT;

union A5O_EVENT
{
   /* This must be the same as the first field of _AL_EVENT_HEADER.  */
   A5O_EVENT_TYPE type;
   /* `any' is to allow the user to access the other fields which are
    * common to all event types, without using some specific type
    * structure.
    */
   A5O_ANY_EVENT      any;
   A5O_DISPLAY_EVENT  display;
   A5O_JOYSTICK_EVENT joystick;
   A5O_KEYBOARD_EVENT keyboard;
   A5O_MOUSE_EVENT    mouse;
   A5O_TIMER_EVENT    timer;
   A5O_TOUCH_EVENT    touch;
   A5O_USER_EVENT     user;
   A5O_DROP_EVENT     drop;
};



/* Event sources */

AL_FUNC(void, al_init_user_event_source, (A5O_EVENT_SOURCE *));
AL_FUNC(void, al_destroy_user_event_source, (A5O_EVENT_SOURCE *));
/* The second argument is A5O_EVENT instead of A5O_USER_EVENT
 * to prevent users passing a pointer to a too-short structure.
 */
AL_FUNC(bool, al_emit_user_event, (A5O_EVENT_SOURCE *, A5O_EVENT *,
                                   void (*dtor)(A5O_USER_EVENT *)));
AL_FUNC(void, al_unref_user_event, (A5O_USER_EVENT *));
AL_FUNC(void, al_set_event_source_data, (A5O_EVENT_SOURCE*, intptr_t data));
AL_FUNC(intptr_t, al_get_event_source_data, (const A5O_EVENT_SOURCE*));



/* Event queues */

/* Type: A5O_EVENT_QUEUE
 */
typedef struct A5O_EVENT_QUEUE A5O_EVENT_QUEUE;

AL_FUNC(A5O_EVENT_QUEUE*, al_create_event_queue, (void));
AL_FUNC(void, al_destroy_event_queue, (A5O_EVENT_QUEUE*));
AL_FUNC(bool, al_is_event_source_registered, (A5O_EVENT_QUEUE *, 
         A5O_EVENT_SOURCE *));
AL_FUNC(void, al_register_event_source, (A5O_EVENT_QUEUE*, A5O_EVENT_SOURCE*));
AL_FUNC(void, al_unregister_event_source, (A5O_EVENT_QUEUE*, A5O_EVENT_SOURCE*));
AL_FUNC(void, al_pause_event_queue, (A5O_EVENT_QUEUE*, bool));
AL_FUNC(bool, al_is_event_queue_paused, (const A5O_EVENT_QUEUE*));
AL_FUNC(bool, al_is_event_queue_empty, (A5O_EVENT_QUEUE*));
AL_FUNC(bool, al_get_next_event, (A5O_EVENT_QUEUE*, A5O_EVENT *ret_event));
AL_FUNC(bool, al_peek_next_event, (A5O_EVENT_QUEUE*, A5O_EVENT *ret_event));
AL_FUNC(bool, al_drop_next_event, (A5O_EVENT_QUEUE*));
AL_FUNC(void, al_flush_event_queue, (A5O_EVENT_QUEUE*));
AL_FUNC(void, al_wait_for_event, (A5O_EVENT_QUEUE*,
                                  A5O_EVENT *ret_event));
AL_FUNC(bool, al_wait_for_event_timed, (A5O_EVENT_QUEUE*,
                                        A5O_EVENT *ret_event,
                                        float secs));
AL_FUNC(bool, al_wait_for_event_until, (A5O_EVENT_QUEUE *queue,
                                        A5O_EVENT *ret_event,
                                        A5O_TIMEOUT *timeout));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
