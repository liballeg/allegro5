#
#
# TODO:
#
# - some documentation targets still missing
# - installing documentation
#
cmake_minimum_required(VERSION 3.12...4.0)


#-----------------------------------------------------------------------------#
#
#   Version
#
#-----------------------------------------------------------------------------#

set(ALLEGRO_VERSION_REGEX_PATTERN "#define ALLEGRO_VERSION_STR[ ]+\"([0-9]+\\.[0-9]+\\.[0-9]+).*\"")
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/include/allegro5/base.h" ALLEGRO_VERSION_MATCH
    REGEX ${ALLEGRO_VERSION_REGEX_PATTERN}
    LIMIT_COUNT 1
)
string(REGEX REPLACE ${ALLEGRO_VERSION_REGEX_PATTERN} "\\1" ALLEGRO_VERSION "${ALLEGRO_VERSION_MATCH}")
if("${ALLEGRO_VERSION}" VERSION_GREATER_EQUAL 5.0.0)
    message(STATUS "[Allegro] Allegro Version: [${ALLEGRO_VERSION}]")
else()
    message(FATAL_ERROR "[Allegro] Invalid Allegro Version: [${ALLEGRO_VERSION}]")
endif()


#-----------------------------------------------------------------------------#
#
#   Project
#
#-----------------------------------------------------------------------------#

# Allow generating custom project name via command-line arguments (#1174)
set(ALLEGRO_PROJECT_NAME "ALLEGRO" CACHE STRING "Project default name")
set(ALLEGRO_PROJECT_SUFFIX "" CACHE STRING "Project suffix")

project(${ALLEGRO_PROJECT_NAME}${ALLEGRO_PROJECT_SUFFIX}
    VERSION ${ALLEGRO_VERSION}
    LANGUAGES C CXX
    DESCRIPTION "Allegro project"
    HOMEPAGE_URL "https://github.com/liballeg/allegro5"
)

include(GNUInstallDirs)

set(ALLEGRO_SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(isMultiConfig)
    if(NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Profile" CACHE STRING 
            "Semicolon separated list of supported configuration types." FORCE)
    endif()
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
            "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Profile."
            FORCE)
    endif()
endif()

if(ANDROID)
    option(WANT_ANDROID_LEGACY "Build for Android 4 (1.6)" OFF)
    set(ANDROID_TARGET "android-12" CACHE STRING "What Android target to compile for.")
endif(ANDROID)
option(ALLEGRO_SDL "Build using the SDL backend (experimental)" OFF)
option(WANT_STATIC_RUNTIME "Whether or not to link the C and C++ runtimes statically (currently only implemented on Windows)" OFF)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Reject in-source builds
include(PreventInSourceBuilds)

# Search in `deps' directories for dependency files.
file(GLOB deps_subdirs
    "${PROJECT_SOURCE_DIR}/deps"
    "${PROJECT_SOURCE_DIR}/deps/*"
    "${PROJECT_BINARY_DIR}/deps"
    "${PROJECT_BINARY_DIR}/deps/*"
    )
foreach(subdir ${deps_subdirs})
    if(EXISTS "${subdir}/include" OR EXISTS "${subdir}/lib")
        if(CMAKE_FIND_ROOT_PATH)
            message(STATUS "Adding ${subdir} to CMAKE_FIND_ROOT_PATH")
            list(APPEND CMAKE_FIND_ROOT_PATH "${subdir}")
        else()
            message(STATUS "Adding ${subdir} to CMAKE_PREFIX_PATH")
            list(APPEND CMAKE_PREFIX_PATH "${subdir}")
        endif()
    endif()
endforeach(subdir)

# Search for C header files in these directories.
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/include
    )

# Put libraries into `lib'.
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

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

if(DEFINED LIB_SUFFIX)
    message(WARNING
        "LIB_SUFFIX is no longer recognized, "
        "use the standard CMAKE_INSTALL_LIBDIR instead"
    )
endif()

option(WANT_FRAMEWORKS "Want frameworks on Mac OS X" off)
option(WANT_EMBED
    "Make frameworks embeddable in application bundles (Mac OS X)" on)

set(FRAMEWORK_INSTALL_PREFIX "/Library/Frameworks" CACHE STRING
    "Directory in which to install Mac OS X frameworks")

option(PREFER_STATIC_DEPS "Whether to prefer static dependencies." off)

#
# Platforms and drivers.
#

