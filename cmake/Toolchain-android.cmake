SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

set(ANDROID_NDK_TOOLCHAIN_ROOT "$ENV{ANDROID_NDK_TOOLCHAIN_ROOT}" CACHE PATH "Path to the Android NDK Standalone Toolchain" )

message( STATUS "Selected Android NDK toolchain: ${ANDROID_NDK_TOOLCHAIN_ROOT}" )
if(NOT EXISTS ${ANDROID_NDK_TOOLCHAIN_ROOT})
   set(ANDROID_NDK_TOOLCHAIN_ROOT "/opt/android-toolchain" CACHE PATH "Path to the Android NDK Standalone Toolchain" )
   message( STATUS "Using default path for toolchain ${ANDROID_NDK_TOOLCHAIN_ROOT}")
   message( STATUS "If you prefer to use a different location, please set the ANDROID_NDK_TOOLCHAIN_ROOT cmake variable.")
endif()
     
if(NOT EXISTS ${ANDROID_NDK_TOOLCHAIN_ROOT})
  message(FATAL_ERROR
  "${ANDROID_NDK_TOOLCHAIN_ROOT} does not exist!
  You should either set an environment variable:
    export ANDROID_NDK_TOOLCHAIN_ROOT=~/my-toolchain
  or put the toolchain in the default path:
    sudo ln -s ~/android-toolchain /opt/android-toolchain
    ")
endif()

find_program(CMAKE_MAKE_PROGRAM make)

#setup build targets, mutually exclusive
set(PossibleArmTargets
  "x86;x86_64;armeabi;armeabi-v7a;armeabi-v7a with NEON;arm64-v8a;mips;mips64")
set(ARM_TARGETS "armeabi-v7a" CACHE STRING 
    "the arm targets for android, recommend armeabi-v7a 
    for floating point support and NEON.")

if(ARM_TARGETS STREQUAL "x86")
    set(ANDROID_ARCH "i686-linux-android")
elseif(ARM_TARGETS STREQUAL "x86_64")
    set(ANDROID_ARCH "x86_64-linux-android")
elseif(ARM_TARGETS STREQUAL "arm64-v8a")
    set(ANDROID_ARCH "aarch64-linux-android")
elseif(ARM_TARGETS STREQUAL "mips")
    set(ANDROID_ARCH "mipsel-linux-android")
elseif(ARM_TARGETS STREQUAL "mips64")
    set(ANDROID_ARCH "mips64el-linux-android")
elseif(ARM_TARGETS STREQUAL "armeabi")
    set(ANDROID_ARCH "arm-linux-androideabi")
    set(ARMEABI true)
    set(NEON false)
elseif(ARM_TARGETS STREQUAL "armeabi-v7a")
    set(ARMEABI true)
    set(ANDROID_ARCH "arm-linux-androideabi")
    set(NEON false)
elseif(ARM_TARGETS STREQUAL "armeabi-v7a with NEON")
    set(ARMEABI true)
    set(ANDROID_ARCH "arm-linux-androideabi")
    set(NEON true)
else()
    message(FATAL_ERROR "Unknown Android target ${ARM_TARGETS}")        
endif()

if(WIN32)
	set(CMAKE_EXECUTABLE_SUFFIX ".exe")
endif()

# specify the cross compiler
SET(CMAKE_C_COMPILER   
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-clang${CMAKE_EXECUTABLE_SUFFIX} CACHE PATH "C compiler" FORCE)
SET(CMAKE_CXX_COMPILER 
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-clang++${CMAKE_EXECUTABLE_SUFFIX} CACHE PATH "C++ compiler" FORCE)
#there may be a way to make cmake deduce these TODO deduce the rest of the tools
set(CMAKE_AR
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-ar${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "archive" FORCE)
set(CMAKE_LINKER
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-ld${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "linker" FORCE)
set(CMAKE_NM
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-nm${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-objcopy${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-objdump${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-strip${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_ARCH}-ranlib${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "ranlib" FORCE)

set_property(CACHE ARM_TARGETS PROPERTY STRINGS ${PossibleArmTargets} )

set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_SOURCE_DIR} CACHE PATH 
    "root for library output, set this to change where
    android libs are installed to")
    
#set these flags for client use
set(LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH_ROOT}/libs/${ARM_TARGETS}
    CACHE PATH "path for android libs" FORCE)
set(CMAKE_INSTALL_PREFIX ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/${ARM_TARGETS}
    CACHE STRING "path for installing" FORCE)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin ${ANDROID_NDK_TOOLCHAIN_ROOT}/arm-linux-androideabi ${ANDROID_NDK_TOOLCHAIN_ROOT}/sysroot ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_PREFIX}/share)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
# only search for libraries and includes in the ndk toolchain
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CMAKE_CXX_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID")
SET(CMAKE_C_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID")
if (ARMEABI)
  #Setup arm specific stuff
  #It is recommended to use the -mthumb compiler flag to force the generation
  #of 16-bit Thumb-1 instructions (the default being 32-bit ARM ones).
  SET(CMAKE_CXX_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID -mthumb")
  SET(CMAKE_C_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID -mthumb")

  #these are required flags for android armv7-a
  if(WANT_ANDROID_LEGACY)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6")
    SET(ALLEGRO_CFG_ANDROID_LEGACY 1)
  else(WANT_ANDROID_LEGACY)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfloat-abi=softfp")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfloat-abi=softfp")
    if(NEON)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    endif()
  endif(WANT_ANDROID_LEGACY)
 
  #-Wl,-L${LIBCPP_LINK_DIR},-lstdc++,-lsupc++
  #-L${LIBCPP_LINK_DIR} -lstdc++ -lsupc++
  #Also, this is *required* to use the following linker flags that routes around
  #a CPU bug in some Cortex-A8 implementations:

  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--fix-cortex-a8 -L${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "linker flags" FORCE)
  SET(CMAKE_MODULE_LINKER_FLAGS "-Wl,--fix-cortex-a8 -L${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "linker flags" FORCE)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")

#set these global flags for cmake client scripts to change behavior
set(ANDROID True)
set(BUILD_ANDROID True)
