#!/usr/bin/env python
import optparse, subprocess, sys, os

def main(argv):
    global options
    p = optparse.OptionParser()
    p.add_option("-p", "--path", help = "Path to the build directory.")
    p.add_option("-u", "--user", help = "Username to use.")
    p.add_option("-d", "--directory", help = "Remote directory to " +
        "use. Should be either 'trunk' which is the default or else " +
        "the version number.", default = "trunk")
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

    destdir = "/home/groups/a/al/alleg/htdocs_parts/staticweb/a5docs"
    destdir += "/" + options.directory

    def run(cmd):
        print ">", cmd
        return subprocess.call(cmd, shell = True,
            stdout = subprocess.PIPE,
            stderr = subprocess.STDOUT)

    print "Copying files.."
    rsync = "rsync --delete -r -z"
    path = os.path.join(options.path, "docs/html/refman/")
    retcode = run("%s %s %s:%s" % (rsync, path, sf, destdir))
    if retcode == 0:
	print("Updated A5 docs at: http://docs.liballeg.org")
    sys.exit(retcode)

if __name__ == "__main__":
    main(sys.argv)

