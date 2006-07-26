#
# TODO:
#
# - programs do not depend on library properly
#
# - sonames on Unix
# - most Unix drivers require testing
#
# - crash on Windows if try to use ALLEGRO_NO_ASM due to clash of calling
#   conventions (I think it was never supported)
# - DLLs on Windows (some "auto-imports" when linking executables)
# - MSVC support
#
# - some documentation targets still missing
# - installing documentation
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#-----------------------------------------------------------------------------#
#
#   Project
#
#-----------------------------------------------------------------------------#

project(ALLEGRO C)
include_directories( . include include/allegro)
include(FileList)

#-----------------------------------------------------------------------------#
#
#   Build options
#
#-----------------------------------------------------------------------------#

option(GRADE_STANDARD "Build the standard library" on)
option(GRADE_DEBUG "Build the debug library" off)
option(GRADE_PROFILE "Build the profiling library" off)

option(SHARED "Build shared libraries" on)
option(STATIC "Build static libraries" on)

option(WANT_ASM "Use hand-written assembly code for i386 (not recommended)" off)
option(WANT_MODULES "Use dynamically loaded modules (Unix)" on)
option(WANT_MAGIC_MAIN "Enable magic main" off)

#
# Platforms and drivers.
#

option(WANT_LINUX_CONSOLE "Linux console support" on)
option(WANT_FBCON "Enable framebuffer graphics driver (Linux)" on)
option(WANT_SVGALIB "Enable SVGAlib graphics driver (Linux)" on)
option(WANT_VGA "Enable VGA graphics driver (Linux)" on)

option(WANT_X11 "X11 support" on)
option(WANT_DGA2 "Enable DGA2 graphics driver (X11)" on)

option(WANT_ALSADIGI "Enable ALSA digital audio driver (Unix)" on)
option(WANT_ALSAMIDI "Enable ALSA MIDI driver (Unix)" on)
option(WANT_ARTS "Enable aRts audio driver (Unix)" on)
option(WANT_ESD "Enable ESD audio driver (Unix)" on)
option(WANT_JACK "Enable JACK audio driver (Unix)" on)
option(WANT_OSSDIGI "Enable OSS digital audio driver (Unix)" on)
option(WANT_OSSMIDI "Enable OSS MIDI driver (Unix)" on)
option(WANT_SGIAL "Enable SGI AL audio driver (Unix)" on)

#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)

#-----------------------------------------------------------------------------#

set(ALLEGRO_EXE_PATH "bin" CACHE INTERNAL "internal")
set(EXECUTABLE_OUTPUT_PATH ${ALLEGRO_EXE_PATH})

set(ALLEGRO_LIB_PATH "lib" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${ALLEGRO_LIB_PATH})

#-----------------------------------------------------------------------------#
#
#   Platform configuration
#
#-----------------------------------------------------------------------------#

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(UsePkgConfig)

# For alplatf.h

if(MINGW)
    set(ALLEGRO_MINGW32 1)
endif(MINGW)
if(UNIX)
    set(ALLEGRO_UNIX 1)
endif(UNIX)

# Add corresponding lines to include/allegro/platform/alunixac.h.in

include(TestBigEndian)
test_big_endian(big_endian)
if(big_endian)
    set(ALLEGRO_BIG_ENDIAN 1)
else(big_endian)
    set(ALLEGRO_LITTLE_ENDIAN 1)
endif(big_endian)

