#!/usr/bin/env python3

"""Configure, compile and test dfk in all possible configurations
"""

from __future__ import print_function
import argparse
import sys
import uuid
import os
import os.path
import stat
import shutil
import subprocess
import random
import platform
from datetime import datetime
from multiprocessing import Pool, Queue, Manager

CMAKE_OPTIONS = {
    "CMAKE_BUILD_TYPE": ("Debug", "Release"),
    "DFK_NAMED_FIBERS": ("ON", "OFF"),
    "DFK_DEBUG": ("ON", "OFF"),
    "DFK_MOCKS": ("ON", "OFF"),
    "DFK_THREADS": ("ON", "OFF"),
    "DFK_HTTP_PIPELINING": ("ON", "OFF"),
    "DFK_LIST_CONSTANT_TIME_SIZE": ("ON", "OFF"),
    "DFK_LIST_MEMORY_OPTIMIZED": ("ON", "OFF"),
    "DFK_AVLTREE_CONSTANT_TIME_SIZE": ("ON", "OFF"),
    "DFK_URLENCODING_HINT_HEURISRICS": ("ON", "OFF"),
    "DFK_STACK_ALIGNMENT": ("16", "64"),
    # Temporary disable those, until build is broken
    "DFK_BUILD_CPP_BINDINGS": ("OFF",),
}

BUILD_SCRIPT = (
"#!/usr/bin/env bash\n"
"set -e\n"
"cd {build_dir}\n"
"cat <<EOT > config.cmake\n"
"{parameters}\n"
"EOT\n"
"cmake -Cconfig.cmake {source_dir}\n"
"make \n"
"make test\n"
)

def all_configurations(options):
    if not options:
        return
    name, values = options[0]
    for val in values:
        params = {name: val}
        tail_is_empty = True
        for tail in all_configurations(options[1:]):
            tail_is_empty = False
            tail.update(params)
            yield tail
        if tail_is_empty:
            yield params

def typeofparam(name, value):
    if value in ["ON", "OFF"]:
        return "BOOL"
    return "STRING"

CMAKE_CONFIG_FORMAT="set({key} {value} CACHE {type} \"\")"

def format_parameters(params, formatstr, joinsep=os.linesep):
    lines = [formatstr.format(key=k, value=v, type=typeofparam(k, v)) \
            for k, v in params.items()]
    return joinsep.join(lines)

def run_job(queue, njob, totaljobs, parameters, source_dir):
    jobid = str(uuid.uuid4())
    scriptwd = os.path.join("/tmp", jobid)
    scriptsh = os.path.join(scriptwd, "run.sh")
    try:
        os.makedirs(scriptwd)
        with open(scriptsh, "w+") as script:
            script.write(BUILD_SCRIPT.format(
                build_dir=scriptwd,
                parameters=format_parameters(parameters, CMAKE_CONFIG_FORMAT),
                source_dir=source_dir))
            os.fchmod(script.fileno(), stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
        fstdout = os.path.join(scriptwd, "stdout")
        fstderr = os.path.join(scriptwd, "stderr")
        subprocess.check_call([scriptsh], stdout=open(fstdout, "w+"),
                stderr=open(fstderr, "w+"))
    except Exception as ex:
        queue.put((njob, ex))
    finally:
        shutil.rmtree(scriptwd, ignore_errors=True)
    queue.put((njob, None))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--randomize", type=str, metavar='seed',
            default=platform.uname().node,
            help=("reorder configurations randomly, "
                  "using seed for initializing random numbers generator"))
    parser.add_argument("-j", "--jobs", metavar='N', default=1, type=int,
            help="allow N parallel jobs")
    parser.add_argument("-s", "--skip", metavar='COUNT', default=None, type=int,
            help="skip first COUNT jobs")
    args = parser.parse_args()
    script_dir = os.path.dirname(os.path.realpath(__file__))
    source_dir = os.path.realpath(os.path.join(script_dir, os.path.pardir))
    paramlist = sorted([(k, v) for k, v in CMAKE_OPTIONS.items()])
    configurations = list(all_configurations(paramlist))
    random.seed(args.randomize)
    random.shuffle(configurations)
    configurations = configurations[args.skip:]
    pool = Pool(processes=args.jobs)
    m = Manager()
    queue = m.Queue()
    jobs = [(queue, i, len(configurations), c, source_dir) \
            for i, c in enumerate(configurations, start=1)]
    pool.starmap_async(run_job, jobs)
    start_time = datetime.now()
    for i in range(1, len(jobs) + 1):
        njob, err = queue.get()
        time_remaining = (len(jobs) - i) * (datetime.now() - start_time) / i
        sys.stdout.write("\r{}/{} jobs complete. Time remaining: {}".format(
            i, len(jobs), time_remaining))
        sys.stdout.flush()
        if err:
            config = configurations[njob]
            params_strlist = ["{} = {}".format(k, v) for k, v in config.items()]
            indent = "  "
            print()
            print("Failed with parameters:")
            print(indent + format_parameters(config, "{key} = {value}",
                os.linesep + indent))
            print("Error:", err)
            print()
            print("Run to reproduce:")
            print()
            print("cmake",
                    format_parameters(config, "-D{key}:{type}={value}", " "),
                    source_dir, "&& make -j && make test")
            print()
            return 1
    sys.stdout.write(os.linesep)
    pool.terminate()
    return 0

if __name__ == "__main__":
    sys.exit(main())
