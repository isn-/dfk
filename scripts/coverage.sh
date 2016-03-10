#!/usr/bin/env bash

lcov --directory . --capture --output-file lcov.info
lcov --remove lcov.info '*/thirdparty/*' --output lcov.info
lcov --remove lcov.info '*/ut/*' --output lcov.info
bash <(curl -s https://codecov.io/bash)
