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

lcov --rc lcov_branch_coverage=1 --directory . --capture --output-file lcov.info
lcov --rc lcov_branch_coverage=1 --remove lcov.info '*/thirdparty/*' --output lcov.info
lcov --rc lcov_branch_coverage=1 --remove lcov.info '*cpm_packages*' --output lcov.info
lcov --rc lcov_branch_coverage=1 --remove lcov.info '*valgrind*' --output lcov.info
lcov --rc lcov_branch_coverage=1 --remove lcov.info '*/ut/*' --output lcov.info

if [ "$TRAVIS" == "true" ]; then
  # deploy
  bash <(curl -s https://codecov.io/bash)
else
  # generate html localy
  mkdir -p doc/coverage
  mv lcov.info doc/coverage/
  pushd doc/coverage
  genhtml --rc genhtml_branch_coverage=1 --function-coverage --branch-coverage lcov.info
  echo
  echo Open file://$(pwd)/index.html in your browser for the coverage report
  popd >/dev/null
fi
