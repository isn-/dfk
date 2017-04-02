#!/usr/bin/env bash

ROOT=$(pwd -P)
PREFIX=$ROOT/cpm_packages

WITH_CURL=1
WITH_HTTP_PARSER=1

REQUIRED_PROGRAMS="wget tar make"

function usage {
  cat <<EOT
Usage: bootstrap.sh [--with-curl] [--without-curl]
  [--with-http-parser] [--without-http-parser]

Options:
 --with-curl
 --without-curl
 --with-http-parser
 --without-http-parser

Description:
 Bootstrap thirdparty modules, which are not built inside dfk project
 and are used as external dependencies. The script is supposed to be
 used by dfk developers who don't want to install extra libraries
 into their system, or by CI system.

    +-------------------------------------------------------------+
    | The script is not intended for distribution developers -    |
    | libraries from the distribution should be used in this case |
    | instead of bootstrap'ed ones.                               |
    +-------------------------------------------------------------+
EOT
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
    ;;
    --with-curl)
      WITH_CURL=1
    ;;
    --without-curl)
      WITH_CURL=
    ;;
    --with-http-parser)
      WITH_HTTP_PARSER=1
    ;;
    --without-http-parser)
      WITH_HTTP_PARSER=
    ;;
    *)
      echo "Did not understand being called with '$@'"
      exit 1
    ;;
  esac
shift
done

if [[ $WITH_CURL ]]; then
  REQUIRED_PROGRAMS="$REQUIRED_PROGRAMS autoconf"
fi

# Check for required programs
for cmd in $REQUIRED_PROGRAMS; do
  echo -n "Checking for $cmd ... "
  if command -v $cmd >/dev/null 2>&1; then
    echo "OK"
  else
    echo "not found. Ensure that $cmd is available in \$PATH"
    exit 1;
  fi;
done


function version {
  echo "$@" | awk -F. '{ printf("%d%03d%03d%03d\n", $1,$2,$3,$4); }'
}
function version_gt() {
  test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1";
}

wget="wget --continue"
wget_version=$(wget --version | head -n1 | awk '{print $3}')
if [ $(version $wget_version) -ge $(version 1.17) ]; then
  wget="$wget --no-hsts"
fi

mkdir -p $PREFIX/tarball
mkdir -p $PREFIX/src
mkdir -p $PREFIX/include
mkdir -p $PREFIX/lib
mkdir -p $PREFIX/build

# ---------- http-parser ----------
if [[ $WITH_HTTP_PARSER ]] ; then

HTTP_PARSER_VERSION=2.7.1
HTTP_PARSER_SOURCE_DIR=$PREFIX/src/http-parser-$HTTP_PARSER_VERSION
HTTP_PARSER_BUILD_DIR=$PREFIX/build/http-parser-$HTTP_PARSER_VERSION
HTTP_PARSER_TARBALL=$PREFIX/tarball/http-parser-v$HTTP_PARSER_VERSION.tar.gz
HTTP_PARSER_URL=https://github.com/nodejs/http-parser/archive/v$HTTP_PARSER_VERSION.tar.gz

[ -e $HTTP_PARSER_TARBALL ] || $wget -O $HTTP_PARSER_TARBALL $HTTP_PARSER_URL
[ -e $HTTP_PARSER_SOURCE_DIR ] || tar xzf $HTTP_PARSER_TARBALL -C $PREFIX/src
cd $HTTP_PARSER_SOURCE_DIR
# CFLAGS replace built-in flags, while CPPFLAGS are appended
make CPPFLAGS=-fPIC -j package
cp libhttp_parser.a $PREFIX/lib
cp http_parser.h $PREFIX/include

fi

# ---------- curl ----------
if [[ $WITH_CURL  ]] ; then

CURL_VERSION=7_53_1
CURL_SOURCE_DIR=$PREFIX/src/curl-curl-$CURL_VERSION
CURL_BUILD_DIR=$PREFIX/build/curl-$CURL_VERSION
CURL_TARBALL=$PREFIX/tarball/curl-$CURL_VERSION.tar.gz
CURL_URL=https://github.com/curl/curl/archive/curl-$CURL_VERSION.tar.gz

[ -e $CURL_TARBALL ] || $wget -O $CURL_TARBALL $CURL_URL
[ -e $CURL_SOURCE_DIR ] || tar xzf $CURL_TARBALL -C $PREFIX/src
cd $CURL_SOURCE_DIR
bash buildconf
./configure --prefix=$PREFIX --disable-ftp --disable-file --disable-ldap \
  --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet \
  --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp \
  --disable-gopher --disable-manual
make -j
make -j install

fi
