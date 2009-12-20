#-----------------------------------------------------------------------------#
#
# aldat library
#

file(GLOB PLUGIN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.c)
file(GLOB PLUGIN_INCS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.inc)

# XXX support plugin "scripts", i.e. linker arguments.
# With CMake it should just be the name of a target to link with.

if(SUPPORT_JPGALLEG)
    list(APPEND PLUGIN_SRCS
        ${CMAKE_SOURCE_DIR}/addons/jpgalleg/plugin/datjpeg.c)
    list(APPEND PLUGIN_INCS
        ${CMAKE_SOURCE_DIR}/addons/jpgalleg/plugin/datjpeg.inc)
    include_directories(${CMAKE_SOURCE_DIR}/addons/jpgalleg/include)
    set(ALDAT_JPGALLEG jpgalleg)
endif(SUPPORT_JPGALLEG)

# Concatenate plugins/*.inc files into a temporary file.
set(plugins ${CMAKE_CURRENT_BINARY_DIR}/plugins)
file(WRITE ${plugins}.tmp "")
foreach(incfile ${PLUGIN_INCS})
    file(READ ${incfile} incfile_contents)
    file(APPEND ${plugins}.tmp "${incfile_contents}")
endforeach(incfile)

# Copy to plugins.h if necessary.
configure_file(${plugins}.tmp ${plugins}.h @ONLY)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(datedit.c
    PROPERTIES OBJECT_DEPENDS ${plugins}.h
    )

# Don't use add_our_library as we don't want to install it.
add_library(aldat STATIC datedit.c ${PLUGIN_SRCS})
target_link_libraries(aldat allegro ${ALDAT_JPGALLEG})

#-----------------------------------------------------------------------------#
#
# Tools
#

function(add_our_tool nm)
    add_our_executable(${nm} ${nm}.c ${ARGN})
    install(TARGETS ${nm}
	    DESTINATION bin
	    )
endfunction()

add_our_tool(colormap)
add_our_tool(dat2c)
add_our_tool(exedat)
add_our_tool(pack)
add_our_tool(rgbmap)
add_our_tool(textconv)

add_our_tool(dat)
add_our_tool(dat2s)
add_our_tool(pat2dat)
target_link_libraries(dat aldat)
target_link_libraries(dat2s aldat)
target_link_libraries(pat2dat aldat)

add_our_executable(grabber WIN32 grabber.c)
target_link_libraries(grabber aldat)
install(TARGETS grabber
        DESTINATION bin
        )

#-----------------------------------------------------------------------------#
#
# Platform-specific tools
#

# XXX add bfixicon when adding Haiku support

if(APPLE)
    add_our_executable(fixbundle macosx/fixbundle.c)
endif(APPLE)

if(WIN32)
    add_our_executable(wfixicon win/wfixicon.c)
endif(WIN32)

if(ALLEGRO_WITH_XWINDOWS)
    add_our_executable(xf2pcx x11/xf2pcx.c)
    add_our_executable(xkeymap x11/xkeymap.c)
endif(ALLEGRO_WITH_XWINDOWS)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
