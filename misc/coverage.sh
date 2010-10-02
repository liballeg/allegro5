#!/bin/sh -e
# Produce test coverage results using lcov.
# Run this from a dedicated build directory unless you don't mind your build
# flags being changed.

BUILDDIR=$(dirname $PWD)
JOBS=${JOBS:-4}

if ! test ../LICENSE.txt
then
    echo "Run this in a build subdirectory." 1>&2
    exit 1
fi

if ! which lcov 2>&1 >/dev/null
then
    echo "lcov is required." 1>&2
    exit 1
fi

# ccache can interfere with gcov by causing profiling data to end up in the
# ~/.ccache directory.
export CCACHE_DISABLE=1

cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS=--coverage \
    -DCMAKE_CXX_FLAGS=--coverage \
    -DCMAKE_EXE_LINKER_FLAGS=--coverage \
    -DWANT_POPUP_EXAMPLES=no \
    ..

lcov -d $BUILDDIR --zerocounters

make -j${JOBS} ex_blend_test
make -j${JOBS} ex_config
make -j${JOBS} ex_dir
make -j${JOBS} ex_path_test
make -j${JOBS} ex_utf8
make -j${JOBS} copy_example_data copy_demo_data

( cd examples
    ./ex_blend_test
    ./ex_config
    ./ex_dir
    ./ex_path_test
    ./ex_utf8
) || true

( cd tests
    make -j${JOBS} run_tests
) || true

lcov -d $BUILDDIR --capture --output allegro_auto.info

genhtml --legend allegro_auto.info --output-directory coverage
