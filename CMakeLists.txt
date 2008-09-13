#
# TODO:
#
# - sonames on Unix
# - most Unix drivers require testing
#
# - some documentation targets still missing
# - installing documentation
#

#-----------------------------------------------------------------------------#
#
#   Build system setup
#
#-----------------------------------------------------------------------------#

cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

option(USE_CXX "Build C++ portions." on)

# Manage (MSVC) build configurations. Ideally, debug projects would only have
# "Debug" configuration available, and release only "Release" ...
# More ideally, we would only have one project and multiple configurations for
# each "GRADE_", but that would require a revamp of this file.
# Note: This needs to be done before the project command.
if(GRADE_DEBUG)
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE INTERNAL "internal")
endif(GRADE_DEBUG)
if(GRADE_STANDARD)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE INTERNAL "internal")
endif(GRADE_STANDARD)
if(GRADE_STANDARD AND GRADE_DEBUG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE INTERNAL "internal")
endif(GRADE_STANDARD AND GRADE_DEBUG)

if(USE_CXX)
    project(ALLEGRO C CXX)
else(USE_CXX)
    project(ALLEGRO C)
endif(USE_CXX)

set(ALLEGRO_VERSION 4.9.5)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Search for C header files in these directories.
include_directories(
    include
    # For out-of-source builds.
    ${CMAKE_BINARY_DIR}/include
    addons/iio
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

option(WANT_MAGIC_MAIN "Enable magic main" off)

set(INSTALL_PREFIX "")

#
# Platforms and drivers.
#

option(WANT_X11 "X11 support" on)

option(WANT_D3D "Enable Direct3D graphics driver (Windows)" on)
option(WANT_D3D9EX "Enable Direct3D 9Ex extensions (Vista)" off)
option(WANT_OPENGL "Enable OpenGL graphics driver (Windows, X11))" on)

#
# Addons.
#

option(WANT_FONT "Enable bitmap font add-on" on)
option(WANT_ACODEC "Enable audio codecs (requires kcm_audio)" on)
option(WANT_KCM_AUDIO "Enable kcm_audio engine" on)
# WANT_ICODEC is off until it checks for Magick++
option(WANT_ICODEC "Enable image loading/saving for most filetypes (requires libMagick)" off)
option(WANT_IIO "Enable IIO image load/save addon" on)
option(WANT_TTF "Enable TTF addon" on)

option(WANT_ALSA "Enable ALSA digital audio driver (Unix)" on)
option(WANT_OSS "Enable OSS digital audio driver (Unix)" on)
option(WANT_OPENAL "Enable OpenAL digital audio driver" on)
option(WANT_DSOUND "Enable Dsound digital audio driver (Windows)" on)


#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)
option(NO_FPU "No floating point unit" off)

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
include(FindPkgConfig)

if(NO_FPU)
    set(ALLEGRO_CFG_NO_FPU 1)
endif(NO_FPU)

if(MSVC_IDE)
    set(COMPILER_MSVC 1)
endif(MSVC_IDE)

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MINGW)
    # For alplatf.h
    set(ALLEGRO_MINGW32 1)
    # Install to MINGDIR
    string(REPLACE "\\" "/" MINGDIR "$ENV{MINGDIR}")
    if(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${MINGDIR})
    else(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
    endif(INSTALL_PREFIX STREQUAL "")
endif(MINGW)

if(UNIX)
    set(ALLEGRO_UNIX 1)
endif(UNIX)

if(COMPILER_MSVC)
    set(ALLEGRO_MSVC 1)
endif(COMPILER_MSVC)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX 1)
    set(ALLEGRO_MACOSX 1)
    set(ALLEGRO_UNIX 0)
    set(WANT_X11 off)

    # This flag is required on some versions of Mac OS X to avoid linker
    # problems with global variables which are not explicitly initialised.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(BORLAND)
    set(ALLEGRO_BCC32 1)
endif(BORLAND)

# Begin tests.

include(TestBigEndian)
test_big_endian(ALLEGRO_BIG_ENDIAN)
if(NOT ALLEGRO_BIG_ENDIAN)
    set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)
