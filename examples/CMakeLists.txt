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
    ../addons/shader
    ../addons/ttf
    ../addons/video
    )

if(SUPPORT_PHYSFS)
    # The physfs example directly includes physfs.h
    include_directories(${PHYSFS_INCLUDE_DIR})
endif()

#-----------------------------------------------------------------------------#

# List dependencies explicitly for loading by Android APKs.

set(AUDIO x${AUDIO_LINK_WITH})
set(ACODEC ${AUDIO} x${ACODEC_LINK_WITH})
set(COLOR x${COLOR_LINK_WITH})
set(DIALOG x${NATIVE_DIALOG_LINK_WITH})
set(FONT x${FONT_LINK_WITH})
set(IMAGE x${IMAGE_LINK_WITH})
set(MEMFILE x${MEMFILE_LINK_WITH})
set(PHYSFS x${PHYSFS_LINK_WITH})
set(PRIM x${PRIMITIVES_LINK_WITH})
set(TTF ${FONT} x${TTF_LINK_WITH})
set(VIDEO x${VIDEO_LINK_WITH})
set(NIHGUI nihgui.cpp ${FONT} ${PRIM})

include(Helper.cmake)

#-----------------------------------------------------------------------------#

# Lists of data files to add to Android APKs, to appear after the DATA keyword.
# DATA_IMAGES is a special case that includes the DATA keyword.
# Examples don't necessarily use every file in the list.

set(DATA_IMAGES
    DATA
    a4_font.tga
    alexlogo.bmp
    alexlogo.png
    allegro.pcx
    bkg.png
    bmpfont.tga
    cursor.tga
    fakeamp.bmp
    fixed_font.tga
    font.tga
    gradient1.bmp
    haiku/air_effect.png
    icon.tga
    mysha.pcx
    mysha256x256.png
    obp.jpg
    texture.tga
    texture2.tga
    )

set(DATA_TTF
    DejaVuSans.ttf
    NotoColorEmoji_Animals.ttf
    a4_font.fnt
    )

set(DATA_AUDIO
    welcome.wav
    welcome.voc
    )

set(DATA_SHADERS
    ex_prim_shader_pixel.glsl
    ex_prim_shader_pixel.hlsl
    ex_prim_shader_vertex.glsl
    ex_prim_shader_vertex.hlsl
    ex_prim_wrap_pixel.glsl
    ex_prim_wrap_pixel.hlsl
    ex_prim_high_vertex.glsl
    ex_prim_high_vertex.hlsl
    ex_prim_high_pixel.glsl
    ex_prim_high_pixel.hlsl
    ex_shader_multitex_pixel.glsl
    ex_shader_multitex_pixel.hlsl
    ex_shader_palette_pixel.glsl
    ex_shader_pixel.glsl
    ex_shader_pixel.hlsl
    ex_shader_vertex.glsl
    ex_shader_vertex.hlsl
    )

