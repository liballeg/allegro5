Building
--------

mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-raspberrypi.cmake
make

Virtual machine
---------------

Building with a cross compiler is not yet documented. Building directly
on a Raspberry Pi or in a Raspberry Pi VM is very similar to building on
"regular" Linux. apt-get install the -dev packages you need, run cmake and
make sure everything you need is found.

For example, you can use an image from:

    http://www.raspberrypi.org/downloads

and a kernel:

    http://xecdesign.com/downloads/linux-qemu/kernel-qemu

With a qemu command line like:

    qemu-system-arm -kernel kernel-qemu -cpu arm1176 -m 256 -M versatilepb \
        -no-reboot -serial stdio -append "root=/dev/sda2 panic=1" \
        -hda 2012-12-16-wheezy-raspbian.img \
        -redir tcp:2222::22 -net nic -net user

You can ssh into the VM with:

    ssh -p 2222 pi@127.0.0.1

Within the guest you can access the host at 10.0.2.2.
