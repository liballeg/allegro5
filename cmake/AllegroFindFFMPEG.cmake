# - Find FFMPEG
#
#  FFMPEG_FOUND       - true if FFMPEG is found
#  FFMPEG_CFLAGS      - required compiler flags
#  FFMPEG_LDFLAGS     - required linker flags

# -lavcodec -lavformat -lswscale

if(A5O_UNIX)
   pkg_check_modules(FFMPEG libavcodec libavformat libswscale libavutil)
endif()

# TODO: Windos and OSX
