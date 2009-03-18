/* Title: Events
 *
 * Overview of the event system
 *
 * An "event" is a thing that happens at some instant in time.  The fact that
 * this event occurred is captured in a piece of data, known also as an
 * "event".
 *
 * Event can be created by some certain types of objects, collectively known as
 * "event sources", and then placed into "event queues", in chronological
 * order.  The user can take events out of event queues, also in chronological
 * order, and examine them.
 *
 * >  event source 1 \
 * >                   \
 * >  event source 2 ---->--- event queue ----->---- user
 * >                   /
 * >  event source 3 /
 * >
 * >               Events are             The user takes events
 * >               generated and          out of the queue,
 * >               placed into		  examines them,
 * >               event queues.          and acts on them.
 */

#ifndef _al_included_events_h
#define _al_included_events_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: ALLEGRO_EVENT_TYPE
 */
typedef unsigned int ALLEGRO_EVENT_TYPE;


/* Enum: ALLEGRO_EVENT_TYPE
 *
 * Each event is of one of the following types
 *
 *  ALLEGRO_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
 *    Fields are: joystick.stick, joystick.axis, joystick.pos (-1.0 to 1.0).
 *
 *  ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
 *    Fields are: joystick.button.
 *    
 *  ALLEGRO_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
 *    Fields are: joystick.button.
 *
 *  ALLEGRO_EVENT_KEY_DOWN - a keyboard key was pressed.
 *    Fields: keyboard.keycode, keyboard.unichar, keyboard.modifiers.
 *
 *  ALLEGRO_EVENT_KEY_REPEAT - a typed character auto-repeated.
 *    Fields: keyboard.keycode (ALLEGRO_KEY_*), keyboard.unichar (unicode
 *    character), keyboard.modifiers (ALLEGRO_KEYMOD_*).
 *
 *  ALLEGRO_EVENT_KEY_UP - a keyboard key was released.
 *    Fields: keyboard.keycode.
 *
 *  ALLEGRO_EVENT_MOUSE_AXES - one or more mouse axis values changed.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.dx, mouse.dy, mouse.dz.
 *
 *  ALLEGRO_EVENT_MOUSE_BUTTON_DOWN - a mouse button was pressed.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.button.
 *
 *  ALLEGRO_EVENT_MOUSE_BUTTON_UP - a mouse button was released.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.button.
 *
 *  ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY - the mouse cursor entered a window
 *    opened by the program.
 *    Fields: mouse.x, mouse.y, mouse.z.
 *
 *  ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY - the mouse cursor leave the boundaries
 *    of a window opened by the program.
 *    Fields: mouse.x, mouse.y, mouse.z.
 *
 *  ALLEGRO_EVENT_TIMER - a timer counter incremented.
 *    Fields: timer.count.
 *
 *  ALLEGRO_EVENT_DISPLAY_EXPOSE - The display (or a portion thereof) has
 *    become visible.
 *    Fields: display.x, display.y, display.width, display.height
 *
 *  ALLEGRO_EVENT_DISPLAY_RESIZE - The window has been resized.
 *    Fields: display.x, display.y, display.width, display.height
 *
 *  ALLEGRO_EVENT_DISPLAY_CLOSE - The close button of the window has been
 *    pressed.
 *
 *  ALLEGRO_EVENT_DISPLAY_LOST - Displays can be lost with some drivers (just
 *    Direct3D?). This means that rendering is impossible. The device will be
 *    restored as soon as it is possible. The program should be able to ignore
 *    this event and continue rendering however it will have no effect.
 *
 *  ALLEGRO_EVENT_DISPLAY_FOUND - Generated when a lost device is regained.
 *    Drawing will no longer be a no-op.
 * 
 *  ALLEGRO_EVENT_DISPLAY_SWITCH_OUT - The window is no longer active, that
 *    is the user might have clicked into another window or "tabbed" away.
 * 
 *  ALLEGRO_EVENT_DISPLAY_SWITCH_IN - The window is the active one again.
 */
enum
{
   ALLEGRO_EVENT_JOYSTICK_AXIS               =  1,
   ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN        =  2,
   ALLEGRO_EVENT_JOYSTICK_BUTTON_UP          =  3,

   ALLEGRO_EVENT_KEY_DOWN                    = 10,
   ALLEGRO_EVENT_KEY_REPEAT                  = 11,
   ALLEGRO_EVENT_KEY_UP                      = 12,

   ALLEGRO_EVENT_MOUSE_AXES                  = 20,
   ALLEGRO_EVENT_MOUSE_BUTTON_DOWN           = 21,
   ALLEGRO_EVENT_MOUSE_BUTTON_UP             = 22,
   ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY         = 23,
   ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY         = 24,
   ALLEGRO_EVENT_MOUSE_WARPED                = 25,

   ALLEGRO_EVENT_TIMER                       = 30,

   ALLEGRO_EVENT_DISPLAY_EXPOSE              = 40,
   ALLEGRO_EVENT_DISPLAY_RESIZE              = 41,
   ALLEGRO_EVENT_DISPLAY_CLOSE               = 42,
   ALLEGRO_EVENT_DISPLAY_LOST                = 43,
   ALLEGRO_EVENT_DISPLAY_FOUND               = 44,
   ALLEGRO_EVENT_DISPLAY_SWITCH_IN           = 45,
   ALLEGRO_EVENT_DISPLAY_SWITCH_OUT          = 46
};


/* ALLEGRO_EVENT_TYPE_IS_USER
 * Not sure if this should be official part of the API or not.
 */
#define ALLEGRO_EVENT_TYPE_IS_USER(t)        ((t) >= 1024)


/* Function: ALLEGRO_GET_EVENT_TYPE
 */
