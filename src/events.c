/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *	Event queues.
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"



/*
 * Note: most event source and event queue functions are actually
 * implemented in platform-specific modules.
 */



/* al_wait_for_specific_event: [primary thread]
 */
bool al_wait_for_specific_event(AL_EVENT_QUEUE *queue,
                                AL_EVENT *ret_event,
                                long msecs,
                                AL_EVENT_SOURCE *source,
                                unsigned long event_mask)
{
   ASSERT(msecs >= 0);

   if (msecs == 0) {
      while (true) {
         al_wait_for_event(queue, ret_event, 0);

         if (((!source) || (ret_event->any.source == source)) &&
             (ret_event->type & event_mask))
            return true;
      }
   }
   else {
      /* XXX: Should get rid of long longs at some stage. */
      /* XXX: Watch out for al_current_time(), which could wrap. */
      long long end, remaining;

      end = al_current_time() + msecs;

      while (true) {
         remaining = end - al_current_time();
         if (remaining <= 0LL)
            return false;

         /* Don't bother trying to wait a second time unless we missed
          * the mark by a large enough amount.  The 1ms below is
          * pretty arbitrary.
          */
         if (remaining < 1)
            return false;

         if (!al_wait_for_event(queue, ret_event, remaining))
            return false;

         if (((!source) || (ret_event->any.source != source)) &&
             (ret_event->type & event_mask))
            return true;
      }
   }
}



/* _al_copy_event:
 *  Copies the contents of the event SRC to DEST.  How many bytes are
 *  actually copied depends on what type of event SRC is.
 */
void _al_copy_event(AL_EVENT *dest, AL_EVENT *src)
{
   ASSERT(dest);
   ASSERT(src->any._size > 0);

   memcpy(dest, src, src->any._size);

   dest->any._refcount = 0;
   dest->any._next = NULL;
   dest->any._next_free = NULL;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
