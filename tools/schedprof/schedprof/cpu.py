#!/usr/bin/env python
# -*- coding: utf-8 -*-

from enum import unique, Enum
from schedprof.enumerated_instance import EnumeratedInstance


@unique
class State(Enum):
    """CPU State"""
    IDLE = "I"
    RUNNING = "R"
    SYSCALL = "D"


class CPU(EnumeratedInstance):
    """Attributes:
        state(State): CPU state
        due(int): Due time for the current task
        coro(Coroutine): Coroutine currently executed
    """
    def __init__(self):
        super(CPU, self).__init__(CPU)
        self.state = State.IDLE
        self.due = 0
        self.coro = None
        self._lastcoro = None

    def idle(self, at):
        """Returns True if CPU will not run any coroutine at the given point of time"""
        return self.coro is None or self.due <= at

    def burning(self, at):
        """Returns True if CPU will be running a coroutine at the given point of time"""
        return not self.idle(at)

    def suspend(self):
        self.due = 0
        self.state = State.IDLE
        self.coro._cpu = None
        self._lastcoro = self.coro
        self.coro = None

    def wakeup(self, coro, state, due):
        """Returns:
            bool: True if cache hit occured.
        """
        assert self.coro is None
        self.state = state
        self.due = due
        self.coro = coro
        coro._cpu = self
        return self.coro == self._lastcoro
