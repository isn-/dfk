#!/usr/bin/env python
# -*- coding: utf-8 -*-

from abc import ABCMeta, abstractmethod
from itertools import chain
from schedprof.enumerated_instance import EnumeratedInstance
from schedprof.op import terminate


class Coroutine(EnumeratedInstance):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(Coroutine, self).__init__(Coroutine)
        self.task_queue = None
        self._head = None
        self._cpu = None
        self._suspended = False

    @abstractmethod
    def run(self):
        pass

    def _run(self):
        return filter(lambda x: x, chain(self.run(), [terminate(self)]))

    def pop_instruction(self):
        if self.suspended():
            raise RuntimeError("Coroutine is suspended. Call resume first.")
        if self.task_queue is None:
            self.task_queue = self._run()
        if not self._head is None:
            head = self._head
            self._head = None
            return head
        return next(self.task_queue)

    def peek_instruction(self):
        if self.task_queue is None:
            self.task_queue = self._run()
        if self._head is None:
            self._head = next(self.task_queue)
        return self._head

    def suspended(self):
        return self._suspended

    def ready(self):
        return not self.suspended() and self._cpu is None

    def suspend(self, instruction):
        assert self._head is None
        self._suspended = True
        self._head = instruction

    def resume(self):
        self._suspended = False


def coromake(entry_point, *args, **kwargs):
    class Coro(Coroutine):
        def __init__(self, func, *args, **kwargs):
            super(Coro, self).__init__()
            self._func = func
            self._args = args
            self._kwargs = kwargs

        def run(self):
            return self._func(*self._args, **self._kwargs)

    return Coro(entry_point, *args, **kwargs)
