#!/usr/bin/env bash
#

find include src ut -name *.c -o -name *.h | xargs clang-format -i -style=file
