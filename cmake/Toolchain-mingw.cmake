# Use this command to build the Windows port of Allegro
# with a mingw cross compiler:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-mingw.cmake .
#
# You will need at least CMake 2.6.0.
# Adjust the following paths to suit your environment.
#
# This file was based on http://www.cmake.org/Wiki/CmakeMingw

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER /opt/cross-tools/bin/i386-mingw32msvc-gcc)
set(CMAKE_CXX_COMPILER /opt/cross-tools/bin/i386-mingw32msvc-g++)

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH /opt/cross-tools)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
