#ifndef __al_included_aintern_atomic_ops_h
#define __al_included_aintern_atomic_ops_h

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)

   /* gcc 4.1 and above have builtin atomic operations. */

   typedef int _AL_ATOMIC;

   AL_INLINE(_AL_ATOMIC,
      _al_fetch_and_add1, (volatile _AL_ATOMIC *ptr),
   {
      return __sync_fetch_and_add(ptr, 1);
   })

   AL_INLINE(_AL_ATOMIC,
      _al_sub1_and_fetch, (volatile _AL_ATOMIC *ptr),
   {
      return __sync_sub_and_fetch(ptr, 1);
   })

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

   /* gcc, x86 or x86-64 */

   typedef int _AL_ATOMIC;

   AL_INLINE(_AL_ATOMIC,
      __al_fetch_and_add, (volatile _AL_ATOMIC *ptr, _AL_ATOMIC value),
   {
      _AL_ATOMIC result;

      __asm__ __volatile__ (
         "lock; xaddl %0, %1"
         : "=r" (result), "=m" (*ptr)
         : "0" (value), "m" (*ptr)
         : "memory"
      );
      return result;
   })

   AL_INLINE(_AL_ATOMIC,
      _al_fetch_and_add1, (volatile _AL_ATOMIC *ptr),
   {
      return __al_fetch_and_add(ptr, 1);
   })

   AL_INLINE(_AL_ATOMIC,
      _al_sub1_and_fetch, (volatile _AL_ATOMIC *ptr),
   {
      _AL_ATOMIC old = __al_fetch_and_add(ptr, -1);
      return old - 1;
   })

#elif defined(_MSC_VER) && _M_IX86 >= 400

   /* MSVC, x86 */
   /* MinGW supports these too, but we already have asm code above. */

   typedef LONG _AL_ATOMIC;

   AL_INLINE(_AL_ATOMIC,
      _al_fetch_and_add1, (volatile _AL_ATOMIC *ptr),
   {
      return InterlockedIncrement(ptr) - 1;
   })

   AL_INLINE(_AL_ATOMIC,
      _al_sub1_and_fetch, (volatile _AL_ATOMIC *ptr),
   {
      return InterlockedDecrement(ptr);
   })

#else

   /* Hope for the best? */
   #warning Atomic operations undefined for your compiler/architecture.

   typedef int _AL_ATOMIC;

   AL_INLINE(_AL_ATOMIC,
      _al_fetch_and_add1, (volatile _AL_ATOMIC *ptr),
   {
      return (*ptr)++;
   })

   AL_INLINE(_AL_ATOMIC,
      _al_sub1_and_fetch, (volatile _AL_ATOMIC *ptr),
   {
      return --(*ptr);
   })

#endif

#endif   /* __al_included_aintern_atomicops_h */

/* vim: set sts=3 sw=3 et: */
