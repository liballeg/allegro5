#
# TODO:
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

# TODO soft-code the version number
set(ALLEGRO_VERSION 4.9.2)

include_directories(
    .
    include
    include/allegro

    # For out-of-source builds.
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}/include/allegro
    ${CMAKE_BINARY_DIR}/include/allegro/platform
    )
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

option(WANT_D3D "Enable Direct3D graphics driver (Windows)" off)

#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)

#-----------------------------------------------------------------------------#

    # Put libraries into `lib' by default.  This must be absolute.
    #
set(ALLEGRO_LIB_PATH "lib" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${ALLEGRO_LIB_PATH})

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

include(SrcTests)

# For alplatf.h

if(MINGW)
    set(ALLEGRO_MINGW32 1)
endif(MINGW)
if(UNIX)
    set(ALLEGRO_UNIX 1)
endif(UNIX)
if(WANT_D3D)
    set(ALLEGRO_D3D 1)
endif(WANT_D3D)

# Add corresponding lines to include/allegro/platform/alunixac.h.in

include(TestBigEndian)
test_big_endian(ALLEGRO_BIG_ENDIAN)
if(NOT ALLEGRO_BIG_ENDIAN)
    set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

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
check_type_size("_Bool" HAVE__BOOL)

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
    # Write up another macro?

    set(SUPPORT_ASM 1)
    
    include(CheckAsm)
    check_MMX(ALLEGRO_MMX)
    check_SSE(ALLEGRO_SSE)
else(WANT_ASM)
    # Added it to alplatf.h.cmake, if this isn't wise just uncomment
    # and remove the set and the entry in alplatf.h.cmake
    #add_definitions(-DALLEGRO_NO_ASM)
    set(ALLEGRO_NO_ASM 1)
endif(WANT_ASM)

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

#
# These are the conventions for this CMakeFile.
#
# The WANT_* variables indicate whether the user wants to have an optional
# feature enabled, i.e. whether they have selected something in the CMake UI.
#
# The CAN_* variables indicate whether a feature *can* be enabled on this
# system/platform.  As these variable values are cached, CAN_ variables could
# be set even though the user has turned a corresponding WANT_* variable
# off---it might have been tested and set in a previous run.
#
# The SUPPORT_* variables are the conjunction of WANT_FEATURE and CAN_FEATURE,
# i.e. the user wants it and the system can support it.
#
# Those variables are internal to the CMake build.  Allegro header files use
# preprocessor constants with names like ALLEGRO_WITH_* and HAVE_*.  Sometimes
# we make use of those variables in this CMakeFile as well, but generally
# that's just due to sloppiness.
#


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

    if(SUPPORT_MODULES)
        set(ALLEGRO_MODULES_PATH_TAIL lib/allegro)
        set(ALLEGRO_MODULES_PATH
            \"${CMAKE_INSTALL_PREFIX}/${ALLEGRO_MODULES_PATH_TAIL}\"
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
            CAN_ALSADIGI
        )
        if(CAN_ALSADIGI)
            set(SUPPORT_ALSADIGI 1)
            set(ALLEGRO_WITH_ALSADIGI 1)
            set(ALLEGRO_ALSA_VERSION 9)
            list(APPEND DRIVER_LIBS_STATIC asound)
        endif(CAN_ALSADIGI)
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
            CAN_ALSAMIDI
        )
        if(CAN_ALSAMIDI)
            set(SUPPORT_ALSAMIDI 1)
            set(ALLEGRO_WITH_ALSAMIDI 1)
            set(ALLEGRO_ALSA_VERSION 9)
            list(APPEND DRIVER_LIBS_STATIC asound)
        endif(CAN_ALSAMIDI)
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

        # XXX we assume that if libossaudio exists then we need to use it.
        check_library_exists(ossaudio _oss_ioctl "" HAVE_LIBOSSAUDIO)
        if(HAVE_LIBOSSAUDIO)
            list(APPEND DRIVER_LIBS_STATIC ossaudio)
            list(APPEND DRIVER_LIBS_SHARED ossaudio)
        endif(HAVE_LIBOSSAUDIO)

    endif(HAVE_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H OR
        HAVE_MACHINE_SOUNDCARD_H OR HAVE_LINUX_SOUNDCARD_H)

    if(WANT_SGIAL)
        check_library_exists(audio alOpenPort "" CAN_SGIAL)
        if(CAN_SGIAL)
            set(SUPPORT_SGIAL 1)
            set(ALLEGRO_WITH_SGIALDIGI 1)
            list(append DRIVER_LIBS_STATIC audio)
        endif(CAN_SGIAL)
    endif(WANT_SGIAL)

endif(UNIX)

#
# Linux-specific
#

if(WANT_LINUX_CONSOLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SUPPORT_LINUX_CONSOLE 1)
        set(ALLEGRO_LINUX 1)
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif(WANT_LINUX_CONSOLE)