check_include_files(dirent.h HAVE_DIRENT_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(linux/awe_voice.h HAVE_LINUX_AWE_VOICE_H)
check_include_files(linux/input.h HAVE_LINUX_INPUT_H)
check_include_files(linux/joystick.h HAVE_LINUX_JOYSTICK_H)
check_include_files(linux/soundcard.h HAVE_LINUX_SOUNDCARD_H)
check_include_files(machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(ndir.h HAVE_NDIR_H)
check_include_files(soundcard.h HAVE_SOUNDCARD_H)
check_include_files(stdbool.h HAVE_STDBOOL_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(sys/dir.h HAVE_SYS_DIR_H)
check_include_files(sys/io.h HAVE_SYS_IO_H)
check_include_files(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_files(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/utsname.h HAVE_SYS_UTSNAME_H)

check_function_exists(getexecname ALLEGRO_SYS_GETEXECNAME)
set(ALLEGRO_HAVE_GETEXECNAME ${ALLEGRO_SYS_GETEXECNAME})
    # ??? both are used in the source
check_function_exists(memcmp HAVE_MEMCMP)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mprotect HAVE_MPROTECT)
check_function_exists(sched_yield ALLEGRO_USE_SCHED_YIELD)
check_function_exists(stricmp HAVE_STRICMP)
check_function_exists(strlwr HAVE_STRLWR)
check_function_exists(strupr HAVE_STRUPR)
check_function_exists(strupr HAVE_STRUPR)

check_c_source_compiles("
    #include <stdbool.h>
    int main(void) {
        _Bool x = false;
        return 0;
    }"
    HAVE__BOOL
    )

check_c_source_compiles("
    #include <sys/procfs.h>
    int main(void) {
        struct prpsinfo psinfo;
        psinfo.pr_argc = 0;
        return 0;
    }"
    ALLEGRO_HAVE_PROCFS_ARGCV
    )

check_c_source_compiles("
    #include <sys/procfs.h> 
    #include <sys/ioctl.h>
    int main(void) {
        struct prpsinfo psinfo; 
        ioctl(0, PIOCPSINFO, &psinfo);
        return 0;
    }"
    ALLEGRO_HAVE_SV_PROCFS
    )

check_c_source_compiles("
    #include <time.h>
    #include <sys/time.h>
    int main(void) {
        return 0;
    }"
    TIME_WITH_SYS_TIME
    )

include(CheckCtors)
check_ctors(ALLEGRO_USE_CONSTRUCTOR)

#-----------------------------------------------------------------------------#

if(WANT_ASM)
    # TODO check if assembly is support on this platform
    set(SUPPORT_ASM 1)
endif(WANT_ASM)

if(NOT SUPPORT_ASM)
    # TODO Consider getting rid of this command-line option and use the
    # config file.
    add_definitions(-DALLEGRO_NO_ASM)
endif(NOT SUPPORT_ASM)

if(WANT_MODULES)
    # Should ask CMake whether it can build modules on this platform.
    set(ALLEGRO_WITH_MODULES 1)
    set(SUPPORT_MODULES 1)
endif(WANT_MODULES)

if(WANT_MAGIC_MAIN)
    set(ALLEGRO_WITH_MAGIC_MAIN 1)
endif(WANT_MAGIC_MAIN)

#-----------------------------------------------------------------------------#
#
#   Driver configuration
#
#-----------------------------------------------------------------------------#

    # Accumulate extra libraries needed for the platform.
    #
set(PLATFORM_LIBS)

    # Accumulate extra libraries needed due to drivers.
    #
set(DRIVER_LIBS_STATIC)
set(DRIVER_LIBS_SHARED)

#
# Unix-specific
#

if(UNIX)

    include(FindThreads)
    if(CMAKE_USE_PTHREADS_INIT)
        set(HAVE_LIBPTHREAD 1)
    endif(CMAKE_USE_PTHREADS_INIT)
    if(HAVE_LIBPTHREAD)
        list(APPEND PLATFORM_LIBS pthread)
    endif(HAVE_LIBPTHREAD)

    # TODO softcode the version number
    if(SUPPORT_MODULES)
        set(ALLEGRO_MODULES_PATH_TAIL lib/allegro/4.3.1)
        set(ALLEGRO_MODULES_PATH
            \"${CMAKE_INSTALL_PREFIX}${ALLEGRO_MODULES_PATH_TAIL}\"
            )
    endif(SUPPORT_MODULES)

    #
    # Sound drivers
    #

    if(WANT_ALSADIGI)
        check_c_source_compiles("
            #include <sys/asoundlib.h>
            int main(void) { 
            #if(SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9 && SND_LIB_SUBMINOR >= 1)
                return 0;
            #else
                return 1;
            #endif
            }"
            SUPPORT_ALSADIGI
        )
        if(SUPPORT_ALSADIGI)
            set(ALLEGRO_WITH_ALSADIGI 1)
            set(ALLEGRO_ALSA_VERSION 9)
            list(APPEND DRIVER_LIBS_STATIC asound)
        endif(SUPPORT_ALSADIGI)
    endif(WANT_ALSADIGI)

    if(WANT_ALSAMIDI)
        check_c_source_compiles("
            #include <sys/asoundlib.h>
            int main(void) { 
            #if(SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9)
                return 0;
            #else
                return 1;
            #endif
            }"
            SUPPORT_ALSAMIDI
        )
        if(SUPPORT_ALSAMIDI)
            set(ALLEGRO_WITH_ALSAMIDI 1)
            set(ALLEGRO_ALSA_VERSION 9)
            list(APPEND DRIVER_LIBS_STATIC asound)
        endif(SUPPORT_ALSAMIDI)
    endif(WANT_ALSAMIDI)

    if(WANT_ARTS)
        # XXX artsc doesn't use pkg-config
        pkgconfig(artsc
            ARTS_INCLUDEDIR ARTS_LIBDIR
            ARTS_LINKFLAGS ARTS_CFLAGS
            )
        if(ARTS_INCLUDEDIR)
            set(SUPPORT_ARTS 1)
            set(ALLEGRO_WITH_ARTSDIGI 1)
            list(APPEND DRIVER_LIBS_STATIC ${ARTS_LINKFLAGS})
        endif(ARTS_INCLUDEDIR)
    endif(WANT_ARTS)

    if(WANT_ESD)
        # XXX esd doesn't use pkg-config
        pkgconfig(esd
            ESD_INCLUDEDIR ESD_LIBDIR
            ESD_LINKFLAGS ESD_CFLAGS
            )
        if(ESD_INCLUDEDIR)
            set(SUPPORT_ESD 1)
            set(ALLEGRO_WITH_ESDDIGI 1)
            list(APPEND DRIVER_LIBS_STATIC ${ESD_LINKFLAGS})
        endif(ESD_INCLUDEDIR)
    endif(WANT_ESD)

    if(WANT_JACK)
        pkgconfig(jack
            JACK_INCLUDEDIR JACK_LIBDIR
            JACK_LINKFLAGS JACK_CFLAGS
            )
        if(JACK_INCLUDEDIR)
            set(SUPPORT_JACK 1)
            set(ALLEGRO_WITH_JACKDIGI 1)
            list(APPEND DRIVER_LIBS_STATIC ${JACK_LINKFLAGS})
        endif(JACK_INCLUDEDIR)
    endif(WANT_JACK)

    if(HAVE_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H OR 
        HAVE_MACHINE_SOUNDCARD_H OR HAVE_LINUX_SOUNDCARD_H)
        
        if(WANT_OSSDIGI)
            set(SUPPORT_OSSDIGI 1)
            set(ALLEGRO_WITH_OSSDIGI 1)
        endif(WANT_OSSDIGI)

        if(WANT_OSSMIDI)
            set(SUPPORT_OSSMIDI 1)
            set(ALLEGRO_WITH_OSSMIDI 1)
        endif(WANT_OSSMIDI)

        check_library_exists(ossaudio _oss_ioctl "" HAVE_LIBOSSAUDIO)
        if(HAVE_LIBOSSAUDIO)
            list(APPEND DRIVER_LIBS_STATIC ossaudio)
            list(APPEND DRIVER_LIBS_SHARED ossaudio)
        endif(HAVE_LIBOSSAUDIO)

    endif(HAVE_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H OR
        HAVE_MACHINE_SOUNDCARD_H OR HAVE_LINUX_SOUNDCARD_H)

    if(WANT_SGIAL)
        check_library_exists(audio alOpenPort "" SUPPORT_SGIAL)
        if(SUPPORT_SGIAL)
            set(ALLEGRO_WITH_SGIALDIGI 1)
            list(append DRIVER_LIBS_STATIC audio)
        endif(SUPPORT_SGIAL)
    endif(WANT_SGIAL)

endif(UNIX)

#
# Linux-specific
#

# TODO check actually on Linux
if(WANT_LINUX_CONSOLE AND UNIX)
    set(SUPPORT_LINUX_CONSOLE 1)
    set(ALLEGRO_LINUX 1)
endif(WANT_LINUX_CONSOLE AND UNIX)

if(SUPPORT_LINUX_CONSOLE)

    if(WANT_FBCON)
        check_c_source_compiles("
            #include <linux/fb.h>
            int main(void) {
                int x = FB_SYNC_ON_GREEN;
                return 0;
            }"
            SUPPORT_FBCON
        )
        if(SUPPORT_FBCON)
            set(ALLEGRO_LINUX_FBCON 1)
        endif(SUPPORT_FBCON)
    endif(WANT_FBCON)

    if(WANT_SVGALIB)
        check_library_exists(vga vga_init "" SUPPORT_SVGALIB)
        if(SUPPORT_SVGALIB)
            set(ALLEGRO_LINUX_SVGALIB 1)
            list(APPEND DRIVER_LIBS_STATIC vga)
        endif(SUPPORT_SVGALIB)

        set(CMAKE_REQUIRED_LIBRARIES vga)
        check_c_source_compiles("
            #include <vga.h>
            int main(void) {
                int x = vga_version;
                return 0;
            }"
            ALLEGRO_LINUX_SVGALIB_HAVE_VGA_VERSION
        )
        set(CMAKE_REQUIRED_LIBRARIES)
    endif(WANT_SVGALIB)

    if(WANT_VGA)
        set(SUPPORT_VGA 1)
        set(ALLEGRO_LINUX_VGA 1)
    endif(WANT_VGA)

endif(SUPPORT_LINUX_CONSOLE)

#
# X Window System
#

if(WANT_X11)
    include(FindX11)
    if(X11_FOUND)
        set(SUPPORT_X11 1)
        set(ALLEGRO_WITH_XWINDOWS 1)
    endif(X11_FOUND)
endif(WANT_X11)

if(SUPPORT_X11)
    set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})

    check_library_exists(Xext XShmQueryExtension ""
        ALLEGRO_XWINDOWS_WITH_SHM)

    check_library_exists(Xcursor XcursorImageCreate ""
        ALLEGRO_XWINDOWS_WITH_XCURSOR)
    if(ALLEGRO_XWINDOWS_WITH_XCURSOR)
        list(APPEND X11_LIBRARIES "Xcursor")
    endif(ALLEGRO_XWINDOWS_WITH_XCURSOR)

    check_library_exists(Xxf86vm XF86VidModeQueryExtension ""
        ALLEGRO_XWINDOWS_WITH_XF86VIDMODE)
    if(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE)
        list(APPEND X11_LIBRARIES "Xxf86vm")
    endif(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE)

    if(WANT_DGA2)
        check_library_exists(Xxf86dga XDGAQueryExtension ""
            ALLEGRO_XWINDOWS_WITH_XF86DGA2)
        if(ALLEGRO_XWINDOWS_WITH_XF86DGA2)
            list(APPEND DRIVER_LIBS_STATIC "Xxf86dga")
        endif(ALLEGRO_XWINDOWS_WITH_XF86DGA2)
    endif(WANT_DGA2)

    check_library_exists(X11 XOpenIM "" ALLEGRO_USE_XIM)

    check_include_files(X11/xpm.h HAVE_X11_XPM_H)
    if(HAVE_X11_XPM_H)
        check_library_exists(Xpm XpmCreatePixmapFromData ""
            ALLEGRO_XWINDOWS_WITH_XPM)
        if(ALLEGRO_XWINDOWS_WITH_XPM)
            list(APPEND X11_LIBRARIES "Xpm")
        endif(ALLEGRO_XWINDOWS_WITH_XPM)
    endif(HAVE_X11_XPM_H)

    set(CMAKE_REQUIRED_LIBRARIES)
endif(SUPPORT_X11)

#
# Produce configuration files.
# All relevant variables must be set before here.
#

configure_file(
    include/allegro/platform/alplatf.h.cmake
    include/allegro/platform/alplatf.h
    )

if(UNIX)

    set(CONFIG_FILE include/allegro/platform/alunixac.h)
    configure_file(${CONFIG_FILE}.cmake ${CONFIG_FILE})
    add_definitions(-DHAVE_CONFIG_H)

    configure_file(misc/allegro-config.in allegro-config)
    install(PROGRAMS allegro-config
            DESTINATION bin
            )

endif(UNIX)

#-----------------------------------------------------------------------------#
#
#   Library
#
#-----------------------------------------------------------------------------#

    # List of source files need to compile Allegro in this configuration on
    # this platform.
    #
set(LIBRARY_SOURCES ${ALLEGRO_SRC_FILES})

    # Libraries that we always need to link against on this platform
    # e.g. -lm on Unix.
    #
set(PLATFORM_LIBS)

    # Whether using hand-written assembly code or C-only.
    #
if(SUPPORT_ASM)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_I386_FILES})
else(SUPPORT_ASM)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_C_FILES})
    if(WIN32)
        list(APPEND LIBRARY_SOURCES src/c/cmiscs.s)
    endif(WIN32)
