# - Try to find MiniMP3 (https://github.com/lieff/minimp3)
# Once done, this will define
#
#  MINIMP3_FOUND - system has MiniMP3
#  MINIMP3_INCLUDE_DIRS - the MiniMP3 include directories

# Look for the header file.
find_path(MINIMP3_INCLUDE_DIRS
    NAMES minimp3.h minimp3_ex.h
    PATHS minimp3
)
mark_as_advanced(MINIMP3_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(MiniMP3 REQUIRED_VARS MINIMP3_INCLUDE_DIRS
                                  FOUND_VAR MINIMP3_FOUND)