check_include_files(linux/joystick.h ALLEGRO_HAVE_LINUX_JOYSTICK_H)
check_include_files(stdbool.h ALLEGRO_HAVE_STDBOOL_H)
check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)
check_include_files(sys/io.h ALLEGRO_HAVE_SYS_IO_H)
check_include_files(sys/stat.h ALLEGRO_HAVE_SYS_STAT_H)
check_include_files(sys/time.h ALLEGRO_HAVE_SYS_TIME_H)
check_include_files(sys/utsname.h ALLEGRO_HAVE_SYS_UTSNAME_H)

check_include_files(soundcard.h ALLEGRO_HAVE_SOUNDCARD_H)
check_include_files(sys/soundcard.h ALLEGRO_HAVE_SYS_SOUNDCARD_H)
check_include_files(machine/soundcard.h ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
check_include_files(linux/soundcard.h ALLEGRO_HAVE_LINUX_SOUNDCARD_H)

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
        set(ALLEGRO_CFG_OPENGL 1)
    endif(OPENGL_FOUND)
endif(WANT_OPENGL)

if(WANT_OPENAL)
    include(FindOpenAL)
    if(OPENAL_FOUND)
        set(SUPPORT_OPENAL 1)
    endif(OPENAL_FOUND)
endif(WANT_OPENAL)

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
    else(CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR
            "Unix port requires pthreads support, not detected.")
    endif(CMAKE_USE_PTHREADS_INIT)

    #
    # Sound drivers
    #

    if(WANT_ALSA)
        check_c_source_compiles("
            #include <sys/asoundlib.h>
            int main(void) {
            #if(SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9 && SND_LIB_SUBMINOR >= 1)
                return 0;
            #else
                return 1;
            #endif
            }"
            CAN_ALSA
        )
        if(CAN_ALSA)
            set(SUPPORT_ALSA 1)
        endif(CAN_ALSA)
    endif(WANT_ALSA)

    if(WANT_OSS)
        if(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_LINUX_SYS_SOUNDCARD_H)
            if(ALLEGRO_MACHINE_SOUNDCARD_H)
                set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
            endif(ALLEGRO_MACHINE_SOUNDCARD_H)
            if(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
                set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_HAVE_LINUX_SOUNDCARD_H)
            endif(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
            if(ALLEGRO_HAVE_SYS_SOUNDCARD_H)
                set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_HAVE_SYS_SOUNDCARD_H)
            endif(ALLEGRO_HAVE_SYS_SOUNDCARD_H)
            if(ALLEGRO_HAVE_SOUNDCARD_H)
                set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_HAVE_SOUNDCARD_H)
            endif(ALLEGRO_HAVE_SOUNDCARD_H)
            check_c_source_compiles("
                #ifdef ALLEGRO_HAVE_SOUNDCARD_H 
                #include <soundcard.h>
                #endif
                #ifdef ALLEGRO_HAVE_SYS_SOUNDCARD_H 
                #include <sys/soundcard.h>
                #endif
                #ifdef ALLEGRO_HAVE_LINUX_SOUNDCARD_H 
                #include <linux/soundcard.h>
                #endif
                #ifdef ALLEGRO_HAVE_MACHINE_SOUNDCARD_H 
                #include <machine/soundcard.h>
                #endif
                int main(void) {
                    audio_buf_info abi;
                    return 0;
                }"
                CAN_OSS
            )
            set(CMAKE_REQUIRED_DEFINITIONS)
            if(CAN_OSS)
                set(SUPPORT_OSS 1)
            endif(CAN_OSS)
        endif(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_LINUX_SYS_SOUNDCARD_H)
    endif(WANT_OSS)
endif(UNIX)

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
        if(GRADE_DEBUG)
            set(D3D_DEBUG_INFO 1)
        endif(GRADE_DEBUG)

        if(WANT_D3D9EX)
            set(ALLEGRO_CFG_D3D9EX 1)
        endif(WANT_D3D9EX)
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

if(COMPILER_MSVC)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_GLEXT_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES})
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED})
endif(COMPILER_MSVC)

# Libraries that we always need to link against on this platform
# e.g. -lm on Unix.
set(PLATFORM_LIBS)

if(ALLEGRO_UNIX) # not MACOSX
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_UNIX_FILES})
    list(APPEND PLATFORM_LIBS m ${CMAKE_THREAD_LIBS_INIT})
