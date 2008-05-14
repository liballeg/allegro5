set(ICODEC_INCLUDE_FILES allegro5/icodec.h)

find_path(MAGICK_INCLUDE_DIR Magick++.h PATHS ENV INCLUDE)
find_library(MAGICK_LIBRARY NAMES Magick++ PATHS ENV LIB)
mark_as_advanced(MAGICK_INCLUDE_DIR MAGICK_LIBRARY)

if(MAGICK_INCLUDE_DIR AND MAGICK_LIBRARY)
    set(CAN_MAGICK 1)
endif(MAGICK_INCLUDE_DIR AND MAGICK_LIBRARY)

if(CAN_MAGICK)
    set(SUPPORT_MAGICK 1)
    include_directories(${ICODEC_INCLUDE_DIR})
    list(APPEND ICODEC_EXTRA_LIBS Magick++)
    list(APPEND ICODEC_SOURCES icodec.cpp)
endif(CAN_MAGICK)

macro(add_icodec_build nm extra_flags)
    macro(really_add_icodec_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${ICODEC_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DA5_ICODEC_SRC"
            OUTPUT_NAME ${nm2}
            )

        target_link_libraries(${target} ${ICODEC_EXTRA_LIBS})

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_icodec_build)

    if(SHARED)
        really_add_icodec_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
    endif(SHARED)

    if(STATIC)
        really_add_icodec_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATIC")
    endif(STATIC)

endmacro(add_icodec_build)

if(SUPPORT_MAGICK)
    if(GRADE_STANDARD)
        add_icodec_build(a5_icodec
            "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
    endif(GRADE_STANDARD)

    if(GRADE_DEBUG)
        add_icodec_build(a5_icodecd
            "-DDEBUGMODE -g")
    endif(GRADE_DEBUG)

    if(GRADE_PROFILE)
        add_icodec_build(a5_icodecp
            "-pg -O2 -funroll-loops -ffast-math")
    endif(GRADE_PROFILE)
endif(SUPPORT_MAGICK)

if(GRADE_STANDARD)
    set(ICODEC_LIB_TO_LINK a5_icodec CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(ICODEC_LIB_TO_LINK a5_icodecd CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(ICODEC_LIB_TO_LINK a5_icodecp CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(ICODEC_LINK_WITH ${ICODEC_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(ICODEC_LINK_WITH ${ICODEC_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

set(ICODEC_LINK_WITH ${ICODEC_LINK_WITH} Magick++)


macro(add_icodec_example nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${ICODEC_LINK_WITH})
endmacro(add_icodec_example)

if(SUPPORT_MAGICK)
    add_icodec_example(ex_icodec ex_icodec.c)
endif(SUPPORT_MAGICK)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

if(SUPPORT_MAGICK)
    install(FILES ${ICODEC_INCLUDE_FILES}
           DESTINATION include/allegro5
           )
endif(SUPPORT_MAGICK)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
