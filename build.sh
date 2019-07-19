#!/bin/bash

set -x

SOURCE_DIR=`pwd`

if [ ! -f "build" ]; then
    mkdir build
fi

cd build \
    && cmake $SOURCE_DIR \
    && make

cd $SOURCE_DIR
if [ ! -d "bin" ]; then
    mkdir bin
fi

cp build/src/webserver bin