#define ALLEGRO_GET_EVENT_TYPE(a, b, c, d)   AL_ID(a, b, c, d)


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
   ALLEGRO_EVENT_TYPE type;                          \
   srctype *source;                                  \
   double timestamp;


typedef struct ALLEGRO_ANY_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_EVENT_SOURCE)
} ALLEGRO_ANY_EVENT;



typedef struct ALLEGRO_DISPLAY_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_DISPLAY)
   int x, y;
   int width, height;
} ALLEGRO_DISPLAY_EVENT;



typedef struct ALLEGRO_JOYSTICK_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_JOYSTICK)
   int stick;
   int axis;
   float pos;
   int button;
} ALLEGRO_JOYSTICK_EVENT;



typedef struct ALLEGRO_KEYBOARD_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_KEYBOARD)
   struct ALLEGRO_DISPLAY *display; /* the window the key was pressed in */
   int keycode;                 /* the physical key pressed */
   unsigned int unichar;        /* unicode character */
   unsigned int modifiers;      /* bitfield */
} ALLEGRO_KEYBOARD_EVENT;



typedef struct ALLEGRO_MOUSE_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_MOUSE)
   struct ALLEGRO_DISPLAY *display;
   /* (display) Window the event originate from */
   /* (x, y) Primary mouse position */
   /* (z) Mouse wheel position (1D 'wheel'), or,  */
   /* (w, z) Mouse wheel position (2D 'ball') */
   int x,  y,  z, w;
   int dx, dy, dz, dw;
   unsigned int button;
} ALLEGRO_MOUSE_EVENT;



typedef struct ALLEGRO_TIMER_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_TIMER)
   long count;
   double error;
} ALLEGRO_TIMER_EVENT;



typedef struct ALLEGRO_USER_EVENT
{
   _AL_EVENT_HEADER(struct ALLEGRO_EVENT_SOURCE)
   struct ALLEGRO_USER_EVENT_DESCRIPTOR *__internal__descr;
   intptr_t data1;
   intptr_t data2;
   intptr_t data3;
   intptr_t data4;
} ALLEGRO_USER_EVENT;



/* Type: ALLEGRO_EVENT
 */
typedef union ALLEGRO_EVENT ALLEGRO_EVENT;

union ALLEGRO_EVENT
{
   /* This must be the same as the first field of _AL_EVENT_HEADER.  */
   ALLEGRO_EVENT_TYPE type;
   /* `any' is to allow the user to access the other fields which are
    * common to all event types, without using some specific type
    * structure.
    */
   ALLEGRO_ANY_EVENT      any;
   ALLEGRO_DISPLAY_EVENT  display;
   ALLEGRO_JOYSTICK_EVENT joystick;
   ALLEGRO_KEYBOARD_EVENT keyboard;
   ALLEGRO_MOUSE_EVENT    mouse;
   ALLEGRO_TIMER_EVENT    timer;
   ALLEGRO_USER_EVENT     user;
};



/* Event sources */

/* Type: ALLEGRO_EVENT_SOURCE
 */
typedef struct ALLEGRO_EVENT_SOURCE ALLEGRO_EVENT_SOURCE;

AL_FUNC(ALLEGRO_EVENT_SOURCE *, al_create_user_event_source, (void));
AL_FUNC(void, al_destroy_user_event_source, (ALLEGRO_EVENT_SOURCE *));
/* The second argument is ALLEGRO_EVENT instead of ALLEGRO_USER_EVENT
 * to prevent users passing a pointer to a too-short structure.
 */
AL_FUNC(bool, al_emit_user_event, (ALLEGRO_EVENT_SOURCE *, ALLEGRO_EVENT *,
                                   void (*dtor)(ALLEGRO_USER_EVENT *)));
AL_FUNC(void, al_unref_user_event, (ALLEGRO_USER_EVENT *));



/* Event queues */

typedef struct ALLEGRO_EVENT_QUEUE ALLEGRO_EVENT_QUEUE;

AL_FUNC(ALLEGRO_EVENT_QUEUE*, al_create_event_queue, (void));
AL_FUNC(void, al_destroy_event_queue, (ALLEGRO_EVENT_QUEUE*));
AL_FUNC(void, al_register_event_source, (ALLEGRO_EVENT_QUEUE*, ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, al_unregister_event_source, (ALLEGRO_EVENT_QUEUE*, ALLEGRO_EVENT_SOURCE*));
AL_FUNC(bool, al_event_queue_is_empty, (ALLEGRO_EVENT_QUEUE*));
AL_FUNC(bool, al_get_next_event, (ALLEGRO_EVENT_QUEUE*, ALLEGRO_EVENT *ret_event));
AL_FUNC(bool, al_peek_next_event, (ALLEGRO_EVENT_QUEUE*, ALLEGRO_EVENT *ret_event));
AL_FUNC(bool, al_drop_next_event, (ALLEGRO_EVENT_QUEUE*));
AL_FUNC(void, al_flush_event_queue, (ALLEGRO_EVENT_QUEUE*));
AL_FUNC(void, al_wait_for_event, (ALLEGRO_EVENT_QUEUE*,
                                  ALLEGRO_EVENT *ret_event));
AL_FUNC(bool, al_wait_for_event_timed, (ALLEGRO_EVENT_QUEUE*,
                                        ALLEGRO_EVENT *ret_event,
                                        float secs));
AL_FUNC(bool, al_wait_for_event_until, (ALLEGRO_EVENT_QUEUE *queue,
                                        ALLEGRO_EVENT *ret_event,
                                        ALLEGRO_TIMEOUT *timeout));


#ifdef __cplusplus
   }
#endif

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
