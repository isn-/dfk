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
for cmd in wget tar make autogen; do
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
LIBUV_VERSION=1.9.1
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

# ---------- http-parser ----------
HTTP_PARSER_VERSION=2.7.1
HTTP_PARSER_SOURCE_DIR=$PREFIX/src/http-parser-$HTTP_PARSER_VERSION
HTTP_PARSER_BUILD_DIR=$PREFIX/build/http-parser-$HTTP_PARSER_VERSION
HTTP_PARSER_TARBALL=$PREFIX/tarball/http-parser-v$HTTP_PARSER_VERSION.tar.gz
HTTP_PARSER_URL=https://github.com/nodejs/http-parser/archive/v$HTTP_PARSER_VERSION.tar.gz

[ -e $HTTP_PARSER_TARBALL ] || wget --continue -O $HTTP_PARSER_TARBALL $HTTP_PARSER_URL
[ -e $HTTP_PARSER_SOURCE_DIR ] || tar xzf $HTTP_PARSER_TARBALL -C $PREFIX/src
cd $HTTP_PARSER_SOURCE_DIR
# CFLAGS replace built-in flags, while CPPFLAGS are appended
make CPPFLAGS=-fPIC -j package
cp libhttp_parser.a $PREFIX/lib
cp http_parser.h $PREFIX/include


# ---------- curl ----------
CURL_VERSION=7_50_1
CURL_SOURCE_DIR=$PREFIX/src/curl-curl-$CURL_VERSION
CURL_BUILD_DIR=$PREFIX/build/curl-$CURL_VERSION
CURL_TARBALL=$PREFIX/tarball/curl-$CURL_VERSION.tar.gz
CURL_URL=https://github.com/curl/curl/archive/curl-$CURL_VERSION.tar.gz

[ -e $CURL_TARBALL ] || wget --continue -O $CURL_TARBALL $CURL_URL
[ -e $CURL_SOURCE_DIR ] || tar xzf $CURL_TARBALL -C $PREFIX/src
cd $CURL_SOURCE_DIR
bash buildconf
./configure --prefix=$PREFIX --disable-ftp --disable-file --disable-ldap \
  --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet \
  --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp \
  --disable-gopher --disable-manual
make -j
make -j install
