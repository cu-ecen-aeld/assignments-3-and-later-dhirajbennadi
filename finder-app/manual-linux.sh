#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    #git show

    echo "Dhiraj Bennadi: Kernel Build Steps"
    # TODO: Add your kernel build steps here
    # Dhiraj
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

#Dhiraj
mkdir bin dev etc home lib lib64 proc sbin sys temp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

#sudo env "PATH=$PATH:/home/dhiraj/DhirajBennadi/Fall2022/AESD/A2/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin"
#sudo --preserve-env=PATH env [/home/dhiraj/DhirajBennadi/Fall2022/AESD/A2/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin]

#sudo env "PATH=$PATH" /home/dhiraj/DhirajBennadi/Fall2022/AESD/A2/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin

sudo env "PATH=$PATH"

echo "*************Sudo Path Command Successful**************"
echo ""

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

echo "*******************************"
echo "Dhiraj Bennadi: Library Dependencies"

cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
# Copy libraries
#${CROSS_COMPILE}-gcc -print-sysroot
export PATH2=$(${CROSS_COMPILE}gcc -print-sysroot)
echo $PATH2
echo "***************Dhiraj Bennadi*********"
ls

cp $PATH2/lib/ld-linux-aarch64.so.1 lib
cp $PATH2/lib64/libm.so.6 lib64
cp $PATH2/lib64/libresolv.so.2 lib64
cp $PATH2/lib64/libc.so.6 lib64

echo "***************Dhiraj Bennadi*********"


# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
pwd
#cd /home/dhiraj/DhirajBennadi/Fall2022/AESD/A3/assignment-2-dhirajbennadi_Part2/finder-app/
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
# cp finder.sh ${OUTDIR}/rootfs/home
# cp finder-test.sh ${OUTDIR}/rootfs/home
# cp writer.c ${OUTDIR}/rootfs/home
# cp writer.sh ${OUTDIR}/rootfs/home
# cp Makefile ${OUTDIR}/rootfs/home

cp -r * ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

cd ..
gzip -f initramfs.cpio
