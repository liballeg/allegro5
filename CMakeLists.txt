#
#
# TODO:
#
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

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Profile."
      FORCE)
endif()
# Restrict configuration types to the selected build type.
# Note: This needs to be done before the project command
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "internal")

# Set the project name.
# We use C++ in a few cases.
project(ALLEGRO C CXX)

set(ALLEGRO_VERSION 5.0.10)
string(REGEX MATCH "^[0-9]+[.][0-9]+" ALLEGRO_SOVERSION ${ALLEGRO_VERSION})
string(REPLACE "." "" ALLEGRO_DLL_SHORTVER ${ALLEGRO_SOVERSION})

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Search in `deps' directories for dependency files.
file(GLOB deps_subdirs
    "${CMAKE_SOURCE_DIR}/deps"
    "${CMAKE_SOURCE_DIR}/deps/*"
    "${CMAKE_BINARY_DIR}/deps"
    "${CMAKE_BINARY_DIR}/deps/*"
    )
foreach(subdir ${deps_subdirs})
    if(EXISTS "${subdir}/include" OR EXISTS "${subdir}/lib")
        message(STATUS "Adding ${subdir} to CMAKE_FIND_ROOT_PATH")
        list(APPEND CMAKE_FIND_ROOT_PATH "${subdir}")
    endif()
endforeach(subdir)

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

if(NOT IPHONE)
    option(SHARED "Build shared libraries" on)
    set(BUILD_SHARED_LIBS ${SHARED})    # actual CMake variable
endif(NOT IPHONE)

# On some 64-bit platforms, libraries should be installed into `lib64'
# instead of `lib'.  Set this to 64 to do that.
set(LIB_SUFFIX "" CACHE STRING "Suffix for 'lib' directories, e.g. '64'")

option(WANT_FRAMEWORKS "Want frameworks on Mac OS X" off)
option(WANT_EMBED
    "Make frameworks embeddable in application bundles (Mac OS X)" on)

set(FRAMEWORK_INSTALL_PREFIX "/Library/Frameworks" CACHE STRING
    "Directory in which to install Mac OS X frameworks")

#
# Platforms and drivers.
#

option(WANT_X11 "X11 support" on)
option(WANT_X11_XF86VIDMODE "X11 XF86VidMode Extension support" on)
option(WANT_X11_XINERAMA "X11 Xinerama Extension support" on)
option(WANT_X11_XRANDR "X11 XRandR Extension support" on)
option(WANT_D3D "Enable Direct3D graphics driver (Windows)" on)
option(WANT_D3D9EX "Enable Direct3D 9Ex extensions (Vista)" off)
option(WANT_OPENGL "Enable OpenGL graphics driver (Windows, X11, OS X))" on)

#
# Addons.
#

option(WANT_FONT "Enable bitmap font add-on" on)
option(WANT_AUDIO "Enable allegro_audio engine" on)
option(WANT_IMAGE "Enable image load/save addon" on)

if (NOT IPHONE)
    option(WANT_IMAGE_JPG "Enable JPEG support in image addon" on)
    option(WANT_IMAGE_PNG "Enable PNG support in image addon" on)
endif (NOT IPHONE)

option(WANT_TTF "Enable TTF addon" on)
option(WANT_COLOR "Enable color addon" on)
option(WANT_MEMFILE "Enable memfile addon" on)
option(WANT_PHYSFS "Enable PhysicsFS addon" on)
option(WANT_PRIMITIVES "Enable primitives addon" on)
option(WANT_NATIVE_DIALOG "Enable native dialog addon" on)

#
# Wrappers.
#

option(WANT_PYTHON_WRAPPER "Enable generation of the Python wrapper" off)

#
# Documentation.
#

option(WANT_DOCS "Generate documentation" on)
option(WANT_DOCS_HTML "Generate HTML documentation" on)
option(WANT_DOCS_MAN "Generate man pages" on)
option(WANT_DOCS_INFO "Generate Info document" off)
option(WANT_DOCS_PDF "Generate PDF document (requires pdflatex)" off)
option(WANT_DOCS_PDF_PAPER "Whether PDF output is destined for paper" off)

