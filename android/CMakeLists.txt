# The android tool on Windows is a batch file wrapper, which cannot be
# started by MSYS shell directly. We invoke it via cmd.exe instead.
# We don't use the full path to avoid problems with spaces,
# and hope that android.bat is somewhere on the PATH.
if(ANDROID_TOOL MATCHES "[.]bat$")
    set(ANDROID_UPDATE_COMMAND
        cmd.exe /c "android.bat update project -p . -t ${ANDROID_TARGET}")
else()
    set(ANDROID_UPDATE_COMMAND
        "${ANDROID_TOOL}" update project -p . -t ${ANDROID_TARGET})
endif()

include(Activity.cmake)

if(WANT_EXAMPLES)
    include(Example.cmake)
endif()

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