option(WANT_X11 "X11 support" on)
option(WANT_X11_XF86VIDMODE "X11 XF86VidMode Extension support" on)
option(WANT_X11_XINERAMA "X11 Xinerama Extension support" on)
option(WANT_X11_XRANDR "X11 XRandR Extension support" on)
option(WANT_X11_XSCREENSAVER "X11 XScreenSaver Extension support" on)
option(WANT_D3D "Enable Direct3D graphics driver (Windows)" on)
option(WANT_D3D9EX "Enable Direct3D 9Ex extensions (Vista)" off)
option(WANT_OPENGL "Enable OpenGL graphics driver (Windows, X11, OS X))" on)
option(WANT_SHADERS_GL "Build GLSL shader support (OpenGL)" on)
option(WANT_SHADERS_D3D "Build HLSL shader support (Direct3D)" on)

set(GL_BUILD_TYPE "auto" CACHE STRING "OpenGL profile to build Allegro against")
set_property(CACHE GL_BUILD_TYPE PROPERTY STRINGS "auto;gl;gles1;gles2+")
set(GL_AUTO_BUILD_TYPE "gl")
option(WANT_GLES3 "Use GLES3-only features in a GLES2+ build" off)

#
# Addons.
#

option(WANT_FONT "Enable bitmap font add-on" on)
option(WANT_AUDIO "Enable allegro_audio engine" on)
option(WANT_IMAGE "Enable image load/save addon" on)

if (NOT IPHONE)
    option(WANT_IMAGE_JPG "Enable JPEG support in image addon" on)
    option(WANT_IMAGE_PNG "Enable PNG support in image addon" on)
    option(WANT_IMAGE_FREEIMAGE "Enable FreeImage support in image addon" on)
endif (NOT IPHONE)
option(WANT_IMAGE_WEBP "Enable WebP support in image addon" on)

option(WANT_TTF "Enable TTF addon" on)
option(WANT_COLOR "Enable color addon" on)
option(WANT_MEMFILE "Enable memfile addon" on)
option(WANT_PHYSFS "Enable PhysicsFS addon" on)
option(WANT_PRIMITIVES "Enable primitives addon" on)
option(WANT_NATIVE_DIALOG "Enable native dialog addon" on)
option(WANT_VIDEO "Enable video player addon" on)

option(WANT_MONOLITH "Include all addons in the main library" off)

#
# Wrappers.
#

option(WANT_PYTHON_WRAPPER "Enable generation of the Python wrapper" off)
option(WANT_LUAJIT "Enable generation of the Luajit wrapper" off)

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
option(WANT_RELEASE_LOGGING "Enable logging even in release mode" on)

if(WANT_RELEASE_LOGGING)
    set(ALLEGRO_CFG_RELEASE_LOGGING 1)
endif()

#
# Minor options.
#

if(NOT IPHONE)
    set(WANT_ALLOW_SSE off CACHE STRING "Allow compiler to use SSE instructions (x86)")
    set_property(CACHE WANT_ALLOW_SSE PROPERTY STRINGS off sse sse2 avx avx2)
endif(NOT IPHONE)

option(NO_FPU "No floating point unit" off)
option(WANT_DLL_TLS "Force use of DllMain for TLS (Windows)" off)
option(WANT_DEMO "Build demo programs" on)
option(WANT_EXAMPLES "Build example programs" on)
option(WANT_POPUP_EXAMPLES "Use popups instead of printf for fatal errors" on)
option(WANT_TESTS "Build test programs" on)

option(WANT_WAIT_EVENT_SLEEP "Use sleep instead of threads in al_wait_for_event (only useful for emscripten without web workers)" off)

if(WANT_WAIT_EVENT_SLEEP)
    set(ALLEGRO_WAIT_EVENT_SLEEP 1)
endif()

#-----------------------------------------------------------------------------#
#
#   Set up compilers
#
#-----------------------------------------------------------------------------#

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
    set(COMPILER_GCC_OR_CLANG 1)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMPILER_GCC_OR_CLANG 1)
endif()

if(WIN32)
    if(WANT_DLL_TLS AND SHARED)
        set(ALLEGRO_CFG_DLL_TLS 1)
    endif(WANT_DLL_TLS AND SHARED)
endif(WIN32)

