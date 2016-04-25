#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
from schedprof.coroutine import Coroutine, coromake
from schedprof.op import *


class CoroutineTest(unittest.TestCase):
    def test_pop(self):
        def func():
            yield cpu(1)
            yield io(3)
        coro = coromake(func)
        self.assertEqual(('cpu', 1), coro.pop_instruction())
        self.assertEqual(('io', 3), coro.pop_instruction())
        self.assertEqual(('terminate', 1, coro), coro.pop_instruction())

    def test_peek(self):
        def func():
            yield cpu(1)
        coro = coromake(func)
        self.assertEqual(('cpu', 1), coro.peek_instruction())
        self.assertEqual(('cpu', 1), coro.pop_instruction())
        self.assertEqual(('terminate', 1, coro), coro.peek_instruction())
        self.assertEqual(('terminate', 1, coro), coro.pop_instruction())

    def test_empty_coro(self):
        def func(): yield
        coro = coromake(func)
        self.assertEqual(('terminate', 1, coro), coro.pop_instruction())

    def test_suspend(self):
        def func():
            yield(cpu(10))
            yield(io(1))
        coro = coromake(func)
        instruction = coro.pop_instruction()
        coro.suspend(instruction)
        self.assertTrue(coro.suspended())
        with self.assertRaises(RuntimeError):
            coro.pop_instruction()
        coro.resume()
        self.assertEqual(('cpu', 10), coro.peek_instruction())
        self.assertEqual(('cpu', 10), coro.pop_instruction())


if __name__ == "__main__":
    sys.exit(unittest.main())
