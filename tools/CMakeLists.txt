#
#   Building libaldat
#

set(plugins_h ${CMAKE_CURRENT_BINARY_DIR}/plugins/plugins.h)
file(GLOB inc_files RELATIVE "" plugins/*.inc)
file(GLOB plugins   RELATIVE "" plugins/*.c)

add_custom_target(plugins_h
    DEPENDS ${plugins_h}
    )
add_custom_command(
    OUTPUT  ${plugins_h}
    DEPENDS ${inc_files}
    COMMAND ${CMAKE_COMMAND}
            -D plugins_h=${plugins_h}
            -D inc_files="${inc_files}"
            -P ${CMAKE_SOURCE_DIR}/cmake/GeneratePlugins.cmake
    )

    # Add this property so that out-of-source builds can find
    # plugins/plugins.h.
    #
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(aldat STATIC datedit.c ${plugins_h} ${plugins})
set_target_properties(aldat PROPERTIES
    COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    )

#
# Datafile tools
#

macro(dat_tool nm)
    add_allegro_executable(${nm} ${nm}.c)
    target_link_libraries(${nm} aldat ${LINK_WITH})
    install(TARGETS ${nm}
            DESTINATION bin
            )
endmacro(dat_tool nm)

dat_tool(dat)
dat_tool(dat2c)
dat_tool(dat2s)
dat_tool(exedat)
dat_tool(grabber)
dat_tool(pat2dat)

#
# Other tools
#

macro(tool nm)
    add_allegro_executable(${nm} ${nm}.c)
    install(TARGETS ${nm}
            DESTINATION bin
            )
endmacro(tool nm)

tool(colormap)
tool(pack)
tool(rgbmap)
tool(textconv)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
