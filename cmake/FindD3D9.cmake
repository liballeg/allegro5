# - Find Direct3D 9
# Find the Direct3D9 includes and libraries
#
#  D3D9_INCLUDE_DIR - where to find d3d9.h
#  D3D9_LIBRARIES   - List of libraries when using D3D9.
#  D3D9_FOUND       - True if D3D9 found.

if(D3D9_INCLUDE_DIR)
    # Already in cache, be silent
    set(D3D9_FIND_QUIETLY TRUE)
endif(D3D9_INCLUDE_DIR)

find_path(D3D9_INCLUDE_DIR d3d9.h
    HINTS "$ENV{DXSDK_DIR}/Include"
    )

find_library(D3D9_LIBRARY
    NAMES d3d9
    HINTS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}"
    )

# Handle the QUIETLY and REQUIRED arguments and set D3D9_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(D3D9 DEFAULT_MSG
    D3D9_INCLUDE_DIR D3D9_LIBRARY)

if(D3D9_FOUND)
    set(D3D9_LIBRARIES ${D3D9_LIBRARY})
else(D3D9_FOUND)
    set(D3D9_LIBRARIES)
endif(D3D9_FOUND)

mark_as_advanced(D3D9_INCLUDE_DIR D3D9_LIBRARY)
