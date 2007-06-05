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

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/*
 * Event type tags
 */

typedef unsigned int AL_EVENT_TYPE;

/* Enum: AL_EVENT_TYPE
 *
 * Each event is of one of the following types
 *
 *  AL_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
 *    Fields are: joystick.stick, joystick.axis, joystick.pos (-1.0 to 1.0).
 *
 *  AL_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
 *    Fields are: joystick.button.
 *    
 *  AL_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
 *    Fields are: joystick.button.
 *
 *  AL_EVENT_KEY_DOWN - a keyboard key was pressed.
 *    Fields: keyboard.keycode, keyboard.unichar, keyboard.modifiers.
 *
 *  AL_EVENT_KEY_REPEAT - a typed character auto-repeated.
 *    Fields: keyboard.keycode (AL_KEY_*), keyboard.unichar (unicode
 *    character), keyboard.modifiers (AL_KEYMOD_*).
 *
 *  AL_EVENT_KEY_UP - a keyboard key was released.
 *    Fields: keyboard.keycode.
 *
 *  AL_EVENT_MOUSE_AXES - one or more mouse axis values changed.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.dx, mouse.dy, mouse.dz.
 *
 *  AL_EVENT_MOUSE_BUTTON_DOWN - a mouse button was pressed.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.button.
 *
 *  AL_EVENT_MOUSE_BUTTON_UP - a mouse button was released.
 *    Fields: mouse.x, mouse.y, mouse.z, mouse.button.
 *
 *  AL_EVENT_MOUSE_ENTER_DISPLAY - the mouse cursor entered a window opened
 *    by the program.  Fields: mouse.x, mouse.y, mouse.z.
 *
 *  AL_EVENT_MOUSE_LEAVE_DISPLAY - the mouse cursor leave the boundaries of a
 *    window opened by the program.
 *    Fields: mouse.x, mouse.y, mouse.z.
 *
 *  AL_EVENT_TIMER - a timer counter incremented.
 *    Fields: timer.count.
 */
enum
{
   AL_EVENT_JOYSTICK_AXIS               =  1,
   AL_EVENT_JOYSTICK_BUTTON_DOWN        =  2,
   AL_EVENT_JOYSTICK_BUTTON_UP          =  3,

   AL_EVENT_KEY_DOWN                    = 10,
   AL_EVENT_KEY_REPEAT                  = 11,
   AL_EVENT_KEY_UP                      = 12,

   AL_EVENT_MOUSE_AXES                  = 20,
   AL_EVENT_MOUSE_BUTTON_DOWN           = 21,
   AL_EVENT_MOUSE_BUTTON_UP             = 22,
   AL_EVENT_MOUSE_ENTER_DISPLAY         = 23,
   AL_EVENT_MOUSE_LEAVE_DISPLAY         = 24,

   AL_EVENT_TIMER                       = 30,

   AL_EVENT_DISPLAY_EXPOSE              = 40,
   AL_EVENT_DISPLAY_RESIZE              = 41,
   AL_EVENT_DISPLAY_CLOSE               = 42
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



/* Type: AL_EVENT
 *
 * An AL_EVENT is a union of all builtin event structures, i.e. it is an
 * object large enough to hold the data of any event type.  All events
 * have the following fields in common:
 *
 * >	AL_EVENT_TYPE	    type;
 * >	AL_EVENT_SOURCE *   any.source;
 * >	unsigned long	    any.timestamp;
 *
 * By examining the type field you can then access type-specific fields.  The
 * any.source field tells you which event source generated that particular
 * event.  The any.timestamp field tells you when the event was generated.  The
 * time is referenced to the same starting point as al_current_time().
 */
/* Although
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



/* Event sources */

/* Type: AL_EVENT_SOURCE
 *
 * An event source is any object which can generate events.  Event sources are
 * usually referred to by distinct types, e.g. AL_KEYBOARD*, but can be
 * casted to AL_EVENT_SOURCE* when used in contexts that accept generic event
 * sources.
 */
typedef struct AL_EVENT_SOURCE AL_EVENT_SOURCE;



/* Event queues */

typedef struct AL_EVENT_QUEUE AL_EVENT_QUEUE;

#define AL_WAIT_FOREVER (-1)

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



AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
