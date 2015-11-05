import os

flags = ['-x', 'c']
for (dirname, __, files) in os.walk(os.path.dirname(os.path.abspath(__file__))):
  for i in files:
    if i == 'flags.make':
      lines = open(os.path.join(dirname, i)).readlines()
      lines = filter(lambda x : x.startswith('CXX_FLAGS') or x.startswith('C_FLAGS') or x.startswith('CXX_DEFINES') or x.startswith('C_DEFINES'), lines)
      f = [a.split()[2:] for a in lines]
      flags += [i for j in f for i in j]

print flags

def FlagsForFile( filename, **kwargs ):
  return {
    'flags': flags,
    'do_cache': True
  }
