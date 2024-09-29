#ifndef A5O_IPHONE
   #error bad include
#endif

#ifndef A5O_LIB_BUILD
#define A5O_MAGIC_MAIN
#define main _al_mangled_main
#ifdef __cplusplus
   extern "C" int _al_mangled_main(int, char **);
#endif
#endif