if(SUPPORT_LINUX_CONSOLE)

    if(WANT_FBCON)
        check_c_source_compiles("
            #include <linux/fb.h>
            int main(void) {
                int x = FB_SYNC_ON_GREEN;
                return 0;
            }"
            CAN_FBCON
        )
        if(CAN_FBCON)
            set(SUPPORT_FBCON 1)
            set(ALLEGRO_LINUX_FBCON 1)
        endif(CAN_FBCON)
    endif(WANT_FBCON)

    if(WANT_SVGALIB)
        check_library_exists(vga vga_init "" CAN_SVGALIB)
        if(CAN_SVGALIB)
            set(SUPPORT_SVGALIB 1)
            set(ALLEGRO_LINUX_SVGALIB 1)
            list(APPEND DRIVER_LIBS_STATIC vga)
        endif(CAN_SVGALIB)

        set(CMAKE_REQUIRED_LIBRARIES vga)
        check_c_source_compiles("
            #include <vga.h>
            int main(void) {
                int x = vga_version;
                return x;
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

    check_library_exists(Xext XShmQueryExtension "" CAN_XSHM)
    if(CAN_XSHM)
        set(ALLEGRO_XWINDOWS_WITH_SHM 1)
    endif(CAN_XSHM)

    check_library_exists(Xcursor XcursorImageCreate "" CAN_XCURSOR)
    if(CAN_XCURSOR)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        list(APPEND X11_LIBRARIES "Xcursor")
    endif(CAN_XCURSOR)

    check_library_exists(Xxf86vm XF86VidModeQueryExtension "" CAN_XF86VIDMODE)
    if(CAN_XF86VIDMODE)
        set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
        list(APPEND X11_LIBRARIES "Xxf86vm")
    endif(CAN_XF86VIDMODE)

    if(WANT_DGA2)
        check_library_exists(Xxf86dga XDGAQueryExtension "" CAN_XF86DGA)
        if(CAN_XF86DGA)
            set(SUPPORT_DGA2 1)
            set(ALLEGRO_XWINDOWS_WITH_XF86DGA2 1)
            if(NOT SUPPORT_MODULES)
                list(APPEND X11_LIBRARIES "Xxf86dga")
            endif(NOT SUPPORT_MODULES)
        endif(CAN_XF86DGA)
    endif(WANT_DGA2)

    check_library_exists(X11 XOpenIM "" CAN_XIM)
    if(CAN_XIM)
        set(ALLEGRO_USE_XIM 1)
    endif(CAN_XIM)

    check_include_files(X11/xpm.h HAVE_X11_XPM_H)
    check_library_exists(Xpm XpmCreatePixmapFromData "" CAN_XPM)
    if(HAVE_X11_XPM_H AND CAN_XPM)
        set(ALLEGRO_XWINDOWS_WITH_XPM 1)
        list(APPEND X11_LIBRARIES "Xpm")
    endif(HAVE_X11_XPM_H AND CAN_XPM)

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

if(MINGW)
    # asmcapa.h wouldn't be necessary if we only supported CMake.
    configure_file(
        cmake/asmcapa.h.cmake
        obj/mingw32/asmcapa.h
        )
endif(MINGW)

if(UNIX)
    set(CONFIG_FILE include/allegro/platform/alunixac.h)
    configure_file(${CONFIG_FILE}.cmake ${CONFIG_FILE})
    add_definitions(-DHAVE_CONFIG_H)
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
endif(SUPPORT_ASM)

    # Assembly code and Windows port both require generated asmdef.inc.
    #

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

add_executable(asmdef src/i386/asmdef.c)
set_target_properties(asmdef PROPERTIES
    OUTPUT_NAME ${ALLEGRO_LIB_PATH}/asmdef
    )
get_target_property(ASMDEF_EXE asmdef LOCATION)

if(MINGW)
    set(ASMDEF_INC ${CMAKE_BINARY_DIR}/obj/mingw32/asmdef.inc)
endif(MINGW)
if(UNIX)
    set(ASMDEF_INC ${CMAKE_BINARY_DIR}/obj/unix/asmdef.inc)
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
    if(NOT SUPPORT_MODULES)
        list(APPEND PLATFORM_LIBS "${DRIVER_LIBS_STATIC}")
    endif(NOT SUPPORT_MODULES)
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
    if(WANT_D3D)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_D3D_FILES})
        list(APPEND PLATFORM_LIBS d3d8 d3dx8)
    endif(WANT_D3D)
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
            set(OBJ ${LIBRARY_OUTPUT_PATH}/${FILE_BASE}.o)
            set(_wflags ${WFLAGS})
            separate_arguments(_wflags)
            add_custom_command(
                OUTPUT ${OBJ}
                DEPENDS ${SOURCE} ${ASMDEF_INC}
                COMMAND
                    ${CMAKE_C_COMPILER} -x assembler-with-cpp
                    -I${CMAKE_SOURCE_DIR}
                    -I${CMAKE_BINARY_DIR}
                    -I${CMAKE_BINARY_DIR}/include ${_wflags}
                    -c ${CMAKE_SOURCE_DIR}/${SOURCE}
                    -o ${OBJ}
                )
            list(APPEND ASM_OBJS ${OBJ})
            list(APPEND ASM_SOURCES ${SOURCE})
        endif(${SOURCE} MATCHES "\\.s$")
    endforeach(SOURCE)

    macro(really_add_allegro_build nam target libtype more_extra_flags)
        add_library(${target} ${libtype} ${LIBRARY_SOURCES} ${ASM_OBJS})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nam}
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
        really_add_allegro_build(${nm}-${ALLEGRO_VERSION} ${nm}_shared SHARED "")
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

