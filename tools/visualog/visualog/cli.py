#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Script parses dfk debug log, matches fiber context switches by regexp
and outputs plantuml diagram that illustrates execution path.
Plantuml diagram source can be viewed either by using web service like [1],
or using a local plantuml app, that could be found at the official site [2].

Log messages are highlighed in the way that each fiber has
it's own unique color.

[1] http://www.planttext.com/planttext, http://plantuml.com/plantuml/form
[2] http://plantuml.com/

"""

from __future__ import print_function
import sys
import re
import hashlib
import fileinput


def main():
    print("@startuml")
    print("--> init")
    create_note = True
    current = "init"
    for line in fileinput.input():
        line = line.strip()
        if create_note:
            rgb = [min(255, i + 100) for i in hashlib.sha1(current.encode()).digest()[:3]]
            color = '#{:02x}{:02x}{:02x}'.format(*rgb)
            print("rnote over {} {}".format(current, color))
            create_note = False
        if line.count("context switch"):
            m = re.search('context switch \{([^\}]*)\} -> \{([^\}]*)\}', line)
            if not create_note:
                print("endrnote")
            from_, to = m.group(1), m.group(2)
            from_ = "init" if from_ == "(nil)" else from_
            to = "init" if to == "(nil)" else to
            print("{} -> {}".format(from_, to))
            create_note = True
            current = to
        else:
            print(line)
    if not create_note:
        print("endrnote")
    print("@enduml")


if __name__ == "__main__":
    sys.exit(main())
