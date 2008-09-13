#!/usr/bin/env python
import optparse, subprocess

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

def main():
    p = optparse.OptionParser()
    p.add_option("-u", "--user", help = "Username to use.")
    options, args = p.parse_args()

    sf = "shell.sf.net"
    if options.user: sf = "%s@shell.sf.net" % options.user
    htdocs = "/home/groups/a/al/alleg/htdocs"

    def run(cmd):
        p = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE,
            stderr = subprocess.STDOUT)
        print ">", p.stdout.read()
    
    version = get_real_version()
    if version:
        old_version = fix_version_in_menu_txt(version, "public/Menu.txt")
        fix_version_in_menu_txt(version, "private/Menu.txt")
        if old_version:
            print "Fixed version from", old_version, "to", version

    print "Creating archives.."
    run("tar cjf private.tar.bz2 -C private html")
    run("tar cjf public.tar.bz2 -C public html")
    print "Uploading archives.."
    run("scp private.tar.bz2 " + sf + ":" + htdocs)
    run("scp public.tar.bz2 " + sf + ":" + htdocs)
    print "Replacing docs.."
    com = "cd " + htdocs
    com += " ; rm -r naturaldocs_internal"
    com += " ; mkdir naturaldocs_internal"
    com += " ; cd naturaldocs_internal"
    com += " ; tar xjf ../private.tar.bz2"
    com += " ; mv html/* . ; rmdir html"
    com += " ; rm ../private.tar.bz2"
    com += " ; cd .."
    com += " ; rm -r naturaldocs"
    com += " ; mkdir naturaldocs"
    com += " ; cd naturaldocs"
    com += " ; tar xjf ../public.tar.bz2"
    com += " ; mv html/* . ; rmdir html"
    com += " ; rm ../public.tar.bz2"
    run("ssh " + sf + " '" + com + "'")
    print "Cleaning up.."
    run("rm private.tar.bz2")
    run("rm public.tar.bz2")
    print("Public docs: http://www.liballeg.org/naturaldocs")
    print("Internal docs: http://www.liballeg.org/naturaldocs_internal")
    
if __name__ == "__main__":
    main()