if(MSVC)
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
    if(CMAKE_FIND_ROOT_PATH)
        list(APPEND CMAKE_FIND_ROOT_PATH "${MINGDIR}")
    else()
        list(APPEND CMAKE_PREFIX_PATH "${MINGDIR}")
    endif()

    # Install to MINGDIR
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX ${MINGDIR}
            CACHE PATH "Install path prefix, prepended onto install directories."
            FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

    # Check for a common problem (at the time of writing).
    run_c_compile_test("
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

if(UNIX AND NOT APPLE AND NOT ANDROID)
    set(ALLEGRO_UNIX 1)
endif(UNIX AND NOT APPLE AND NOT ANDROID)

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
    set(ALLEGRO_LITTLE_ENDIAN 1)
    set(WANT_X11 off)
    set(GL_AUTO_BUILD_TYPE "gles2+")
    set(WANT_GLES3 yes)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics -framework QuartzCore -framework UIKit -framework Foundation -framework CoreFoundation -framework MobileCoreServices")
endif(IPHONE)

if(BORLAND)
    set(ALLEGRO_BCC32 1)
endif(BORLAND)

if(ANDROID)
   set(ALLEGRO_ANDROID 1)
   set(ALLEGRO_CFG_PTHREADS_TLS 1)
   set(WANT_X11 off)
   set(GL_AUTO_BUILD_TYPE "gles2+")
endif(ANDROID)

if(ALLEGRO_RASPBERRYPI)
   set(ALLEGRO_CFG_PTHREADS_TLS 1)
   set(GL_AUTO_BUILD_TYPE "gles2+")
   set(ALLEGRO_EXCLUDE_GLX 1)
endif(ALLEGRO_RASPBERRYPI)

if(EMSCRIPTEN)
   set(GL_AUTO_BUILD_TYPE "gles2+")
   set(ALLEGRO_LITTLE_ENDIAN 1)
   if (NOT ALLEGRO_SDL)
      message(FATAL_ERROR
         "Emscripten support currently requires the SDL backend (set ALLEGRO_SDL)")
   endif(NOT ALLEGRO_SDL)
endif()

if(ALLEGRO_SDL)
   set(ALLEGRO_UNIX 0)
   set(ALLEGRO_MACOSX 0)
   set(WANT_X11 off)
   include(FindSDL2)
endif(ALLEGRO_SDL)

# Tell the compiler it can use SSE instructions on x86 architectures.
# If compatibility with Pentium 2's and below is required then the user
# should switch WANT_ALLOW_SSE off.

# Workaround for a possible bug in CMake.  Even if we set this variable in
# the toolchain file when cross-compiling, as we should, it is empty.
if(NOT CMAKE_SYSTEM_PROCESSOR AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_SYSTEM_PROCESSOR i686)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(ARCH_X86 1)
endif()
# CMake reports "x86" on my Windows Vista machine
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|AMD64")
    set(ARCH_X86 1)
endif()

if(ARCH_X86 AND WANT_ALLOW_SSE)
    string(TOUPPER ${WANT_ALLOW_SSE} arch_uc)
    message(STATUS "Allowing compiler to use ${arch_uc} instructions")
    add_compile_options(
        $<$<BOOL:${COMPILER_GCC_OR_CLANG}>:-m${WANT_ALLOW_SSE}>
        $<$<BOOL:${COMPILER_MSVC}>:/arch:${arch_uc}>
        # Flags for other compilers should be added here.
        )
endif()

#-----------------------------------------------------------------------------#
#
#   Build types
#
#-----------------------------------------------------------------------------#

# Warnings.

if(COMPILER_GCC_OR_CLANG)
    set(WFLAGS "-W -Wall -Wpointer-arith")
    set(WFLAGS_C_ONLY "-Wmissing-declarations -Wstrict-prototypes")
    if(ALLEGRO_UNIX)
        # Unfortunately we can't use this flag when magic main is used,
        # the mangled_main won't have a forward declaration.
        set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wmissing-prototypes")
    endif(ALLEGRO_UNIX)
    if(STRICT_WARN)
        set(WFLAGS "${WFLAGS} -Werror")
    endif()
endif()
if(COMPILER_MSVC)
    set(WFLAGS "/W3 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        # Clang is more strict than MSVC's compiler here.
        set(WFLAGS "${WFLAGS} -Wno-c++11-narrowing")
    endif()
endif(COMPILER_MSVC)

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
if(COMPILER_GCC_OR_CLANG)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    mark_as_advanced(
        CMAKE_C_FLAGS_PROFILE
        CMAKE_CXX_FLAGS_PROFILE
        CMAKE_EXE_LINKER_FLAGS_PROFILE
        CMAKE_SHARED_LINKER_FLAGS_PROFILE
        )
endif(COMPILER_GCC_OR_CLANG)

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

if(NOT ALLEGRO_BIG_ENDIAN AND NOT ALLEGRO_LITTLE_ENDIAN)
    test_big_endian(ALLEGRO_BIG_ENDIAN)
    if(NOT ALLEGRO_BIG_ENDIAN)
        set(ALLEGRO_LITTLE_ENDIAN 1)
    endif(NOT ALLEGRO_BIG_ENDIAN)
endif(NOT ALLEGRO_BIG_ENDIAN AND NOT ALLEGRO_LITTLE_ENDIAN)

check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)
check_include_files(linux/input.h ALLEGRO_HAVE_LINUX_INPUT_H)
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
check_include_files(sal.h ALLEGRO_HAVE_SAL_H)

check_function_exists(getexecname ALLEGRO_HAVE_GETEXECNAME)
check_function_exists(mkstemp ALLEGRO_HAVE_MKSTEMP)
check_function_exists(mmap ALLEGRO_HAVE_MMAP)
check_function_exists(mprotect ALLEGRO_HAVE_MPROTECT)
check_function_exists(sched_yield ALLEGRO_HAVE_SCHED_YIELD)
check_function_exists(sysconf ALLEGRO_HAVE_SYSCONF)
check_function_exists(fseeko ALLEGRO_HAVE_FSEEKO)
check_function_exists(ftello ALLEGRO_HAVE_FTELLO)
check_function_exists(strerror_r ALLEGRO_HAVE_STRERROR_R)
check_function_exists(strerror_s ALLEGRO_HAVE_STRERROR_S)
if(WIN32)
    check_function_exists(_ftelli64 ALLEGRO_HAVE_FTELLI64)
    check_function_exists(_fseeki64 ALLEGRO_HAVE_FSEEKI64)
endif()

check_type_size("_Bool" ALLEGRO_HAVE__BOOL)

run_c_compile_test("
    #include <sys/procfs.h>
    int main(void) {
        struct prpsinfo psinfo;
        psinfo.pr_argc = 0;
        return 0;
    }"
    ALLEGRO_HAVE_PROCFS_ARGCV
    )

run_c_compile_test("
    #include <sys/procfs.h>
    #include <sys/ioctl.h>
    int main(void) {
        struct prpsinfo psinfo;
        ioctl(0, PIOCPSINFO, &psinfo);
        return 0;
    }"
    ALLEGRO_HAVE_SV_PROCFS_H
    )

run_c_compile_test("
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


set(SPACER "===========================")
set(CONFIGURATION_SUMMARY "Configuration summary\n${SPACER}\n")

if(PREFER_STATIC_DEPS)
   set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}"
                                   "${CMAKE_SHARED_LIBRARY_SUFFIX}"
                                   "${CMAKE_SHARED_MODULE_SUFFIX}")
endif()

set(OPENGL_SUMMARY "- OpenGL: NO")
if(WANT_OPENGL)
    find_package(OpenGL)

    if(GL_BUILD_TYPE MATCHES "auto")
        set(GL_REAL_BUILD_TYPE ${GL_AUTO_BUILD_TYPE})
    else()
        set(GL_REAL_BUILD_TYPE ${GL_BUILD_TYPE})
    endif()

    if(GL_REAL_BUILD_TYPE MATCHES "gl$")
        if(OPENGL_FOUND AND OPENGL_GLU_FOUND)
            set(SUPPORT_OPENGL 1)
            set(ALLEGRO_CFG_OPENGL 1)
            set(ALLEGRO_CFG_OPENGL_FIXED_FUNCTION 1)
            # We assume that the OpenGL library is not very old.
            set(ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE 1)
            if(OPENGL_opengl_LIBRARY) # prefer GLVND if available
                set(OPENGL_LIBRARIES ${OPENGL_opengl_LIBRARY} ${OPENGL_glu_LIBRARY})
            endif()
            include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
            set(OPENGL_SUMMARY "- OpenGL: Desktop")
        else(OPENGL_FOUND)
            if(GL_BUILD_TYPE MATCHES "auto")
                set(GL_REAL_BUILD_TYPE "gles2+")
            endif()
        endif()
    endif()

    if(GL_REAL_BUILD_TYPE MATCHES "gles2")

        # on cmake 2.8.1 OpenGL ES is not found in the iphone case
        # XXX incorporate IPHONE case into FindOpenGLES/2
        if(IPHONE)
            set(OPENGLES2_FOUND 1)
            set(OPENGLES2_LIBRARIES "-framework OpenGLES")
        elseif(EMSCRIPTEN)
            set(OPENGLES2_FOUND 1)
        else()
            find_package(OpenGLES2)
        endif()

        if(OPENGLES2_FOUND)
            set(SUPPORT_OPENGL 1)
            set(SUPPORT_OPENGLES 1)
            set(SUPPORT_OPENGLES2 1)
            set(ALLEGRO_CFG_OPENGL 1)
            set(ALLEGRO_CFG_OPENGLES 1)
            set(ALLEGRO_CFG_OPENGLES2 1)
            set(ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE 1)
            set(OPENGL_LIBRARIES ${OPENGLES2_LIBRARIES})
            set(OPENGLES_INCLUDE_DIR ${OPENGLES2_INCLUDE_DIR})
            set(OPENGL_SUMMARY "- OpenGL: ES 2.0")

            if(WANT_GLES3 AND NOT ANDROID)
                set(SUPPORT_OPENGLES3 1)
                set(ALLEGRO_CFG_OPENGLES3 1)
                set(OPENGL_SUMMARY "- OpenGL: ES 3.0")
            elseif(WANT_GLES3 AND ANDROID)
                find_package(OpenGLES3)
                if(OPENGLES3_FOUND)
                    set(SUPPORT_OPENGLES3 1)
                    set(ALLEGRO_CFG_OPENGLES3 1)
                    set(OPENGL_LIBRARIES ${OPENGLES3_LIBRARIES})
                    list(APPEND OPENGLES_INCLUDE_DIR ${OPENGLES3_INCLUDE_DIR})
                    set(OPENGL_SUMMARY "- OpenGL: ES 3.0")
                endif(OPENGLES3_FOUND)
            endif()

            if(NOT IPHONE)
                include_directories(SYSTEM ${OPENGLES_INCLUDE_DIR})
            endif()
        else(OPENGLES2_FOUND)
            if(GL_BUILD_TYPE MATCHES "auto")
                set(GL_REAL_BUILD_TYPE "gles1")
            endif()
        endif(OPENGLES2_FOUND)
    endif()

    if(GL_REAL_BUILD_TYPE MATCHES "gles1")

        # on cmake 2.8.1 OpenGL ES is not found in the iphone case
        # XXX incorporate IPHONE case into FindOpenGLES/2
        if(IPHONE)
            set(OPENGLES1_FOUND 1)
            set(OPENGLES1_LIBRARIES "-framework OpenGLES")
        else()
            find_package(OpenGLES1)
        endif()

        if(OPENGLES1_FOUND)
            set(SUPPORT_OPENGL 1)
            set(SUPPORT_OPENGLES 1)
            set(SUPPORT_OPENGLES1 1)
            set(ALLEGRO_CFG_OPENGL 1)
            set(ALLEGRO_CFG_OPENGLES 1)
            set(ALLEGRO_CFG_OPENGLES1 1)
            set(ALLEGRO_CFG_OPENGL_FIXED_FUNCTION 1)
            set(OPENGL_SUMMARY "- OpenGL: ES 1.1")
            set(OPENGL_LIBRARIES ${OPENGLES1_LIBRARIES})
            if(NOT IPHONE)
                include_directories(SYSTEM ${OPENGLES1_INCLUDE_DIR})
            endif()
        endif(OPENGLES1_FOUND)

    endif()

