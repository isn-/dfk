#!/usr/bin/env bash

DOXYGEN=doxygen

function build {
  CLEANUP="no"
  if [ -f build/dfk/config.h ]; then
    if [ ! -f include/dfk/config.h ]; then
      CLEANUP="yes"
      cp build/dfk/config.h include/dfk/
    fi
  fi
  VERSION=$(git describe --tags  --long)
  ( cat Doxyfile ; echo "PROJECT_NUMBER=$VERSION" ) | $DOXYGEN -

  if [ "$CLEANUP" == "yes" ]; then
    rm include/dfk/config.h
  fi
}

if [ "$TRAVIS" == "true" ]; then
  if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
    exit 0;
  fi

  # download and unpack latest doxygen release
  wget http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.11.linux.bin.tar.gz
  tar xzvf doxygen-1.8.11.linux.bin.tar.gz
  DOXYGEN="$(pwd)/doxygen-1.8.11/bin/doxygen"

  # fetch all commits and tags to make git describe working
  git fetch --unshallow

  build

  git config --global user.email "travis@extrn.org"
  git config --global user.name "Travis CI"

  pushd doc/html
  echo dfk.extrn.org > CNAME
  git init
  git add .
  git commit -m "Deploy to Github Pages"
  git push --force --quiet "https://$GITHUB_TOKEN@github.com/$TRAVIS_REPO_SLUG.git" master:gh-pages  > /dev/null 2>&1
  popd
  exit 0
fi

build
