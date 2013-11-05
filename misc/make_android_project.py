#!/usr/bin/env python

import optparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

def main():
    options = parse_args(sys.argv)
    path = options.path

    create_project(options)
    touch_manifest(path + "/AndroidManifest.xml", options)
    touch_strings(path + "/res/values/strings.xml", options)
    create_localgen_properties(path + "/localgen.properties", options)
    create_custom_rules_xml(path + "/custom_rules.xml")

    activity_filename = "{0}/src/{1}/{2}.java".format(options.path,
        slashy(options.package), options.activity)
    create_activity(activity_filename, options)

    create_jni(options)

def parse_args(argv):
    p = optparse.OptionParser()

    # These options are the same as to 'android create project'.
    p.add_option("-p", "--path",
            default=None,
            help="The new project's directory.")
    p.add_option("-n", "--name",
            default="example",
            help="Project name.")
    p.add_option("-a", "--activity",
            default="Activity",
            help="Name of the default Activity that is created.")
    p.add_option("-k", "--package",
            default="org.liballeg.example",
            help="Android package name for the application.")
    p.add_option("-t", "--target",
            default="android-10",
            help="Target ID of the new project.")

    # These are our own options.
    p.add_option("--android-tool",
            default="android",
            help="Path to android tool.")
    p.add_option("--load-libs",
            default="",
            help="Space separated list of libraries to load.")
    p.add_option("--load-app",
            default=None,
            help="Name of application shared object.")
    p.add_option("--bin-dir",
            default=None,
            help="Path to directory containing program binary.")
    p.add_option("--lib-dir",
            default=None,
            help="Path to directory containing libraries.")
    p.add_option("--jar-libs-dir",
            default="",
            help="Path to directory containing JARs.")
    p.add_option("--stl",
            default="",
            help="STL implementation to use, if any.")

    (options, args) = p.parse_args()
    if options.path == None:
        options.path = options.name
    if options.load_app == None:
        options.load_app = options.name
    if options.bin_dir == None:
        options.bin_dir = options.path
    if options.lib_dir == None:
        options.lib_dir = options.path
    return options

def create_project(options):
    subprocess.check_call([
        options.android_tool, "create", "project",
        "-p", options.path,
        "-n", options.name,
        "-k", options.package,
        "-a", options.activity,
        "-t", options.target
        ])

def touch_manifest(filename, options):
    ET.register_namespace("android",
        "http://schemas.android.com/apk/res/android")
    tree = ET.parse(filename)
    root = tree.find(".")
    ET.SubElement(root, "uses-sdk", {
        "android:minSdkVersion": "10"
    })
    ET.SubElement(root, "uses-permission", {
        "android:name": "android.permission.WRITE_EXTERNAL_STORAGE"
    })
    application = tree.find("./application")
    application.set("android:debuggable", "true")
    activity = tree.find("./application/activity")
    activity.set("android:launchMode", "singleTask")
    activity.set("android:screenOrientation", "unspecified")
    activity.set("android:configChanges", "screenLayout|uiMode|orientation")
    ET.SubElement(activity, "meta-data", {
        "android:name": "org.liballeg.app_name",
        "android:value": options.load_app
    })
    tree.write(filename, "utf-8", xml_declaration=True)

def touch_strings(filename, options):
    tree = ET.parse(filename)
    app_name = tree.find("string[@name='app_name']")
    app_name.text = options.name
    tree.write(filename, "utf-8", xml_declaration=True)

def create_localgen_properties(filename, options):
    f = open(filename, "w")
    f.write("jar.libs.dir={0}\n".format(options.jar_libs_dir))
    f.close()

def create_custom_rules_xml(filename):
    f = open(filename, "w")
    f.write('''\
        <project>
            <property file="localgen.properties" />
            <target name="-pre-compile">
                <path id="project.all.jars.path">
                    <path path="${toString:project.all.jars.path}"/>
                    <fileset dir="${jar.libs.dir}">
                        <include name="*.jar"/>
                    </fileset>
                </path>
            </target>
        </project>
    ''')
    f.close()

def create_activity(filename, options):
    libs = options.load_libs.split()
    if options.stl:
        libs.insert(0, options.stl)
    stmts = "\n\t\t".join([
        'System.loadLibrary("{0}");'.format(lib) for lib in libs
    ])
    f = open(filename, "w")
    f.write('''\
        package {PACKAGE};
        public class {ACTIVITY} extends org.liballeg.app.AllegroActivity {{
            static {{
                {STMTS}
            }}
        }}
    '''.format(
        PACKAGE=options.package,
        ACTIVITY=options.activity,
        STMTS=stmts
    ))
    f.close()

def slashy(s):
    return s.replace(".", "/")

def create_jni(options):
    path = options.path
    jni_path = path + "/jni"
    application_mk_path = jni_path + "/Application.mk"
    android_mk_path = jni_path + "/Android.mk"
    rel_bin_dir = os.path.relpath(options.bin_dir, jni_path)
    rel_lib_dir = os.path.relpath(options.lib_dir, jni_path)

    mkdir(jni_path)

    f = open(application_mk_path, "w")
    if options.stl:
        f.write("APP_STL := {0}\n".format(options.stl))
    f.close()

    f = open(android_mk_path, "w")
    f.write('''
        LOCAL_PATH := $(call my-dir)
        REL_BIN_DIR := {REL_BIN_DIR}
        REL_LIB_DIR := {REL_LIB_DIR}

        include $(CLEAR_VARS)
        LOCAL_MODULE := {NAME}
        LOCAL_SRC_FILES := $(REL_BIN_DIR)/lib{LOAD_APP}.so
        LOCAL_SHARED_LIBRARIES := {STL}
        include $(PREBUILT_SHARED_LIBRARY)
    '''.format(
        NAME=options.name,
        LOAD_APP=options.load_app,
        STL=options.stl,
        REL_BIN_DIR=rel_bin_dir,
        REL_LIB_DIR=rel_lib_dir
    ))

    for lib in options.load_libs.split():
        f.write('''
            include $(CLEAR_VARS)
            LOCAL_MODULE := {NAME}
            LOCAL_SRC_FILES := $(REL_LIB_DIR)/lib{NAME}.so
            include $(PREBUILT_SHARED_LIBRARY)
        '''.format(
            NAME=lib
        ))
    f.close()

def mkdir(path):
    if not os.path.exists(path):
        os.mkdir(path)

if __name__ == "__main__":
    main()

# vim: set sts=4 sw=4 et:
