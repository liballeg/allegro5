#!/usr/bin/env python
import optparse, subprocess, sys, os

def main(argv):
    global options
    p = optparse.OptionParser()
    p.add_option("-p", "--path", help = "Path to the build directory.")
    p.add_option("-u", "--user", help = "Username to use.")
    options, args = p.parse_args()

    if options.user:
        sf = "%s,alleg@web.sf.net" % options.user
    else:
        sys.stderr.write("SourceForge user name required (-u).\n")
        p.print_help()
        sys.exit(-1)
    
    if not options.path:
        sys.stderr.write("Build path required (-p).\n")
        p.print_help()
        sys.exit(-1)

    htdocs = "/home/groups/a/al/alleg/htdocs"

    def run(cmd):
        print ">", cmd
        p = subprocess.Popen(cmd, shell = True,
            stdout = subprocess.PIPE,
            stderr = subprocess.STDOUT)
        print ">", p.stdout.read()

    print "Copying files.."
    rsync = "rsync --delete -r -z"
    path = os.path.join(options.path, "docs/html/refman")
    run("%s %s %s:%s/a5docs/" % (rsync, path, sf, htdocs))

    print("Updated A5 docs at: http://docs.liballeg.org")

if __name__ == "__main__":
    main(sys.argv)

