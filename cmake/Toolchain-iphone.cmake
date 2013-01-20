
# cmake 2.8.2 will not generate Xcode projects for iOS, but still generates makefiles. Update.
if(CMAKE_VERSION VERSION_EQUAL 2.8.2)
	message(STATUS "Warning: cmake 2.8.2 will generate Xcode projects. Update cmake.")
endif()

if(CMAKE_VERSION VERSION_LESS 2.8.10)
	message(FATAL_ERROR "Please get the latest version of cmake. There are issues with earlier versions and iOS.")
endif()

SET (IPHONE 1)
SET (ALLEGRO_CFG_OPENGLES 1)
SET (SDKROOT iphone)
SET (AL_DEPS deps_iphone)

include(CMakeForceCompiler)
CMAKE_FORCE_C_COMPILER(clang GNU)
CMAKE_FORCE_CXX_COMPILER(clang++ GNU)
SET(CMAKE_SIZEOF_VOID_P 4)
set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")

