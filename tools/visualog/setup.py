from setuptools import setup

setup(
    name="dfk-visualog",
    setup_requires=['vcversioner'],
    author="Stanislav Ivochkin",
    author_email="isn@extrn.org",
    description=("DFK library log visualizer via plantuml"),
    license="BSD",
    url="https://github.com/isn-/dfk",
    packages=['visualog'],
    entry_points={
        "console_scripts": [
            "dfk-visualog = visualog.cli:main"
        ]
    }
)
