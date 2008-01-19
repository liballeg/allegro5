#!/usr/bin/env python
import re, sys, os, pysvn, shutil

"""Simple helper script to create the release .zip, .tar.gz and .tar.bz2."""

def get_version():
    """Find the version."""
    regexp = re.compile('^#define AGL_VERSION_STR "(.*?)".*$', re.M)
    match = regexp.search(file("include/alleggl.h").read())
    return match.group(1).replace(" ", "_").lower()

def get_files():
    """Find all SVN files to include in the release."""

    client = pysvn.Client()
    files = client.status(".", recurse = True, get_all = True)

    return [f.path for f in files if f.is_versioned and f.entry and
        f.entry.kind == pysvn.node_kind.file]

if __name__ == "__main__":
    version = get_version()
    dist_path = os.path.join("dist", "alleggl")
    release_name = "alleggl-" + version

    print "Copying distribution files.."
    shutil.rmtree("dist", True)
    os.mkdir("dist")
    os.mkdir(dist_path)

    files = get_files()

    # copy source files
    for f in files:
        target = os.path.join(dist_path, f)
        target_dir = os.path.dirname(target)
        try: os.makedirs(target_dir)
        except OSError: pass

        shutil.copy2(f, target_dir)

    # run some stuff so the user will not have to
    print "Running build scripts.."
    os.chdir(dist_path)
    os.system("autoheader")
    os.system("autoconf")
    os.system("misc/mkalias.sh")
    os.system("rm autom4te.cache/ -fr");
    
    # docs
    print "Generating documentation.."
    os.chdir("docs")
    os.system("doxygen > /dev/null")

    # zip it up
    print "Generating archives"
    os.chdir("../..")
    os.system("zip -r '%s.zip' alleggl > /dev/null" % release_name)
    os.system("tar czf '%s.tar.gz' alleggl" % release_name)
    os.system("tar cjf '%s.tar.bz2' alleggl" % release_name)

    print "All done!"
    print "Make sure you have used an up-to-date and correctly tagged SVN tree!"
