#
# TODO:
#
# - sonames on Unix
# - most Unix drivers require testing
#
# - DLLs on Windows (some "auto-imports" when linking executables)
# - MSVC support
#
# - some documentation targets still missing
# - installing documentation
#

#-----------------------------------------------------------------------------#
#
#   Build system setup
#
#-----------------------------------------------------------------------------#

option(USE_CXX "Build C++ portions." on)

if(USE_CXX)
    project(ALLEGRO C CXX)
else(USE_CXX)
    project(ALLEGRO C)
endif(USE_CXX)

set(ALLEGRO_VERSION 4.9.4)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Search for C header files in these directories.
include_directories(
    include
    # For out-of-source builds.
    ${CMAKE_BINARY_DIR}/include
    )

# Put libraries into `lib' by default.  This path must be absolute.
set(ALLEGRO_LIB_PATH "lib" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${ALLEGRO_LIB_PATH})
mark_as_advanced(LIBRARY_OUTPUT_PATH)

# Lists of all the source files.
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

option(WANT_D3D "Enable Direct3D graphics driver (Windows)" on)
option(WANT_OPENGL "Enable OpenGL graphics driver (Windows, X11))" on)

#
# Addons.
#

option(WANT_FONT "Enable bitmap font add-on" on)
# WANT_AUDIO is off until it checks for OpenAL on the system.
option(WANT_AUDIO "Enable audio engine" off)
option(WANT_ACODEC "Enable audio codecs (requires audio)" on)
# WANT_ICODEC is off until it checks for Magick++
option(WANT_ICODEC "Enable image loading/saving for most filetypes (requires libMagick)" off)

#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)
option(WANT_OLD_EXAMPLES "Build examples from < 4.9" off)
option(WANT_OLD_TESTS "Build tests from < 4.9" off)
option(WANT_OLD_TOOLS "Build tools from < 4.9" off)

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

if(MSVC_IDE)
    set(COMPILER_MSVC 1)
endif(MSVC_IDE)

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MINGW_CROSS)
    set(WIN32 1)
endif(MINGW_CROSS)

if(MINGW)
    # For alplatf.h
    set(ALLEGRO_MINGW32 1)
    # Install to MINGDIR
    string(REPLACE "\\" "/" MINGDIR "$ENV{MINGDIR}")
    set(CMAKE_INSTALL_PREFIX ${MINGDIR})
endif(MINGW)

if(UNIX)
    set(ALLEGRO_UNIX 1)
endif(UNIX)

if(COMPILER_MSVC)
    set(ALLEGRO_MSVC 1)
endif(COMPILER_MSVC)

# Begin tests.

include(TestBigEndian)
test_big_endian(ALLEGRO_BIG_ENDIAN)
if(NOT ALLEGRO_BIG_ENDIAN)
    set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)
check_include_files(linux/awe_voice.h ALLEGRO_HAVE_LINUX_AWE_VOICE_H)
check_include_files(linux/input.h ALLEGRO_HAVE_LINUX_INPUT_H)
check_include_files(linux/joystick.h ALLEGRO_HAVE_LINUX_JOYSTICK_H)
check_include_files(linux/soundcard.h ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
check_include_files(machine/soundcard.h ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
check_include_files(soundcard.h ALLEGRO_HAVE_SOUNDCARD_H)
check_include_files(stdbool.h ALLEGRO_HAVE_STDBOOL_H)
check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)
check_include_files(sys/io.h ALLEGRO_HAVE_SYS_IO_H)
check_include_files(sys/soundcard.h ALLEGRO_HAVE_SYS_SOUNDCARD_H)
check_include_files(sys/stat.h ALLEGRO_HAVE_SYS_STAT_H)
check_include_files(sys/time.h ALLEGRO_HAVE_SYS_TIME_H)
check_include_files(sys/utsname.h ALLEGRO_HAVE_SYS_UTSNAME_H)

check_function_exists(getexecname ALLEGRO_HAVE_GETEXECNAME)
check_function_exists(mkstemp ALLEGRO_HAVE_MKSTEMP)
check_function_exists(mmap ALLEGRO_HAVE_MMAP)
check_function_exists(mprotect ALLEGRO_HAVE_MPROTECT)
check_function_exists(sched_yield ALLEGRO_HAVE_SCHED_YIELD)
check_function_exists(stricmp ALLEGRO_HAVE_STRICMP)
check_function_exists(strlwr ALLEGRO_HAVE_STRLWR)
check_function_exists(strupr ALLEGRO_HAVE_STRUPR)
check_function_exists(sysconf, ALLEGRO_HAVE_SYSCONF)

check_type_size("_Bool" ALLEGRO_HAVE__BOOL)

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
    ALLEGRO_HAVE_SV_PROCFS_H
    )

include(CheckCtors)
check_ctors(ALLEGRO_USE_CONSTRUCTOR)

#-----------------------------------------------------------------------------#

