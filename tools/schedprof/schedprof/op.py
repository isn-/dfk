#!/usr/bin/env python
# -*- coding: utf-8 -*-

def cpu(ns):
    return ('cpu', ns)

def lock(mutex):
    return ('lock', mutex)

def unlock(mutex):
    return ('unlock', mutex)

def io(ns):
    return ('io', ns)

def spawn(coro):
    # We assume that spawning a new coroutine takes 1 ns
    return ('spawn', 1, coro)

def terminate(coro):
    # We assume that cleaning up a coroutine takes 1 ns
    return ('terminate', 1, coro)
