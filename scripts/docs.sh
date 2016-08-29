#!/usr/bin/env bash
# Generate doxygen documentation

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
  ( cat Doxyfile ; echo "PROJECT_NUMBER=$VERSION" ; echo "HTML_OUTPUT=doc/html/internal" ; echo "INTERNAL_DOCS=YES" ) | $DOXYGEN -

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

  build

  git config --global user.email "travis@extrn.org"
  git config --global user.name "Travis CI"

  pushd doc/html
# Documentation is now available at https://dfk.extrn.org
#  echo dfk.extrn.org > CNAME
  git init
  git add .
  git commit -m "Deploy to Github Pages"
  git push --force --quiet "https://$GITHUB_TOKEN@github.com/$TRAVIS_REPO_SLUG.git" master:gh-pages > /dev/null 2>&1
  popd
  exit 0
else
  build
  echo
  echo Open $(pwd)/doc/html/index.html in your browser for the generated documentation
fi