# We no longer support hand-written assembly routines.
set(ALLEGRO_NO_ASM 1)

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
# preprocessor constants with names like ALLEGRO_WITH_* and ALLEGRO_HAVE_*.
# Sometimes we make use of those variables in this CMakeFile as well, but
# generally that's just due to sloppiness.
#

if(WANT_OPENGL)
    include(FindOpenGL)
    if(OPENGL_FOUND)
        set(SUPPORT_OPENGL 1)
    endif(OPENGL_FOUND)

    if(SUPPORT_OPENGL)
       set(ALLEGRO_CFG_OPENGL 1)
    endif(SUPPORT_OPENGL)
endif(WANT_OPENGL)

# Accumulate extra libraries needed due to drivers.
set(DRIVER_LIBS_STATIC)
set(DRIVER_LIBS_SHARED)

#
# Unix-specific
#

if(UNIX)

    include(FindThreads)
    if(CMAKE_USE_PTHREADS_INIT)
        set(ALLEGRO_HAVE_LIBPTHREAD 1)
    endif(CMAKE_USE_PTHREADS_INIT)

    if(SUPPORT_MODULES)
        set(ALLEGRO_MODULES_PATH_TAIL lib/allegro)
        # This is a #define variable.
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

    if(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR 
        ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
        
        if(WANT_OSSDIGI)
            set(SUPPORT_OSSDIGI 1)
            set(ALLEGRO_WITH_OSSDIGI 1)
        endif(WANT_OSSDIGI)

        if(WANT_OSSMIDI)
            set(SUPPORT_OSSMIDI 1)
            set(ALLEGRO_WITH_OSSMIDI 1)
        endif(WANT_OSSMIDI)

        # XXX we assume that if libossaudio exists then we need to use it.
        check_library_exists(ossaudio _oss_ioctl "" ALLEGRO_HAVE_LIBOSSAUDIO)
        if(ALLEGRO_HAVE_LIBOSSAUDIO)
            list(APPEND DRIVER_LIBS_STATIC ossaudio)
            list(APPEND DRIVER_LIBS_SHARED ossaudio)
        endif(ALLEGRO_HAVE_LIBOSSAUDIO)

    endif(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR
        ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_HAVE_LINUX_SOUNDCARD_H)

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
        set(ALLEGRO_XWINDOWS_WITH_XIM 1)
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
# Windows
#

if(WIN32)
    if(WANT_D3D)
        check_include_files(d3d9.h HAVE_D3D9)
        if(HAVE_D3D9)
            set(SUPPORT_D3D 1)
        endif(HAVE_D3D9)
    endif(WANT_D3D)
    
    if(SUPPORT_D3D)
        set(ALLEGRO_CFG_D3D 1)
    endif(SUPPORT_D3D)
endif(WIN32)

#
# Produce configuration files.
# All relevant variables must be set before here.
#

configure_file(
    include/allegro5/platform/alplatf.h.cmake
    include/allegro5/platform/alplatf.h
    )

if(MINGW)
    # asmcapa.h wouldn't be necessary if we only supported CMake.
    # XXX we shouldn't need asmcapa.h any more
    configure_file(
        cmake/asmcapa.h.cmake
        obj/mingw32/asmcapa.h
        )
endif(MINGW)

if(UNIX)
    configure_file(
        include/allegro5/platform/alunixac.h.cmake
        include/allegro5/platform/alunixac.h
        )
endif(UNIX)

#-----------------------------------------------------------------------------#
#
#   Library
#
#-----------------------------------------------------------------------------#

# List of source files need to compile Allegro in this configuration on
# this platform.
set(LIBRARY_SOURCES
    ${ALLEGRO_SRC_FILES}
    ${ALLEGRO_SRC_C_FILES}
    )

# Libraries that we always need to link against on this platform
# e.g. -lm on Unix.
set(PLATFORM_LIBS)

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
    if(SUPPORT_D3D)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_D3D_FILES})
        list(APPEND PLATFORM_LIBS d3d9)
    endif(SUPPORT_D3D)
endif(WIN32)

