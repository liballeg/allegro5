include_directories(.)

set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/a5_color.h
    )

macro(add_color_build nm extra_flags)
    macro(really_add_color_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${COLOR_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DA5_COLOR_SRC"
            OUTPUT_NAME ${nm2}
            )

        install(TARGETS ${target}
                DESTINATION "lib${LIB_SUFFIX}"
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_color_build)

    if(SHARED)
        really_add_color_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
    endif(SHARED)

    if(STATIC)
        really_add_color_build(${nm}-static ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_color_build)

if(GRADE_STANDARD)
    add_color_build(a5_color
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_color_build(a5_color-debug
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_color_build(a5_color-profile
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)

if(GRADE_STANDARD)
    set(COLOR_LIB_TO_LINK a5_color CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(COLOR_LIB_TO_LINK a5_color-debug CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(COLOR_LIB_TO_LINK a5_color-profile CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(COLOR_LINK_WITH ${COLOR_LIB_TO_LINK}_static CACHE INTERNAL "internal")
    set(COLOR_LINK_WITH ${COLOR_LIB_TO_LINK}_static PARENT_SCOPE)
    set(COLOR_LINK_WITH ${COLOR_LINK_WITH} CACHE INTERNAL "internal")
else(STATIC)
    set(COLOR_LINK_WITH ${COLOR_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
    set(COLOR_LINK_WITH ${COLOR_LIB_TO_LINK}_shared PARENT_SCOPE)
    set(COLOR_LINK_WITH ${COLOR_LINK_WITH} CACHE INTERNAL "internal")
endif(STATIC)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${COLOR_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