file(GLOB DATA_HAIKU RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/data data/haiku/*)
set(DATA_HAIKU ${DATA_HAIKU})

#-----------------------------------------------------------------------------#

example(ex_config DATA sample.cfg)
example(ex_dir ${DATA_IMAGES})
example(ex_file CONSOLE ${DATA_IMAGES})
example(ex_file_slice CONSOLE)
example(ex_get_path)
example(ex_memfile CONSOLE ${MEMFILE})
example(ex_monitorinfo)
example(ex_path)
example(ex_path_test)
example(ex_user_events)
example(ex_inject_events)

if(NOT MSVC)
    # UTF-8 strings are problematic under MSVC.
    example(ex_utf8)
endif(NOT MSVC)

if(ANDROID)
    example(ex_android ${IMAGE} ${PRIM} ${DATA_IMAGES})
endif(ANDROID)

example(ex_bitmap ${IMAGE} ${DATA_IMAGES})
example(ex_bitmap_file ${IMAGE} ${DATA_IMAGES})
example(ex_bitmap_flip ${IMAGE} ${FONT} ${DATA_IMAGES})
example(ex_blend ${FONT} ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_blend2 ex_blend2.cpp ${NIHGUI} ${IMAGE} ${DATA_IMAGES})
example(ex_blend_bench ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_blend_test ${PRIM})
example(ex_blend_target ${IMAGE} ${DATA_IMAGES})
example(ex_blit ${FONT} ${IMAGE} ${COLOR} ${DATA_IMAGES})
example(ex_clip ${FONT} ${COLOR})
example(ex_clipboard ${IMAGE} ${FONT} ${DATA_IMAGES})
example(ex_color ex_color.cpp ${NIHGUI} ${TTF} ${COLOR} DATA ${DATA_TTF})
example(ex_color2 ex_color2.c ${FONT} ${COLOR} ${PRIM})
example(ex_color_gradient ex_color_gradient.c ${TTF} ${COLOR} ${PRIM} DATA ${DATA_TTF})
example(ex_compressed ${IMAGE} ${FONT} ${DATA_IMAGES})
example(ex_convert CONSOLE ${IMAGE})
example(ex_cpu ${FONT})
example(ex_depth_mask ${IMAGE} ${TTF} ${DATA_IMAGES} ${DATA_TTF})
example(ex_depth_target ${IMAGE} ${FONT} ${COLOR} ${PRIM})
example(ex_disable_screensaver ${FONT})
example(ex_display_events ${FONT} ${PRIM})
example(ex_display_options ${FONT} ${PRIM})
example(ex_drag_and_drop ${IMAGE} ${FONT} ${TTF} ${COLOR} ${PRIM})
example(ex_draw ${FONT} ${IMAGE} ${COLOR} ${PRIM} ${DATA_IMAGES})
example(ex_draw_bitmap ${IMAGE} ${FONT} ${DATA_IMAGES})
example(ex_drawpixels)
example(ex_dualies ${IMAGE})
example(ex_emoji ${TTF} ${COLOR} ${PRIM} ${IMAGE} DATA ${DATA_TTF})
example(ex_expose ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_filter ${FONT} ${IMAGE} ${COLOR} ${DATA_IMAGES})
example(ex_fs_resize ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_fs_window ${IMAGE} ${PRIM} ${FONT} ${DATA_IMAGES})
example(ex_icon ${IMAGE} ${DATA_IMAGES})
example(ex_icon2 ${IMAGE} ${DATA_IMAGES})
example(ex_haptic ${PRIM})
example(ex_haptic2 ex_haptic2.cpp ${NIHGUI} ${TTF} DATA ${DATA_TTF})
example(ex_joystick_events ${PRIM} ${FONT})
example(ex_joystick_hotplugging ${PRIM} ${FONT} ${COLOR})
example(ex_keyboard_events)
example(ex_keyboard_focus)
example(ex_lines ${PRIM})
example(ex_loading_thread ${IMAGE} ${FONT} ${PRIM} ${DATA_IMAGES})
example(ex_lockbitmap)
example(ex_membmp ${FONT} ${IMAGE} ${DATA_IMAGES})
example(ex_mouse ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_mouse_cursor ${FONT} ${IMAGE} ${DATA_IMAGES})
example(ex_mouse_events ${FONT} ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_mouse_focus)
example(ex_mouse_warp ${FONT} ${PRIM})
example(ex_multisample ${FONT} ${COLOR} ${PRIM})
example(ex_multisample_target ${IMAGE} ${FONT} ${COLOR} ${PRIM})
example(ex_multiwin ${IMAGE})
example(ex_nodisplay ${IMAGE} ${DATA_IMAGES})
example(ex_noframe ${IMAGE} ${DATA_IMAGES})
example(ex_physfs ${PHYSFS} ${IMAGE} ${DATA_IMAGES})
example(ex_pixelformat ex_pixelformat.cpp ${NIHGUI} ${IMAGE} ${DATA_IMAGES})
example(ex_polygon ${FONT} ${PRIM})
example(ex_premulalpha ${FONT})
example(ex_prim ${FONT} ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_prim_shader ${PRIM} DATA ${DATA_SHADERS})
example(ex_prim_wrap ${IMAGE} ${FONT} ${PRIM} ${DATA_IMAGES} ${DATA_SHADERS})
example(ex_reparent ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_resize ${PRIM})
example(ex_resize2 ${IMAGE} ${FONT})
example(ex_rotate ${IMAGE} ${DATA_IMAGES})
example(ex_scale ${IMAGE} ${DATA_IMAGES})
example(ex_shader ex_shader.cpp ${IMAGE} ${DATA_IMAGES} ${DATA_SHADERS})
example(ex_shader_multitex ${IMAGE} ${DATA_IMAGES} ${DATA_SHADERS})
example(ex_shader_target ${IMAGE} ${DATA_IMAGES} ${DATA_SHADERS})
example(ex_subbitmap ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_threads ${PRIM})
example(ex_threads2)
example(ex_timedwait ${FONT})
example(ex_timer ${FONT} ${PRIM})
example(ex_timer_pause)
example(ex_touch_input ${PRIM})
example(ex_transform ${FONT} ${IMAGE} ${PRIM} ${DATA_IMAGES})
example(ex_vertex_buffer ${FONT} ${PRIM})
example(ex_vsync ${FONT} ${IMAGE} ${PRIM})
example(ex_windows ${FONT} ${IMAGE})
example(ex_window_constraints ${FONT} ${IMAGE})
example(ex_window_maximized ${FONT} ${IMAGE} ${PRIM} ${DIALOG})
example(ex_window_title ${IMAGE} ${FONT} ${DATA_IMAGES})
example(ex_winfull)
example(ex_video ${AUDIO} ${FONT} ${PRIM} ${VIDEO})

if(WANT_D3D AND D3DX9_FOUND)
    example(ex_d3d ex_d3d.cpp EXTRA_LIBS ${D3DX9_LIBRARY})
endif()

if(SUPPORT_OPENGL AND NOT SUPPORT_OPENGLES)
    example(ex_gldepth ${FONT} ${IMAGE})
    example(ex_glext)
    example(ex_opengl)
    example(ex_opengl_pixel_shader ${IMAGE})
endif()
if(SUPPORT_OPENGL AND NOT IPHONE AND NOT ALLEGRO_RASPBERRYPI)
    example(ex_palette ${IMAGE} ${COLOR} ${DATA_IMAGES} ${DATA_SHADERS})
endif()

example(ex_font ${FONT} ${IMAGE} ${DATA_IMAGES})
example(ex_font_justify ex_font_justify.cpp ${NIHGUI} ${IMAGE} ${TTF} ${DATA_IMAGES} ${DATA_TTF})
example(ex_font_multiline ex_font_multiline.cpp ${NIHGUI} ${IMAGE} ${TTF} ${COLOR} ${DATA_IMAGES} ${DATA_TTF})
example(ex_logo ${FONT} ${TTF} ${IMAGE} ${PRIM} DATA ${DATA_TTF})
example(ex_projection ${TTF} ${IMAGE} ${DATA_IMAGES} ${DATA_TTF})
example(ex_projection2 ${PRIM} ${FONT} ${IMAGE} ${DATA_IMAGES})
example(ex_camera ${FONT} ${COLOR} ${PRIM} ${IMAGE})
example(ex_ttf ${TTF} ${PRIM} ${IMAGE} DATA ${DATA_TTF} ex_ttf.ini)

example(ex_acodec CONSOLE ${AUDIO} ${ACODEC})
example(ex_acodec_multi CONSOLE ${AUDIO} ${ACODEC})
example(ex_audio_devices ${AUDIO})
example(ex_audio_chain ex_audio_chain.cpp ${AUDIO} ${ACODEC} ${PRIM} ${FONT} ${TTF} DATA ${DATA_TTF} ${DATA_HAIKU})
example(ex_audio_props ex_audio_props.cpp ${NIHGUI} ${ACODEC} DATA ${DATA_AUDIO})
example(ex_audio_simple CONSOLE ${AUDIO} ${ACODEC} ${FONT})
example(ex_audio_timer ${AUDIO} ${FONT})
example(ex_haiku ${AUDIO} ${ACODEC} ${IMAGE} ${DATA_IMAGES} ${DATA_HAIKU})
example(ex_kcm_direct CONSOLE ${AUDIO} ${ACODEC})
example(ex_mixer_chain CONSOLE ${AUDIO} ${ACODEC})
example(ex_mixer_pp ${AUDIO} ${ACODEC} ${PRIM} ${IMAGE} ${DATA_IMAGES} ${DATA_AUDIO})
example(ex_record ${AUDIO} ${ACODEC} ${PRIM})
example(ex_record_name ${AUDIO} ${ACODEC} ${PRIM} ${IMAGE} ${FONT})
example(ex_resample_test ${AUDIO})
example(ex_saw ${AUDIO})
example(ex_stream_file CONSOLE ${AUDIO} ${ACODEC})
example(ex_stream_seek ${AUDIO} ${ACODEC} ${PRIM} ${FONT} ${IMAGE} ${DATA_IMAGES} ${DATA_AUDIO})
example(ex_synth ex_synth.cpp ${NIHGUI} ${AUDIO} ${TTF} DATA ${DATA_TTF})

example(ex_native_filechooser ${DIALOG} ${FONT} ${IMAGE} ${COLOR} ${DATA_IMAGES})
example(ex_menu ${DIALOG} ${IMAGE})

# In some configurations CURL pulls in dependencies which we don't check for.
# This example isn't important so it's disabled by default to prevent problems.
option(WANT_CURL_EXAMPLE "Build ex_curl example" off)
if(WANT_CURL_EXAMPLE)
    find_package(CURL)
    if(CURL_FOUND)
        if(WIN32)
            # select() is in the Winsock library.
            example(ex_curl ${IMAGE} EXTRA_LIBS ${CURL_LIBRARIES} ws2_32)
        else(WIN32)
            example(ex_curl ${IMAGE} EXTRA_LIBS ${CURL_LIBRARIES})
        endif(WIN32)
    endif(CURL_FOUND)
endif(WANT_CURL_EXAMPLE)

# Only build the enet examples if libenet is installed
find_package(ENet)
if(ENET_FOUND)
    example(ex_enet_client ${PRIM} EXTRA_LIBS ${ENET_LIBRARIES})
    example(ex_enet_server EXTRA_LIBS ${ENET_LIBRARIES})
endif()

# example(ex_ogre3d ex_ogre3d.cpp)
# include_directories(/usr/include/OGRE)
# target_link_libraries(ex_ogre3d OgreMain)

# This is useful for developers to add temporary test programs.
include(${CMAKE_CURRENT_SOURCE_DIR}/local_examples.cmake OPTIONAL)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
