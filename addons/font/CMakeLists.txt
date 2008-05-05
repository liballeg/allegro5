set(FONT_SOURCES font.c fontbmp.c readfont.c text.c)
set(FONT_INCLUDE_FILES allegro5/a5_font.h)

macro(add_font_build nm extra_flags)
    macro(really_add_font_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${FONT_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm2}
            )

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_font_build)

    if(SHARED)
        really_add_font_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
    endif(SHARED)

    if(STATIC)
        really_add_font_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATIC")
    endif(STATIC)

endmacro(add_font_build)

if(GRADE_STANDARD)
    add_font_build(a5_font
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_font_build(a5_fontd
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_font_build(a5_fontp
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)


if(GRADE_STANDARD)
    set(FONT_LIB_TO_LINK a5_font CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(FONT_LIB_TO_LINK a5_fontd CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(FONT_LIB_TO_LINK a5_fontp CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

macro(add_font_example nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${FONT_LINK_WITH})
endmacro(add_font_example)

add_font_example(ex_font ex_font.c)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${FONT_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

