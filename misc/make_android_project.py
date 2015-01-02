#!/usr/bin/env python

import optparse
import os
import os.path
import subprocess
import sys
import xml.etree.ElementTree as ET

def main():
    options = parse_args(sys.argv)
    check_options(options)
    create_project(options)
    touch_manifest(options)
    touch_strings(options)
    create_localgen_properties(options)
    create_custom_rules_xml(options)
    create_activity(options)
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
            default="android-12",
            help="Target ID of the new project.")

    # These are our own options.
    p.add_option("--android-tool",
            default="android",
            help="Path to android tool.")
    p.add_option("--app-abi",
            default="armeabi",
            help="ABI being targeted.")
    p.add_option("--load-lib",
            action="append",
            default=[],
            help="Paths to shared libraries to load.")
    p.add_option("--load-app",
            default=None,
            help="Path to application shared object.")
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
        options.load_app = \
            "{0}/bin/lib{1}.so".format(options.path, options.name)
    return options

def check_options(options):
    check_name_for_load_library(options.load_app)
    for lib in options.load_lib:
        check_name_for_load_library(lib)

def check_name_for_load_library(path):
    basename = os.path.basename(path)
    if not name_for_load_library(basename):
        raise Exception("System.loadLibrary would not find " + basename)

def name_for_load_library(path):
    basename = os.path.basename(path)
    if basename.startswith("lib") and basename.endswith(".so"):
        return basename[3:-3]
    else:
        return None

def create_project(options):
    subprocess.check_call([
        options.android_tool, "create", "project",
        "-p", options.path,
        "-n", options.name,
        "-k", options.package,
        "-a", options.activity,
        "-t", options.target
        ])

def touch_manifest(options):
    filename = options.path + "/AndroidManifest.xml"
    ET.register_namespace("android",
        "http://schemas.android.com/apk/res/android")
    tree = ET.parse(filename)
    root = tree.find(".")
    ET.SubElement(root, "uses-sdk", {
        "android:minSdkVersion": "12"
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
    tree.write(filename, "utf-8", xml_declaration=True)

def touch_strings(options):
    filename = options.path + "/res/values/strings.xml"
    tree = ET.parse(filename)
    app_name = tree.find("string[@name='app_name']")
    app_name.text = options.name
    tree.write(filename, "utf-8", xml_declaration=True)

def create_localgen_properties(options):
    filename = options.path + "/localgen.properties"
    f = open(filename, "w")
    f.write("jar.libs.dir={0}\n".format(options.jar_libs_dir))
    f.close()

def create_custom_rules_xml(options):
    filename = options.path + "/custom_rules.xml"
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

def create_activity(options):
    filename = "{0}/src/{1}/{2}.java".format(options.path,
        slashy(options.package), options.activity)
    f = open(filename, "w")
    if options.stl:
        load_stl_stmt = load_library_stmt(options.stl)
    else:
        load_stl_stmt = ""
    stmts = "\n\t\t".join([
        maybe_load_library_stmt(lib) for lib in options.load_lib
    ])
    load_app = os.path.basename(options.load_app)
    f.write('''\
        package {PACKAGE};
        public class {ACTIVITY} extends org.liballeg.android.AllegroActivity {{
            static {{
                {LOAD_STL_STMT}
                {STMTS}
            }}
            public {ACTIVITY}() {{
                super("{LOAD_APP}");
            }}
        }}
    '''.format(
        PACKAGE=options.package,
        ACTIVITY=options.activity,
        LOAD_STL_STMT=load_stl_stmt,
        STMTS=stmts,
        LOAD_APP=load_app
    ))
    f.close()

def maybe_load_library_stmt(filename):
    basename = os.path.basename(filename)
    name = name_for_load_library(basename)
    if name:
        return load_library_stmt(name)
    else:
        # Android can't load this anyway.
        return "/* ignored {0} */".format(basename)

def load_library_stmt(name):
    return 'System.loadLibrary("{0}");'.format(name)

def slashy(s):
    return s.replace(".", "/")

def create_jni(options):
    jni_path = options.path + "/jni"
    mkdir(jni_path)

    application_mk_path = jni_path + "/Application.mk"
    f = open(application_mk_path, "w")
    f.write("APP_ABI := {0}\n".format(options.app_abi))
    if options.stl:
        f.write("APP_STL := {0}\n".format(options.stl))
    f.close()

    android_mk_path = jni_path + "/Android.mk"
    load_app_relpath = unix_path(os.path.relpath(options.load_app, jni_path))

    f = open(android_mk_path, "w")
    f.write('''
        LOCAL_PATH := $(call my-dir)

        include $(CLEAR_VARS)
        LOCAL_MODULE := {NAME}
        LOCAL_SRC_FILES := {LOAD_APP_RELPATH}
        LOCAL_SHARED_LIBRARIES := {STL}
        include $(PREBUILT_SHARED_LIBRARY)
    '''.format(
        NAME=options.name,
        LOAD_APP_RELPATH=load_app_relpath,
        STL=options.stl
    ))
    for load_lib in options.load_lib:
        f.write(prebuilt_shared_lib_block(load_lib, jni_path))
    f.close()

def prebuilt_shared_lib_block(load_lib, jni_path):
    name = os.path.basename(load_lib)
    relpath = unix_path(os.path.relpath(load_lib, jni_path))
    return '''
        include $(CLEAR_VARS)
        LOCAL_MODULE := {NAME}
        LOCAL_SRC_FILES := {RELPATH}
        include $(PREBUILT_SHARED_LIBRARY)
    '''.format(
        NAME=name,
        RELPATH=relpath
    )

    # Always use Unix directory separator for paths in makefiles.
def unix_path(path):
    if os.path == '/':
        return path
    else:
        return '/'.join(path.split(os.sep))

def mkdir(path):
    if not os.path.exists(path):
        os.mkdir(path)

if __name__ == "__main__":
    main()

# vim: set sts=4 sw=4 et:
