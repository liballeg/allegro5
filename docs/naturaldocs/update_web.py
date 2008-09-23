#!/usr/bin/env python
import optparse, subprocess, sys

def fix_version_in_menu_txt(real_version, menu_txt):
    try:
        f = open(menu_txt)
    except OsError:
        return None
    version = None
    output = ""
    for line in f:
        if line.startswith("SubTitle: Version"):
            version = line[17:].strip()
            if version == real_version: return None
            output += "SubTitle: Version %s\n" % real_version
        else:
            output += line
    f.close()
    if version:
        f = open(menu_txt, "w")
        f.write(output)
        f.close()
    return version

def get_real_version():
    for line in open("../../include/allegro5/base.h"):
        if line.startswith("#define ALLEGRO_VERSION_STR"):
            quote1 = line.find('"') + 1
            quote2 = line.find('"', quote1)
            return line[quote1:quote2]
    return None

def main(argv):
    p = optparse.OptionParser()
    p.add_option("-u", "--user", help = "Username to use.")
    options, args = p.parse_args()

    if options.user: sf = "%s,alleg@web.sf.net" % options.user
    else:
        sys.stderr.write("SourceForge user name required.\n")
        p.print_help()
        sys.exit(-1)
    htdocs = "/home/groups/a/al/alleg/htdocs"

    def run(cmd):
        print ">", cmd
        p = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE,
            stderr = subprocess.STDOUT)
        print ">", p.stdout.read()
    
    version = get_real_version()
    if version:
        old_version = fix_version_in_menu_txt(version, "public/Menu.txt")
        fix_version_in_menu_txt(version, "private/Menu.txt")
        if old_version:
            print "Fixed version from", old_version, "to", version

    print "Generating docs.."
    run("make")

    print "Copying files.."
    rsync = "rsync --delete -r -z"
    run("%s public/html/* %s:htdocs/naturaldocs/" % (rsync, sf))
    run("%s private/html/* %s:htdocs/naturaldocs_internal/" % (rsync, sf))
    
    print("Public docs: http://www.liballeg.org/naturaldocs")
    print("Internal docs: http://www.liballeg.org/naturaldocs_internal")
    
if __name__ == "__main__":
    main(sys.argv)

