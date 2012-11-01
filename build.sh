#!/bin/sh -e

cd EXA
. ./viv-configure.sh
make
cd ../DRI_1.10.4
. ./viv-configure.sh
make
cd ..
