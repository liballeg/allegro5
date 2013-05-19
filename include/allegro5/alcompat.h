#ifndef __al_included_allegro5_alcompat_h
#define __al_included_allegro5_alcompat_h

#ifdef __cplusplus
   extern "C" {
#endif


#define ALLEGRO_DST_COLOR                 (ALLEGRO_DEST_COLOR)
#define ALLEGRO_INVERSE_DST_COLOR         (ALLEGRO_INVERSE_DEST_COLOR)

#define al_current_time()                 (al_get_time())
#define al_event_queue_is_empty(q)        (al_is_event_queue_empty(q))


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