#
# For developers.
#

option(STRICT_WARN "Halt at warnings" off)
option(WANT_MUDFLAP "Enable gcc mudflap (requires gcc 4.0+)" off)

#
# Minor options.
#

if(NOT IPHONE)
    option(WANT_ALLOW_SSE "Allow compiler to use SSE instructions (x86)" on)
endif(NOT IPHONE)

option(NO_FPU "No floating point unit" off)
option(WANT_DLL_TLS "Force use of DllMain for TLS (Windows)" on)
option(WANT_DEMO "Build demo programs" on)
option(WANT_EXAMPLES "Build example programs" on)
option(WANT_POPUP_EXAMPLES "Use popups instead of printf for fatal errors" on)
option(WANT_TESTS "Build test programs" on)

#-----------------------------------------------------------------------------#
#
#   Set up compilers
#
#-----------------------------------------------------------------------------#

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(WIN32)
    if(WANT_DLL_TLS AND SHARED)
        set(ALLEGRO_CFG_DLL_TLS 1)
    endif(WANT_DLL_TLS AND SHARED)
endif(WIN32)

if(MSVC)
    # MSVC never needs DLL TLS and it just confuses
    set(ALLEGRO_CFG_DLL_TLS 0)
    set(COMPILER_MSVC 1)
    set(ALLEGRO_MSVC 1)

    # Guess VCINSTALLDIR from the value of CMAKE_C_COMPILER if it's not set.
    # XXX CMAKE_C_COMPILER will often be simply "cl" so this won't work.
    if("$ENV{VCINSTALLDIR}" STREQUAL "")
        string(REGEX REPLACE "/bin/[^/]*$" "" VCINSTALLDIR "${CMAKE_C_COMPILER}")
        message(STATUS "Guessed MSVC directory: ${VCINSTALLDIR}")
    else("$ENV{VCINSTALLDIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{VCINSTALLDIR}" VCINSTALLDIR)
        message(STATUS "Using VCINSTALLDIR: ${VCINSTALLDIR}")
    endif("$ENV{VCINSTALLDIR}" STREQUAL "")

    # Install in VCINSTALLDIR by default
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX ${VCINSTALLDIR}
            CACHE PATH "Install path prefix, prepended onto install directories."
            FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

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
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX ${MINGDIR}
            CACHE PATH "Install path prefix, prepended onto install directories."
            FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

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
            "Missing DM_POSITION. Please update your MinGW "
            "w32api package, delete CMakeCache.txt and try again.")
    endif(NOT HAVE_DM_POSITION)
endif(MINGW)

if(UNIX AND NOT APPLE)
    set(ALLEGRO_UNIX 1)
endif(UNIX AND NOT APPLE)

if(APPLE AND NOT IPHONE)
    set(MACOSX 1)
endif(APPLE AND NOT IPHONE)

if(MACOSX)
    set(ALLEGRO_MACOSX 1)
    set(ALLEGRO_CFG_PTHREADS_TLS 1)
    set(ALLEGRO_UNIX 0)
    set(WANT_X11 off)

    # This flag is required on some versions of Mac OS X to avoid linker
    # problems with global variables which are not explicitly initialised.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
endif(MACOSX)

if(IPHONE)
    set(ALLEGRO_CFG_PTHREADS_TLS 1)
    set(ALLEGRO_IPHONE 1)
    set(ALLEGRO_UNIX 0)
    set(WANT_X11 off)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics -framework QuartzCore -framework UIKit -framework Foundation -framework CoreFoundation")
endif(IPHONE)

if(BORLAND)
    set(ALLEGRO_BCC32 1)
endif(BORLAND)

# Tell the compiler it can use SSE instructions on x86 architectures.
# If compatibility with Pentium 2's and below is required then the user
# should switch WANT_ALLOW_SSE off.

# Workaround for a possible bug in CMake.  Even if we set this variable in
# the toolchain file when cross-compiling, as we should, it is empty.
if(NOT CMAKE_SYSTEM_PROCESSOR AND CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(CMAKE_SYSTEM_PROCESSOR i686)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(ARCH_X86 1)
endif()
# CMake reports "x86" on my Windows Vista machine
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    set(ARCH_X86 1)
endif()

if(ARCH_X86 AND WANT_ALLOW_SSE)
    if(COMPILER_GCC)
        message(STATUS "Allowing GCC to use SSE instructions")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse")
    endif(COMPILER_GCC)

    # Flags for other compilers should be added here.

    if(COMPILER_MSVC)
        message(STATUS "Allowing MSVC to use SSE instructions")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE")
    endif(COMPILER_MSVC)
endif()

if(GP2XWIZ)
    set(ALLEGRO_GP2XWIZ 1)
    set(CMAKE_C_COMPILER "arm-openwiz-linux-gnu-gcc")
    set(CMAKE_CXX_COMPILER "arm-openwiz-linux-gnu-g++")
    set(CMAKE_SIZEOF_UNSIGNED_SHORT "2")
    set(WANT_X11 off)
    set(ALLEGRO_CFG_PTHREADS_TLS 1)
    set(SUPPORT_OPENGL 1)
    set(ALLEGRO_CFG_OPENGL 1)
    include_directories(SYSTEM ${CMAKE_FIND_ROOT_PATH}/include)
    link_directories(${CMAKE_FIND_ROOT_PATH}/lib)
    set(CMAKE_C_FLAGS_RELEASE "-mcpu=arm926ej-s -mtune=arm926ej-s -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "-mcpu=arm926ej-s -mtune=arm926ej-s -O3")
    set(WANT_DEMO off)
endif(GP2XWIZ)

#-----------------------------------------------------------------------------#
#
#   Build types
#
#-----------------------------------------------------------------------------#

# Warnings.

if(STRICT_WARN)
    if(COMPILER_GCC)
        set(WFLAGS "-W -Wall -Werror -Wpointer-arith")
        set(WFLAGS_C_ONLY "-Wmissing-declarations -Wstrict-prototypes")
        if(ALLEGRO_UNIX)
            # Unfortunately we can't use this flag when magic main is used,
            # the mangled_main won't have a forward declaration.
            set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wmissing-prototypes")
        endif(ALLEGRO_UNIX)
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

if(WIN32 AND COMPILER_GCC)
    # Helps to ensure the Windows port remains compatible with MSVC.
    set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wdeclaration-after-statement")
endif(WIN32 AND COMPILER_GCC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS} ${WFLAGS_C_ONLY}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

if(WANT_MUDFLAP AND COMPILER_GCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmudflapth")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmudflapth")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fmudflapth -lmudflapth")
endif(WANT_MUDFLAP AND COMPILER_GCC)

# Debugging.

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUGMODE=1 -DD3D_DEBUG_INFO")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1 -DD3D_DEBUG_INFO")

# Profiling.

list(APPEND CMAKE_BUILD_CONFIGURATIONS Profile)
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

if(NOT IPHONE)
    test_big_endian(ALLEGRO_BIG_ENDIAN)
endif(NOT IPHONE)

if(NOT ALLEGRO_BIG_ENDIAN)
    set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)
# On some systems including linux/joystick.h without sys/types.h results
# in conflicting definitions of fd_set.
check_include_files("sys/types.h;linux/joystick.h" ALLEGRO_HAVE_LINUX_JOYSTICK_H)
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
check_include_files(sys/inotify.h ALLEGRO_HAVE_SYS_INOTIFY_H)
check_include_files(sys/timerfd.h ALLEGRO_HAVE_SYS_TIMERFD_H)

check_function_exists(getexecname ALLEGRO_HAVE_GETEXECNAME)
check_function_exists(mkstemp ALLEGRO_HAVE_MKSTEMP)
check_function_exists(mmap ALLEGRO_HAVE_MMAP)
check_function_exists(mprotect ALLEGRO_HAVE_MPROTECT)
check_function_exists(sched_yield ALLEGRO_HAVE_SCHED_YIELD)
check_function_exists(sysconf ALLEGRO_HAVE_SYSCONF)
# These functions exist on Wiz but don't work
if(NOT GP2XWIZ)
    check_function_exists(fseeko ALLEGRO_HAVE_FSEEKO)
    check_function_exists(ftello ALLEGRO_HAVE_FTELLO)
endif(NOT GP2XWIZ)

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

check_c_source_compiles("
    #include <stdarg.h>
    int main(void) {
        va_list a, b;
        va_copy(a, b);
        return 0;
    }"
    ALLEGRO_HAVE_VA_COPY
    )

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

    # on cmake 2.8.1 OpenGL ES is not found in the iphone case
    if(IPHONE)
        set(OPENGL_LIBRARIES "-framework OpenGLES")
        set(OPENGL_gl_LIBRARY "-framework OpenGLES")
        set(OPENGL_glu_LIBRARY "")
    endif(IPHONE)

    if(OPENGL_FOUND)
        set(SUPPORT_OPENGL 1)
        set(ALLEGRO_CFG_OPENGL 1)
    endif(OPENGL_FOUND)
endif(WANT_OPENGL)


#
# Unix-specific
#

if(UNIX) # includes MACOSX
    if(NOT GP2XWIZ AND NOT IPHONE)
        find_package(Threads)
        if(NOT CMAKE_USE_PTHREADS_INIT)
            message(FATAL_ERROR
                "Unix port requires pthreads support, not detected.")
        endif(NOT CMAKE_USE_PTHREADS_INIT)
    endif()
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

    check_library_exists(Xcursor XcursorImageCreate "" CAN_XCURSOR)
    if(CAN_XCURSOR)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        list(APPEND X11_LIBRARIES "Xcursor")
    else(CAN_XCURSOR)
        message(FATAL_ERROR "X11 support requires Xcursor library.")
    endif(CAN_XCURSOR)

    if(WANT_X11_XF86VIDMODE)
        check_include_file("X11/extensions/xf86vmode.h" HAVE_XF86VIDMODE_H)
        check_library_exists(Xxf86vm XF86VidModeQueryExtension "" CAN_XF86VIDMODE)
        if(CAN_XF86VIDMODE AND HAVE_XF86VIDMODE_H)
            set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
            list(APPEND X11_LIBRARIES "Xxf86vm")
        endif()
    endif(WANT_X11_XF86VIDMODE)

    if(WANT_X11_XINERAMA)
        check_include_file("X11/extensions/Xinerama.h" HAVE_XINERAMA_H)
        check_library_exists(Xinerama XineramaQueryExtension "" CAN_XINERAMA)
        if(CAN_XINERAMA AND HAVE_XINERAMA_H)
            set(ALLEGRO_XWINDOWS_WITH_XINERAMA 1)
            list(APPEND X11_LIBRARIES "Xinerama")
        endif()
    endif(WANT_X11_XINERAMA)

    if(WANT_X11_XRANDR)
        check_include_file("X11/extensions/Xrandr.h" HAVE_XRANDR_H)
        check_library_exists(Xrandr XRRQueryExtension "" CAN_XRANDR)
        if(CAN_XRANDR AND HAVE_XRANDR_H)
            set(ALLEGRO_XWINDOWS_WITH_XRANDR 1)
            list(APPEND X11_LIBRARIES "Xrandr")
        endif()
    endif(WANT_X11_XRANDR)

    check_library_exists(X11 XOpenIM "" CAN_XIM)
    if(CAN_XIM)
        set(ALLEGRO_XWINDOWS_WITH_XIM 1)
    endif(CAN_XIM)

    set(CMAKE_REQUIRED_LIBRARIES)
endif(SUPPORT_X11)

#
# Windows
#

