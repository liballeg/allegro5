if(MACOSX)
    set(SOURCES osx_main.m)
    set(SUPPORT_ALLEGRO_MAIN 1)
endif(MACOSX)

if(NOT SUPPORT_ALLEGRO_MAIN)
    set(SOURCES generic_main.c)
    set(SUPPORT_ALLEGRO_MAIN 1)
endif(NOT SUPPORT_ALLEGRO_MAIN)

add_our_library(allegro_main
   "${SOURCES}"
   "-DALLEGRO_SRC"
   "${ALLEGRO_LINK_WITH}"
   )

set_our_framework_properties(allegro_main AllegroMain-${ALLEGRO_SOVERSION})

install_our_library(allegro_main)

set(SUPPORT_ALLEGRO_MAIN 1 PARENT_SCOPE)
set(ALLEGRO_MAIN_LINK_WITH allegro_main PARENT_SCOPE)

# vim: set sts=4 sw=4 et:
