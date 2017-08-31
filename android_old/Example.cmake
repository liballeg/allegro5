if(NOT ALLEGRO_LINK_WITH OR NOT PRIMITIVES_LINK_WITH OR NOT IMAGE_LINK_WITH)
    message("WARNING: not building example apk for Android")
    return()
endif()

set(EXAMPLE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/example)
set(EXAMPLE_DIR ${CMAKE_CURRENT_BINARY_DIR}/example)
set(EXAMPLE_APK ${CMAKE_CURRENT_SOURCE_DIR}/example/bin/example-debug.apk)

set(EXAMPLE_SOURCES
    ${EXAMPLE_DIR}/AndroidManifest.xml
    ${EXAMPLE_DIR}/build.xml
    ${EXAMPLE_DIR}/local.properties
    ${EXAMPLE_DIR}/project.properties
    ${EXAMPLE_DIR}/csrc/main.c
    ${EXAMPLE_DIR}/src/org/liballeg/example/ExampleActivity.java
    )

add_custom_command(
    OUTPUT ${EXAMPLE_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${EXAMPLE_SRC}
    ${EXAMPLE_DIR}
    )

add_custom_command(
    OUTPUT ${EXAMPLE_DIR}/local.properties
    WORKING_DIRECTORY ${EXAMPLE_DIR}
    COMMAND ${ANDROID_UPDATE_COMMAND}
    VERBATIM
    )

configure_file(
    ${EXAMPLE_SRC}/localgen.properties.in
    ${EXAMPLE_DIR}/localgen.properties
    )
configure_file(
    ${EXAMPLE_SRC}/project.properties.in
    ${EXAMPLE_DIR}/project.properties
    @ONLY)

file(RELATIVE_PATH RELATIVE_LIB_DIR
    ${EXAMPLE_DIR}/jni ${LIBRARY_OUTPUT_PATH})
append_lib_type_suffix(LIB_TYPE_SUFFIX)
configure_file(
    ${EXAMPLE_SRC}/jni/localgen.mk.in
    ${EXAMPLE_DIR}/jni/localgen.mk
    )

add_custom_command(
    OUTPUT ${EXAMPLE_APK}
    DEPENDS ${EXAMPLE_SOURCES}
            ${ACTIVITY_JAR}
            ${ALLEGRO_LINK_WITH}
            ${PRIMITIVES_LINK_WITH}
            ${IMAGE_LINK_WITH}
    WORKING_DIRECTORY ${EXAMPLE_DIR}
    COMMAND ${NDK_BUILD}
    COMMAND ${ANT_COMMAND} debug
    )

add_custom_target(apk
    ALL
    DEPENDS ${EXAMPLE_APK}
    )

#-----------------------------------------------------------------------------#

add_custom_target(install_apk
    DEPENDS ${EXAMPLE_APK}
    WORKING_DIRECTORY ${EXAMPLE_DIR}
    COMMAND adb -d install -r ${EXAMPLE_APK}
    )

add_custom_target(run_apk
    DEPENDS install_apk
    COMMAND adb -d shell
            'am start -a android.intent.action.MAIN -n org.liballeg.example/.ExampleActivity'
    )

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
