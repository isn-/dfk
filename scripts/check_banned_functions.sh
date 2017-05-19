#!/usr/bin/env bash
# The script checks that banned function are not used in the dfk source code.
# A list of banned functions:
#
# |  Function        | Alternative    | Alternative header |
# | ---------------- | -------------- |------------------- |
# | dfk__io          | DFK_IO         | <dfk/internal.h>   |
# | dfk__resume      | DFK_RESUME     | <dfk/internal.h>   |
# | dfk__terminate   | DFK_TERMINATE  | <dfk/internal.h>   |
# | dfk__yielded     | DFK_YIELDED    | <dfk/internal.h>   |
# | dfk__this_fiber  | DFK_THIS_FIBER | <dfk/internal.h>   |
# | dfk__suspend     | DFK_SUSPEND    | <dfk/internal.h>   |
# | dfk__iosuspend   | DFK_IOSUSPEND  | <dfk/internal.h>   |
# | dfk__ioresume    | DFK_IORESUME   | <dfk/internal.h>   |
# | dfk__postpone    | DFK_POSTPONE   | <dfk/internal.h>   |
#

srcdir="$1"
banned=('dfk__io[^_a-zA-Z0-9]'
  dfk__resume
  dfk__terminate
  dfk__yielded
  dfk__this_fiber
  dfk__suspend
  dfk__iosuspend
  dfk__ioresume
  dfk__postpone
)

ret=0
for f in ${banned[*]}; do
  grep -HIrn "$f" \
    --exclude-dir cpm_packages \
    --exclude-dir doc \
    --exclude-dir thirdparty \
    --exclude-dir build \
    --include '*.c' \
    --include '*.h' \
    --include '*.cpp' \
    --include '*.hpp' ${srcdir}
  if [ $? -eq 0 ]; then ret=1; fi
done
exit $ret
