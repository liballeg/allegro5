#
# TODO:
#
# - sonames on Unix
# - some documentation targets still missing
# - installing documentation
#

#-----------------------------------------------------------------------------#
#
#   Build system setup
#
#-----------------------------------------------------------------------------#

cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Set the project name.
# We use C++ in a few cases.
project(ALLEGRO C CXX)

set(ALLEGRO_VERSION 4.9.8)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Search for C header files in these directories.
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    )

# Put libraries into `lib'.
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Lists of all the source files.
include(FileList)

# Our own CMake macros and functions.
include(Common)

#-----------------------------------------------------------------------------#
#
#   Build options
#
#-----------------------------------------------------------------------------#

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Profile."
      FORCE)
endif()

option(SHARED "Build shared libraries" on)
set(BUILD_SHARED_LIBS ${SHARED})    # actual CMake variable

# On some 64-bit platforms, libraries should be installed into `lib64'
# instead of `lib'.  Set this to 64 to do that.
set(LIB_SUFFIX "" CACHE STRING "Suffix for 'lib' directories, e.g. '64'")

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
# icodec is off because it's currently broken.
option(WANT_ICODEC "Enable image loading/saving for most filetypes (requires libMagick; broken)" off)
option(WANT_IIO "Enable IIO image load/save addon" on)
option(WANT_JPG "Enable JPEG support in IIO" on)
option(WANT_PNG "Enable PNG support in IIO" on)
option(WANT_TTF "Enable TTF addon" on)
option(WANT_COLOR "Enable color addon" on)

option(WANT_ALSA "Enable ALSA digital audio driver (Unix)" on)
option(WANT_OSS "Enable OSS digital audio driver (Unix)" on)
option(WANT_OPENAL "Enable OpenAL digital audio driver" on)
option(WANT_DSOUND "Enable Dsound digital audio driver (Windows)" on)

#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)

#
# Minor options.
#

option(WANT_MAGIC_MAIN "Enable magic main" off)
option(NO_FPU "No floating point unit" off)

#-----------------------------------------------------------------------------#
#
#   Set up compilers
#
#-----------------------------------------------------------------------------#

include(CheckCSourceCompiles)

