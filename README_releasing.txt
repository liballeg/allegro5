How to Make a Release
---------------------

This guide explains how to make an Allegro release. This all assumes that we're
on Linux.

1.  Starting with the master branch, pick a commit which you want to be the base of
    the release branch. Typically this will be HEAD, but if there are some
    unbaked commits there, you may want to use an earlier commit. Call this
    branch with the new release name (e.g. 5.2.2 in this case):

        git branch 5.2.2 master

    From now on, aside from changing the version numbers, all commits on this
    release branch should be cherry-picked from the master branch. For all
    cherry-picks, prefer using the -x switch, like so:

        git cherry-pick -x badf00d

2.  On the master branch, bump the version to the next release in
    `include/allegro5/base.h` by using the `misc/fixver.sh` script. Commit this
    change. For example:

        misc/fixver.sh 5 2 3 GIT

3.  Write a changelog file. This is located in docs/src/changes-5.2.txt.

    Typically you will want to look through the commits made since the last
    release, e.g. using `git log <last_release>..<this_release>` (e.g. `git log
    5.2.1..5.2.2`). Follow the format of the previous changelogs. It's up to
    you how to determine who are the 'main developers'. For the 5.1.9+ release,
    I've abitrarily determined it to be developers who've committed 95% of the
    lines of code (this isn't very important). You probably will want to have
    other developers check it over in case something is wrong/missing. Commit
    this change.

4.  We are now done with the master branch. You can push these changes to
    github. Check out the release branch now.

5.  Cherry-pick the commit with the changelog onto this branch.

6.  Remove the "GIT" suffix and increase the version by 1. This can be done via
    `misc/fixver.sh` script. Commit this change. For example:

       misc/fixver.sh 5 2 2 0

7.  Tag the previous commit with the same version number and the release number
    (e.g. "5.2.2.0" if you're releasing 5.2.2. An example command would be:

        git tag -a -m "Tag 5.2.2.0" 5.2.2.0

8.  Create the source archives by running `misc/create_release_archives.sh` and
    passing in the release version. This will create 3 source archives (.tar.gz,
    .7z and .zip) in the current directory. And example invocation would be:

        ./misc/create_release_archives.sh 5.2.2.0

10. At this point you could do some additional checks (like making binaries).

11.  Upload the source archives to github. Go to the releases tab, and make a
    new release with the tag you just created.

12. Build the docs, including the pdf. Add these to the website via the
    liballeg.github.io repository.

13. Make an announcement on the website. This involves making a news item,
    changing the download area and copy-pasting the change list.

14. Make an announcement on Discord. You're done!
