#!/bin/bash
#
# options
# [NODRI=1] [SWAP_SINGLE_PARAMETER=1] [NEW_FBDEV_API=1] [BUSID_HAS_NUMBER=1] [clean] [install]
# examples:
# for clean, use "./fastbuild clean"
# for make, use "./fastbuild"
# for make non-dri version, use "./fastbuild NODRI=1"
# for install, use "sudo ./fastbuild.sh install". Default path: /usr. 
#   To change to other place, use "prefix=<absolute dir>"
#
# for xorg-1.14, use "SWAP_SINGLE_PARAMETER=1 NEW_FBDEV_API=1"
# for kernel 3.5.7, use "BUSID_HAS_NUMBER=1"
# for yocto, use "YOCTO=1"

# example: for Yocto imx_3.5.7_1.0.0_alpha release, command is
# ./fastbuild YOCTO=1 BUSID_HAS_NUMBER=1 && ./fastbuild install && sync
# Note: update when X Server is not running

# Cross build:
# ./fastbuild <CROSS_COMPILE=... sysroot=...> [other options]
# example:
# PATH=/opt/poky/1.4.1/sysroots/i686-pokysdk-linux/usr/bin:/opt/poky/1.4.1/sysroots/i686-pokysdk-linux/usr/bin/cortexa9hf-vfp-neon-poky-linux-gnueabi:$PATH ./fastbuild.sh CROSS_COMPILE=arm-poky-linux-gnueabi- sysroot=/opt/poky/1.4.1/sysroots/cortexa9hf-vfp-neon-poky-linux-gnueabi/ BUILD_HARD_VFP=1 YOCTO=1 BUSID_HAS_NUMBER=1 prefix=/home/zhenyong/zy/Gpu/share/test/
# PATH=/opt/poky/1.4.1/sysroots/i686-pokysdk-linux/usr/bin:/opt/poky/1.4.1/sysroots/i686-pokysdk-linux/usr/bin/cortexa9hf-vfp-neon-poky-linux-gnueabi:$PATH ./fastbuild.sh CROSS_COMPILE=arm-poky-linux-gnueabi- sysroot=/opt/poky/1.4.1/sysroots/cortexa9hf-vfp-neon-poky-linux-gnueabi/ BUILD_HARD_VFP=1 YOCTO=1 BUSID_HAS_NUMBER=1 prefix=/home/zhenyong/zy/Gpu/share/test/ install
 

make -C EXA/src/ -f makefile.linux $* || exit 1
make -C DRI_1.10.4/src/ -f makefile.linux $* || exit 1

exit 0

