#!/bin/sh
export BINUTILS_VERSION=2.42
export GCC_VERSION=13.2.0
# Prepare
export PREFIX="$PWD/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p $PREFIX

tar xf binutils-${BINUTILS_VERSION}.tar.xz
tar xf gcc-${GCC_VERSION}.tar.xz

# Compile binutils
mkdir build-binutils
cd build-binutils
../binutils-${BINUTILS_VERSION}/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

# Compile GCC
cd ..
mkdir build-gcc
cd build-gcc
../gcc-${GCC_VERSION}/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j 8 all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc