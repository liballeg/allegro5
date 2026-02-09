How to Make a Release
---------------------

This guide explains how to make an Allegro release. This all assumes that we're
on Linux.

1.  First, build the library with the addons, and run the
    `./misc/compare_abi.sh` to check if ABI compatibility has been maintained
    (see README_abi.txt). Fix any regressions and along the way make sure that
    the newly added public APIs are correctly classified as unstable and are
    documented.

2.  Starting with the master branch, pick a commit which you want to be the base of
    the release branch. Typically this will be HEAD, but if there are some
    underbaked commits there, you may want to use an earlier commit. Call this
    branch with the new release name (e.g. 5.2.2 in this case):

        git branch 5.2.2 master

    From now on, aside from changing the version numbers, all commits on this
    release branch should be cherry-picked from the master branch. For all
    cherry-picks, prefer using the -x switch, like so:

        git cherry-pick -x badf00d

3.  On the master branch, bump the version to the next release in
    `include/allegro5/base.h` by using the `misc/fixver.sh` script. Commit this
    change. For example:

        misc/fixver.sh 5 2 3 GIT

4.  Write a changelog file. This is located in docs/src/changes-5.2.txt.

    Typically you will want to look through the commits made since the last
    release, e.g. using `git log <last_release>..<this_release>` (e.g. `git log
    5.2.1..5.2.2`). Follow the format of the previous changelogs. Commit this
    change.

5.  Generate the new ABI files using `./misc/generate_abi.sh`. Commit the
    changes.

6.  We are now done with the master branch. Check out the release branch now.

7.  Cherry-pick the commit with the changelog and the ABI changes onto this
    branch.

8.  Remove the "GIT" suffix and increase the version by 1. This can be done via
    `misc/fixver.sh` script. Commit this change. For example:

       misc/fixver.sh 5 2 2 0

9.  Tag the previous commit with the same version number and the release number
    (e.g. "5.2.2.0" if you're releasing 5.2.2. An example command would be:

        git tag -a -m "Tag 5.2.2.0" 5.2.2.0

10. Create the source archives by running `misc/create_release_archives.sh` and
    passing in the release version. This will create 3 source archives (.tar.gz,
    .7z and .zip) in the current directory. An example invocation would be:

        misc/create_release_archives.sh 5.2.2.0

11. At this point you could do some additional checks (like making binaries).

12. If all checks are good, push the master and release branches to github
    (with the --tags option).

13. Upload the source archives to github. Go to the releases tab, and make a
    new release with the tag you just created.

14. Build the docs, including the pdf. Add these to the website via the
    liballeg.github.io repository.

15. Make an announcement on the website. This involves making a news item,
    changing the download area and copy-pasting the change list.

16. Make an announcement on Discord. You're done!
