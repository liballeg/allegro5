set(INCLUDE_FILES allegro5/a5_native_dialog.h)

if(APPLE)
   set(SOURCES dialog.c osx_dialog.m)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(APPLE)

if(NOT SUPPORT_NATIVE_DIALOG)
    pkg_check_modules(GTK gtk+-2.0)
    if(GTK_FOUND)
        set(SOURCES dialog.c gtk_dialog.c)
        include_directories(${GTK_INCLUDE_DIRS})

        # For example, on Windows, GTK+ requires gcc -mms-bitfields (MinGW).
        foreach(arg ${GTK_CFLAGS})
            set(GTK_CFLAGS_STRING "${GTK_CFLAGS_STRING} ${arg}")
        endforeach(arg)
        set_source_files_properties(
            gtk_dialog.c
            PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS_STRING}"
            )

        set(NATIVE_DIALOG_LIBRARIES ${GTK_LIBRARIES})
        set(SUPPORT_NATIVE_DIALOG 1)
    endif(GTK_FOUND)
endif(NOT SUPPORT_NATIVE_DIALOG)

if(SUPPORT_NATIVE_DIALOG)
   configure_file(
       allegro5/internal/aintern_native_dialog_cfg.h.cmake
       ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_native_dialog_cfg.h
       )

   add_our_library(a5_dialog -${ALLEGRO_VERSION}
       "${SOURCES}"
       "-DA5_NATIVE_DIALOG_SRC"
       "${NATIVE_DIALOG_LIBRARIES};${ALLEGRO_LINK_WITH}"
       )

   set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
   set(NATIVE_DIALOG_LINK_WITH a5_dialog PARENT_SCOPE)

   install(FILES ${INCLUDE_FILES}
           DESTINATION include/allegro5
           )
endif(SUPPORT_NATIVE_DIALOG)
