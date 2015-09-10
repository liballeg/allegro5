How to Make a Release
---------------------

This guide explains how to make an Allegro release. This all assumes that we're
on Linux.

1.  Write a changelog file. This is located in docs/src/changes-5.x.txt.

    Typically you will want to look through the commits made since the last
    release, e.g. using `git log <last_release_tag>..HEAD` (e.g.
    `git log 5.1.8..HEAD`). Follow the format of the previous changelogs. It's
    up to you how to determine who are the 'main developers'. For the 5.1.9
    release, I've abitrarily determined it to be developers who've committed 95%
    of the lines of code (this isn't very important). You probably will want to
    have other developers check it over in case something is wrong/missing.

2.  Commit the changelog changes.

3.  Bump the version from "GIT" to "WIP" if it's unstable, or remove "GIT" if
    it's stable, while preserving the major, minor and patch numbers, but
    increasing the release number to 1. Also, update the dates at this time.
    This is all done in `include/allegro5/base.h`. Commit this change.

4.  Tag the previous commit with the same version number (e.g. "5.1.9" if you're
    releasing 5.1.9. An example command would be:

        git tag -a -m "Tag 5.1.9 (WIP)" 5.1.9

5.  Create the source archives by running `misc/create_release_archives.sh` and
    passing in the release version. This will create 3 source archives (.tar.gz,
    .7z and .zip) in the current directory. And example invocation would be:

        ./misc/create_release_archives.sh 5.1.9

6.  Upload these archives to Sourceforge. Make a directory with the release
    number at https://sourceforge.net/projects/alleg/files/allegro (or
    allegro-unstable) and upload the 3 archives there.

7.  If this is a stable release, you also need to generate new documentation.
    Just run the build with documentation enabled, and then create a new
    directory in /home/project-web/alleg/htdocs_parts/staticweb/a5docs (this is
    Sourceforge FTP, at web.sourceforge.net:22). You also need to create a
    symlink to this new directory in /home/project-web/alleg/htdocs/a5docs (I
    believe logging into the shell is the best way to do this).

    The unstable documentation is currently automatically generated (courtesy of
    a cron script maintained by Elias), so if this is an unstable release,
    nothing needs to be done.

8.  Bump the version to the next release. E.g. "5.1.9 (WIP)" becomes
    "5.1.10 (GIT)". Make sure to also decrease the release number back to 0.
    This is all done in `include/allegro5/base.h`. Also, change ALLEGRO_VERSION
    in `CMakeLists.txt`. Commit this change. At this point you can push to
    sourceforge. Don't forget to push with `git push --tags`!

9.  Update website. For release announcement, add a new file to the en/news
    directory (in the allegrowww2 repository). Change the files in the changes
    or changes-unstable directory (as appropriate). Change the download links
    in en/downloads. Run the makefile and then upload the files in the OUT
    directory to /home/project-web/alleg/htdocs_parts/generatedweb (again,
    Sourceforge FTP). Commit after running the makefile (as it updates some
    files).

10. Make an announcement on allegro.cc. You're done!
