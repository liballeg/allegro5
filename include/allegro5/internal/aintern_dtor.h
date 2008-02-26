#ifndef _al_included_aintern_dtor_h
#define _al_included_aintern_dtor_h

AL_BEGIN_EXTERN_C


AL_FUNC(void, _al_init_destructors, (void));
AL_FUNC(void, _al_register_destructor, (void *object, void (*func)(void*)));
AL_FUNC(void, _al_unregister_destructor, (void *object));


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
