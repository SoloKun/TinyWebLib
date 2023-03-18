#!/bin/bash

set - e


SOURCE_DIR=`pwd`

if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

if [ ! -d `pwd`/include ]; then
    mkdir `pwd`/include
fi

if [ ! -d `pwd`/lib ]; then
    mkdir `pwd`/lib
fi

rm -fr ${SOURCE_DIR}/build/*
cd ${SOURCE_DIR}/build &&
cmake .. && make install


cp ${SOURCE_DIR}/include/tiny_network -r /usr/local/include 


cp ${SOURCE_DIR}/lib/libtiny_network.so /usr/local/lib


ldconfig