endif(WANT_OPENGL)

set(CONFIGURATION_SUMMARY "${CONFIGURATION_SUMMARY}${OPENGL_SUMMARY}\n")

#
# Unix-specific
#

if(UNIX) # includes MACOSX
    if(NOT IPHONE AND NOT ANDROID)
        find_package(Threads)
        if(NOT CMAKE_USE_PTHREADS_INIT)
            message(FATAL_ERROR
                "Unix port requires pthreads support, not detected.")
        endif(NOT CMAKE_USE_PTHREADS_INIT)
    endif()
endif(UNIX)

#
# SDL
#

if(ALLEGRO_SDL AND NOT SUPPORT_OPENGL)
    message(FATAL_ERROR "SDL support currently requires OpenGL or OpenGL ES support.")
endif(ALLEGRO_SDL AND NOT SUPPORT_OPENGL)

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
    message(FATAL_ERROR "X11 support currently requires OpenGL or OpenGL ES support.")
endif(SUPPORT_X11 AND NOT SUPPORT_OPENGL)

if(SUPPORT_X11)
    set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})

    if (OpenGL_GLX_FOUND)
        set(OPENGL_LIBRARIES "${OPENGL_LIBRARIES}" "${OPENGL_glx_LIBRARY}")
    endif()

    if (NOT OPENGL_LIBRARIES MATCHES "(libGL\\.|libGLX\\.)" AND NOT ALLEGRO_EXCLUDE_GLX)
       message(FATAL_ERROR "X11 support currently requires GLX (set OPENGL_glx_LIBRARY and OpenGL_GLX_FOUND).")
    endif()

    check_library_exists(Xcursor XcursorImageCreate "" CAN_XCURSOR)
    if(CAN_XCURSOR)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        find_library(XCURSOR_LIB "Xcursor")
        list(APPEND X11_LIBRARIES "${XCURSOR_LIB}")
    else(CAN_XCURSOR)
        message(FATAL_ERROR "X11 support requires Xcursor library.")
    endif(CAN_XCURSOR)

    check_include_file("X11/extensions/XInput2.h" CAN_XINPUT2)
    run_c_compile_test("
        #include <X11/extensions/XInput2.h>
        int main(void) { (void)XI_TouchBegin; return 0; }"
        XINPUT2_COMPILES
    )
    if(CAN_XINPUT2 AND XINPUT2_COMPILES)
        list(APPEND X11_LIBRARIES "Xi")
        set(ALLEGRO_XWINDOWS_WITH_XINPUT2 1)
    else()
        message("XInput2 library version 2.2 or higher not found, disabling touch support.")
    endif()

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
            find_library(XINERAMA_LIB "Xinerama")
            list(APPEND X11_LIBRARIES "${XINERAMA_LIB}")
        endif()
    endif(WANT_X11_XINERAMA)

    if(WANT_X11_XRANDR)
        check_include_file("X11/extensions/Xrandr.h" HAVE_XRANDR_H)
        check_library_exists(Xrandr XRRQueryExtension "" CAN_XRANDR)
        if(CAN_XRANDR AND HAVE_XRANDR_H)
            set(ALLEGRO_XWINDOWS_WITH_XRANDR 1)
            find_library(XRANDR_LIB "Xrandr")
            list(APPEND X11_LIBRARIES "${XRANDR_LIB}")
        endif()
    endif(WANT_X11_XRANDR)

    if(WANT_X11_XSCREENSAVER)
        check_include_file("X11/extensions/scrnsaver.h" HAVE_XSCREENSAVER_H)
        check_library_exists(Xss XScreenSaverQueryExtension "" CAN_XSCREENSAVER)
        if(CAN_XSCREENSAVER AND HAVE_XSCREENSAVER_H)
            set(ALLEGRO_XWINDOWS_WITH_XSCREENSAVER 1)
            find_library(XSCREENSAVER_LIB "Xss")
            list(APPEND X11_LIBRARIES "${XSCREENSAVER_LIB}")
        endif()
    endif(WANT_X11_XSCREENSAVER)

    if(NOT ALLEGRO_RASPBERRYPI)
        check_library_exists(X11 XOpenIM "" CAN_XIM)
        if(CAN_XIM)
            set(ALLEGRO_XWINDOWS_WITH_XIM 1)
        endif(CAN_XIM)
    endif(NOT ALLEGRO_RASPBERRYPI)

    set(CMAKE_REQUIRED_LIBRARIES)