endif(SUPPORT_ASM)

    # Assembly code and Windows port both require generated asmdef.inc.
    #
add_executable(asmdef src/i386/asmdef.c)
get_target_property(ASMDEF_EXE asmdef LOCATION)

if(MINGW)
    set(ASMDEF_INC obj/mingw32/asmdef.inc)
endif(MINGW)
if(UNIX)
    set(ASMDEF_INC obj/unix/asmdef.inc)
endif(UNIX)

add_custom_target(asmdef_inc
    DEPENDS ${ASMDEF_INC}
    )
add_custom_command(
    OUTPUT ${ASMDEF_INC}
    DEPENDS asmdef
    COMMAND ${ASMDEF_EXE} ${ASMDEF_INC}
    )

if(UNIX)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_UNIX_FILES})
    list(APPEND PLATFORM_LIBS m pthread)
endif(UNIX)

if(SUPPORT_LINUX_CONSOLE)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_LINUX_FILES})
endif(SUPPORT_LINUX_CONSOLE)

if(SUPPORT_X11)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_X_FILES})
    list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
endif(SUPPORT_X11)

if(WIN32)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_WIN_FILES})
    # TODO this still needs to be produced with misc/fixdll.sh
    if(MINGW)
        list(APPEND LIBRARY_SOURCES lib/mingw32/allegro.def)
    endif(MINGW)
    list(APPEND PLATFORM_LIBS
        kernel32 user32 gdi32 comdlg32 ole32 dinput ddraw dxguid winmm dsound
        )
