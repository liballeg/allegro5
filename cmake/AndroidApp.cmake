find_program(ANDROID_TOOL android CMAKE_FIND_ROOT_PATH_BOTH)
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
    set(bin_dir "${project_dir}/bin")
    set(lib_dir "${LIBRARY_OUTPUT_PATH}")

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
                --bin-dir "${bin_dir}"
                --lib-dir "${lib_dir}"
                --jar-libs-dir "${lib_dir}"
                "--stl=${stl}" # allows empty
        VERBATIM
        )

    # How to make the APK.
    add_custom_command(
        OUTPUT ${apk_path}
        DEPENDS ${prog_target} ${lib_targets} ${project_sources} ${ACTIVITY_JAR}
        WORKING_DIRECTORY ${project_dir}
        COMMAND ${NDK_BUILD}
        COMMAND ${ANT_COMMAND} debug
        )

    add_custom_target(${prog}_apk
        ALL
        DEPENDS ${apk_path}
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
