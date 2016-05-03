from __future__ import print_function
import os

def is_c_compiler_flag(line):
    return (line.startswith('C_FLAGS')
        or line.startswith('C_DEFINES')
        or line.startswith('C_INCLUDES'))

flags = ['-x', 'c']
for (dirname, __, files) in os.walk(os.path.dirname(os.path.abspath(__file__))):
  for i in files:
    if i == 'flags.make':
      lines = open(os.path.join(dirname, i)).readlines()
      lines = filter(lambda x : is_c_compiler_flag(x), lines)
      f = [a.split()[2:] for a in lines]
      flags += [i for j in f for i in j]

print(flags)

def FlagsForFile( filename, **kwargs ):
  return {
    'flags': flags,
    'do_cache': True
  }
