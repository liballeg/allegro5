macro(example nm libs)
    add_allegro_executable(${nm} ${libs} ${nm}.c)
endmacro(example nm libs)

macro(example_cxx nm libs)
    add_allegro_executable(${nm} ${libs} ${nm}.cpp)
endmacro(example_cxx nm libs)


if(WANT_FONT)
    include_directories(../addons/font)
    set(LIBS ${FONT_LINK_WITH} ${IIO_LINK_WITH})
    example(exnew_events ${LIBS})
    example(exnew_bitmap_target ${LIBS})
    example(exnew_membmp ${LIBS})
    example(exnew_blend ${LIBS})
    example(exnew_timer ${LIBS})
    example(exnew_windows ${LIBS})
endif(WANT_FONT)


if(WANT_IIO)
    example(exnew_bitmap ${IIO_LINK_WITH})
    example(exnew_dualies ${IIO_LINK_WITH})
    example(exnew_fs_resize ${IIO_LINK_WITH})
    example(exnew_icon ${IIO_LINK_WITH})
    example(exnew_lockbitmap ${IIO_LINK_WITH})
    example(exnew_mouse ${IIO_LINK_WITH})
    example(exnew_mouse_events ${IIO_LINK_WITH})
    example(exnew_multiwin ${IIO_LINK_WITH})
    example(exnew_scale ${IIO_LINK_WITH})
    example(exnew_noframe ${IIO_LINK_WITH})
endif(WANT_IIO)


example(exnew_drawpixels NOTHING)
example(exnew_lines NOTHING)
example(exnew_lockscreen NOTHING)
example(exnew_monitorinfo NOTHING)
example(exnew_resize NOTHING)
example(exnew_threads NOTHING)
example(exnew_threads2 NOTHING)
example(exnew_timedwait NOTHING)


if(SUPPORT_D3D)
    example_cxx(exnew_d3d d3dx9)
endif(SUPPORT_D3D)

if(SUPPORT_OPENGL)
    example(exnew_opengl NOTHING)
endif(SUPPORT_OPENGL)


#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
