include_directories(
    ../addons/acodec
    ../addons/audio
    ../addons/color
    ../addons/font
    ../addons/image
    ../addons/main
    ../addons/memfile
    ../addons/native_dialog
    ../addons/physfs
    ../addons/primitives
    ../addons/ttf
    )

if(SUPPORT_PHYSFS)
    # The physfs example directly includes physfs.h
    include_directories(${PHYSFS_INCLUDE_DIR})
endif()

#-----------------------------------------------------------------------------#

# Conditionally build an example program.  If any of its arguments is the exact
# string "x", do nothing.  Otherwise strip off the "x" prefixes on arguments
# and call the example macro.
function(example name)
    set(is_console)
    set(args)
    foreach(arg ${ARGN})
        if(arg STREQUAL "x")
            message(STATUS "Not building ${name}")
            return()
        endif()
        if(arg STREQUAL "CONSOLE")
            set(is_console ON)
        else()
            string(REGEX REPLACE "^x" "" arg ${arg})
            list(APPEND args ${arg})
        endif()
    endforeach(arg)

    if(WIN32)
        if(is_console)
            # We need stdout and stderr available from cmd.exe,
            # so we must not use WIN32 here.
            set(EXECUTABLE_TYPE)
        else()
            set(EXECUTABLE_TYPE "WIN32")
        endif(is_console)
    endif(WIN32)

    if(IPHONE)
        set(EXECUTABLE_TYPE MACOSX_BUNDLE)
    endif(IPHONE)

    add_example(${name} ${args})
endfunction(example)

macro(add_example nm)
    if(WANT_POPUP_EXAMPLES AND SUPPORT_NATIVE_DIALOG)
        add_our_executable(${nm} ${ARGN} ${ALLEGRO_LINK_WITH} ${ALLEGRO_MAIN_LINK_WITH} ${NATIVE_DIALOG_LINK_WITH})
    else()
        add_our_executable(${nm} ${ARGN} ${ALLEGRO_LINK_WITH} ${ALLEGRO_MAIN_LINK_WITH} )
    endif()
endmacro(add_example nm)

#-----------------------------------------------------------------------------#

set(AUDIO x${AUDIO_LINK_WITH})
set(ACODEC x${ACODEC_LINK_WITH})
set(COLOR x${COLOR_LINK_WITH})
set(DIALOG x${NATIVE_DIALOG_LINK_WITH})
set(FONT x${FONT_LINK_WITH})
set(IMAGE x${IMAGE_LINK_WITH})
set(MEMFILE x${MEMFILE_LINK_WITH})
set(PHYSFS x${PHYSFS_LINK_WITH})
set(PRIM x${PRIMITIVES_LINK_WITH})
set(TTF x${TTF_LINK_WITH})

set(NIHGUI nihgui.cpp ${FONT} ${PRIM})

#-----------------------------------------------------------------------------#

example(ex_config)
example(ex_dir)
example(ex_file_slice CONSOLE)
example(ex_get_path)
example(ex_memfile CONSOLE ${MEMFILE})
example(ex_monitorinfo)
example(ex_path)
example(ex_path_test)
example(ex_user_events)

if(NOT MSVC)
    # UTF-8 strings are problematic under MSVC.
    example(ex_utf8)
endif(NOT MSVC)

example(ex_bitmap ${IMAGE})
example(ex_bitmap_flip ${IMAGE} ${FONT})
example(ex_bitmap_target ${FONT} ${IMAGE} ${PRIM})
example(ex_blend ${FONT} ${IMAGE} ${PRIM})
example(ex_blend2 ex_blend2.cpp ${NIHGUI} ${IMAGE})
example(ex_blend_bench ${IMAGE} ${PRIM})
example(ex_blend_test ${PRIM})
example(ex_blit ${FONT} ${IMAGE} ${COLOR})
example(ex_clip ${FONT} ${IMAGE} ${COLOR})
example(ex_color ex_color.cpp ${NIHGUI} ${TTF} ${COLOR})
example(ex_convert CONSOLE ${IMAGE})
example(ex_disable_screensaver ${FONT} ${IMAGE})
example(ex_display_events ${FONT} ${IMAGE} ${PRIM})
example(ex_display_options ${FONT} ${IMAGE} ${PRIM})
example(ex_draw ${FONT} ${IMAGE} ${COLOR} ${PRIM})
example(ex_draw_bitmap ${IMAGE} ${FONT})
example(ex_drawpixels)
example(ex_dualies ${IMAGE})
example(ex_expose ${IMAGE} ${PRIM})
example(ex_filter ${FONT} ${IMAGE} ${COLOR})
example(ex_fs_resize ${IMAGE} ${PRIM})
example(ex_fs_window ${IMAGE} ${PRIM} ${FONT})
example(ex_icon ${IMAGE})
example(ex_icon2 ${IMAGE})
example(ex_joystick_events ${PRIM})
example(ex_joystick_hotplugging ${PRIM})
example(ex_keyboard_events)
example(ex_keyboard_focus)
example(ex_lines ${PRIM})
example(ex_lockbitmap)
example(ex_membmp ${FONT} ${IMAGE})
example(ex_mouse ${IMAGE} ${PRIM})
example(ex_mouse_cursor ${FONT} ${IMAGE})
example(ex_mouse_events ${FONT} ${IMAGE} ${PRIM})
example(ex_mouse_focus)
example(ex_multisample ${FONT} ${COLOR} ${PRIM})
example(ex_multiwin ${IMAGE})
example(ex_nodisplay ${IMAGE})
example(ex_noframe ${IMAGE})
example(ex_physfs ${PHYSFS} ${IMAGE})
example(ex_pixelformat ex_pixelformat.cpp ${NIHGUI} ${IMAGE})
example(ex_premulalpha ${IMAGE} ${FONT})
example(ex_prim ${FONT} ${IMAGE} ${PRIM})
example(ex_resize ${PRIM})
example(ex_resize2 ${IMAGE})
example(ex_rotate ${IMAGE})
example(ex_scale ${IMAGE})
example(ex_subbitmap ${IMAGE} ${PRIM})
example(ex_threads ${PRIM})
example(ex_threads2)
example(ex_timedwait)
example(ex_timer ${FONT} ${IMAGE} ${PRIM})
example(ex_transform ${FONT} ${IMAGE} ${PRIM})
example(ex_vsync ${FONT} ${IMAGE})
example(ex_warp_mouse ${FONT} ${PRIM} ${IMAGE})
example(ex_windows ${FONT} ${IMAGE})
example(ex_winfull)

