find_program(ANDROID_TOOL android NAMES android android.bat CMAKE_FIND_ROOT_PATH_BOTH)
find_program(NDK_BUILD ndk-build CMAKE_FIND_ROOT_PATH_BOTH)
find_program(ANT ant CMAKE_FIND_ROOT_PATH_BOTH)
set(ANT_COMMAND "${ANT}" -noinput -nouserlib -noclasspath -quiet)

if(NOT ANDROID_TOOL)
    message(FATAL_ERROR "android tool not found")
endif()
if(NOT NDK_BUILD)
    message(FATAL_ERROR "ndk-build not found")
endif()
if(NOT ANT)
    message(FATAL_ERROR "ant not found")
endif()

function(add_android_app prog sources lib_targets stl)

    set(prog_target ${prog})
    set(package "org.liballeg.examples.${prog}")
    set(package_slash "org/liballeg/examples/${prog}")
    set(activity "Activity")

    # The Android project directory.
    set(project_dir "${CMAKE_CURRENT_BINARY_DIR}/${prog}.project")

    # The source files in the Android project directory.
    set(project_sources
        ${project_dir}/AndroidManifest.xml
        ${project_dir}/build.xml
        ${project_dir}/jni/Android.mk
        ${project_dir}/jni/Application.mk
        ${project_dir}/local.properties
        ${project_dir}/localgen.properties
        ${project_dir}/project.properties
        ${project_dir}/src/${package_slash}/${activity}.java
        )

    # The APK to output.
    set(apk_path "${project_dir}/bin/${prog}-debug.apk")

    # Build the application as a shared library.
    add_library(${prog_target} SHARED ${sources})
    set_target_properties(${prog_target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${project_dir}/bin)
    target_link_libraries(${prog_target} ${stl} ${lib_targets})

    # Get the path of the application's shared object.
    get_target_property(load_app ${prog_target} LOCATION)

    # Get the paths of the shared libraries that it links with.
    # Each shared library has dependencies of its own.
    # We do not go deeper than that...
    set(load_libs)
    foreach(_target ${lib_targets})
        get_target_property(_dep_libs ${_target} ANDROID_LOAD_LIBS)
        if(_dep_libs)
            foreach(_dep_lib ${_dep_libs})
                if(_dep_lib MATCHES "[.]so$")
                    list(APPEND load_libs "--load-lib=${_dep_lib}")
                endif()
            endforeach()
        endif()

        get_target_property(_target_location ${_target} LOCATION)
        if(_target_location MATCHES "[.]so$")
            list(APPEND load_libs "--load-lib=${_target_location}")
        endif()
    endforeach()

    # Create the project directory.
    add_custom_command(
        OUTPUT ${project_sources}
        COMMAND ${CMAKE_SOURCE_DIR}/misc/make_android_project.py
                "--android-tool=${ANDROID_TOOL}"
                -p "${project_dir}"
                -n "${prog}"
                -k "${package}"
                -a "${activity}"
                "--app-abi=${ARM_TARGETS}"
                ${load_libs}
                "--load-app=${load_app}"
                "--jar-libs-dir=${LIBRARY_OUTPUT_PATH}"
                "--stl=${stl}" # allows empty
                "--target=${ANDROID_TARGET}"
        VERBATIM
        )

    # How to make the APK.
    # Both target-level `jar' and file-level ${ACTIVITY_JAR} dependencies are
    # listed.  The former otherwise make doesn't know how to make the .jar file
    # if called in some directories.  The latter so the APK is rebuilt if the
    # .jar file has been updated.  I don't know why.
    add_custom_command(
        OUTPUT ${apk_path}
        DEPENDS ${prog_target} ${lib_targets} ${project_sources} ${ACTIVITY_JAR}
        WORKING_DIRECTORY ${project_dir}
        COMMAND ${NDK_BUILD}
        COMMAND ${ANT_COMMAND} debug
        )

    add_custom_target(${prog}_apk
        ALL
        DEPENDS jar ${apk_path}
        )

    # Useful targets for testing.
    add_custom_target(install_${prog}
        DEPENDS ${prog}_apk
        COMMAND adb -d install -r ${apk_path}
        )

    add_custom_target(run_${prog}
        DEPENDS install_${prog}
        COMMAND adb -d shell
            'am start -a android.intent.action.MAIN -n ${package}/.${activity}'
        )
endfunction()

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
