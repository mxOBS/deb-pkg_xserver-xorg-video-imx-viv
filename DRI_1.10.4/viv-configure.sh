#!/bin/sh

#Build Configuration
#-------------------
export AQROOT=$PWD/../../..
export CFLAGS='-I${AQROOT}/build/sdk/include -I../../EXA/src/vivante_gal -g -Wa,-mimplicit-it=thumb -lm -ldl -ldrm -lX11'

./autogen.sh --prefix=/usr --libdir '/usr/lib'  --disable-static

echo "End of configuration"