endif(WIN32)

#-----------------------------------------------------------------------------#

if(STRICT_WARN)
    set(WFLAGS "-W -Wall -Wstrict-prototypes -Wno-unused-parameter -Werror")
else(STRICT_WARN)
    set(WFLAGS "-W -Wall -Wno-unused")
endif(STRICT_WARN)

macro(add_allegro_build nm extra_flags)

    set(LIBRARY_CFLAGS
        "-DALLEGRO_LIB_BUILD -DALLEGRO_SRC ${WFLAGS} ${extra_flags}"
        )

    #
    # Assembly files.
    # I can't get it to work by adding the asm source file to the library, so
    # we add the object file to the library instead.
    #
    set(ASM_OBJS)
    set(ASM_SOURCES)
    foreach(SOURCE ${LIBRARY_SOURCES})
        if(${SOURCE} MATCHES "\\.s$")
            get_filename_component(FILE_BASE ${SOURCE} NAME_WE)
            set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_OUTPUT_PATH}/${FILE_BASE}.o)
            set(_wflags ${WFLAGS})
            separate_arguments(_wflags)
            add_custom_command(
                OUTPUT ${OBJ}
                DEPENDS ${SOURCE} ${ASMDEF_INC}
                COMMAND
                    ${CMAKE_C_COMPILER} -x assembler-with-cpp
                    -I. -Iinclude ${_wflags} -c ${SOURCE} -o ${OBJ}
                )
            list(APPEND ASM_OBJS ${OBJ})
            list(APPEND ASM_SOURCES ${SOURCE})
        endif(${SOURCE} MATCHES "\\.s$")
    endforeach(SOURCE)

    macro(really_add_allegro_build nm target libtype more_extra_flags)
        add_library(${target} ${libtype} ${LIBRARY_SOURCES} ${ASM_OBJS})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm}
            )
        if(SUPPORT_ASM)
            add_dependencies(${target} asmdef_inc)
        endif(SUPPORT_ASM)

        target_link_libraries(${target} ${PLATFORM_LIBS})

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_allegro_build)

    if(SHARED)
        really_add_allegro_build(${nm} ${nm}_shared SHARED "")
    endif(SHARED)

    if(STATIC)
        really_add_allegro_build(${nm} ${nm}_static STATIC
            "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_allegro_build)

if(GRADE_STANDARD)
    add_allegro_build(alleg
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_allegro_build(alld
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_allegro_build(allp
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)

#
# Install header files.
#

install(FILES ${ALLEGRO_INCLUDE_FILES}
        DESTINATION include
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_FILES}
        DESTINATION include/allegro
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES}
        DESTINATION include/allegro/inline
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES}
        DESTINATION include/allegro/internal
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES}
        DESTINATION include/allegro/platform
        )

