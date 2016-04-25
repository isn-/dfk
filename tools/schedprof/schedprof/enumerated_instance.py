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
        if not "_instance_counter" in holder.__dict__:
            holder._instance_counter = 0
        self.__n = holder._instance_counter
        holder._instance_counter += 1

    @property
    def n(self):
        """int: Instance unique number"""
        return self.__n

    def __repr__(self):
        return "<{} {}>".format(self.__class__.__name__, self.n)

    @classmethod
    def reset_instance_counter(cls):
        """Reset instance counter to zero.

        Newly created instance will obtain number starting from zero.
        Already created instance are not affected.

        Examples:
            >>> class Foo(EnumeratedInstance):
            ...    def __init__(self):
            ...        super(Foo, self).__init__(Foo)
            >>> foo = Foo()
            >>> print(foo)
            <Foo 0>
            >>> Foo.reset_instance_counter()
            >>> bar = Foo()
            >>> print(bar)
            <Foo 0>
            >>> print(foo)
            <Foo 0>
        """
        cls._instance_counter = 0


if __name__ == "__main__":
    import doctest
    doctest.testmod()
