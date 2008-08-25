include_directories(../font)
include_directories(`pkg-config freetype2 --cflags-only-I`)
include_directories(.)

set(TTF_EXTRA_LIBS freetype z)
set(TTF_SOURCES ttf.c)

set(SUPPORT_TTF 1 PARENT_SCOPE)

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    include/allegro5/internal/aintern_ttf_cfg.h
)

set(TTF_INCLUDE_FILES
    allegro5/a5_ttf.h
    )

macro(add_ttf_build nm extra_flags)
    macro(really_add_ttf_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${TTF_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DA5_TTF_SRC"
            OUTPUT_NAME ${nm2}
            )

        target_link_libraries(${target} ${TTF_EXTRA_LIBS})

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_ttf_build)

    if(SHARED)
        really_add_ttf_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
        target_link_libraries(${nm}_shared ${FONT_LIB_TO_LINK}_shared)
        target_link_libraries(${nm}_shared ${IIO_LIB_TO_LINK}_shared)
    endif(SHARED)

    if(STATIC)
        really_add_ttf_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_ttf_build)

if(GRADE_STANDARD)
    add_ttf_build(a5_ttf
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_ttf_build(a5_ttfd
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_ttf_build(a5_ttfp
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)

if(GRADE_STANDARD)
    set(TTF_LIB_TO_LINK a5_ttf CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(TTF_LIB_TO_LINK a5_ttfd CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(TTF_LIB_TO_LINK a5_ttfp CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(TTF_LINK_WITH ${TTF_LIB_TO_LINK}_static CACHE INTERNAL "internal")
    set(TTF_LINK_WITH ${TTF_LIB_TO_LINK}_static PARENT_SCOPE)
    set(TTF_LINK_WITH ${TTF_LINK_WITH} ${FONT_LIB_TO_LINK}_static ${IIO_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(TTF_LINK_WITH ${TTF_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
    set(TTF_LINK_WITH ${TTF_LIB_TO_LINK}_shared PARENT_SCOPE)
    set(TTF_LINK_WITH ${TTF_LINK_WITH} ${FONT_LIB_TO_LINK}_shared ${IIO_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

example(ex_ttf ${TTF_LINK_WITH})

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${TTF_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