#-----------------------------------------------------------------------------#
#
#   Modules (Unix)
#
#-----------------------------------------------------------------------------#

if(SUPPORT_MODULES)

    macro(add_module target files)
        add_library(${target} MODULE ${files})
        set_target_properties(${target}
            PROPERTIES
            PREFIX ""
            COMPILE_FLAGS "-DALLEGRO_MODULE"
            )
        target_link_libraries(${target} ${ARGN})

        install(TARGETS ${target}
                DESTINATION ${ALLEGRO_MODULES_PATH_TAIL}
                PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(add_module)

    if(SUPPORT_FBCON)
        add_module(alleg-fbcon "${ALLEGRO_MODULE_FBCON_FILES}")
    endif(SUPPORT_FBCON)

    if(SUPPORT_SVGALIB)
        add_module(alleg-svgalib "${ALLEGRO_MODULE_SVGALIB_FILES}"
            vga)
    endif(SUPPORT_SVGALIB)

    if(SUPPORT_VGA)
        add_module(alleg-vga "${ALLEGRO_MODULE_VGA_FILES}")
    endif(SUPPORT_VGA)

    if(ALLEGRO_XWINDOWS_WITH_XF86DGA2)
        add_module(alleg-dga2 "${ALLEGRO_MODULE_DGA2_FILES}"
            -L${X11_LIBRARY_DIR} Xxf86dga)
    endif(ALLEGRO_XWINDOWS_WITH_XF86DGA2)

    if(SUPPORT_ALSADIGI)
        add_module(alleg-alsadigi "${ALLEGRO_MODULE_ALSADIGI_FILES}"
            asound)
    endif(SUPPORT_ALSADIGI)

    if(SUPPORT_ALSAMIDI)
        add_module(alleg-alsamidi "${ALLEGRO_MODULE_ALSAMIDI_FILES}"
            asound)
    endif(SUPPORT_ALSAMIDI)

    if(SUPPORT_ARTS)
        add_module(alleg-arts "${ALLEGRO_MODULE_ARTS_FILES}"
            ${ARTS_LINKFLAGS})
    endif(SUPPORT_ARTS)

    if(SUPPORT_ESD)
        add_module(alleg-esd "${ALLEGRO_MODULE_ESD_FILES}"
            ${ESD_LINKFLAGS})
    endif(SUPPORT_ESD)

    if(SUPPORT_JACK)
        add_module(alleg-jackdigi ${ALLEGRO_MODULE_JACK_FILES}
            ${JACK_LINKFLAGS})
    endif(SUPPORT_JACK)

    if(SUPPORT_SGIAL)
        add_module(alleg-sgialdigi ${ALLEGRO_MODULE_SGIAL_FILES}
            audio)
    endif(SUPPORT_SGIAL)

endif(SUPPORT_MODULES)

#-----------------------------------------------------------------------------#
#
#   Programs
#
#-----------------------------------------------------------------------------#

if(GRADE_STANDARD)
    set(LINK_WITH alleg)
endif(GRADE_STANDARD)
if(NOT LINK_WITH AND GRADE_DEBUG)
    set(LINK_WITH alld)
endif(NOT LINK_WITH AND GRADE_DEBUG)
if(NOT LINK_WITH AND GRADE_PROFILE)
    set(LINK_WITH allp)
endif(NOT LINK_WITH AND GRADE_PROFILE)

if(NOT SHARED)
    set(MAYBE_DRIVER_LIBS ${DRIVER_LIBS_STATIC})
    set(MAYBE_ALLEGRO_STATICLINK "-DALLEGRO_STATICLINK")
endif(NOT SHARED)

set(PROGRAM_LIBS
    -L${CMAKE_BINARY_DIR}/${LIBRARY_OUTPUT_PATH} ${LINK_WITH}
    ${PLATFORM_LIBS} ${MAYBE_DRIVER_LIBS}
    )

macro(add_allegro_executable nm)
    add_executable(${nm} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${PROGRAM_LIBS})
endmacro(add_allegro_executable)

add_subdirectory(demo)
add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(tools)

#-----------------------------------------------------------------------------#
#
#   Documentation
#
#-----------------------------------------------------------------------------#

add_subdirectory(docs)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
