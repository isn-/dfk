#!/usr/bin/env python
# -*- coding: utf-8 -*-

from abc import ABCMeta, abstractmethod
from schedprof.coroutine import Coroutine
from schedprof.cpu import CPU, State


class Scheduler(object, metaclass=ABCMeta):
    def __init__(self):
        self._now = 0
        self.cpus = []
        self.coros = []
        self._idle_cputime = 0
        self._context_switches = 0
        self._cache_hits = 0

    def status_string(self):
        stats = [i.state.value + ("  " if i.coro is None else "{0:02}".format(i.coro.n)) for i in self.cpus]
        ret = "Now is {}\n".format(self._now)
        ret += "  CPU stats:\n"
        ret += "  {0:3}: {1}".format(self._now, (" ".join(stats)).strip())
        idle_coroutines = [str(i) for i in self.coros if i._cpu is None]
        if idle_coroutines:
            ret += "\n  Idle coroutines:\n    "
            ret += "  ".join([str(i) for i in idle_coroutines])
        return ret

    @abstractmethod
    def map_(self):
        """Scheduler function.

        Returned list of (CPU, Coroutine) tuples means what coroutine particular
        CPU should execute next.
        Subclass can use self.cpus, self.coros methods to make a decision.
        Also Coroutine.peek_instruction() function might be useful.
        """
        pass

    def run_program(self, program, ncpu):
        assert isinstance(program, Coroutine)
        self._now = 0
        self._idle_cputime = 0
        self._context_switches = 0
        self._cache_hits = 0
        self.coros = [program]
        self.cpus = [CPU() for _ in range(ncpu)]
        while True:
            # Finalize completed tasks
            for cpu in filter(lambda cpu: not cpu.coro is None and cpu.due <= self._now, self.cpus):
                cpu.suspend()
            # Map pending coros to CPUs by doing actual scheduling
            # Scheduler.map_() method should be implemented in the subclass
            schedule = self.map_()
            # Ensure returned CPUs are ready to accept new tasks
            for cpu, _ in schedule:
                if cpu.burning(self._now):
                    raise RuntimeError("Coroutine was mapped to burning CPU")
            # Fetch next instruction from coroutine, assign CPU according to the
            # schedule and compute execution due time
            for cpu, coro in schedule:
                task = coro.pop_instruction()
                # Skip mutex operations, not implemented yet
                while not task is None and task[0] in ['unlock', 'lock']:
                    task = coro.pop_instruction()
                if task is None:
                    # Coroutine has terminated
                    self.coros.remove(coro)
                if task[0] == 'spawn':
                    spawned_coro = task[2]
                    self.coros.append(spawned_coro)
                if task[0] == 'terminate':
                    coro_to_kill = task[2]
                    self.coros.remove(coro_to_kill)
                if task[0] in ['spawn', 'io', 'cpu', 'terminate']:
                    self._context_switches += 1
                    cachehit = cpu.wakeup(coro, State.RUNNING, self._now + task[1])
                    if cachehit:
                        self._cache_hits += 1
            if all([cpu.idle(self._now) for cpu in self.cpus]) and len(self.coros) == 0:
                # All CPUs are idle and there are no pending coros -
                # program reached it's end
                break
            # Nearest point in future that may required re-scheduling
            nextnow = min([cpu.due for cpu in self.cpus if cpu.due > self._now])
            # Compute CPU idle time
            nidlecpus = len([None for cpu in self.cpus if cpu.idle(self._now)])
            self._idle_cputime += nidlecpus * (nextnow - self._now)
            # Jump to the next scheduling point
            self._now = nextnow
        total_cputime = len(self.cpus) * self._now
        burning_cputime = total_cputime - self._idle_cputime
        return (self._now, total_cputime, burning_cputime, self._context_switches, self._cache_hits)


class DumbScheduler(Scheduler):
    """Maps first N idle coroutines to first M idle CPU resulting
    into min(N, M) scheduled tasks.
    """
    def map_(self):
        idle_cpus = [i for i in self.cpus if i.idle(self._now)]
        idle_coros = [i for i in self.coros if i._cpu is None]
        return list(zip(idle_cpus, idle_coros))


def print_stat(stat):
    print("Elapsed time: {}\nTotal CPU time: {}\nBurning CPU time: {}\nContext switches: {}\nCache hits: {}".format(*stat))
    print("Cache hit rate: {:.2%}".format(stat[4] / stat[3]))
    print("CPU utilization: {:.2%}".format(stat[2] / stat[1]))
    print("Parallel speedup: {:.4}".format(stat[2] / stat[0]))
