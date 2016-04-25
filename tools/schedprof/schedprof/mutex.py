#!/usr/bin/env python
# -*- coding: utf-8 -*-

from schedprof.enumerated_instance import EnumeratedInstance


class Mutex(EnumeratedInstance):
    def __init__(self):
        super(Mutex, self).__init__(Mutex)
        self.acquired_by = None
        self.wait_queue = []
