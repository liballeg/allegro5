# - Try to find WebP.
# Once done, this will define
#
#  WEBP_FOUND - system has WebP.
#  WEBP_INCLUDE_DIRS - the WebP. include directories
#  WEBP_LIBRARIES - link these to use WebP.
#
# Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
# Copyright (C) 2013 Igalia S.L.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_WEBP QUIET libwebp libsharpyuv)

# Look for the header file.
find_path(WEBP_INCLUDE_DIRS
    NAMES webp/decode.h
    HINTS ${PC_WEBP_INCLUDEDIR} ${PC_WEBP_INCLUDE_DIRS}
)
mark_as_advanced(WEBP_INCLUDE_DIRS)

# Look for the library.
find_library(
    WEBP_LIBRARY
    NAMES webp
    HINTS ${PC_WEBP_LIBDIR} ${PC_WEBP_LIBRARY_DIRS}
)

if (WEBP_LIBRARY)
  list(APPEND WEBP_LIBRARIES ${WEBP_LIBRARY})
endif()
# As of 1.3, libsharpyuv is split off from the main library.
find_library(
    SHARPYUV_LIBRARY
    NAMES sharpyuv
    HINTS ${PC_WEBP_LIBDIR} ${PC_WEBP_LIBRARY_DIRS}
)
if (SHARPYUV_LIBRARY)
  list(APPEND WEBP_LIBRARIES ${SHARPYUV_LIBRARY})
endif()

set(WEBP_LIBRARIES ${WEBP_LIBRARIES} CACHE STRING "WebP libraries")
mark_as_advanced(WEBP_LIBRARIES)

include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(WebP REQUIRED_VARS WEBP_INCLUDE_DIRS WEBP_LIBRARIES
                                  FOUND_VAR WEBP_FOUND)
