set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    tri_soft.c
    prim_directx.c
    prim_opengl.c
    prim_soft.c
    primitives.c
    )
set(PRIMITIVES_INCLUDE_FILES allegro5/a5_primitives.h)

macro(add_primitives_build nm extra_flags)
    macro(really_add_primitives_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${PRIMITIVES_SOURCES})

        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DALLEGRO_PRIMITIVES_SRC"
            OUTPUT_NAME ${nm2}
            )

        install(TARGETS ${target}
                DESTINATION "lib${LIB_SUFFIX}"
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_primitives_build)

    if(SHARED)
        really_add_primitives_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${IIO_ADDONS_LINK_WITH})
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
    endif(SHARED)

    if(STATIC)
        really_add_primitives_build(${nm}-static ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_primitives_build)

if(GRADE_STANDARD)
    add_primitives_build(a5_primitives
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_primitives_build(a5_primitives-debug
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_primitives_build(a5_primitives-profile
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)


if(GRADE_STANDARD)
    set(PRIMITIVES_LIB_TO_LINK a5_primitives CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(PRIMITIVES_LIB_TO_LINK a5_primitives-debug CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(PRIMITIVES_LIB_TO_LINK a5_primitives-profile CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LIB_TO_LINK}_static PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LIB_TO_LINK}_shared PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)


#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${PRIMITIVES_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
