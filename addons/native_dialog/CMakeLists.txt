# Use GTK dialogs by default (basically, if the current platform doesn't
# provide anything else.
set(WANT_GTK_DIALOG 1)

if(APPLE)
   set(WANT_GTK_DIALOG 0)
   set(SOURCES osx_dialog.m)
   set(INCLUDE_FILES allegro5/a5_native_dialog.h)

   add_our_library(a5_dialog -${ALLEGRO_VERSION}
       "${SOURCES}"
       "-DA5_NATIVE_DIALOG_SRC"
       ""
       )

   set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
   set(NATIVE_DIALOG_LINK_WITH a5_dialog PARENT_SCOPE)

   install(FILES ${INCLUDE_FILES}
           DESTINATION include/allegro5
           )
endif(APPLE)

if(WANT_GTK_DIALOG)
   pkg_check_modules(GTK gtk+-2.0)

   if(GTK_FOUND)

   set(SOURCES_GTK gtk_dialog.c)
   set(SOURCES ${SOURCES_GTK})
   set(INCLUDE_FILES allegro5/a5_native_dialog.h)

   configure_file(
       allegro5/internal/aintern_native_dialog_cfg.h.cmake
       ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_native_dialog_cfg.h
       )

   include_directories(${GTK_INCLUDE_DIRS})

   add_our_library(a5_dialog -${ALLEGRO_VERSION}
       "${SOURCES}"
       "-DA5_NATIVE_DIALOG_SRC"
       "${GTK_LIBRARIES};${ALLEGRO_LINK_WITH}"
       )

# For example, on Windows, GTK+ requires gcc -mms-bitfields (MinGW).
   foreach(arg ${GTK_CFLAGS})
       set(GTK_CFLAGS_STRING "${GTK_CFLAGS_STRING} ${arg}")
   endforeach(arg)
   set_source_files_properties(
       gtk_dialog.c
       PROPERTIES
       COMPILE_FLAGS "${GTK_CFLAGS_STRING}"
       )

   set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
   set(NATIVE_DIALOG_LINK_WITH a5_dialog PARENT_SCOPE)

   install(FILES ${INCLUDE_FILES}
           DESTINATION include/allegro5
           )

   endif(GTK_FOUND)
endif(WANT_GTK_DIALOG)
