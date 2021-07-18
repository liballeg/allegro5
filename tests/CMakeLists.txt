if(NOT ALLEGRO_LINK_WITH OR NOT ALLEGRO_MAIN_LINK_WITH OR
        NOT IMAGE_LINK_WITH OR NOT COLOR_LINK_WITH OR
        NOT FONT_LINK_WITH OR NOT TTF_LINK_WITH OR
        NOT PRIMITIVES_LINK_WITH)
    message(STATUS "Not building tests due to missing library. "
        "Have: ${ALLEGRO_LINK_WITH} ${ALLEGRO_MAIN_LINK_WITH} "
        "${IMAGE_LINK_WITH} ${COLOR_LINK_WITH} "
        "${FONT_LINK_WITH} ${TTF_LINK_WITH} "
        "${PRIMITIVES_LINK_WITH}")
    return()
endif()

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
    )

if(MSVC)
    set(EXECUTABLE_TYPE)
endif(MSVC)

if(WANT_MONOLITH)
    set(LINK_WITH ${ALLEGRO_MONOLITH_LINK_WITH})
else()
    set(LINK_WITH ${ALLEGRO_LINK_WITH}
        ${ALLEGRO_MAIN_LINK_WITH}
        ${IMAGE_LINK_WITH}
        ${COLOR_LINK_WITH}
        ${FONT_LINK_WITH}
        ${TTF_LINK_WITH}
        ${PRIMITIVES_LINK_WITH}
        ${SHADER_LINK_WITH})
endif()

#-----------------------------------------------------------------------------#
#
#   Main test driver
#
#-----------------------------------------------------------------------------#

add_our_executable(
    test_driver
    LIBS
    ${LINK_WITH}
    )

set(test_files
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bitmaps.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bitmaps2.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_blend.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_locking.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_locking2.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_backbuffer.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_image.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_fonts.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prim.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prim2.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_convert.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ciede2000.ini
    )

add_dependencies(test_driver copy_example_data)

#-----------------------------------------------------------------------------#
#
#   Standalone tests
#
#-----------------------------------------------------------------------------#

add_our_executable(
    test_list
    LIBS
    ${LINK_WITH}
    )

#-----------------------------------------------------------------------------#
#
#   Commands
#
#-----------------------------------------------------------------------------#

add_custom_target(run_standalone_tests
    DEPENDS test_list
    COMMAND test_list
    )

add_custom_target(run_tests
    DEPENDS test_driver
    COMMAND test_driver ${test_files}
    )

add_custom_target(run_tests_gl
    DEPENDS test_driver
    COMMAND test_driver --force-opengl ${test_files}
    )

add_custom_target(run_tests_gl12
    DEPENDS test_driver
    COMMAND test_driver --force-opengl-1.2 ${test_files}
    )

add_custom_target(run_tests_gl20
    DEPENDS test_driver
    COMMAND test_driver --force-opengl-2.0 ${test_files}
    )

add_custom_target(run_tests_d3d
    DEPENDS test_driver
    COMMAND test_driver --force-d3d ${test_files}
    )

add_custom_target(run_tests_wine
    DEPENDS test_driver
    COMMAND wine test_driver ${test_files}
    )

add_custom_target(run_tests_wine_gl
    DEPENDS test_driver
    COMMAND wine test_driver --force-opengl ${test_files}
    )

add_custom_target(run_tests_shaders
    DEPENDS test_driver
    COMMAND test_driver --use-shaders ${test_files}
    )

# vim: set sts=4 sw=4 et:
