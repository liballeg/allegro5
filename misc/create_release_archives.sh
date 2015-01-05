#!/bin/sh
set -e

ver=$1

if [ -z "$ver" ]
then
    echo "Please pass a version."
    exit 1
fi

if git show $ver -- 2>&1 | grep -q 'fatal'
then
    echo "$ver is not a valid revision."
    exit 1
fi

export LC_ALL=en_US

repo=$PWD
tmpdir=$( mktemp -d )

git archive --format=tar --prefix=allegro/ ${ver} | ( cd $tmpdir && tar xf - )

( cd "$tmpdir/allegro"
    for file in $( find -type f -printf '%P\n' )
    do
        echo $file
        commit=$( cd $repo && git rev-list ${ver} "$file" | head -n 1 )
        mtime=$( cd $repo && git show --pretty=format:%ai $commit | head -n 1)
        touch -d "$mtime" "$file"
    done
)

( cd "$tmpdir/allegro"
    ./misc/zipup.sh allegro-$ver.zip
    mv .dist/*.* ..
)

(cd "$tmpdir"
    rm -rf allegro
    unzip allegro-$ver.zip
    ./allegro/misc/mkunixdists.sh allegro-$ver.zip
)

mv "$tmpdir"/*.* ./

rm -rf -- "$tmpdir"

echo "Done and done!"
