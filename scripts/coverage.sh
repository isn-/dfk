#!/usr/bin/env bash
# Create test coverage report
#
# Prerequisite:
# * installed lcov,
# * dfk library built with DFK_COVERAGE=ON
#
# Can be run in two modes:
# * default (no special environment variables set) - create local html report
# * CI ("$TRAVIS" == "true") - upload coverage report to codecov.io

if [ "$TRAVIS" == "true" ]; then
  # lcov on Travis (ubuntu 12.04) does not recognize --rc option
  lcov_args=""
  genhtml_args=""
else
  lcov_args="--rc lcov_branch_coverage=1"
  genhtml_args="--rc genhtml_branch_coverage=1"
fi

lcov $lcov_args --directory . --capture --output-file lcov.info
lcov $lcov_args --remove lcov.info '*/thirdparty/*' --output lcov.info
lcov $lcov_args --remove lcov.info '*cpm_packages*' --output lcov.info
lcov $lcov_args --remove lcov.info '*valgrind*' --output lcov.info
lcov $lcov_args --remove lcov.info '*/ut/*' --output lcov.info

if [ "$TRAVIS" == "true" ]; then
  # deploy
  curl -s https://codecov.io/bash | bash -s - -X gcov
else
  # generate html localy
  mkdir -p doc/coverage
  mv lcov.info doc/coverage/
  pushd doc/coverage
  genhtml $genhtml_args --function-coverage --branch-coverage lcov.info
  echo
  echo Open file://$(pwd)/index.html in your browser for the coverage report
  popd >/dev/null
fi
