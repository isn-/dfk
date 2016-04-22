#!/usr/bin/env python
# -*- coding: utf-8 -*-

from abc import ABCMeta, abstractmethod
from itertools import chain
from schedprof.enumerated_instance import EnumeratedInstance
from schedprof.op import terminate


class Coroutine(EnumeratedInstance, metaclass=ABCMeta):
    def __init__(self):
        super(Coroutine, self).__init__(Coroutine)
        self.task_queue = None
        self._head = None
        self._cpu = None

    @abstractmethod
    def run(self):
        pass

    def _run(self):
        return chain(self.run(), [terminate(self)])

    def pop_instruction(self):
        if self.task_queue is None:
            self.task_queue = self._run()
        if not self._head is None:
            head = self._head
            self._head = None
            return head
        return next(self.task_queue)

    def peek_instruction(self):
        if self._head is None:
            self._head = next(self.task_queue)
        return self._head
