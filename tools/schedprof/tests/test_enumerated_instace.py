#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import doctest
import schedprof.enumerated_instance


def load_tests(loader, tests, ignore):
    tests.addTests(doctest.DocTestSuite(schedprof.enumerated_instance))
    return tests


if __name__ == "__main__":
    sys.exit(unittest.main())
