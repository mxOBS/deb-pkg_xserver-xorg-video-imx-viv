#!/bin/bash

export XACCVER=3.0.35-4.0.0
export DEBFILE=xorg-1.10.4-acc-viv-bin-$XACCVER.deb
echo "Creating xserver acceleration release package ..."
rm -r release
mkdir release || exit 1
mkdir release/DEBIAN || exit 1
mkdir -p release/usr/lib/xorg/modules/drivers/ || exit 1
mkdir -p release/usr/lib/xorg/modules/extensions/ || exit 1
cp EXA/src/vivante_drv.so release/usr/lib/xorg/modules/drivers/ &&  cp DRI_1.10.4/src/libdri.so release/usr/lib/xorg/modules/extensions/ || exit 1
echo "Package: XAcc" > release/DEBIAN/control
echo "Version: $XACCVER" >> release/DEBIAN/control
echo "Architecture: armel" >> release/DEBIAN/control
echo "Description: Xserver acceleration with Vivante gpu drivers" >> release/DEBIAN/control
dpkg -b release $DEBFILE || exit 1
echo "Package $DEBFILE is generated"
rm -r release
exit 0

