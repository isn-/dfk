#!/usr/bin/env bash

headers=$(find src include -name *.h)
sources=$(find src include -name *.c)
ut_headers=$(find ut -name *.h)
ut_sources=$(find ut -name *.c)

echo Library lines of code: $(cat $headers $sources | wc -l)
echo Tests lines of code: $(cat $ut_headers $ut_sources | wc -l)
echo Assertions in library: $(grep assert\( $headers $sources | wc -l)
echo Unit tests: $(grep -P "^TEST(_F)?\(" $ut_headers $ut_sources | wc -l)
