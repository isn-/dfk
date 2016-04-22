import os
import os.path
import re
import subprocess
from setuptools import setup

def get_version_from_git():
    cwd = os.path.dirname(os.path.abspath(__file__))
    p = subprocess.Popen(["git", "describe", "--long", "--tags"], cwd=cwd, stdout=subprocess.PIPE)
    (stdin, _) = p.communicate()
    version = stdin.decode().strip()
    reg = re.compile(r'v([0-9]+)\.([0-9]+)\.([0-9]+)(-([0-9]+)-[0-9a-z]+)?')
    m = reg.match(version)
    revision = m.group(5) or 0
    version = "{0}.{1}.{2}.{3}".format(*m.group(1, 2, 3), revision)
    return version

setup(
    name="dfk-schedprof",
    version=get_version_from_git(),
    author="Stanislav Ivochkin",
    author_email="isn@extrn.org",
    description=("Profiler for DFK library thread scheduler"),
    license="BSD",
    url="https://github.com/isn-/dfk",
    packages=['schedprof'],
    entry_points={
        "console_scripts": [
            "dfk-schedprof = schedprof.cli:main"
        ]
    }
)
