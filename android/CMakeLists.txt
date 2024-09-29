set(PROJECT ${CMAKE_CURRENT_BINARY_DIR}/gradle_project)
set(PROJECT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/gradle_project)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(AAR_NAME allegro-debug.aar)
   set(ASSEMBLE assembleDebug)
else()
   set(AAR_NAME allegro-release.aar)
   set(ASSEMBLE assembleRelease)
endif()
set(A5O_AAR ${LIBRARY_OUTPUT_PATH}/${AAR_NAME})
set(A5O_AAR ${A5O_AAR} PARENT_SCOPE) 

if (NOT EXISTS "$ENV{ANDROID_HOME}")
  set(ENV{ANDROID_HOME} "$ENV{HOME}/Android/Sdk")
endif()

if (NOT EXISTS "$ENV{ANDROID_HOME}")
  message(FATAL_ERROR "Please set the ANDROID_HOME environment variable
to the location of your Android SDK, or install Android Studio in the
default location ($ENV{HOME}/Android/Sdk))")
endif()

set(ANDROID_HOME $ENV{ANDROID_HOME})

message(STATUS "Using Android SDK from ${ANDROID_HOME}.")

file(GLOB javas "${PROJECT_SOURCE}/allegro/src/main/java/org/liballeg/android/*.java")

configure_file(
    ${PROJECT_SOURCE}/local.properties
    ${PROJECT}/local.properties)

set(GRADLE_PROJECT allegro)
configure_file(
    ${PROJECT_SOURCE}/settings.gradle
    ${PROJECT}/settings.gradle)

set(COPY_FILES
    ${PROJECT_SOURCE}/allegro/src/main/AndroidManifest.xml
    ${PROJECT_SOURCE}/gradle.properties
    ${PROJECT_SOURCE}/build.gradle
    ${PROJECT_SOURCE}/gradlew
    ${PROJECT_SOURCE}/gradle/wrapper/gradle-wrapper.jar
    ${PROJECT_SOURCE}/gradle/wrapper/gradle-wrapper.properties
    ${PROJECT_SOURCE}/allegro/build.gradle
    ${PROJECT_SOURCE}/allegro/src/main/AndroidManifest.xml
    ${javas}
    )

list(APPEND COPIED_FILES ${PROJECT}/local.properties)

foreach(copy ${COPY_FILES})
    string(REPLACE "${PROJECT_SOURCE}/" "${PROJECT}/" target ${copy})
    add_custom_command(
        OUTPUT ${target}
        DEPENDS ${copy}
        COMMAND ${CMAKE_COMMAND} -E copy
        ${copy}
        ${target}
    )
    list(APPEND COPIED_FILES ${target})
endforeach()

add_custom_command(
    OUTPUT ${A5O_AAR}
    DEPENDS ${COPIED_FILES}
    WORKING_DIRECTORY ${PROJECT}
    COMMAND ./gradlew ${ASSEMBLE}
    COMMAND ${CMAKE_COMMAND} -E copy allegro/build/outputs/aar/${AAR_NAME} ${A5O_AAR}
    )

add_custom_target(aar ALL
    DEPENDS ${A5O_AAR})