if(WIN32)
    find_package(DInput)
    if(DINPUT_FOUND)
        # At least some copies of dinput.h redefine some types multiple times.
        # We must add the directory as a system directory so that the compiler
        # will suppress such errors.
        include_directories(SYSTEM ${DINPUT_INCLUDE_DIR})
    else(DINPUT_FOUND)
        message(FATAL_ERROR
            "Windows port requires DirectInput (not found).")
    endif(DINPUT_FOUND)

    if(WANT_D3D)
        find_package(D3D9)
        if(D3D9_FOUND)
            set(SUPPORT_D3D 1)
            set(ALLEGRO_CFG_D3D 1)
        endif(D3D9_FOUND)
    endif(WANT_D3D)

    if(SUPPORT_D3D)
        find_package(D3DX9)
        if(D3DX9_FOUND)
            include_directories(BEFORE SYSTEM ${D3DX9_INCLUDE_DIR})
        endif(D3DX9_FOUND)
    endif(SUPPORT_D3D)

    if(WANT_D3D9EX AND SUPPORT_D3D)
        set(ALLEGRO_CFG_D3D9EX 1)
    endif(WANT_D3D9EX AND SUPPORT_D3D)

endif(WIN32)

#-----------------------------------------------------------------------------#
#
#   Produce configuration file
#
#-----------------------------------------------------------------------------#

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
            "Both D3D and OpenGL must be present for the SHARED build. "
            "Disabling SHARED build.")
        set(SHARED off)
        set(BUILD_SHARED_LIBS off)
    endif(NOT SUPPORT_D3D OR NOT SUPPORT_OPENGL)
endif(WIN32 AND SHARED)

# List of source files need to compile Allegro in this configuration on
# this platform.
set(LIBRARY_SOURCES
    ${ALLEGRO_SRC_FILES}
    )

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
    list(APPEND PLATFORM_LIBS
        kernel32 user32 gdi32 comdlg32 ole32 winmm psapi shlwapi
        )
    if(SUPPORT_D3D)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_D3D_FILES})
        list(APPEND PLATFORM_LIBS ${D3D9_LIBRARIES})
    endif(SUPPORT_D3D)
    list(APPEND PLATFORM_LIBS ${DINPUT_LIBRARIES})
    if(MINGW AND NOT SHARED)
        list(APPEND PLATFORM_LIBS stdc++)
    endif(MINGW AND NOT SHARED)
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
    if(NOT GP2XWIZ)
        list(APPEND PLATFORM_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
    endif(NOT GP2XWIZ)
endif(SUPPORT_OPENGL)

if(GP2XWIZ)
    list(APPEND PLATFORM_LIBS wiz pthread-0.10 dl)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_GP2XWIZ_FILES})
endif(GP2XWIZ)

if(IPHONE)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_IPHONE_FILES})
endif(IPHONE)

# Header files that we need to install.
set(ALLEGRO_PUBLIC_HEADERS
    ${ALLEGRO_INCLUDE_ALLEGRO_FILES}
    ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES}
    ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES}
    ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES}
    )
if(WIN32)
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${ALLEGRO_INCLUDE_ALLEGRO_WINDOWS_FILES}
        )
endif(WIN32)
if(MACOSX)
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${ALLEGRO_INCLUDE_ALLEGRO_MACOSX_FILES}
        )
endif(MACOSX)
if(IPHONE)
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${ALLEGRO_INCLUDE_ALLEGRO_IPHONE_FILES}
        )
endif(IPHONE)
if(SUPPORT_OPENGL)
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES}
        ${ALLEGRO_INCLUDE_ALLEGRO_OPENGL_GLEXT_FILES}
        )
endif(SUPPORT_OPENGL)

foreach(genfile ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED})
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/${genfile}
        )
endforeach(genfile)

set_our_header_properties(${ALLEGRO_PUBLIC_HEADERS})

