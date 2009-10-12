set(INCLUDE_FILES allegro5/allegro_native_dialog.h)

if(APPLE)
   set(SOURCES dialog.c osx_dialog.m)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(APPLE)

if(WIN32)
   set(SOURCES dialog.c win_dialog.c)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(WIN32)

if(NOT SUPPORT_NATIVE_DIALOG AND NOT GP2XWIZ)
    pkg_check_modules(GTK gtk+-2.0)
    pkg_check_modules(GT gthread-2.0)
    if(GTK_FOUND AND GT_FOUND)
        set(SOURCES dialog.c gtk_dialog.c)
        include_directories(SYSTEM ${GTK_INCLUDE_DIRS})

        # For example, on Windows, GTK+ requires gcc -mms-bitfields (MinGW).
        foreach(arg ${GTK_CFLAGS})
            set(GTK_CFLAGS_STRING "${GTK_CFLAGS_STRING} ${arg}")
        endforeach(arg)

        # STRICT_WARN will cause gcc to warn about a function prototype in
        # gtkitemfactory.h so disable that warning explicitly.
        if(COMPILER_GCC)
            set(GTK_CFLAGS_STRING "${GTK_CFLAGS_STRING} -Wno-strict-prototypes")
        endif(COMPILER_GCC)

        set_source_files_properties(
            gtk_dialog.c
            PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS_STRING}"
            )

        set(NATIVE_DIALOG_LIBRARIES ${GTK_LIBRARIES} ${GT_LIBRARIES})

        set(SUPPORT_NATIVE_DIALOG 1)
    endif(GTK_FOUND AND GT_FOUND)
endif(NOT SUPPORT_NATIVE_DIALOG AND NOT GP2XWIZ)

if(SUPPORT_NATIVE_DIALOG)
   configure_file(
       allegro5/internal/aintern_native_dialog_cfg.h.cmake
       ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_native_dialog_cfg.h
       )

   add_our_library(allegro_dialog -${ALLEGRO_VERSION}
       "${SOURCES}"
       "-DALLEGRO_NATIVE_DIALOG_SRC"
       "${NATIVE_DIALOG_LIBRARIES};${ALLEGRO_LINK_WITH}"
       )

   set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
   set(NATIVE_DIALOG_LINK_WITH allegro_dialog PARENT_SCOPE)

   install(FILES ${INCLUDE_FILES}
           DESTINATION include/allegro5
           )
endif(SUPPORT_NATIVE_DIALOG)

# vim: set sts=4 sw=4 et:
