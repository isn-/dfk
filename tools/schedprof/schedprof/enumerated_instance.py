#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Contains EnumeratedInstance mixin"""

class EnumeratedInstance(object):
    """Mixin that add instance counter to class.

    Each instance of EnumeratedInstance subclass is assigned a
    unique sequential number that can be later accessed via
    readonly `n' property.

    Args:
        holder(class): Class to store instance counter.

    Examples:
        >>> class Foo(EnumeratedInstance):
        ...    def __init__(self):
        ...        super(Foo, self).__init__(Foo)
        >>> foo = Foo()
        >>> print(foo.n)
        0
        >>> bar = Foo()
        >>> print(bar.n)
        1
        >>> print(bar)
        <Foo 1>
    """
    def __init__(self, holder):
        assert issubclass(holder, EnumeratedInstance)
        if not "instance_counter" in holder.__dict__:
            holder.instance_counter = 0
        self.__n = holder.instance_counter
        holder.instance_counter = holder.instance_counter + 1

    @property
    def n(self):
        """int: Instance unique number"""
        return self.__n

    def __repr__(self):
        return "<{} {}>".format(self.__class__.__name__, self.n)


if __name__ == "__main__":
    import doctest
    doctest.testmod()
