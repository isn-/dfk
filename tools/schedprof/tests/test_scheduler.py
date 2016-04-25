#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
from schedprof.scheduler import DumbScheduler
from schedprof.cpu import CPU
from schedprof.coroutine import Coroutine, coromake
from schedprof.mutex import Mutex
from schedprof.op import *


class CPUMonitor(object):
    def __init__(self):
        self.history = []

    def __call__(self, now, cpu, coro, task):
        self.history.append((now, str(cpu), str(coro), str(task)))


class TestDumbSchdeduler(unittest.TestCase):
    def setUp(self):
        Coroutine.reset_instance_counter()
        CPU.reset_instance_counter()
        self.mon = CPUMonitor()
        self.scheduler = DumbScheduler()

    def test_hello_world(self):
        def main():
            yield cpu(1)
            yield io(1)
            yield cpu(1)

        main = coromake(main)
        self.scheduler.run_program(main, 1, self.mon)
        self.assertListEqual(self.mon.history, [
            (0, "<CPU 0>", "<Coro 0>", "('cpu', 1)"),
            (1, "<CPU 0>", "<Coro 0>", "('io', 1)"),
            (2, "<CPU 0>", "<Coro 0>", "('cpu', 1)"),
            (3, "<CPU 0>", "<Coro 0>", "('terminate', 1, <Coro 0>)"),
        ])

    def test_two_threads(self):
        def threadmain():
            yield cpu(1)

        def main():
            yield spawn(coromake(threadmain))
            yield spawn(coromake(threadmain))

        main = coromake(main)
        self.scheduler.run_program(main, 2, self.mon)
        self.assertListEqual(self.mon.history, [
            (0, "<CPU 0>", "<Coro 0>", "('spawn', 1, <Coro 1>)"),
            (1, "<CPU 0>", "<Coro 0>", "('spawn', 1, <Coro 2>)"),
            (1, "<CPU 1>", "<Coro 1>", "('cpu', 1)"),
            (2, "<CPU 0>", "<Coro 0>", "('terminate', 1, <Coro 0>)"),
            (2, "<CPU 1>", "<Coro 1>", "('terminate', 1, <Coro 1>)"),
            (3, "<CPU 0>", "<Coro 2>", "('cpu', 1)"),
            (4, "<CPU 0>", "<Coro 2>", "('terminate', 1, <Coro 2>)"),
        ])

    def test_mutex(self):
        def threadmain(mutex):
            yield cpu(1)
            yield lock(mutex)
            yield io(10)
            yield unlock(mutex)

        def main():
            mutex = Mutex()
            yield spawn(coromake(threadmain, mutex))
            yield spawn(coromake(threadmain, mutex))

        main = coromake(main)
        self.scheduler.run_program(main, 2, self.mon)
        self.assertListEqual(self.mon.history, [
            (0, "<CPU 0>", "<Coro 0>", "('spawn', 1, <Coro 1>)"),
            (1, "<CPU 0>", "<Coro 0>", "('spawn', 1, <Coro 2>)"),
            (1, "<CPU 1>", "<Coro 1>", "('cpu', 1)"),
            (2, "<CPU 0>", "<Coro 0>", "('terminate', 1, <Coro 0>)"),
            (2, "<CPU 1>", "<Coro 1>", "('lock', 1, <Mutex 0>)"),
            (3, "<CPU 0>", "<Coro 1>", "('io', 10)"),
            (3, "<CPU 1>", "<Coro 2>", "('cpu', 1)"),
            (4, "<CPU 1>", "<Coro 2>", "('lock', 1, <Mutex 0>)"),
            (13, "<CPU 0>", "<Coro 1>", "('unlock', 1, <Mutex 0>)"),
            (14, "<CPU 0>", "<Coro 1>", "('terminate', 1, <Coro 1>)"),
            (14, "<CPU 1>", "<Coro 2>", "('lock', 1, <Mutex 0>)"),
            (15, "<CPU 0>", "<Coro 2>", "('io', 10)"),
            (25, "<CPU 0>", "<Coro 2>", "('unlock', 1, <Mutex 0>)"),
            (26, "<CPU 0>", "<Coro 2>", "('terminate', 1, <Coro 2>)"),
        ])


if __name__ == "__main__":
    sys.exit(unittest.main())
