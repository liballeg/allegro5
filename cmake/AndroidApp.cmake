function(add_android_app prog sources)
    set(PROJECT "${CMAKE_CURRENT_BINARY_DIR}/${prog}.project")
    set(PROJECT_SOURCE ${CMAKE_SOURCE_DIR}/android/gradle_project)
    set(native "${PROJECT}/app/src/main/jniLibs/${ANDROID_ABI}/libnative-lib.so")
    set(GRADLE_PROJECT app)
    set(ANDROID_HOME $ENV{ANDROID_HOME})
    set(AAR ${PROJECT}/app/libs/allegro.aar)
    set(adb ${ANDROID_HOME}/platform-tools/adb)
    set(APP_ID ${prog})

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
       set(SUFFIX "-debug")
    else()
       set(SUFFIX "")
    endif()

    configure_file(
        ${PROJECT_SOURCE}/settings.gradle
        ${PROJECT}/settings.gradle)

    configure_file(
        ${PROJECT_SOURCE}/local.properties
        ${PROJECT}/local.properties)

    configure_file(
        ${PROJECT_SOURCE}/app/src/main/AndroidManifest.xml
        ${PROJECT}/app/src/main/AndroidManifest.xml)

    configure_file(
        ${PROJECT_SOURCE}/app/build.gradle
        ${PROJECT}/app/build.gradle)

    set(COPY_FILES
        ${PROJECT_SOURCE}/gradle.properties
        ${PROJECT_SOURCE}/build.gradle
        ${PROJECT_SOURCE}/gradlew
        ${PROJECT_SOURCE}/gradle/wrapper/gradle-wrapper.jar
        ${PROJECT_SOURCE}/gradle/wrapper/gradle-wrapper.properties
        ${PROJECT_SOURCE}/app/src/main/java/org/liballeg/app/MainActivity.java
        )

    get_property(JNI_LIBS GLOBAL PROPERTY JNI_LIBS)
    foreach(lib ${JNI_LIBS})
        set(jnilib ${PROJECT}/app/src/main/jniLibs/${ANDROID_ABI}/lib${lib}${SUFFIX}.so)
        add_custom_command(
            OUTPUT ${jnilib}
            DEPENDS ${lib}
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${lib}> ${jnilib})
        list(APPEND jnilibs ${jnilib})
    endforeach()

    list(APPEND COPIED_FILES ${PROJECT}/local.properties)
    list(APPEND COPIED_FILES ${PROJECT}/app/src/main/AndroidManifest.xml)
    list(APPEND COPIED_FILES ${PROJECT}/app/build.gradle)

    foreach(copy ${COPY_FILES})
        string(REPLACE "${PROJECT_SOURCE}/" "${PROJECT}/" target ${copy})
        add_custom_command(
            OUTPUT ${target}
            DEPENDS ${copy}
            COMMAND ${CMAKE_COMMAND} -E copy ${copy} ${target}
        )
        list(APPEND COPIED_FILES ${target})
    endforeach()

    # The APK to output. We always build the examples in debug mode as
    # a release version would need to be signed.
    set(apk_path "${PROJECT}/app/build/outputs/apk/debug/app-debug.apk")

    # Build the application as a shared library.
    add_library(${prog} SHARED ${sources})
    set_target_properties(${prog} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT})
    target_link_libraries(${prog} ${JNI_LIBS})

    # Get the path of the application's shared object.
    add_custom_command(
        OUTPUT ${native}
        DEPENDS ${prog}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${prog}> ${native}
        )
        
    add_custom_command(
        OUTPUT ${AAR}
        DEPENDS ${A5O_AAR}
        COMMAND ${CMAKE_COMMAND} -E copy ${A5O_AAR} ${AAR}
        )

    # How to make the APK.
    add_custom_command(
        OUTPUT ${apk_path}
        DEPENDS ${native} ${jnilibs} ${COPIED_FILES} ${AAR}
        WORKING_DIRECTORY ${PROJECT}
        COMMAND ./gradlew assembleDebug
        )

    add_custom_target(${prog}_apk
        ALL
        DEPENDS aar ${apk_path}
        )

    # Useful targets for testing.
    add_custom_target(install_${prog}
        DEPENDS ${prog}_apk
        COMMAND ${adb} install -r ${apk_path}
        )

    add_custom_target(run_${prog}
        DEPENDS install_${prog}
        COMMAND ${adb} shell
            'am start -a android.intent.action.MAIN -n org.liballeg.${APP_ID}/org.liballeg.app.MainActivity'
        )
endfunction()
