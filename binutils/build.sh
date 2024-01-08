#!/bin/bash

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

function fetch_and_build {
    pushd "$SCRIPTPATH"

    if [ "$1" == "-f" ]; then
        rm -rf ./build
    fi

    if [ -d ./build ]; then
        popd
        return
    fi

    rm -rf ./src

    version=2.41
    if [ ! -f binutils-$version.tar.gz ]; then
        wget https://ftp.gnu.org/gnu/binutils/binutils-$version.tar.gz || exit 1
    fi
    tar xzf binutils-$version.tar.gz || exit 1
    mv ./binutils-$version ./src || exit 1

    cd src

    pushd libiberty || exit 1
    CFLAGS="-fPIC -O3" ./configure --enable-targets=all --without-zstd || exit 1
    make -j`nproc` || exit 1
    popd

    pushd zlib || exit 1
    CFLAGS="-fPIC -O3" ./configure --enable-targets=all --without-zstd || exit 1
    make -j`nproc` || exit 1
    popd

    pushd libsframe || exit 1
    CFLAGS="-fPIC -O3" ./configure --enable-targets=all --without-zstd || exit 1
    make -j`nproc` || exit 1
    popd

    pushd bfd || exit 1
    LD_LIBRARY_PATH="`pwd`/../libiberty:`pwd`/../zlib:`pwd`/../libsframe" CFLAGS="-fPIC -O3" \
        ./configure --enable-shared --enable-targets=all --without-zstd || exit 1
    make -j`nproc` || exit 1
    popd

    cd .. || exit 1
    mkdir build || exit 1
    cd build || exit 1

    cp "../src/zlib/libz.a" . || exit 1
    cp "../src/bfd/.libs/libbfd.a" . || exit 1
    cp "../src/libiberty/libiberty.a" . || exit 1
    cp "../src/libsframe/.libs/libsframe.a" . || exit 1

    cd ..
    rm -rf ./src
    popd
}

fetch_and_build $@
