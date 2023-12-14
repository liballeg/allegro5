set(NATIVE_DIALOG_INCLUDE_FILES allegro5/allegro_native_dialog.h)

set_our_header_properties(${NATIVE_DIALOG_INCLUDE_FILES})

set(NATIVE_DIALOG_SOURCES
    dialog.c
    textlog.c
    menu.c
    )

set(GTK_NATIVE_DIALOG_SOURCES
    gtk_dialog.c
    gtk_filesel.c
    gtk_menu.c
    gtk_msgbox.c
    gtk_textlog.c
    gtk_thread.c
    gtk_xgtk.c
    )

if(APPLE AND NOT IPHONE)
   list(APPEND NATIVE_DIALOG_SOURCES osx_dialog.m)
   set(ALLEGRO_CFG_NATIVE_DIALOG_OSX 1)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(APPLE AND NOT IPHONE)

if(APPLE AND IPHONE)
   list(APPEND NATIVE_DIALOG_SOURCES iphone_dialog.m)
   set(ALLEGRO_CFG_NATIVE_DIALOG_IPHONE 1)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(APPLE AND IPHONE)

if(ANDROID)
    list(APPEND NATIVE_DIALOG_SOURCES android_dialog.c)
    set(ALLEGRO_CFG_NATIVE_DIALOG_ANDROID 1)
    set(SUPPORT_NATIVE_DIALOG 1)
endif(ANDROID)

if(WIN32)
   list(APPEND NATIVE_DIALOG_SOURCES win_dialog.c)
   set(ALLEGRO_CFG_NATIVE_DIALOG_WINDOWS 1)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(WIN32)

if(NOT SUPPORT_NATIVE_DIALOG AND SUPPORT_X11 AND NOT ALLEGRO_RASPBERRYPI)
    pkg_check_modules(GTK gtk+-3.0)
    pkg_check_modules(GT gthread-2.0)
    if(GTK_FOUND AND GT_FOUND)
        list(APPEND NATIVE_DIALOG_SOURCES ${GTK_NATIVE_DIALOG_SOURCES})
        list(APPEND NATIVE_DIALOG_INCLUDE_DIRECTORIES
            ${GTK_INCLUDE_DIRS} ${GT_INCLUDE_DIRS})
        list(APPEND NATIVE_DIALOG_LIBRARIES ${GTK_LIBRARIES} ${GT_LIBRARIES})
        set_source_files_properties(
            ${GTK_NATIVE_DIALOG_SOURCES}
            PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS_STRING}"
            )
        set(ALLEGRO_CFG_NATIVE_DIALOG_GTK 1)
        set(SUPPORT_NATIVE_DIALOG 1)
    endif()
endif()

if(SUPPORT_NATIVE_DIALOG)
   configure_file(
       allegro5/internal/aintern_native_dialog_cfg.h.cmake
       ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_native_dialog_cfg.h
       )
   
   include_directories(SYSTEM ${NATIVE_DIALOG_INCLUDE_DIRECTORIES})

   # Note: allegro_dialog NOT allegro_native_dialog.
   add_our_addon_library(allegro_dialog
       AllegroDialog-${ALLEGRO_SOVERSION}
       "${NATIVE_DIALOG_SOURCES};${NATIVE_DIALOG_INCLUDE_FILES}"
       "-DALLEGRO_NATIVE_DIALOG_SRC"
       "${NATIVE_DIALOG_LIBRARIES};${ALLEGRO_LINK_WITH}"
       )

   install_our_headers(${NATIVE_DIALOG_INCLUDE_FILES})

   # Note: allegro_dialog NOT allegro_native_dialog.
   add_addon2(native_dialog allegro_dialog)
endif(SUPPORT_NATIVE_DIALOG)

# vim: set sts=4 sw=4 et:
