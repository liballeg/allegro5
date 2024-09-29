# - Find Open Sound System
#
#  OSS_FOUND       - True if OSS headers found.

# This file is Allegro-specific and requires the following variables to be
# set elsewhere:
#   A5O_HAVE_MACHINE_SOUNDCARD_H
#   A5O_HAVE_LINUX_SOUNDCARD_H
#   A5O_HAVE_SYS_SOUNDCARD_H
#   A5O_HAVE_SOUNDCARD_H


if(OSS_INCLUDE_DIR)
    # Already in cache, be silent
    set(OSS_FIND_QUIETLY TRUE)
endif(OSS_INCLUDE_DIR)

set(CMAKE_REQUIRED_DEFINITIONS)

if(A5O_HAVE_SOUNDCARD_H OR A5O_HAVE_SYS_SOUNDCARD_H OR
        A5O_HAVE_MACHINE_SOUNDCARD_H OR A5O_LINUX_SYS_SOUNDCARD_H)

    if(A5O_HAVE_MACHINE_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS -DA5O_HAVE_MACHINE_SOUNDCARD_H)
    endif(A5O_HAVE_MACHINE_SOUNDCARD_H)

    if(A5O_HAVE_LINUX_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DA5O_HAVE_LINUX_SOUNDCARD_H)
    endif(A5O_HAVE_LINUX_SOUNDCARD_H)

    if(A5O_HAVE_SYS_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DA5O_HAVE_SYS_SOUNDCARD_H)
    endif(A5O_HAVE_SYS_SOUNDCARD_H)

    if(A5O_HAVE_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DA5O_HAVE_SOUNDCARD_H)
    endif(A5O_HAVE_SOUNDCARD_H)

    run_c_compile_test("
        #ifdef A5O_HAVE_SOUNDCARD_H
        #include <soundcard.h>
        #endif
        #ifdef A5O_HAVE_SYS_SOUNDCARD_H
        #include <sys/soundcard.h>
        #endif
        #ifdef A5O_HAVE_LINUX_SOUNDCARD_H
        #include <linux/soundcard.h>
        #endif
        #ifdef A5O_HAVE_MACHINE_SOUNDCARD_H
        #include <machine/soundcard.h>
        #endif
        int main(void) {
            audio_buf_info abi;
            return 0;
        }"
        OSS_COMPILES
    )

    set(CMAKE_REQUIRED_DEFINITIONS)

endif(A5O_HAVE_SOUNDCARD_H OR A5O_HAVE_SYS_SOUNDCARD_H OR
    A5O_HAVE_MACHINE_SOUNDCARD_H OR A5O_LINUX_SYS_SOUNDCARD_H)

# Handle the QUIETLY and REQUIRED arguments and set OSS_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OSS DEFAULT_MSG
    OSS_COMPILES)

mark_as_advanced(OSS_COMPILES)
