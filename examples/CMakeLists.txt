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

#-----------------------------------------------------------------------------#

# Conditionally build an example program.  If any of its arguments is the exact
# string "x", do nothing.  Otherwise strip off the "x" prefixes on arguments
# and call the example macro.
function(example name)
    set(args)
    foreach(arg ${ARGN})
        if(arg STREQUAL "x")
            message(STATUS "Not building ${name}")
            return()
        endif()
        string(REGEX REPLACE "^x" "" arg ${arg})
        list(APPEND args ${arg})
    endforeach(arg)
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
set(ACODEC x${FLAC_LINK_WITH} x${VORBIS_LINK_WITH})
set(COLOR x${COLOR_LINK_WITH})
set(DIALOG x${NATIVE_DIALOG_LINK_WITH})
set(FONT x${FONT_LINK_WITH})
set(IMAGE x${IMAGE_LINK_WITH})
set(MEMFILE x${MEMFILE_LINK_WITH})
set(PHYSFS x${PHYSFS_LINK_WITH})
set(PRIM x${PRIMITIVES_LINK_WITH})
set(TTF x${TTF_LINK_WITH})

set(NIHGUI nihgui.cpp ${FONT} ${IMAGE} ${PRIM})

#-----------------------------------------------------------------------------#

# FIXME: make this an optional argument to example()
if(MSVC)
    set(EXECUTABLE_TYPE)
endif(MSVC)
    example(ex_config)
    example(ex_dir)
    example(ex_get_path)
    example(ex_memfile ${MEMFILE})
    example(ex_monitorinfo)
    example(ex_path)
    example(ex_path_test)
    example(ex_user_events)
    example(ex_utf8)
if(MSVC)
    set(EXECUTABLE_TYPE "WIN32")
endif(MSVC)

example(ex_bitmap ${IMAGE})
example(ex_bitmap_flip ${IMAGE} ${FONT})
example(ex_bitmap_target ${FONT} ${PRIM})
example(ex_blend ${FONT} ${PRIM})
example(ex_blend2 ex_blend2.cpp ${NIHGUI})
example(ex_blend_bench ${IMAGE})
example(ex_blend_test ${PRIM})
example(ex_blit ${FONT} ${COLOR})
example(ex_clip ${FONT} ${COLOR})
example(ex_color ex_color.cpp ${NIHGUI} ${TTF} ${COLOR})
example(ex_convert ${IMAGE})
example(ex_disable_screensaver ${FONT})
example(ex_display_options ${FONT})
example(ex_draw ${FONT} ${COLOR} ${PRIM})
example(ex_draw_bitmap ${IMAGE} ${FONT})
example(ex_drawpixels)
example(ex_dualies ${IMAGE})
example(ex_expose ${IMAGE} ${PRIM})
example(ex_fs_resize ${IMAGE} ${PRIM})
example(ex_icon ${IMAGE})
example(ex_joystick_events ${PRIM})
example(ex_keyboard_events ${FONT} ${IMAGE})
example(ex_keyboard_focus)
example(ex_lines ${PRIM})
example(ex_lockbitmap ${IMAGE})
example(ex_lockscreen)
example(ex_membmp ${FONT} ${IMAGE})
example(ex_mouse ${IMAGE} ${PRIM})
example(ex_mouse_cursor ${FONT} ${IMAGE})
example(ex_mouse_events ${FONT} ${IMAGE} ${PRIM})
example(ex_mouse_focus)
example(ex_multisample ${PRIM})
example(ex_multiwin ${IMAGE})
example(ex_noframe ${IMAGE})
example(ex_physfs ${PHYSFS} ${IMAGE})
example(ex_pixelformat ex_pixelformat.cpp ${NIHGUI})
example(ex_prim ${FONT} ${IMAGE} ${PRIM})
example(ex_resize ${PRIM})
example(ex_resize2 ${IMAGE})
example(ex_rotate ${IMAGE})
example(ex_scale ${IMAGE})
example(ex_subbitmap ${IMAGE} ${PRIM})
example(ex_threads)
example(ex_threads2)
example(ex_timedwait)
example(ex_timer ${FONT} ${PRIM})
example(ex_transform ${FONT} ${IMAGE})
example(ex_vsync)
example(ex_warp_mouse ${FONT} ${PRIM})
example(ex_windows ${FONT})
example(ex_winfull)

if(SUPPORT_D3D)
    example(ex_d3d ex_d3d.cpp d3dx9)
endif(SUPPORT_D3D)

if(SUPPORT_OPENGL)
    example(ex_gldepth ${FONT} ${IMAGE})
    example(ex_glext)
    example(ex_opengl)
    example(ex_opengl_pixel_shader ${IMAGE})
endif(SUPPORT_OPENGL)

example(ex_font ${FONT} ${IMAGE})
example(ex_font_justify ex_font_justify.cpp ${NIHGUI} ${TTF})
example(ex_logo ${FONT} ${TTF} ${IMAGE} ${PRIM})
example(ex_ttf ${TTF} ${PRIM})

# Currently both FLAC and Vorbis are required because the examples
# unconditionally initialise the addons.
example(ex_acodec ${AUDIO} ${ACODEC})
example(ex_acodec_multi ${AUDIO} ${ACODEC})
example(ex_audio_props ex_audio_props.cpp ${NIHGUI} ${AUDIO} ${ACODEC})
example(ex_audio_simple ${AUDIO} ${ACODEC})
example(ex_kcm_direct ${AUDIO} ${ACODEC})
example(ex_mixer_chain ${AUDIO} ${ACODEC})
example(ex_saw ${AUDIO})
example(ex_stream_file ${AUDIO} ${ACODEC})
example(ex_stream_seek ${AUDIO} ${ACODEC} ${PRIM} ${TTF})
example(ex_synth ex_synth.cpp ${NIHGUI} ${AUDIO} ${TTF})

example(ex_native_filechooser ${DIALOG} ${FONT} ${COLOR})

find_package(CURL)
if(CURL_FOUND)
    if(WIN32)
        # select() is in the Winsock library.
        example(ex_curl ${CURL_LIBRARIES} ${IMAGE} ws2_32)
    else(WIN32)
        example(ex_curl ${CURL_LIBRARIES} ${IMAGE})
    endif(WIN32)
endif(CURL_FOUND)

# Not yet added:
# ex_gp2x
# ex_iphone

copy_data_dir_to_build(copy_example_data data)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
