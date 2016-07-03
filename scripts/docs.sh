#!/usr/bin/env bash

function build {
  VERSION=$(git describe --tags  --long)
  ( cat Doxyfile ; echo "PROJECT_NUMBER=$VERSION" ) | doxygen -
}

if [ "$TRAVIS" == "true" ]; then
  if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
    exit 0;
  fi

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
