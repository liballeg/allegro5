#ifndef __al_included_allegro5_alcompat_h
#define __al_included_allegro5_alcompat_h

#ifdef __cplusplus
   extern "C" {
#endif


#define A5O_DST_COLOR                 (A5O_DEST_COLOR)
#define A5O_INVERSE_DST_COLOR         (A5O_INVERSE_DEST_COLOR)

#define al_convert_bitmaps()              (al_convert_memory_bitmaps())
#define al_current_time()                 (al_get_time())
#define al_event_queue_is_empty(q)        (al_is_event_queue_empty(q))
#define al_toggle_display_flag(d, f, o)   (al_set_display_flag((d), (f), (o)))


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
