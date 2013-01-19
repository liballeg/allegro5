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
 *      Thread local storage.
 *
 *      See LICENSE.txt for copyright information.
 */

#if defined(ALLEGRO_MSVC) || defined(ALLEGRO_BCC32)
   #define THREAD_LOCAL_QUALIFIER __declspec(thread)
#else
   #define THREAD_LOCAL_QUALIFIER __thread
#endif

static THREAD_LOCAL_QUALIFIER thread_local_state _tls;


void _al_tls_init_once(void)
{
   /* nothing */
}


static thread_local_state *tls_get(void)
{
   static THREAD_LOCAL_QUALIFIER thread_local_state *ptr = NULL;
   if (!ptr) {
      ptr = &_tls;
      initialize_tls_values(ptr);
   }
   return ptr;
}


/* vim: set ft=c sts=3 sw=3 et: */
