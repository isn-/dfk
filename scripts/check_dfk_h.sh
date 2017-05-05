#!/usr/bin/env bash
# The script checks that <dfk.h> and <dfk.hpp> are not included anywhere
# in the dfk sources.
#
# Header files <dfk.h> and <dfk.hpp> are for external usage only. To increate
# compilation speed and keep #include's meaningful, including these headers
# directly inside dfk sources is prohibited.

srcdir="$1"
grep -HIrn 'include *<dfk\.h\(pp\)\?>' \
  --exclude-dir samples \
  --exclude-dir cpm_packages \
  --exclude-dir doc \
  --exclude-dir thirdparty \
  --exclude-dir build \
  --include '*.c' \
  --include '*.h' \
  --include '*.cpp' \
  --include '*.hpp' ${srcdir} && exit 1