endif(SUPPORT_X11)

#
# Windows
#

if(WIN32)
    find_package(DirectX)

    if(DINPUT_FOUND)
        # At least some copies of dinput.h redefine some types multiple times.
        # We must add the directory as a system directory so that the compiler
        # will suppress such errors.
        include_directories(${DINPUT_INCLUDE_DIR})
    else(DINPUT_FOUND)
        message(FATAL_ERROR
            "Windows port requires DirectInput (not found).")
    endif(DINPUT_FOUND)

    # Xinput support for xbox compatible controllers.
    if(XINPUT_FOUND)
        run_c_compile_test("
            #include <xinput.h>
            XINPUT_CAPABILITIES_EX x;
            int main(void) { return 0; }"
            HAVE_XINPUT_CAPABILITIES_EX
        )
        if(HAVE_XINPUT_CAPABILITIES_EX)
            set(ALLEGRO_CFG_HAVE_XINPUT_CAPABILITIES_EX 1)
        endif()
        set(SUPPORT_XINPUT 1)
        set(ALLEGRO_CFG_XINPUT 1)
    endif(XINPUT_FOUND)

    if(WANT_D3D)
        if(D3D9_FOUND)
            set(SUPPORT_D3D 1)
            set(ALLEGRO_CFG_D3D 1)
        endif()
    endif()

    if(SUPPORT_D3D)
        if(D3DX9_FOUND)
            include_directories(BEFORE ${D3DX9_INCLUDE_DIR})
            set(ALLEGRO_CFG_D3DX9 1)
        else()
            message("D3DX9 not found, necessary for custom shaders and "
                    "compressed textures.")
        endif()
    endif()

    if(WANT_D3D9EX AND SUPPORT_D3D)
        set(ALLEGRO_CFG_D3D9EX 1)
    endif(WANT_D3D9EX AND SUPPORT_D3D)

endif(WIN32)

#-----------------------------------------------------------------------------#
#
#   Main library
#
#-----------------------------------------------------------------------------#

if(WIN32 AND SHARED)
    if(NOT SUPPORT_D3D OR NOT SUPPORT_OPENGL)
        message("WARNING: "
            "Both D3D and OpenGL should be present for the SHARED build "
            " to ensure maximum DLL compatibility.")
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

if(SUPPORT_X11 AND NOT ALLEGRO_RASPBERRYPI)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_X_FILES})
    list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
endif(SUPPORT_X11 AND NOT ALLEGRO_RASPBERRYPI)

if(WIN32)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_WIN_FILES})
    list(APPEND PLATFORM_LIBS
        user32 gdi32 comdlg32 ole32 winmm kernel32 psapi shlwapi
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

if(ALLEGRO_MACOSX)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_MACOSX_FILES})
    find_library(APPKIT_LIBRARY AppKit)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(CARBON_LIBRARY Carbon)
    find_library(UNIFORMTYPEIDENTIFIERS_LIBRARY UniformTypeIdentifiers)
    list(APPEND PLATFORM_LIBS ${APPKIT_LIBRARY})
    list(APPEND PLATFORM_LIBS ${IOKIT_LIBRARY})
    list(APPEND PLATFORM_LIBS ${COREVIDEO_LIBRARY})
    list(APPEND PLATFORM_LIBS ${CARBON_LIBRARY})
    list(APPEND PLATFORM_LIBS ${UNIFORMTYPEIDENTIFIERS_LIBRARY})
endif(ALLEGRO_MACOSX)

if(IPHONE)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_IPHONE_FILES})
endif(IPHONE)

if(ANDROID)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_ANDROID_FILES})
    list(APPEND PLATFORM_LIBS m log)
endif(ANDROID)

if(ALLEGRO_RASPBERRYPI)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_RASPBERRYPI_FILES})
    list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
endif(ALLEGRO_RASPBERRYPI)

if(SUPPORT_OPENGL)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_OPENGL_FILES})
    if(WIN32)
        list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_WGL_FILES})
    endif(WIN32)
    list(APPEND PLATFORM_LIBS ${OPENGL_LIBRARIES})
endif(SUPPORT_OPENGL)

if(SUPPORT_OPENGL AND WANT_SHADERS_GL AND NOT ALLEGRO_CFG_OPENGLES1)
    set(ALLEGRO_CFG_SHADER_GLSL 1)
endif()

if(SUPPORT_D3D AND WANT_SHADERS_D3D AND D3DX9_FOUND)
    set(ALLEGRO_CFG_SHADER_HLSL 1)
endif()

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
if(SUPPORT_X11 AND NOT ALLEGRO_RASPBERRYPI)
    list(APPEND ALLEGRO_PUBLIC_HEADERS
        ${ALLEGRO_INCLUDE_ALLEGRO_X_FILES}
        )
endif(SUPPORT_X11 AND NOT ALLEGRO_RASPBERRYPI)
if(ANDROID)
    list(APPEND ALLEGRO_PUBLIC_HEADERS ${ALLEGRO_INCLUDE_ALLEGRO_ANDROID_FILES})
endif(ANDROID)
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

if(ALLEGRO_SDL)
    list(APPEND LIBRARY_SOURCES ${ALLEGRO_SRC_SDL_FILES})
    list(APPEND PLATFORM_LIBS ${SDL2_LIBRARY} m)
    include_directories(${SDL2_INCLUDE_DIR})
endif(ALLEGRO_SDL)

set_our_header_properties(${ALLEGRO_PUBLIC_HEADERS})
unset(ALLEGRO_TARGETS CACHE)

if(NOT WANT_MONOLITH)
    # ALLEGRO_LIB_BUILD is defined for all Allegro sources (core and addon)
    # ALLEGRO_SRC is defined only while compiling the core sources (its use is
    # to get the DLL #defines right under Windows for creating DLL export
    # functions when it is defined or import DLL functions when it is not).
    add_our_library(allegro
        Allegro-${ALLEGRO_SOVERSION}
        "${LIBRARY_SOURCES};${ALLEGRO_PUBLIC_HEADERS}"
        "${LIBRARY_CFLAGS} -DALLEGRO_SRC"
        "${PLATFORM_LIBS}"
        )
    target_include_directories(allegro INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_BINARY_DIR}/include>"
        )
endif(NOT WANT_MONOLITH)

# Addons and examples should link with this target.
set(ALLEGRO_LINK_WITH allegro)

install_our_headers(${ALLEGRO_PUBLIC_HEADERS})

#-----------------------------------------------------------------------------#
#
# Add-ons
#
#-----------------------------------------------------------------------------#

set(MONOLITH_SOURCES ${LIBRARY_SOURCES})

add_subdirectory(addons)

if(WANT_MONOLITH)
    include_directories(${MONOLITH_INCLUDE_DIRECTORIES})

    # For OSX, we need to set the header location inside the framework for each
    # file (relative to the current directory, so for example when the
    # AllegroImage framework is built we need to set the path for
    # allegro5/allegro_image.h but when the AllegroMonolith framework is built
    # we need to set the path for addons/image/allegro5/allegro_image.h -
    # otherwise the header is silently missing).
    set_our_header_properties(${MONOLITH_HEADERS})

    link_directories(${MONOLITH_LINK_DIRECTORIES})
    add_our_library(allegro_monolith
        AllegroMonolith-${ALLEGRO_SOVERSION}
        "${MONOLITH_SOURCES};${ALLEGRO_PUBLIC_HEADERS}"
        "${LIBRARY_CFLAGS} -DALLEGRO_SRC ${MONOLITH_DEFINES}"
        "${PLATFORM_LIBS};${MONOLITH_LIBRARIES}"
        )
    target_include_directories(allegro_monolith INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_BINARY_DIR}/include>"
        )

    set(ALLEGRO_MONOLITH_LINK_WITH allegro_monolith)
endif(WANT_MONOLITH)

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
    ${PROJECT_BINARY_DIR}/include/allegro5/platform/alplatf.h
    )

#-----------------------------------------------------------------------------#
#
#   Android
#
#-----------------------------------------------------------------------------#

if(ANDROID)
    include(AndroidApp)
    add_subdirectory(android)
endif(ANDROID)

#-----------------------------------------------------------------------------#
#
# Demo
#
#-----------------------------------------------------------------------------#

if(NOT MSVC80 AND WANT_DEMO) # XXX disabled because it breaks MSVC's intellisense for some reason
    add_subdirectory(demos/cosmic_protector)
    add_subdirectory(demos/speed)
    add_subdirectory(demos/skater)
    add_subdirectory(demos/shooter)
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
# Example data
#
#-----------------------------------------------------------------------------#

copy_data_dir_to_build(copy_example_data
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/data"
    "${CMAKE_CURRENT_BINARY_DIR}/examples/data"
    )

#-----------------------------------------------------------------------------#
#
# contrib/Luajit
#
#-----------------------------------------------------------------------------#

if(WANT_LUAJIT)
    add_subdirectory(contrib/luajit)
endif(WANT_LUAJIT)

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

if(WANT_MONOLITH)
    set(PKG_CONFIG_FILES allegro_monolith)
else()
    set(PKG_CONFIG_FILES allegro ${ADDON_PKG_CONFIG_FILES})
endif(WANT_MONOLITH)

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
    foreach(versuffix 5)
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
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
                )
        endforeach(name)
    endforeach(versuffix)
endif(INSTALL_PKG_CONFIG_FILES)

# Generate CMake package-config files
include(CMakePackageConfigHelpers)

set(PKG_NAME "Allegro")
string(TOLOWER ${PKG_NAME} PKG_NAME_LOWER)

set(PKG_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_NAME_LOWER}")
set(PKG_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake")
set(PKG_TARGETS_NAME "${PKG_NAME}Targets")
set(PKG_CONFIG_FILE "${PKG_GENERATED_DIR}/${PKG_NAME}Config.cmake")
set(PKG_VERSION_FILE "${PKG_GENERATED_DIR}/${PKG_NAME}ConfigVersion.cmake")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/${PKG_NAME}Config.cmake.in"
  ${PKG_CONFIG_FILE}
  INSTALL_DESTINATION ${PKG_CONFIG_INSTALL_DIR}
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(${PKG_VERSION_FILE}
  VERSION ${ALLEGRO_VERSION}
  COMPATIBILITY AnyNewerVersion
)
install(
  TARGETS ${ALLEGRO_TARGETS}
  EXPORT ${PKG_TARGETS_NAME}
)
install(
  EXPORT ${PKG_TARGETS_NAME}
  NAMESPACE ${PKG_NAME}::
  DESTINATION ${PKG_CONFIG_INSTALL_DIR}
)
install(
  FILES ${PKG_CONFIG_FILE} ${PKG_VERSION_FILE}
  DESTINATION ${PKG_CONFIG_INSTALL_DIR}
)

# Export build tree to ease development and debugging
export(EXPORT ${PKG_TARGETS_NAME}
  FILE "${PKG_GENERATED_DIR}/${PKG_NAME}Targets.cmake"
  NAMESPACE ${PKG_NAME}::
)

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
#
#   Configuration summary
#
#-----------------------------------------------------------------------------#

# Output a report string showing what features are supported.
message(NOTICE "${SPACER}\n${CONFIGURATION_SUMMARY}${SPACER}\n")


#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