set(INSTALL_PREFIX "")

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
    set(COMPILER_MSVC 1)
    set(ALLEGRO_MSVC 1)

    # Guess VCINSTALLDIR from the value of CMAKE_C_COMPILER if it's not set.
    if("$ENV{VCINSTALLDIR}" STREQUAL "")
        string(REGEX REPLACE "/bin/[^/]*$" "" VCINSTALLDIR "${CMAKE_C_COMPILER}")
        message(STATUS "Guessed MSVC directory: ${VCINSTALLDIR}")
    else("$ENV{VCINSTALLDIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{VCINSTALLDIR}" VCINSTALLDIR)
        message(STATUS "Using VCINSTALLDIR: ${VCINSTALLDIR}")
    endif("$ENV{VCINSTALLDIR}" STREQUAL "")

    # Install in VCINSTALLDIR by default
    if(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${VCINSTALLDIR})
    else(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
    endif(INSTALL_PREFIX STREQUAL "")

    set(EXECUTABLE_TYPE "WIN32")
endif(MSVC)

if(MINGW)
    # For alplatf.h
    set(ALLEGRO_MINGW32 1)

    # Guess MINGDIR from the value of CMAKE_C_COMPILER if it's not set.
    if("$ENV{MINGDIR}" STREQUAL "")
        string(REGEX REPLACE "/bin/[^/]*$" "" MINGDIR "${CMAKE_C_COMPILER}")
        message(STATUS "Guessed MinGW directory: ${MINGDIR}")
    else("$ENV{MINGDIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{MINGDIR}" MINGDIR)
        message(STATUS "Using MINGDIR: ${MINGDIR}")
    endif("$ENV{MINGDIR}" STREQUAL "")

    # Search in MINGDIR for headers and libraries.
    set(CMAKE_PREFIX_PATH "${MINGDIR}")

    # Install to MINGDIR
    if(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${MINGDIR})
    else(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
    endif(INSTALL_PREFIX STREQUAL "")

    # Check for a common problem (at the time of writing).
    check_c_source_compiles("
        #include <windows.h>
        int main(void)
        {
            int x = DM_POSITION;
            return 0;
        }"
        HAVE_DM_POSITION)
    if(NOT HAVE_DM_POSITION)
        message(FATAL_ERROR
            "Missing DM_POSITION. Please update your MinGW"
            "w32api package, delete CMakeCache.txt and try again.")
    endif(NOT HAVE_DM_POSITION)
endif(MINGW)

if(UNIX AND NOT APPLE)
    set(ALLEGRO_UNIX 1)
endif(UNIX AND NOT APPLE)

if(APPLE)
    set(MACOSX 1)
    set(ALLEGRO_MACOSX 1)
    set(ALLEGRO_UNIX 0)
    set(WANT_X11 off)

    # This flag is required on some versions of Mac OS X to avoid linker
    # problems with global variables which are not explicitly initialised.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
endif(APPLE)

if(BORLAND)
    set(ALLEGRO_BCC32 1)
endif(BORLAND)

#-----------------------------------------------------------------------------#
#
#   Build types
#
#-----------------------------------------------------------------------------#

# Warnings.

if(STRICT_WARN)
    if(COMPILER_GCC)
        set(WFLAGS "-W -Wall -Wstrict-prototypes -Werror")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
        set(WFLAGS "/W4 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
    endif(COMPILER_MSVC)
else(STRICT_WARN)
    if(COMPILER_GCC)
        set(WFLAGS "-W -Wall")
    endif(COMPILER_GCC)
    if(COMPILER_MSVC)
        set(WFLAGS "/W3 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
    endif(COMPILER_MSVC)
endif(STRICT_WARN)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

# Debugging.

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUGMODE=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1")

# Profiling.

list(APPEND CMAKE_BUILD_TYPES Profile)
if(COMPILER_GCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    mark_as_advanced(
        CMAKE_C_FLAGS_PROFILE
        CMAKE_CXX_FLAGS_PROFILE
        CMAKE_EXE_LINKER_FLAGS_PROFILE
        )
endif(COMPILER_GCC)

#-----------------------------------------------------------------------------#
#
#   Begin tests
#
#-----------------------------------------------------------------------------#

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(FindPkgConfig)

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
check_include_files(time.h ALLEGRO_HAVE_TIME_H)
check_include_files(sys/utsname.h ALLEGRO_HAVE_SYS_UTSNAME_H)
check_include_files(sys/types.h ALLEGRO_HAVE_SYS_TYPES_H)
check_include_files(soundcard.h ALLEGRO_HAVE_SOUNDCARD_H)
check_include_files(sys/soundcard.h ALLEGRO_HAVE_SYS_SOUNDCARD_H)
check_include_files(machine/soundcard.h ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
check_include_files(linux/soundcard.h ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
check_include_files(libkern/OSAtomic.h ALLEGRO_HAVE_OSATOMIC_H)

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
    find_package(OpenGL)
    if(OPENGL_FOUND)
        set(SUPPORT_OPENGL 1)
        set(ALLEGRO_CFG_OPENGL 1)
    endif(OPENGL_FOUND)
endif(WANT_OPENGL)

if(WANT_OPENAL)
    find_package(OpenAL)
    mark_as_advanced(OPENAL_INCLUDE_DIR OPENAL_LIBRARY)
    if(OPENAL_FOUND)
        set(SUPPORT_OPENAL 1)
    endif(OPENAL_FOUND)
endif(WANT_OPENAL)

#
# Unix-specific
#

if(UNIX) # includes MACOSX

    find_package(Threads)
    if(CMAKE_USE_PTHREADS_INIT)
        set(ALLEGRO_HAVE_LIBPTHREAD 1)
    else(CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR
            "Unix port requires pthreads support, not detected.")
    endif(CMAKE_USE_PTHREADS_INIT)

    if(WANT_ALSA)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            set(SUPPORT_ALSA 1)
        endif(ALSA_FOUND)
    endif(WANT_ALSA)

    if(WANT_OSS)
        include(AllegroFindOSS)
        if(OSS_FOUND)
            set(SUPPORT_OSS 1)
        endif(OSS_FOUND)
    endif(WANT_OSS)

endif(UNIX)

#
# X Window System
#

if(WANT_X11)
    find_package(X11)
    if(X11_FOUND)
        set(SUPPORT_X11 1)
        set(ALLEGRO_WITH_XWINDOWS 1)
    endif(X11_FOUND)
endif(WANT_X11)

if(ALLEGRO_UNIX AND NOT SUPPORT_X11 AND WANT_X11) # not MACOSX
    message(FATAL_ERROR
        "X11 not found. You may need to install X11 development libraries.")
endif(ALLEGRO_UNIX AND NOT SUPPORT_X11 AND WANT_X11)

if(SUPPORT_X11 AND NOT SUPPORT_OPENGL)
    message(FATAL_ERROR "X11 support currently requires OpenGL support.")
endif(SUPPORT_X11 AND NOT SUPPORT_OPENGL)

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
    else(CAN_XCURSOR)
        message(FATAL_ERROR "X11 support requires Xcursor library.")
    endif(CAN_XCURSOR)

    check_library_exists(Xxf86vm XF86VidModeQueryExtension "" CAN_XF86VIDMODE)
    if(CAN_XF86VIDMODE)
        set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
        list(APPEND X11_LIBRARIES "Xxf86vm")
    else(CAN_XF86VIDMODE)
        message(
            "WARNING: missing Xxf86vm (XF86VidMode) library. "
            "Fullscreen support will be disabled.")
    endif(CAN_XF86VIDMODE)

    check_library_exists(Xinerama XineramaQueryExtension "" CAN_XINERAMA)
    if(CAN_XINERAMA)
        set(ALLEGRO_XWINDOWS_WITH_XINERAMA 1)
        list(APPEND X11_LIBRARIES "Xinerama")
    endif(CAN_XINERAMA)

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
    find_package(DInput)
    if(DINPUT_FOUND)
        if(MSVC)
            include_directories(${DINPUT_INCLUDE_DIR})
        endif(MSVC)
    else(DINPUT_FOUND)
        message(FATAL_ERROR
            "Windows port requires DirectInput (not found).")
    endif(DINPUT_FOUND)


    if(WANT_D3D)
        find_package(D3D9)
        if(D3D9_FOUND)
            set(SUPPORT_D3D 1)
        endif(D3D9_FOUND)
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

    if(WANT_DSOUND)
        find_package(DSound)
        if(DSOUND_FOUND)
            set(SUPPORT_DSOUND 1)
        endif(DSOUND_FOUND)
    endif(WANT_DSOUND)
endif(WIN32)

#-----------------------------------------------------------------------------#
#
#   Produce configuration file
#
#-----------------------------------------------------------------------------#

# We no longer support hand-written assembly routines.
set(ALLEGRO_NO_ASM 1)

if(WANT_MAGIC_MAIN)
    set(ALLEGRO_WITH_MAGIC_MAIN 1)
endif(WANT_MAGIC_MAIN)

if(NO_FPU)
    set(ALLEGRO_CFG_NO_FPU 1)
endif(NO_FPU)

# All relevant variables must be set before here.
configure_file(
    include/allegro5/platform/alplatf.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/platform/alplatf.h
    )

#-----------------------------------------------------------------------------#
#
#   Main library
#
#-----------------------------------------------------------------------------#

# On Windows, disable shared build if either D3D or OpenGL are not present,
# because it would produce a incompatible DLL.
if(WIN32 AND SHARED)
    if(NOT SUPPORT_D3D OR NOT SUPPORT_OPENGL)
        message(
            "Both D3D and OpenGL must be present for the SHARED build.
            Disabling SHARED build.")
        set(SHARED off)
    endif(NOT SUPPORT_D3D OR NOT SUPPORT_OPENGL)
endif(WIN32 AND SHARED)

# List of source files need to compile Allegro in this configuration on
# this platform.
set(LIBRARY_SOURCES
    ${ALLEGRO_SRC_FILES}
    ${ALLEGRO_SRC_C_FILES}
    )

# For intellisense.
if(COMPILER_MSVC)
    list(APPEND LIBRARY_SOURCES
        ${ALLEGRO_INCLUDE_ALLEGRO_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_GLEXT_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED}
        )
endif(COMPILER_MSVC)

# Libraries that we always need to link against on this platform.
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
    if(MINGW AND SHARED)
        list(APPEND LIBRARY_SOURCES lib/mingw32/allegro.def)
    endif(MINGW AND SHARED)
    if(COMPILER_MSVC AND SHARED)
        list(APPEND LIBRARY_SOURCES lib/msvc/allegro.def)
    endif(COMPILER_MSVC AND SHARED)
    list(APPEND PLATFORM_LIBS
        kernel32 user32 gdi32 comdlg32 ole32 dxguid winmm psapi
        )
    if(SUPPORT_D3D)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_D3D_FILES})
        list(APPEND PLATFORM_LIBS ${D3D9_LIBRARIES})
    endif(SUPPORT_D3D)
    list(APPEND PLATFORM_LIBS ${DINPUT_LIBRARIES})
endif(WIN32)

if(MACOSX)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_MACOSX_FILES})
    find_library(APPKIT_LIBRARY AppKit)
    find_library(IOKIT_LIBRARY IOKit)
    list(APPEND PLATFORM_LIBS ${APPKIT_LIBRARY})
    list(APPEND PLATFORM_LIBS ${IOKIT_LIBRARY})
endif(MACOSX)

if(SUPPORT_OPENGL)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_OPENGL_FILES})
    if(WIN32)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_WGL_FILES})
    endif(WIN32)
    list(APPEND PLATFORM_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
endif(SUPPORT_OPENGL)

# XXX remove ALLEGRO_LIB_BUILD everywhere in favour of ALLEGRO_SRC
add_our_library(allegro -${ALLEGRO_VERSION}
    "${LIBRARY_SOURCES}"
    "${LIBRARY_CFLAGS} -DALLEGRO_LIB_BUILD -DALLEGRO_SRC"
    "${PLATFORM_LIBS}"
    )

# Addons and examples should link with this target.
set(ALLEGRO_LINK_WITH allegro)

# Install header files.

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

macro(example nm)
    add_our_executable(${nm} ${ARGN} ${ALLEGRO_LINK_WITH})
endmacro(example nm)

add_subdirectory(examples)

#-----------------------------------------------------------------------------#
#
#   allegro-config script
#
#-----------------------------------------------------------------------------#

if(UNIX)

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(INCLUDE_PREFIX "\${prefix}")
    # XXX these should be configurable separately
    set(bindir "\${exec_prefix}/bin")
    set(includedir "\${prefix}/include")
    set(libdir "\${exec_prefix}/lib")

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
# vim: set sts=4 sw=4 et:
