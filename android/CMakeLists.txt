find_program(ANDROID_TOOL android CMAKE_FIND_ROOT_PATH_BOTH)
find_program(ANT ant CMAKE_FIND_ROOT_PATH_BOTH)
find_program(NDK_BUILD ndk-build CMAKE_FIND_ROOT_PATH_BOTH)

if(NOT ANDROID_TOOL)
    message(FATAL_ERROR "WARNING: android tool not found")
endif()
if(NOT ANT)
    message(FATAL_ERROR "WARNING: ant not found")
endif()
if(NOT NDK_BUILD)
    message(FATAL_ERROR "WARNING: ndk-build not found")
endif()

set(ANT_COMMAND "${ANT}" -noinput -nouserlib -noclasspath -quiet)

include(Activity.cmake)
include(Example.cmake)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
