#!/bin/bash
#
# options
# [NODRI=1] [clean] [install]
# examples:
# for clean, use "./fastbuild clean"
# for make, use "./fastbuild"
# for make non-dri version, use "./fastbuild NODRI=1"
# for install, use "sudo ./fastbuild.sh install". Default path: /usr. 
#   To change to other place, use "prefix=<absolute dir>"

make -C EXA/src/ -f makefile.linux $* || exit 1
make -C DRI_1.10.4/src/ -f makefile.linux $* || exit 1

exit 0

