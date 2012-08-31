#!/bin/sh

#Build Configuration
#-------------------
export AQROOT=$PWD/../../..
export LDFLAGSVIV="${AQROOT}/build/sdk/drivers -lGAL -lm -ldl"
export CFLAGS='-DDISABLE_VIVANTE_DRI -I${AQROOT}/build/sdk/include -L${LDFLAGSVIV}'

./autogen.sh --prefix=/usr --libdir '/usr/lib'  --disable-static

echo "End of configuration"