if(SUPPORT_OPENGL)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_OPENGL_FILES})
    if(WIN32)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_WGL_FILES})
    endif(WIN32)
    list(APPEND PLATFORM_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
endif(SUPPORT_OPENGL)

#-----------------------------------------------------------------------------#

if(STRICT_WARN)
    if(COMPILER_GCC)
        set(WFLAGS "-W -Wall -Wstrict-prototypes -Wno-unused-parameter -Werror")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
        set(WFLAGS "/W4 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
    endif(COMPILER_MSVC)
else(STRICT_WARN)
    if(COMPILER_GCC)
        set(WFLAGS "-W -Wall -Wno-unused")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
        set(WFLAGS "/W3 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
    endif(COMPILER_MSVC)
endif(STRICT_WARN)

macro(add_allegro_build nm extra_flags)

    set(LIBRARY_CFLAGS
        "-DALLEGRO_LIB_BUILD -DALLEGRO_SRC ${WFLAGS} ${extra_flags}"
        )

    macro(really_add_allegro_build nam target libtype more_extra_flags)
        set(OUTPUT_LIBRARY ${nam})
        add_library(${target} ${libtype} ${LIBRARY_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nam}
            )

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
        really_add_allegro_build(${nm}_s-${ALLEGRO_VERSION} ${nm}_static STATIC
            "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_allegro_build)

if(GRADE_STANDARD)
    if(COMPILER_GCC)
        add_allegro_build(alleg
            "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
       	add_allegro_build(alleg "")
    endif(COMPILER_MSVC)
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    if(COMPILER_GCC)
        add_allegro_build(allegd
            "-DDEBUGMODE -g")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
       	add_allegro_build(allegd "-DDEBUGMODE")
    endif(COMPILER_MSVC)
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    if(COMPILER_GCC)
        add_allegro_build(allegp
            "-pg -O2 -funroll-loops -ffast-math")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
       	add_allegro_build(allegp "")
    endif(COMPILER_MSVC)
endif(GRADE_PROFILE)

#
# Install header files.
#

install(FILES ${ALLEGRO_INCLUDE_FILES}
        DESTINATION include
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_FILES}
        DESTINATION include/allegro5
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES}
        DESTINATION include/allegro5/inline
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES}
        DESTINATION include/allegro5/internal
        )
install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES}
        DESTINATION include/allegro5/platform
        )

if(SUPPORT_OPENGL)
        install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES}
                DESTINATION include/allegro5/opengl
        )
        install(FILES ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_GLEXT_FILES}
                DESTINATION include/allegro5/opengl/GLext
        )
endif(SUPPORT_OPENGL)

foreach(genfile ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED})
    install(FILES ${CMAKE_BINARY_DIR}/${genfile}
            DESTINATION include/allegro5/platform
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
            COMPILE_FLAGS "-DALLEGRO_MODULE ${WFLAGS}"
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

if(MSVC)
    set(EXECUTABLE_TYPE "WIN32")
endif(MSVC)

# Select libraries to link the programs with.  We choose
# standard, debug, profile in that order, and prefer statically linked
# over dynamically linked.  It would be worth making this configurable.

if(GRADE_STANDARD)
    set(LIB_TO_LINK alleg)
    set(FONT_LIB_TO_LINK a5_font)
    if(COMPILER_MSVC)
        set(EXE_CFLAGS "")
    elseif(COMPILER_GCC)
        set(EXE_CFLAGS "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
    endif(COMPILER_MSVC)
elseif(GRADE_DEBUG)
    set(LIB_TO_LINK allegd)
    set(FONT_LIB_TO_LINK a5_fontd)
    if(COMPILER_MSVC)
        set(EXE_CFLAGS "-DDEBUGMODE")
    elseif(COMPILER_GCC)
        set(EXE_CFLAGS "-DDEBUGMODE -g")
    endif(COMPILER_MSVC)
elseif(GRADE_PROFILE)
    set(LIB_TO_LINK allegp)
    set(FONT_LIB_TO_LINK a5_fontp)
    if(COMPILER_MSVC)
        set(EXE_CFLAGS "")
    elseif(COMPILER_GCC)
        set(EXE_CFLAGS "-pg -O2 -funroll-loops -ffast-math")
    endif(COMPILER_MSVC)
endif(GRADE_STANDARD)

if(STATIC)
    set(LINK_WITH ${LIB_TO_LINK}_static)
    set(MAYBE_ALLEGRO_STATICLINK "-DALLEGRO_STATICLINK")
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_static)
else(STATIC)
    set(LINK_WITH ${LIB_TO_LINK}_shared)
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_shared)
endif(STATIC)

macro(add_allegro_executable nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
endmacro(add_allegro_executable)

#-----------------------------------------------------------------------------#
#
# Add-ons
#
#-----------------------------------------------------------------------------#

add_subdirectory(addons)

#-----------------------------------------------------------------------------#
#
# Demo
#
#-----------------------------------------------------------------------------#

if(NOT MSVC) # XXX disabled because it breaks MSVC's intellisense for some reason
    if(USE_CXX AND WANT_FONT)
        add_subdirectory(demo)
    endif(USE_CXX AND WANT_FONT)
endif(NOT MSVC)

#-----------------------------------------------------------------------------#
#
# Examples
#
#-----------------------------------------------------------------------------#

add_subdirectory(examples)

if(WANT_OLD_TESTS)
    add_subdirectory(tests)
endif(WANT_OLD_TESTS)

if(WANT_OLD_TOOLS)
    add_subdirectory(tools)
endif(WANT_OLD_TOOLS)

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

    # @ONLY prevents substitution of ${VAR} forms.
    configure_file(
        misc/allegro5-config.in allegro5-config
        @ONLY
        )
    install(PROGRAMS ${CMAKE_BINARY_DIR}/allegro5-config
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
# XXX is this only needed because of asmcapa.h?
make_directory(obj/mingw32)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
