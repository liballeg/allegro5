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
    p.add_option("--jar-libs-dir",
            default="",
            help="Path to directory containing JARs.")

    (options, args) = p.parse_args()
    if options.path == None:
        options.path = options.name
    if options.load_app == None:
        options.load_app = options.name
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
    f = open(filename, "w")
    stmts = "\n\t\t".join([
        'System.loadLibrary("{0}");'.format(lib)
        for lib in options.load_libs.split()
    ])
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

if __name__ == "__main__":
    main()

# vim: set sts=4 sw=4 et:
