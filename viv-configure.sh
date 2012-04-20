#!/bin/sh

#Build Configuration
#-------------------
export LDFLAGSVIV="/vivante/sdk/drivers -lGAL -lm -ldl"
export CFLAGS='-g -I/vivante/sdk/include -L${LDFLAGSVIV}'

./autogen.sh --prefix=/usr --libdir '/usr/lib'  --disable-static

echo "End of configuration"

