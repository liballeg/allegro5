include_directories(
    ../addons/iio
    ../addons/font
    ../addons/ttf
    ../addons/kcm_audio
    ../addons/acodec
    ../addons/color
    ../addons/memfile
    ../addons/physfs
    ../addons/primitives
    ../addons/native_dialog
    )

if(SUPPORT_FONT)
    example(ex_display_options ${FONT_LINK_WITH})
    example(ex_bitmap_flip ${FONT_LINK_WITH})
    example(ex_bitmap_target ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_blend ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_blend2 ex_blend2.cpp nihgui.cpp ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_font ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_joystick_events ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_keyboard_events ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_membmp ${FONT_LINK_WITH})
    example(ex_mouse_cursor ${FONT_LINK_WITH})
    example(ex_pixelformat ex_pixelformat.cpp nihgui.cpp
        ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_timer ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_windows ${FONT_LINK_WITH})
    example(ex_disable_screensaver ${FONT_LINK_WITH})
    example(ex_blit ${FONT_LINK_WITH} ${COLOR_LINK_WITH})
    example(ex_clip ${FONT_LINK_WITH} ${COLOR_LINK_WITH})
    example(ex_warp_mouse ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_draw ${FONT_LINK_WITH} ${COLOR_LINK_WITH} ${PRIMITIVES_LINK_WITH})
endif(SUPPORT_FONT)

if(SUPPORT_IIO)
    example(ex_bitmap ${IIO_LINK_WITH})
    example(ex_blend_bench ${IIO_LINK_WITH})
    example(ex_convert ${IIO_LINK_WITH})
    example(ex_dualies ${IIO_LINK_WITH})
    example(ex_expose ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_fs_resize ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_icon ${IIO_LINK_WITH})
    example(ex_lockbitmap ${IIO_LINK_WITH})
    example(ex_mouse ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_mouse_events ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_multiwin ${IIO_LINK_WITH})
    example(ex_noframe ${IIO_LINK_WITH})
    example(ex_resize2 ${IIO_LINK_WITH})
    example(ex_rotate ${IIO_LINK_WITH})
    example(ex_scale ${IIO_LINK_WITH})
    example(ex_subbitmap ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_winfull ${IIO_LINK_WITH})
endif(SUPPORT_IIO)

if(SUPPORT_KCM_AUDIO)
    example(ex_saw ${AUDIO_LINK_WITH})
endif(SUPPORT_KCM_AUDIO)

if(SUPPORT_ACODEC)
    example(ex_acodec ${ACODEC_LINK_WITH})
    example(ex_acodec_multi ${ACODEC_LINK_WITH})
    example(ex_kcm_direct ${ACODEC_LINK_WITH})
    example(ex_audio_props ex_audio_props.cpp nihgui.cpp ${ACODEC_LINK_WITH}
        ${FONT_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_audio_simple ${ACODEC_LINK_WITH})
    example(ex_mixer_chain ${ACODEC_LINK_WITH})
    example(ex_stream_file ${ACODEC_LINK_WITH})
endif(SUPPORT_ACODEC)

if(SUPPORT_TTF)
    example(ex_font_justify ex_font_justify.cpp nihgui.cpp ${TTF_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_ttf ${TTF_LINK_WITH} ${PRIMITIVES_LINK_WITH})
    example(ex_logo  ${TTF_LINK_WITH} ${PRIMITIVES_LINK_WITH})
endif(SUPPORT_TTF)

if(SUPPORT_COLOR AND SUPPORT_TTF)
    example(ex_color ex_color.cpp nihgui.cpp ${COLOR_LINK_WITH} ${TTF_LINK_WITH} ${FONT_LINK_WITH} ${IIO_LINK_WITH} ${PRIMITIVES_LINK_WITH})
endif(SUPPORT_COLOR AND SUPPORT_TTF)

example(ex_config)
example(ex_drawpixels)
example(ex_keyboard_focus)
example(ex_lines ${PRIMITIVES_LINK_WITH})
example(ex_lockscreen)
example(ex_monitorinfo)
example(ex_mouse_focus)
# FIXME: make this an optional argument to example()
if(MSVC)
    set(EXECUTABLE_TYPE)
endif(MSVC)
    example(ex_get_path)
    example(ex_path)
    example(ex_path_test)
    example(ex_utf8)
if(MSVC)
    set(EXECUTABLE_TYPE "WIN32")
endif(MSVC)
example(ex_resize ${PRIMITIVES_LINK_WITH})
example(ex_threads)
example(ex_threads2)
example(ex_timedwait)
example(ex_user_events)
example(ex_multisample ${PRIMITIVES_LINK_WITH})
example(ex_vsync)
example(ex_blend_test ${PRIMITIVES_LINK_WITH})
example(ex_dir)

if(SUPPORT_D3D)
    example(ex_d3d ex_d3d.cpp d3dx9)
endif(SUPPORT_D3D)

if(SUPPORT_OPENGL)
    example(ex_glext)
    example(ex_opengl)
endif(SUPPORT_OPENGL)

if(SUPPORT_OPENGL AND SUPPORT_FONT)
    example(ex_gldepth ${FONT_LINK_WITH})
endif(SUPPORT_OPENGL AND SUPPORT_FONT)

if(SUPPORT_MEMFILE)
   example(ex_memfile ${MEMFILE_LINK_WITH})
endif(SUPPORT_MEMFILE)

if(SUPPORT_PHYSFS AND SUPPORT_IIO)
   example(ex_physfs ${PHYSFS_LINK_WITH} ${PHYSFS_LIBRARIES} ${IIO_LINK_WITH})
endif(SUPPORT_PHYSFS AND SUPPORT_IIO)

if(SUPPORT_PRIMITIVES AND SUPPORT_TTF AND SUPPORT_ACODEC)
    example(ex_stream_seek ${PRIMITIVES_LINK_WITH} ${ACODEC_LINK_WITH} ${TTF_LINK_WITH})
endif(SUPPORT_PRIMITIVES AND SUPPORT_TTF AND SUPPORT_ACODEC)

if(SUPPORT_PRIMITIVES AND SUPPORT_FONT)
    include_directories(../addons/primitives)
    example(ex_prim ${PRIMITIVES_LINK_WITH} ${FONT_LINK_WITH}
        ${IIO_LINK_WITH})
endif(SUPPORT_PRIMITIVES AND SUPPORT_FONT)

if(SUPPORT_NATIVE_DIALOG AND SUPPORT_FONT AND SUPPORT_COLOR)
    example(ex_native_filechooser ${NATIVE_DIALOG_LINK_WITH}
        ${FONT_LINK_WITH} ${COLOR_LINK_WITH})
endif()

find_package(CURL)
if(CURL_FOUND AND SUPPORT_IIO)
    example(ex_curl ${CURL_LIBRARIES} ${IIO_LINK_WITH})
endif(CURL_FOUND AND SUPPORT_IIO)

copy_data_dir_to_build(copy_example_data data)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
