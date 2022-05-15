if(ALLEGRO_MACOSX)
    set(MAIN_SOURCES osx_main.m)
    set(SUPPORT_ALLEGRO_MAIN 1)
endif(ALLEGRO_MACOSX)

if(NOT SUPPORT_ALLEGRO_MAIN)
    set(MAIN_SOURCES generic_main.c)
    set(SUPPORT_ALLEGRO_MAIN 1)
endif(NOT SUPPORT_ALLEGRO_MAIN)

add_our_addon_library(allegro_main
   AllegroMain-${ALLEGRO_SOVERSION}
   "${MAIN_SOURCES}"
   "-DALLEGRO_SRC"
   "${ALLEGRO_LINK_WITH}"
   )

add_addon(main)

# vim: set sts=4 sw=4 et:
