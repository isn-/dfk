#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys
import click
import multiprocessing
from schedprof.op import *
from schedprof.scheduler import print_stat, DumbScheduler
from schedprof.coroutine import Coroutine
from schedprof.mutex import Mutex


@click.group()
def cli():
    pass


@cli.command()
@click.option("--ncpu", default=multiprocessing.cpu_count())
@click.option("--scheduler", type=click.Choice(["dumb", "random", "fifo"]), default="dumb")
@click.option("--connections", default=32)
def demo(ncpu, scheduler, connections):
    class ConnectionCoroutine(Coroutine):
        def __init__(self, global_lock):
            super(ConnectionCoroutine, self).__init__()
            self.gl = global_lock

        def run(self):
            yield io(3)
            yield cpu(1)
            yield lock(self.gl)
            yield cpu(1)
            yield unlock(self.gl)
            yield io(5)
            yield cpu(6)
            yield lock(self.gl)
            yield cpu(1)
            yield unlock(self.gl)
            yield io(3)

    class MainCoroutine(Coroutine):
        def __init__(self, connections):
            super(MainCoroutine, self).__init__()
            self.connections = connections

        def run(self):
            global_lock = Mutex()
            yield cpu(10)
            maxconn = self.connections
            while maxconn > 0:
                yield io(3)
                yield spawn(ConnectionCoroutine(global_lock))
                yield cpu(1)
                maxconn = maxconn - 1

    if scheduler == "dumb":
        sched = DumbScheduler()
    else:
        raise ValueError("The scheduler is not implemented yet")
    stat = sched.run_program(MainCoroutine(connections), ncpu)
    print_stat(stat)


def main():
    return cli()


if __name__ == "__main__":
    sys.exit(main())
