set(ACTIVITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/allegro_activity)
set(ACTIVITY_JAR ${LIBRARY_OUTPUT_PATH}/Allegro5.jar)

file(GLOB javas "${ACTIVITY_DIR}/src/*.java")

set(ACTIVITY_SOURCES
    ${ACTIVITY_DIR}/AndroidManifest.xml
    ${ACTIVITY_DIR}/build.xml
    ${ACTIVITY_DIR}/local.properties
    ${ACTIVITY_DIR}/project.properties
    ${javas}
    )

add_custom_command(
    OUTPUT ${ACTIVITY_DIR}/local.properties
    WORKING_DIRECTORY ${ACTIVITY_DIR}
    COMMAND ${ANDROID_UPDATE_COMMAND}
    VERBATIM
    )

configure_file(
    ${ACTIVITY_DIR}/localgen.properties.in
    ${ACTIVITY_DIR}/localgen.properties
    )
configure_file(
    ${ACTIVITY_DIR}/project.properties.in
    ${ACTIVITY_DIR}/project.properties
    @ONLY)

add_custom_command(
    OUTPUT ${ACTIVITY_JAR}
    DEPENDS ${ACTIVITY_SOURCES}
    WORKING_DIRECTORY ${ACTIVITY_DIR}
    COMMAND ${ANT_COMMAND} debug jar
    VERBATIM
    )

add_custom_target(jar
    ALL
    DEPENDS ${ACTIVITY_JAR}
    )

# Make this available for building examples in other directories.
set(ACTIVITY_JAR ${ACTIVITY_JAR} PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
