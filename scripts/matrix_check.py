#!/usr/bin/env python

from __future__ import print_function
import sys
import uuid
import os
import os.path
import stat
import shutil
import subprocess


CMAKE_OPTIONS = {
    "CMAKE_BUILD_TYPE": ("Debug", "Release"),
    "DFK_NAMED_COROUTINES": ("ON", "OFF"),
    "DFK_DEBUG": ("ON", "OFF"),
    "DFK_MOCKS": ("ON", "OFF"),
    "DFK_THREADS": ("ON", "OFF"),
    "DFK_HTTP_PIPELINING": ("ON", "OFF"),
    "DFK_LIST_CONSTANT_TIME_SIZE": ("ON", "OFF"),
    "DFK_LIST_MEMORY_OPTIMIZED": ("ON", "OFF"),
    # Temporary disable those, until build is broken
    "DFK_BUILD_SAMPLES": ("OFF",),
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
"make -j\n"
"make test\n"
)

def all_parameters(options):
    if not options:
        return
    name, values = options[0]
    for val in values:
        params = {name: val}
        tail_is_empty = True
        for tail in all_parameters(options[1:]):
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

def run_job(parameters, source_dir):
    jobid = str(uuid.uuid4())
    scriptsh = os.path.join("/tmp", jobid + ".sh")
    scriptwd = os.path.join("/tmp", jobid)
    try:
        with open(scriptsh, "wa") as script:
            script.write(BUILD_SCRIPT.format(
                build_dir=scriptwd,
                parameters=format_parameters(parameters, CMAKE_CONFIG_FORMAT),
                source_dir=source_dir))
            os.fchmod(script.fileno(), stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
        os.makedirs(scriptwd)
        fstdout = os.path.join(scriptwd, "stdout")
        fstderr = os.path.join(scriptwd, "stderr")
        subprocess.check_call([scriptsh], stdout=open(fstdout, "wa"),
                stderr=open(fstderr, "wa"))
    finally:
        os.remove(scriptsh)
        shutil.rmtree(scriptwd, ignore_errors=True)

def main():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    source_dir = os.path.realpath(os.path.join(script_dir, os.path.pardir))
    paramlist = sorted([(k, v) for k, v in CMAKE_OPTIONS.items()])
    njobs = len(list(all_parameters(paramlist)))
    for i, params in enumerate(all_parameters(paramlist), start=1):
        try:
            print("Run {}/{} job ...".format(i, njobs))
            run_job(params, source_dir)
        except Exception as ex:
            params_strlist = ["{} = {}".format(k, v) for k, v in params.items()]
            indent = "  "
            print("Failed with parameters:")
            print(indent + format_parameters(params, "{key} = {value}",
                os.linesep + indent))
            print()
            print("Run to reproduce:")
            print()
            print("cmake",
                    format_parameters(params, "-D{key}:{type}={value}", " "),
                    source_dir, "&& make -j && make test")
            return 1

if __name__ == "__main__":
    sys.exit(main())
