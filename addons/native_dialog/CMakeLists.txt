set(NATIVE_DIALOG_INCLUDE_FILES allegro5/allegro_native_dialog.h)

set_our_header_properties(${NATIVE_DIALOG_INCLUDE_FILES})

set(NATIVE_DIALOG_SOURCES
    dialog.c
    textlog.c
    )

set(GTK_NATIVE_DIALOG_SOURCES
    gtk_dialog.c
    gtk_filesel.c
    gtk_msgbox.c
    gtk_textlog.c
    gtk_thread.c
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

if(WIN32)
   list(APPEND NATIVE_DIALOG_SOURCES win_dialog.c)
   set(ALLEGRO_CFG_NATIVE_DIALOG_WINDOWS 1)
   set(SUPPORT_NATIVE_DIALOG 1)
endif(WIN32)

if(NOT SUPPORT_NATIVE_DIALOG AND NOT GP2XWIZ AND NOT IPHONE)
    pkg_check_modules(GTK gtk+-2.0)
    pkg_check_modules(GT gthread-2.0)
    if(GTK_FOUND AND GT_FOUND)
        list(APPEND NATIVE_DIALOG_SOURCES ${GTK_NATIVE_DIALOG_SOURCES})
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
            ${GTK_NATIVE_DIALOG_SOURCES}
            PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS_STRING}"
            )

        set(NATIVE_DIALOG_LIBRARIES ${GTK_LIBRARIES} ${GT_LIBRARIES})

        set(ALLEGRO_CFG_NATIVE_DIALOG_GTK 1)
        set(SUPPORT_NATIVE_DIALOG 1)
    endif(GTK_FOUND AND GT_FOUND)
endif()

if(SUPPORT_NATIVE_DIALOG)
   configure_file(
       allegro5/internal/aintern_native_dialog_cfg.h.cmake
       ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_native_dialog_cfg.h
       )

   # Note: allegro_dialog NOT allegro_native_dialog.
   add_our_library(allegro_dialog
       "${NATIVE_DIALOG_SOURCES};${NATIVE_DIALOG_INCLUDE_FILES}"
       "-DALLEGRO_NATIVE_DIALOG_SRC"
       "${NATIVE_DIALOG_LIBRARIES};${ALLEGRO_LINK_WITH}"
       )

   # Note: allegro_dialog NOT allegro_native_dialog.
   set_our_framework_properties(allegro_dialog AllegroDialog-${ALLEGRO_SOVERSION})

   install_our_library(allegro_dialog)
   install_our_headers(${NATIVE_DIALOG_INCLUDE_FILES})

   set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
   set(NATIVE_DIALOG_LINK_WITH allegro_dialog PARENT_SCOPE)
endif(SUPPORT_NATIVE_DIALOG)

# vim: set sts=4 sw=4 et:
