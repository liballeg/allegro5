# - Find XInput
# Find the Xinput includes and libraries
#
#  XINPUT_INCLUDE_DIR - where to find xinput.h
#  XINPUT_LIBRARIES   - List of libraries when using xinput.
#  XINPUT_FOUND       - True if dinput found.

if(XINPUT_INCLUDE_DIR)
    # Already in cache, be silent
    set(XINPUT_FIND_QUIETLY TRUE)
endif(XINPUT_INCLUDE_DIR)

find_path(XINPUT_INCLUDE_DIR xinput.h
    HINTS "$ENV{DXSDK_DIR}/Include"
    DOC "Windows XInput support header"
    )

find_library(XINPUT_LIBRARY
    NAMES XInput xinput
    HINTS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}"
    DOC "Windows XInput support library"
    )

# Handle the QUIETLY and REQUIRED arguments and set XINPUT_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XINPUT DEFAULT_MSG
    XINPUT_INCLUDE_DIR XINPUT_LIBRARY)

if(XINPUT_FOUND)
    set(XINPUT_LIBRARIES ${XINPUT_LIBRARY})
else(XINPUT_FOUND)
    set(XINPUT_LIBRARIES)
endif(XINPUT_FOUND)

mark_as_advanced(XINPUT_INCLUDE_DIR XINPUT_LIBRARY)
