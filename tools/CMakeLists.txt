set(TOOLS_EXE_PATH "" CACHE INTERNAL "internal")
set(EXECUTABLE_OUTPUT_PATH ${TOOLS_EXE_PATH})

set(TOOLS_LIB_PATH "" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${TOOLS_LIB_PATH})

#-----------------------------------------------------------------------------#

#
#   Building libaldat
#

# Note: these lines are duplicated in GeneratePlugins.cmake
set(plugins_h "plugins/plugins.h")
file(GLOB inc_files RELATIVE "" plugins/*.inc)

add_custom_command(
    OUTPUT  ${plugins_h}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/GeneratePlugins.cmake
    DEPENDS ${inc_files}
    )

file(GLOB plugins RELATIVE "" plugins/*.c)
add_library(aldat STATIC datedit.c ${plugins} ${plugins_h})
set_target_properties(aldat PROPERTIES
    COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    )

#
# Datafile tools
#

macro(dat_tool nm)
    add_allegro_executable(${nm} ${nm}.c)
    target_link_libraries(${nm} aldat ${PROGRAM_LIBS})
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