# XXX liballeg_unshareable should not be needed if WANT_ASM is disabled.
# But allegro-config still expects it for now.
if(SHARED)
    add_library(alleg_unsharable STATIC ${ASM_OBJS})
    set_target_properties(alleg_unsharable
        PROPERTIES
        COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
        OUTPUT_NAME alleg_unsharable
        LINKER_LANGUAGE C
        )

    target_link_libraries(alleg_unsharable ${PLATFORM_LIBS})

    install(
        TARGETS alleg_unsharable
        DESTINATION lib
        LIBRARY PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ             GROUP_EXECUTE
            WORLD_READ             WORLD_EXECUTE
        )
endif(SHARED)

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
foreach(genfile ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED})
    install(FILES ${CMAKE_BINARY_DIR}/${genfile}
            DESTINATION include/allegro/platform
            )
endforeach(genfile)

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
                DESTINATION ${ALLEGRO_MODULES_PATH_TAIL}/${ALLEGRO_VERSION}
                PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(add_module)

    install(FILES modules.lst
            DESTINATION ${ALLEGRO_MODULES_PATH_TAIL}/${ALLEGRO_VERSION}
            )

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

    if(SUPPORT_DGA2)
        add_module(alleg-dga2 "${ALLEGRO_MODULE_DGA2_FILES}"
            -L${X11_LIBRARY_DIR} Xxf86dga)
    endif(SUPPORT_DGA2)

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

    # Select an Allegro library to link the programs with.  We choose
    # standard, debug, profile in that order, and prefer statically linked
    # over dynamically linked.  It would be worth making this configurable.
    #
if(GRADE_STANDARD)
    set(LIB_TO_LINK alleg)
endif(GRADE_STANDARD)
if(NOT LIB_TO_LINK AND GRADE_DEBUG)
    set(LIB_TO_LINK alld)
endif(NOT LIB_TO_LINK AND GRADE_DEBUG)
if(NOT LIB_TO_LINK AND GRADE_PROFILE)
    set(LIB_TO_LINK allp)
endif(NOT LIB_TO_LINK AND GRADE_PROFILE)

if(STATIC)
    set(LINK_WITH ${LIB_TO_LINK}_static)
    set(MAYBE_ALLEGRO_STATICLINK "-DALLEGRO_STATICLINK")
else(STATIC)
    set(LINK_WITH ${LIB_TO_LINK}_shared)
endif(STATIC)

macro(add_allegro_executable nm)
    add_executable(${nm} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
endmacro(add_allegro_executable)

#add_subdirectory(demo)
#add_subdirectory(examples)
#add_subdirectory(tests)
#add_subdirectory(tools)

#-----------------------------------------------------------------------------#
#
#   allegro-config script
#
#-----------------------------------------------------------------------------#

if(UNIX)

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(INCLUDE_PREFIX "${prefix}")
    # XXX these should be configurable separately
    set(bindir "${prefix}/bin")
    set(includedir "${prefix}/include")
    set(libdir "${prefix}/lib")

    if(SHARED)
        set(LINK_WITH_STATIC_LIBS no)
    else(SHARED)
        set(LINK_WITH_STATIC_LIBS yes)
    endif(SHARED)

    set(LDFLAGS)
    if(SUPPORT_MODULES)
        set(LDFLAGS "-Wl,--export-dynamic ${LDFLAGS}")
    endif(SUPPORT_MODULES)

    set(LIBS)
    foreach(lib ${PLATFORM_LIBS})
        if("${lib}" MATCHES "^-l|^/")
            set(LIBS "${LIBS}${lib} ")
        else("${lib}" MATCHES "^-l|^/")
            set(LIBS "${LIBS}-l${lib} ")
        endif("${lib}" MATCHES "^-l|^/")
    endforeach(lib)

    configure_file(misc/allegro-config.in allegro-config)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/allegro-config
            DESTINATION bin
            )
endif(UNIX)

#-----------------------------------------------------------------------------#
#
#   Documentation
#
#-----------------------------------------------------------------------------#

add_subdirectory(docs)

#-----------------------------------------------------------------------------#

    # Recreate directory structure for out-of-source builds.
    #
make_directory(obj/mingw32)
make_directory(obj/unix)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
