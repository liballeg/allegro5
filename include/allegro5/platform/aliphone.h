#ifndef ALLEGRO_IPHONE
   #error bad include
#endif

#ifndef ALLEGRO_LIB_BUILD
#define main _mangled_main
#undef END_OF_MAIN
#define END_OF_MAIN() int (*_mangled_main_address)(int argc, char **argv) = (void *)_mangled_main;
#endif