endif(ALLEGRO_UNIX)

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
    if(COMPILER_MSVC)
        list(APPEND LIBRARY_SOURCES lib/msvc/allegro.def)
    endif(COMPILER_MSVC)
    list(APPEND PLATFORM_LIBS
        kernel32 user32 gdi32 comdlg32 ole32 dinput8 dxguid winmm
        )
    if(SUPPORT_D3D)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_D3D_FILES})
        list(APPEND PLATFORM_LIBS d3d9)
    endif(SUPPORT_D3D)
endif(WIN32)

if(MACOSX)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_MACOSX_FILES})
    # XXX someone who knows something about Macs should check this.
    list(APPEND PLATFORM_LIBS "-framework AppKit")
    list(APPEND PLATFORM_LIBS "-framework IOKit")
endif(MACOSX)

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
        "-DALLEGRO_LIB_BUILD ${WFLAGS} ${extra_flags}"
        )

    # Suppress errors about _mangled_main_address being undefined
    # on Mac OS X.
    if(MACOSX)
        set(LIBRARY_LINK_FLAGS "-flat_namespace -undefined suppress")
    endif(MACOSX)

    macro(really_add_allegro_build nam target libtype more_extra_flags)
        set(OUTPUT_LIBRARY ${nam})
        add_library(${target} ${libtype} ${LIBRARY_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DALLEGRO_SRC"
            LINK_FLAGS "${LIBRARY_LINK_FLAGS}"
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
    if(BORLAND)
        add_allegro_build(alleg "-O2")
    endif(BORLAND)
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    if(COMPILER_GCC)
        add_allegro_build(allegd
            "-DDEBUGMODE -g2 -O0")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
        add_allegro_build(allegd "-DDEBUGMODE")
    endif(COMPILER_MSVC)
    if(BORLAND)
        add_allegro_build(allegd "-DDEBUGMODE=1 -v")
    endif(BORLAND)
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
#   Programs
#
#-----------------------------------------------------------------------------#

if(COMPILER_MSVC)
    set(EXECUTABLE_TYPE "WIN32")
endif(COMPILER_MSVC)

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
        set(EXE_CFLAGS "-DDEBUGMODE -g2 -O0")
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

# shared addons are always linked to a shared allegro lib
# (static addons are not linked to anything)
set(ADDONS_LINK_WITH ${LIB_TO_LINK}_shared)

macro(add_allegro_executable nm)
    # Grab libs from ${ARGN}, source file is presupposed to be last
    # This will have to change if example ever get more than 1 source file each
    # (or they'll have to create another macro)
    set(lib "")
    set(libs "")
    set(ARGS "")

    list(APPEND ARGS ${ARGN})

    list(GET ARGS -1 SRCFILE)

    foreach(count RANGE 2 ${ARGC})
        if(NOT ${count} EQUAL ${ARGC})
            list(GET ARGS 0 lib)
            list(REMOVE_AT ARGS 0)
            list(APPEND libs ${lib})
        endif(NOT ${count} EQUAL ${ARGC})
    endforeach(count RANGE 2 ${ARGC})

    add_executable(${nm} ${EXECUTABLE_TYPE} ${SRCFILE})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        )

    if(NOT "${libs}" MATCHES ".*NOTHING.*")
        target_link_libraries(${nm} ${libs})
    endif(NOT "${libs}" MATCHES ".*NOTHING.*")
    
    target_link_libraries(${nm} ${LINK_WITH})
    
    if(STATIC)
        target_link_libraries(${nm} ${PLATFORM_LIBS})
    endif(STATIC)

endmacro(add_allegro_executable)


macro(example nm)
    add_allegro_executable(${nm} ${ARGN} ${nm}.c)
endmacro(example nm libs)

macro(example_cxx nm)
    add_allegro_executable(${nm} ${ARGN} ${nm}.cpp)
endmacro(example_cxx nm libs)



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

if(NOT MSVC80) # XXX disabled because it breaks MSVC's intellisense for some reason
    add_subdirectory(demo)
endif(NOT MSVC80)

#-----------------------------------------------------------------------------#
#
# Examples
#
#-----------------------------------------------------------------------------#

add_subdirectory(examples)

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
