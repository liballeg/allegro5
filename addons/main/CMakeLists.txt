if(A5O_MACOSX)
    set(MAIN_SOURCES osx_main.m)
    set(SUPPORT_A5O_MAIN 1)
endif(A5O_MACOSX)

if(NOT SUPPORT_A5O_MAIN)
    set(MAIN_SOURCES generic_main.c)
    set(SUPPORT_A5O_MAIN 1)
endif(NOT SUPPORT_A5O_MAIN)

add_our_addon_library(allegro_main
   AllegroMain-${A5O_SOVERSION}
   "${MAIN_SOURCES}"
   "-DA5O_SRC"
   "${A5O_LINK_WITH}"
   )

add_addon(main)

# vim: set sts=4 sw=4 et:
