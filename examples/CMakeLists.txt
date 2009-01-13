include_directories(
    ../addons/iio
    ../addons/font
    ../addons/ttf
    ../addons/icodec
    ../addons/kcm_audio
    ../addons/acodec
    ../addons/color
    ../addons/memfile
    )

if(SUPPORT_FONT)
    example(ex_bitmap_target ${FONT_LINK_WITH})
    example(ex_blend ${FONT_LINK_WITH})
    example(ex_blend2 ex_blend2.cpp nihgui.cpp ${FONT_LINK_WITH})
    example(ex_font ${FONT_LINK_WITH})
    example(ex_joystick_events ${FONT_LINK_WITH})
    example(ex_keyboard_events ${FONT_LINK_WITH})
    example(ex_membmp ${FONT_LINK_WITH})
    example(ex_mouse_cursor ${FONT_LINK_WITH})
    example(ex_pixelformat ${FONT_LINK_WITH})
    example(ex_timer ${FONT_LINK_WITH})
    example(ex_windows ${FONT_LINK_WITH})
    example(ex_disable_screensaver ${FONT_LINK_WITH})
endif(SUPPORT_FONT)

if(SUPPORT_IIO)
    example(ex_bitmap ${IIO_LINK_WITH})
    example(ex_bitmap_flip ${IIO_LINK_WITH})
    example(ex_convert ${IIO_LINK_WITH})
    example(ex_dualies ${IIO_LINK_WITH})
    example(ex_expose ${IIO_LINK_WITH})
    example(ex_fs_resize ${IIO_LINK_WITH})
    example(ex_icon ${IIO_LINK_WITH})
    example(ex_lockbitmap ${IIO_LINK_WITH})
    example(ex_mouse ${IIO_LINK_WITH})
    example(ex_mouse_events ${IIO_LINK_WITH})
    example(ex_multiwin ${IIO_LINK_WITH})
    example(ex_noframe ${IIO_LINK_WITH})
    example(ex_resize2 ${IIO_LINK_WITH})
    example(ex_scale ${IIO_LINK_WITH})
    example(ex_subbitmap ${IIO_LINK_WITH})
    example(ex_winfull ${IIO_LINK_WITH})
endif(SUPPORT_IIO)

if(SUPPORT_KCM_AUDIO)
    example(ex_saw ${AUDIO_LINK_WITH})
endif(SUPPORT_KCM_AUDIO)

if(SUPPORT_ACODEC)
    example(ex_acodec ${ACODEC_LINK_WITH})
    example(ex_acodec_multi ${ACODEC_LINK_WITH})
    example(ex_audio_simple ${ACODEC_LINK_WITH})
    example(ex_mixer_chain ${ACODEC_LINK_WITH})
    example(ex_stream_file ${ACODEC_LINK_WITH})
endif(SUPPORT_ACODEC)

if(SUPPORT_TTF)
    example(ex_ttf ${TTF_LINK_WITH})
endif(SUPPORT_TTF)

if(SUPPORT_ICODEC)
    example(ex_icodec ${ICODEC_LINK_WITH})
endif(SUPPORT_ICODEC)

if(SUPPORT_COLOR AND SUPPORT_TTF)
    example(ex_color ex_color.cpp nihgui.cpp ${COLOR_LINK_WITH} ${TTF_LINK_WITH} ${FONT_LINK_WITH} ${IIO_LINK_WITH})
endif(SUPPORT_COLOR AND SUPPORT_TTF)

example(ex_config)
example(ex_drawpixels)
example(ex_keyboard_focus)
example(ex_lines)
example(ex_lockscreen)
example(ex_monitorinfo)
example(ex_mouse_focus)
# FIXME: make this an optional argument to example()
if(MSVC)
    set(EXECUTABLE_TYPE)
endif(MSVC)
    example(ex_get_path)
    example(ex_path)
if(MSVC)
    set(EXECUTABLE_TYPE "WIN32")
endif(MSVC)
example(ex_resize)
example(ex_threads)
example(ex_threads2)
example(ex_timedwait)
example(ex_user_events)

if(SUPPORT_D3D)
    example(ex_d3d ex_d3d.cpp d3dx9)
endif(SUPPORT_D3D)

if(SUPPORT_OPENGL)
    example(ex_glext)
    example(ex_opengl)
endif(SUPPORT_OPENGL)

if(SUPPORT_MEMFILE)
   example(ex_memfile ${MEMFILE_LINK_WITH})
endif(SUPPORT_MEMFILE)

copy_data_dir_to_build(copy_example_data data)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
