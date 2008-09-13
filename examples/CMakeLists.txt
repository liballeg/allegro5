if(SUPPORT_FONT)
    include_directories(../addons/font)
    set(LIBS ${FONT_LINK_WITH} ${IIO_LINK_WITH})
    example(ex_events ${LIBS})
    example(ex_bitmap_target ${LIBS})
    example(ex_membmp ${LIBS})
    example(ex_blend ${LIBS})
    example(ex_pixelformat ${LIBS})
    example(ex_timer ${LIBS})
    example(ex_windows ${LIBS})
    example(ex_font ${LIBS})
endif(SUPPORT_FONT)

if(SUPPORT_IIO)
    example(ex_bitmap ${IIO_LINK_WITH})
    example(ex_convert ${IIO_LINK_WITH})
    example(ex_dualies ${IIO_LINK_WITH})
    example(ex_fs_resize ${IIO_LINK_WITH})
    example(ex_icon ${IIO_LINK_WITH})
    example(ex_lockbitmap ${IIO_LINK_WITH})
    example(ex_mouse ${IIO_LINK_WITH})
    example(ex_mouse_cursor ${IIO_LINK_WITH})
    example(ex_mouse_events ${IIO_LINK_WITH})
    example(ex_multiwin ${IIO_LINK_WITH})
    example(ex_resize2 ${IIO_LINK_WITH})
    example(ex_scale ${IIO_LINK_WITH})
    example(ex_noframe ${IIO_LINK_WITH})
    example(ex_expose ${IIO_LINK_WITH})
    example(ex_winfull ${IIO_LINK_WITH})
endif(SUPPORT_IIO)

if(SUPPORT_KCM_AUDIO)
    include_directories(../addons/kcm_audio)
    example(ex_saw ${AUDIO_LINK_WITH})
endif(SUPPORT_KCM_AUDIO)

if(SUPPORT_TTF)
    include_directories(../addons/ttf)
    example(ex_ttf ${TTF_LINK_WITH} ${FONT_LINK_WITH} ${IIO_LINK_WITH})
endif(SUPPORT_TTF)

if(SUPPORT_ACODEC)
    include_directories(../addons/acodec)
    example(ex_acodec ${ACODEC_LINK_WITH} ${AUDIO_LINK_WITH})
    example(ex_acodec_multi ${ACODEC_LINK_WITH} ${AUDIO_LINK_WITH})
    #disable until the streaming is done
    #example(ex_acodec_stream ${ACODEC_LINK_WITH})
    example(ex_stream_file ${ACODEC_LINK_WITH} ${AUDIO_LINK_WITH})

    #example(ex_acodec_mixer ${ACODEC_LINK_WITH})
endif(SUPPORT_ACODEC)


example(ex_drawpixels NOTHING)
example(ex_lines NOTHING)
example(ex_lockscreen NOTHING)
example(ex_monitorinfo NOTHING)
example(ex_resize NOTHING)
example(ex_threads NOTHING)
example(ex_threads2 NOTHING)
example(ex_timedwait NOTHING)


if(SUPPORT_D3D)
    example_cxx(ex_d3d d3dx9)
endif(SUPPORT_D3D)

if(SUPPORT_OPENGL)
    example(ex_opengl NOTHING)
    example(ex_glext NOTHING)
endif(SUPPORT_OPENGL)


#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
