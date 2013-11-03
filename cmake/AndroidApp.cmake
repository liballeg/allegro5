find_program(ANDROID_TOOL android CMAKE_FIND_ROOT_PATH_BOTH)
find_program(NDK_BUILD ndk-build CMAKE_FIND_ROOT_PATH_BOTH)
find_program(ANT ant CMAKE_FIND_ROOT_PATH_BOTH)
set(ANT_COMMAND "${ANT}" -noinput -nouserlib -noclasspath -quiet)

function(add_android_app prog sources lib_targets)

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
    target_link_libraries(${prog_target} ${lib_targets})

    # Get the name by which to load the application's shared library,
    # most likely just ${prog}.
    get_target_property(_target_location ${prog_target} LOCATION)
    get_filename_component(_target_filename ${_target_location} NAME_WE)
    string(REGEX REPLACE "^lib" "" load_app "${_target_filename}")

    # Get the base names of the shared libraries that it links with.
    set(load_libs)
    foreach(_target ${lib_targets})
        get_target_property(_target_location ${_target} LOCATION)
        get_filename_component(_target_basename ${_target_location} NAME_WE)
        string(REGEX REPLACE "^lib" "" load_lib "${_target_basename}")
        set(load_libs "${load_libs} ${load_lib}")
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
                --load-libs "${load_libs}"
                --load-app "${load_app}"
                --jar-libs-dir "${LIBRARY_OUTPUT_PATH}"
        VERBATIM
        )

    # XXX hardcoded
    set(abi armeabi)

    # Copy each shared library from elsewhere in the build tree into the
    # project libs directory, where they can be found by ant.
    set(project_libs)
    foreach(_target ${prog_target} ${lib_targets})
        get_target_property(_target_location ${_target} LOCATION)
        get_filename_component(_target_filename ${_target_location} NAME)
        set(_dest ${project_dir}/libs/${abi}/${_target_filename})

        add_custom_command(
            OUTPUT ${_dest}
            DEPENDS ${_target}
            COMMAND ${CMAKE_COMMAND} -E copy ${_target_location} ${_dest}
            )

        list(APPEND project_libs ${_dest})
    endforeach()

    # How to make the APK.
    add_custom_command(
        OUTPUT ${apk_path}
        DEPENDS ${prog_target}
                ${project_sources}
                ${project_libs}
                jar
        WORKING_DIRECTORY ${project_dir}
        COMMAND ${ANT_COMMAND} debug
        )

    add_custom_target(${prog}_apk
        ALL
        DEPENDS ${apk_path}
        )

    # Useful targets for testing.
    add_custom_target(install_${prog}_apk
        DEPENDS ${prog}_apk
        COMMAND adb -d install -r ${apk_path}
        )

    add_custom_target(run_${prog}_apk
        DEPENDS install_${prog}_apk
        COMMAND adb -d shell
            'am start -a android.intent.action.MAIN -n ${package}/.${activity}'
        )
endfunction()

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
