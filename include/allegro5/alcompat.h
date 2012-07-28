#ifndef __al_included_allegro5_alcompat_h
#define __al_included_allegro5_alcompat_h

#ifdef __cplusplus
   extern "C" {
#endif


#define al_current_time()                 (al_get_time())
#define al_event_queue_is_empty(q)        (al_is_event_queue_empty(q))
#define al_toggle_display_flag(d, f, o)   (al_set_display_flag((d), (f), (o)))
#define al_ortho_transform(tr, l, r, b, t, n, f) (al_orthographic_transform((tr), (l), (t), (n), (r), (b), (f)))


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
