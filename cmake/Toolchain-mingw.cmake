# Use this command to build the Windows port of Allegro
# with a mingw cross compiler:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-mingw.cmake .
#
# or for out of source:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw.cmake ..
#
# You will need at least CMake 2.6.0.
#
# Adjust the following paths to suit your environment.
# The following assumes you have unpacked the package from
#   http://www.libsdl.org/extras/win32/cross/
# into /usr/local.
#
# This file was based on http://www.cmake.org/Wiki/CmakeMingw

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER /usr/local/cross-tools/bin/i386-mingw32-gcc)
set(CMAKE_CXX_COMPILER /usr/local/cross-tools/bin/i386-mingw32-g++)

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH /usr/local/cross-tools)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