if(WANT_D3D AND D3DX9_FOUND)
    example(ex_d3d ex_d3d.cpp ${D3DX9_LIBRARY})
endif(WANT_D3D AND D3DX9_FOUND)

if(SUPPORT_OPENGL AND NOT IPHONE)
    example(ex_gldepth ${FONT} ${IMAGE})
    example(ex_glext)
    example(ex_opengl)
    example(ex_opengl_pixel_shader ${IMAGE})
endif(SUPPORT_OPENGL AND NOT IPHONE)

example(ex_font ${FONT} ${IMAGE})
example(ex_font_justify ex_font_justify.cpp ${NIHGUI} ${IMAGE} ${TTF})
example(ex_logo ${FONT} ${TTF} ${IMAGE} ${PRIM})
example(ex_ttf ${TTF} ${PRIM})

example(ex_acodec CONSOLE ${AUDIO} ${ACODEC})
example(ex_acodec_multi CONSOLE ${AUDIO} ${ACODEC})
example(ex_audio_chain ex_audio_chain.cpp ${AUDIO} ${ACODEC} ${PRIM} ${FONT} ${TTF})
example(ex_audio_props CONSOLE ex_audio_props.cpp ${NIHGUI} ${IMAGE} ${AUDIO} ${ACODEC})
example(ex_audio_simple CONSOLE ${AUDIO} ${ACODEC})
example(ex_audio_timer ${AUDIO} ${FONT})
example(ex_haiku ${AUDIO} ${ACODEC} ${IMAGE})
example(ex_kcm_direct CONSOLE ${AUDIO} ${ACODEC})
example(ex_mixer_chain CONSOLE ${AUDIO} ${ACODEC})
example(ex_mixer_pp ${AUDIO} ${ACODEC} ${PRIM} ${IMAGE})
example(ex_resample_test ${AUDIO})
example(ex_saw ${AUDIO})
example(ex_stream_file CONSOLE ${AUDIO} ${ACODEC})
example(ex_stream_seek CONSOLE ${AUDIO} ${ACODEC} ${PRIM} ${FONT} ${IMAGE})
example(ex_synth ex_synth.cpp ${NIHGUI} ${AUDIO} ${TTF})

example(ex_native_filechooser ${DIALOG} ${FONT} ${IMAGE} ${COLOR})

# In some configurations CURL pulls in dependencies which we don't check for.
# This example isn't important so it's disabled by default to prevent problems.
option(WANT_CURL_EXAMPLE "Build ex_curl example" off)
if(WANT_CURL_EXAMPLE)
    find_package(CURL)
    if(CURL_FOUND)
        if(WIN32)
            # select() is in the Winsock library.
            example(ex_curl ${CURL_LIBRARIES} ${IMAGE} ws2_32)
        else(WIN32)
            example(ex_curl ${CURL_LIBRARIES} ${IMAGE})
        endif(WIN32)
    endif(CURL_FOUND)
endif(WANT_CURL_EXAMPLE)

# Not yet added:
# ex_gp2x
# ex_iphone

# example(ex_ogre3d ex_ogre3d.cpp)
# include_directories(/usr/include/OGRE)
# target_link_libraries(ex_ogre3d OgreMain)

copy_data_dir_to_build(copy_example_data data .)

# This is useful for developers to add temporary test programs.
include(${CMAKE_CURRENT_SOURCE_DIR}/local_examples.cmake OPTIONAL)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