# ALLEGRO_LIB_BUILD is defined for all Allegro sources (core and addon)
# ALLEGRO_SRC is defined only while compiling the core sources (its use is
# to get the DLL #defines right under Windows for creating DLL export functions
# when it is defined or import DLL functions when it is not).
add_our_library(allegro
    "${LIBRARY_SOURCES};${ALLEGRO_PUBLIC_HEADERS}"
    "${LIBRARY_CFLAGS} -DALLEGRO_SRC"
    "${PLATFORM_LIBS}"
    )

# Allegro 4 has taken "Allegro" as the name of the framework.
set_our_framework_properties(allegro Allegro-${ALLEGRO_SOVERSION})

install_our_library(allegro)
install_our_headers(${ALLEGRO_PUBLIC_HEADERS})

# Addons and examples should link with this target.
set(ALLEGRO_LINK_WITH allegro)

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

if(NOT MSVC80 AND WANT_DEMO) # XXX disabled because it breaks MSVC's intellisense for some reason
    add_subdirectory(demos/cosmic_protector)
    add_subdirectory(demos/speed)
endif(NOT MSVC80 AND WANT_DEMO)

#-----------------------------------------------------------------------------#
#
# Examples
#
#-----------------------------------------------------------------------------#

if(WANT_EXAMPLES)
    add_subdirectory(examples)
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
#
# Tests
#
#-----------------------------------------------------------------------------#

if(WANT_TESTS)
    add_subdirectory(tests)
endif(WANT_TESTS)

#-----------------------------------------------------------------------------#
#
#   pkg-config files
#
#-----------------------------------------------------------------------------#

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(INCLUDE_PREFIX "\${prefix}")
# XXX these should be configurable separately
set(bindir "\${exec_prefix}/bin")
set(includedir "\${prefix}/include")
set(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")

set(PKG_CONFIG_FILES allegro ${ADDON_PKG_CONFIG_FILES})

# Install pkg-config files on Unix, and when cross-compiling on Unix.

if(UNIX AND NOT WANT_FRAMEWORKS AND NOT IPHONE)
    set(INSTALL_PKG_CONFIG_FILES true)
endif()
if(CMAKE_CROSSCOMPILING AND CMAKE_HOST_UNIX)
    set(INSTALL_PKG_CONFIG_FILES true)
endif()

if(INSTALL_PKG_CONFIG_FILES)
    append_lib_type_suffix(lib_type)
    append_lib_linkage_suffix(lib_linkage)

    # Our pkg-config files are now named allegro*-5.pc, which will
    # work across branches. Allegro 5.0.8 and prior used the names
    # allegro*-5.0.pc so on the 5.0 branch we will continue to install
    # those files, for backwards compatibility.
    foreach(versuffix 5 ${ALLEGRO_SOVERSION})
        foreach(name ${PKG_CONFIG_FILES})
            if (SHARED)
                set(outname ${name}${lib_type}-${versuffix}.pc)
            else (SHARED)
                # For static linking: get extra libraries to link with.
                get_target_property(link_with ${name} static_link_with)
                set(outname ${name}${lib_type}-static-${versuffix}.pc)
            endif (SHARED)
            configure_file(
                misc/${name}.pc.in
                ${LIBRARY_OUTPUT_PATH}/pkgconfig/${outname}
                @ONLY
                )
            install(FILES ${LIBRARY_OUTPUT_PATH}/pkgconfig/${outname}
                DESTINATION "lib${LIB_SUFFIX}/pkgconfig"
                )
        endforeach(name)
    endforeach(versuffix)
endif(INSTALL_PKG_CONFIG_FILES)

#-----------------------------------------------------------------------------#
#
#   Documentation
#
#-----------------------------------------------------------------------------#

if(WANT_DOCS)
    add_subdirectory(docs)
endif(WANT_DOCS)

#-----------------------------------------------------------------------------#
#
#   Wrappers
#
#-----------------------------------------------------------------------------#

if(WANT_PYTHON_WRAPPER)
    add_subdirectory(python)
endif(WANT_PYTHON_WRAPPER)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
