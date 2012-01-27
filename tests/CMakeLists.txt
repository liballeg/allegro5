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
    ../addons/ttf
    )

if(MSVC)
    set(EXECUTABLE_TYPE)
endif(MSVC)

add_our_executable(
    test_driver
    ${ALLEGRO_LINK_WITH}
    ${ALLEGRO_MAIN_LINK_WITH}
    ${IMAGE_LINK_WITH}
    ${COLOR_LINK_WITH}
    ${FONT_LINK_WITH}
    ${TTF_LINK_WITH}
    ${PRIMITIVES_LINK_WITH}
    )

add_custom_target(run_tests
    DEPENDS test_driver
    COMMAND test_driver
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
    )

# vim: set sts=4 sw=4 et:
