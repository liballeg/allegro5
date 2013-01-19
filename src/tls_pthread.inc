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


static pthread_key_t tls_key = 0;
static thread_local_state _tls;


static void tls_dtor(void *ptr)
{
   al_free(ptr);
}


void _al_tls_init_once(void)
{
   pthread_key_create(&tls_key, tls_dtor);
}


static thread_local_state *pthreads_thread_init(void)
{
   /* Allocate and copy the 'template' object */
   thread_local_state *ptr = (thread_local_state *)al_malloc(sizeof(thread_local_state));
   memcpy(ptr, &_tls, sizeof(thread_local_state));
   pthread_setspecific(tls_key, ptr);
   return ptr;
}


/* This function is short so it can hopefully be inlined. */
static thread_local_state *tls_get(void)
{
   thread_local_state *ptr = (thread_local_state*)pthread_getspecific(tls_key);
   if (ptr == NULL) {
      /* Must create object. */
      ptr = pthreads_thread_init();
      initialize_tls_values(ptr);
   }
   return ptr;
}


/* vim: set ft=c sts=3 sw=3: */
