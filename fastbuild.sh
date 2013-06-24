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

make -C EXA/src/ -f makefile.linux $* || exit 1
make -C DRI_1.10.4/src/ -f makefile.linux $* || exit 1

exit 0

