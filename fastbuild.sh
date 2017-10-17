#!/bin/bash
#
#
# options
# [XSERVER_GREATER_THAN_13=1] [BUSID_HAS_NUMBER=1] [clean] [install]
# for install, use "sudo ./fastbuild.sh install". Default path: /usr. 
#   To change to other place, use "prefix=<absolute dir>"
#
# for kernel 3.5.7, use "BUSID_HAS_NUMBER=1"
# for yocto, use "YOCTO=1"

# --------------------------------------------------------------------------------
# Cross build:
# --------------------------------------------------------------------------------
# if xserver version is 1.14, export this variable:
# bash>export XSERVER_GREATER_THAN_13=1
# build and install:
# bash>export PATH=<toolchain/bin>:$PATH
# bash>export CROSS_COMPILE=<toolchain prefix>
# bash>./fastbuild.sh sysroot=<system root> [BUILD_HARD_VFP=1] [YOCTO=1] [BUSID_HAS_NUMBER=1]
# bash>./fastbuild.sh [prefix=<absolute path to install>] install

# --------------------------------------------------------------------------------
# Native build:
# --------------------------------------------------------------------------------
# if xserver version is 1.14, export this variable:
# bash>export XSERVER_GREATER_THAN_13=1
# build and install:
# bash>./fastbuild.sh [BUILD_HARD_VFP=1] [YOCTO=1] [BUSID_HAS_NUMBER=1]
# bash>sudo ./fastbuild.sh install

make -C EXA/src/ -f makefile.linux $* || exit 1
if [ "$XSERVER_GREATER_THAN_13" != "1" ]; then
  make -C DRI_1.10.4/src/ -f makefile.linux $* || exit 1
fi
make -C FslExt/src/ -f makefile.linux $* || exit 1
make -C util/autohdmi/ -f makefile.linux $* || exit 1
make -C util/pandisplay/ -f makefile.linux $* || exit 1

exit 0

