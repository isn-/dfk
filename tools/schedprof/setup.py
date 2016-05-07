from setuptools import setup
setup(
    name="dfk-schedprof",
    setup_requires=['vcversioner'],
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
