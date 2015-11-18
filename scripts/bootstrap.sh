#!/usr/bin/env bash
# Bootstrap thirdparty modules, which are not built inside dfk project
# and are used as external dependencies. The script is supposed to be
# used by dfk developers which don't want to install extra libraries
# into their system, or by CI system.
# The script is not intended for distribution developers - libraries
# from the distribution should be used in this case instead of
# bootstrap'ed ones.

ROOT=$(pwd -P)
PREFIX=$ROOT/cpm_packages

# Check for required programs
for cmd in wget tar make; do
  echo -n "Checking for $cmd ... "
  if command -v $cmd >/dev/null 2>&1; then
    echo "OK"
  else
    echo "not found. Ensure that $cmd is available in \$PATH"
    exit 1;
  fi;
done

mkdir -p $PREFIX/tarball
mkdir -p $PREFIX/src
mkdir -p $PREFIX/include
mkdir -p $PREFIX/lib
mkdir -p $PREFIX/build

# ---------- libuv ----------
LIBUV_VERSION=1.7.5
LIBUV_SOURCE_DIR=$PREFIX/src/libuv-$LIBUV_VERSION
LIBUV_BUILD_DIR=$PREFIX/build/libuv-$LIBUV_VERSION
LIBUV_TARBALL=$PREFIX/tarball/libuv-v$LIBUV_VERSION.tar.gz
LIBUV_URL=https://github.com/libuv/libuv/archive/v$LIBUV_VERSION.tar.gz

[ -e $LIBUV_TARBALL ] || wget --continue -O $LIBUV_TARBALL $LIBUV_URL
[ -e $LIBUV_SOURCE_DIR ] || tar xzf $LIBUV_TARBALL -C $PREFIX/src
cd $LIBUV_SOURCE_DIR
./autogen.sh
mkdir -r $LIBUV_BUILD_DIR && cd $LIBUV_BUILD_DIR
CFLAGS=-fPIC CPPFLAGS=-fPIC ./configure --prefix=$PREFIX --enable-shared=no --enable-static=yes
make -j
make install
