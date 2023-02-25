# This function will prevent in-source builds
# Based on https://github.com/InsightSoftwareConsortium/ITK/blob/1b5da45dc706e6d6803b52740fa50e0e5e7705e9/CMake/PreventInSourceBuilds.cmake
function(PreventInSourceBuilds)
  # make sure the user doesn't play dirty with symlinks
  get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
  get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

  # disallow in-source builds
  if("${srcdir}" STREQUAL "${bindir}")
    message(FATAL_ERROR
      "Allegro must not be configured to build in the source directory.\n"
      "Please refer to README.md\n"
      "Quitting configuration step")
  endif()
endfunction()

PreventInSourceBuilds()
