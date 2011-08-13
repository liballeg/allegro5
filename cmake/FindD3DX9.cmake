# - Find Direct3D 9 Extensions
# Find the Direct3D9 Extensions includes and libraries
#
#  D3DX9_INCLUDE_DIR - where to find d3d9x.h
#  D3DX9_LIBRARIES   - List of libraries when using D3DX9.
#  D3DX9_FOUND       - True if D3DX9 found.

if(D3DX9_INCLUDE_DIR)
    # Already in cache, be silent
    set(D3DX9_FIND_QUIETLY TRUE)
endif(D3DX9_INCLUDE_DIR)

find_path(D3DX9_INCLUDE_DIR d3dx9.h
    HINTS "$ENV{DXSDK_DIR}/Include"
    )

find_library(D3DX9_LIBRARY
    NAMES d3dx9
    HINTS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}"
    )

# Handle the QUIETLY and REQUIRED arguments and set D3D9X_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(D3DX9 DEFAULT_MSG
    D3DX9_INCLUDE_DIR D3DX9_LIBRARY)

mark_as_advanced(D3DX9_INCLUDE_DIR D3DX9_LIBRARY)
