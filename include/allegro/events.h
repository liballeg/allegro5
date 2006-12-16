#ifndef _al_included_events_h
#define _al_included_events_h

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/*
 * Event type tags
 *
 * Note: these have to be usable in masks.
 */

typedef unsigned int AL_EVENT_TYPE;

enum
{
   AL_EVENT_JOYSTICK_AXIS               = 0x0001,
   AL_EVENT_JOYSTICK_BUTTON_DOWN        = 0x0002,
   AL_EVENT_JOYSTICK_BUTTON_UP          = 0x0004,

   AL_EVENT_KEY_DOWN                    = 0x0010,
   AL_EVENT_KEY_REPEAT                  = 0x0020,
   AL_EVENT_KEY_UP                      = 0x0040,

   AL_EVENT_MOUSE_AXES                  = 0x0100,
   AL_EVENT_MOUSE_BUTTON_DOWN           = 0x0200,
   AL_EVENT_MOUSE_BUTTON_UP             = 0x0400,
   AL_EVENT_MOUSE_ENTER_DISPLAY         = 0x0800,
   AL_EVENT_MOUSE_LEAVE_DISPLAY         = 0x1000,

   AL_EVENT_TIMER                       = 0x2000,

   AL_EVENT_DISPLAY_EXPOSE              = 0x4000
};



/*
 * Event structures
 *
 * All event types have the following fields in common, which are
 * semantically read-only.
 *
 *  type      -- the type of event this is
 *  timestamp -- when this event was generated
 *  source    -- which event source generated this event
 *
 * There are a couple of other fields, which are internal to the
 * implementation (don't touch).
 *
 *
 * For people writing event sources: The common fields must be at the
 * very start of each event structure, i.e. put _AL_EVENT_HEADER at the
 * front.  If you are managing your own event lists, be careful not to
 * mix up _next and _next_free -- the resultant bugs can be elusive.
 */

#define _AL_EVENT_HEADER(srctype)               \
   AL_EVENT_TYPE type;                          \
   srctype *source;                             \
   unsigned long timestamp;                     \
   signed int _refcount;       /* internal */   \
   union AL_EVENT *_next;      /* internal */   \
   union AL_EVENT *_next_free  /* internal */



typedef struct AL_DISPLAY_EVENT
{
   _AL_EVENT_HEADER(struct AL_DISPLAY);
   int x, y;
   int width, height;
} AL_DISPLAY_EVENT;



typedef struct AL_JOYSTICK_EVENT
{
   _AL_EVENT_HEADER(struct AL_JOYSTICK);
   int stick;
   int axis;
   float pos;
   int button;
} AL_JOYSTICK_EVENT;



typedef struct AL_KEYBOARD_EVENT
{
   _AL_EVENT_HEADER(struct AL_KEYBOARD);
   struct AL_DISPLAY *__display__dont_use_yet__;
   int keycode;                 /* the physical key pressed */
   unsigned int unichar;        /* unicode character */
   unsigned int modifiers;      /* bitfield */
} AL_KEYBOARD_EVENT;



typedef struct AL_MOUSE_EVENT
{
   _AL_EVENT_HEADER(struct AL_MOUSE);
   struct AL_DISPLAY *__display__dont_use_yet__;
   /* (x, y) Primary mouse position */
   /* (z) Mouse wheel position (1D 'wheel'), or,  */
   /* (w, z) Mouse wheel position (2D 'ball') */
   int x,  y,  z, w;
   int dx, dy, dz, dw;
   unsigned int button;
} AL_MOUSE_EVENT;



typedef struct AL_TIMER_EVENT
{
   _AL_EVENT_HEADER(struct AL_TIMER);
   long count;
} AL_TIMER_EVENT;



/* An AL_EVENT is a union of all builtin event structures.  Although
 * we cannot extend this union later with user event structures, the
 * API is designed so that pointers to any user event structures can
 * be reliably cast to and from pointers to AL_EVENTs.  In that way,
 * user events are _almost_ as legitimate as builtin events.
 */
typedef union AL_EVENT AL_EVENT;

union AL_EVENT
{
   /* This must be the same as the first field of _AL_EVENT_HEADER.  */
   AL_EVENT_TYPE type;
   /* This is to allow the user to access the other fields which are
    * common to all event types, without using some specific type
    * structure.  A C hack.
    */
   struct {
      _AL_EVENT_HEADER(struct AL_EVENT_SOURCE);
   } any;
   AL_DISPLAY_EVENT  display;
   AL_JOYSTICK_EVENT joystick;
   AL_KEYBOARD_EVENT keyboard;
   AL_MOUSE_EVENT    mouse;
   AL_TIMER_EVENT    timer;
};



/* internals */

enum
{
   _AL_ALL_DISPLAY_EVENTS = (AL_EVENT_DISPLAY_EXPOSE),

   _AL_ALL_JOYSTICK_EVENTS = (AL_EVENT_JOYSTICK_AXIS |
                              AL_EVENT_JOYSTICK_BUTTON_DOWN |
                              AL_EVENT_JOYSTICK_BUTTON_UP),

   _AL_ALL_KEYBOARD_EVENTS = (AL_EVENT_KEY_DOWN |
                              AL_EVENT_KEY_REPEAT |
                              AL_EVENT_KEY_UP),

   _AL_ALL_MOUSE_EVENTS = (AL_EVENT_MOUSE_AXES |
                           AL_EVENT_MOUSE_BUTTON_DOWN |
                           AL_EVENT_MOUSE_BUTTON_UP |
                           AL_EVENT_MOUSE_ENTER_DISPLAY |
                           AL_EVENT_MOUSE_LEAVE_DISPLAY)
};



/* Event sources */

typedef struct AL_EVENT_SOURCE AL_EVENT_SOURCE;

AL_FUNC(void, al_event_source_set_mask, (AL_EVENT_SOURCE*, AL_EVENT_TYPE mask));
AL_FUNC(AL_EVENT_TYPE, al_event_source_mask, (AL_EVENT_SOURCE*));



/* Event queues */

typedef struct AL_EVENT_QUEUE AL_EVENT_QUEUE;

AL_FUNC(AL_EVENT_QUEUE*, al_create_event_queue, (void));
AL_FUNC(void, al_destroy_event_queue, (AL_EVENT_QUEUE*));
AL_FUNC(void, al_register_event_source, (AL_EVENT_QUEUE*, AL_EVENT_SOURCE*));
AL_FUNC(void, al_unregister_event_source, (AL_EVENT_QUEUE*, AL_EVENT_SOURCE*));
AL_FUNC(bool, al_event_queue_is_empty, (AL_EVENT_QUEUE*));
AL_FUNC(bool, al_get_next_event, (AL_EVENT_QUEUE*, AL_EVENT *ret_event));
AL_FUNC(bool, al_peek_next_event, (AL_EVENT_QUEUE*, AL_EVENT *ret_event));
AL_FUNC(void, al_drop_next_event, (AL_EVENT_QUEUE*));
AL_FUNC(void, al_flush_event_queue, (AL_EVENT_QUEUE*));
AL_FUNC(bool, al_wait_for_event, (AL_EVENT_QUEUE*,
                                  AL_EVENT *ret_event,
                                  long msecs));
AL_FUNC(bool, al_wait_for_specific_event, (AL_EVENT_QUEUE*,
                                           AL_EVENT *ret_event,
                                           long msecs,
                                           AL_EVENT_SOURCE *source_or_null,
                                           unsigned long event_mask));



AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
