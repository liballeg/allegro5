Building
--------

mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-raspberrypi.cmake
make

Notes
-----

Building with a cross compiler is not yet documented. Building directly
on a Raspberry Pi or in a Raspberry Pi VM is very similar to building on
"regular" Linux. apt-get install the -dev packages you need, run cmake and
make sure everything you need is found.

