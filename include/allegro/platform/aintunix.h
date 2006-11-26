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
 *      Some definitions for internal use by the Unix library code.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#ifndef AINTUNIX_H
#define AINTUNIX_H

/* Need right now for XKeyEvent --pw */
#ifdef ALLEGRO_WITH_XWINDOWS
#include <X11/Xlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

   /* Macros to enable and disable interrupts */
   #define DISABLE() _unix_bg_man->disable_interrupts()
   #define ENABLE()  _unix_bg_man->enable_interrupts()


   /* Helper for locating config files */
   AL_FUNC(int, _unix_find_resource, (char *dest, AL_CONST char *resource, int size));


   /* Generic system driver entry for finding the executable */
   AL_FUNC(void, _unix_get_executable_name, (char *output, int size));

   /* Generic system driver entry for retrievng system paths */
   AL_FUNC(int32_t _unix_get_path, (uint32_t id, char *dir, size_t size));

   /* Helper for setting os_type */
   AL_FUNC(void, _unix_read_os_type, (void));


   /* Helper for yield CPU */
   AL_FUNC(void, _unix_yield_timeslice, (void));


   /* Unix rest function */
   AL_FUNC(void, _unix_rest, (unsigned int, AL_METHOD(void, callback, (void))));


   /* Module support */
   AL_FUNC(void, _unix_load_modules, (int system_driver_id));
   AL_FUNC(void, _unix_unload_modules, (void));


   /* Dynamic driver lists, for modules */
   AL_VAR(_DRIVER_INFO *, _unix_gfx_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_digi_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_midi_driver_list);
   AL_FUNC(void, _unix_driver_lists_init, (void));
   AL_FUNC(void, _unix_driver_lists_shutdown, (void));
   AL_FUNC(void, _unix_register_gfx_driver, (int id, GFX_DRIVER *driver, int autodetect, int priority));
   AL_FUNC(void, _unix_register_digi_driver, (int id, DIGI_DRIVER *driver, int autodetect, int priority));
   AL_FUNC(void, _unix_register_midi_driver, (int id, MIDI_DRIVER *driver, int autodetect, int priority));


   /* File system helpers */
   AL_FUNC(void, _unix_guess_file_encoding, (void));

#ifdef ALLEGRO_WITH_XWINDOWS
   AL_FUNCPTR(void, _xwin_mouse_interrupt, (int x, int y, int z, int buttons));

   AL_ARRAY(_DRIVER_INFO, _xwin_gfx_driver_list);
   AL_ARRAY(_DRIVER_INFO, _al_xwin_keyboard_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_mouse_driver_list);

   AL_FUNC(void, _xwin_handle_input, (void));
   AL_FUNC(void, _xwin_private_handle_input, (void));

   #define XLOCK()                              \
      do {                                      \
         _al_mutex_lock(&_xwin.mutex);		\
         _xwin.lock_count++;                    \
      } while (0)

   #define XUNLOCK()                            \
      do {                                      \
         _al_mutex_unlock(&_xwin.mutex);	\
         _xwin.lock_count--;                    \
      } while (0)

#endif


#ifdef ALLEGRO_WITH_OSSDIGI
   /* So the setup program can read what we detected */
   AL_VAR(int, _oss_fragsize);
   AL_VAR(int, _oss_numfrags);
#endif


#ifdef __cplusplus
}
#endif


#ifdef ALLEGRO_LINUX
   #include "aintlnx.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Typedef for background functions, called frequently in the background.
 * `threaded' is nonzero if the function is being called from a thread.
 */
typedef void (*bg_func) (int threaded);

/* Background function manager -- responsible for calling background
 * functions.  `int' methods return -1 on failure, 0 on success. */
struct bg_manager
{
   int multi_threaded;
   int (*init) (void);
   void (*exit) (void);
   int (*register_func) (bg_func f);
   int (*unregister_func) (bg_func f);
   void (*enable_interrupts) (void);
   void (*disable_interrupts) (void);
   int (*interrupts_disabled) (void);
};

extern struct bg_manager _bg_man_pthreads;

extern struct bg_manager *_unix_bg_man;


#ifdef __cplusplus
}
#endif



/*----------------------------------------------------------------------*
 *									*
 *	New stuff							*
 *									*
 *----------------------------------------------------------------------*/

/* TODO: integrate this above */
/* TODO: replace bg_man */

#include <pthread.h>
#include "allegro/internal/aintern.h"

AL_BEGIN_EXTERN_C

/* threads */
struct _AL_THREAD
{
    /* private: */
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
    void (*proc)(_AL_THREAD *self, void *arg);
    void *arg;
};

AL_INLINE(bool, _al_thread_should_stop, (_AL_THREAD *t),
{
    bool ret;
    pthread_mutex_lock(&t->mutex);
    ret = t->should_stop;
    pthread_mutex_unlock(&t->mutex);
    return ret;
})

struct _AL_MUTEX
{
    bool inited;
    pthread_mutex_t mutex;
};

#define _AL_MUTEX_UNINITED	       { false, PTHREAD_MUTEX_INITIALIZER }
				       /* makes no sense, but shuts gcc up */
#define _AL_MARK_MUTEX_UNINITED(M)     do { M.inited = false; } while (0)

AL_FUNC(void, _al_mutex_init, (_AL_MUTEX*));
AL_FUNC(void, _al_mutex_destroy, (_AL_MUTEX*));
AL_INLINE(void, _al_mutex_lock, (_AL_MUTEX *m),
{
   if (m->inited)
      pthread_mutex_lock(&m->mutex);
})
AL_INLINE(void, _al_mutex_unlock, (_AL_MUTEX *m),
{
   if (m->inited)
      pthread_mutex_unlock(&m->mutex);
})

struct _AL_COND
{
   pthread_cond_t cond;
};

AL_INLINE(void, _al_cond_init, (_AL_COND *cond),
{
   pthread_cond_init(&cond->cond, NULL);
})

AL_INLINE(void, _al_cond_destroy, (_AL_COND *cond),
{
   pthread_cond_destroy(&cond->cond);
})

AL_INLINE(void, _al_cond_wait, (_AL_COND *cond, _AL_MUTEX *mutex),
{
   pthread_cond_wait(&cond->cond, &mutex->mutex);
})

AL_INLINE(void, _al_cond_broadcast, (_AL_COND *cond),
{
   pthread_cond_broadcast(&cond->cond);
})

AL_INLINE(void, _al_cond_signal, (_AL_COND *cond),
{
   pthread_cond_signal(&cond->cond);
})


/* time */
AL_FUNC(void, _al_unix_init_time, (void));

/* fdwatch */
void _al_unix_start_watching_fd(int fd, void (*callback)(void *), void *cb_data);
void _al_unix_stop_watching_fd(int fd);

AL_END_EXTERN_C


#endif /* ifndef AINTUNIX_H */
