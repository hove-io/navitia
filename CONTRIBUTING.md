# Contributing


## Overview on how-to contribute

Dev branch: [![Last build](https://ci.navitia.io/job/navitia_dev/badge/icon)](https://ci.navitia.io/job/navitia_dev)

Fork the github repo, create a new branch from dev, and submit your pull request!

Make sure to run the tests before submitting the pull request (`make test` in the build directory,
you may also run `make docker_test` for important contributions), and have a wee look at what follows.


## Build Navitia

 If you want to build navitia, please refer to the
 [installation documentation](https://github.com/canaltp/navitia/blob/dev/install.rst).

 You can also check the [automated build script](https://github.com/canaltp/navitia/blob/dev/build_navitia.sh)
 which is meant to be a step by step tutorial for compiling and using navitia with ubuntu 16.04.


## Code Organisation

At the root of the repository, several directories can be found:

1. source: contains the navitia source code (c++ and python)
2. third_party: third party developped modules
3. documentation: all the navitia documentation
4. (debug|release): by convention, the build repositories


## Tools

* Gcc (or clang) as the C++ compiler (g++)
* CMake for the build system
* Python for the api


## Git hooks

The project provides a few git hooks that you should use to prevent any issue.
The hooks are managed by [pre-commit](https://pre-commit.com/) that you need to
[install](https://pre-commit.com/#install).
Then, install the hooks with:
```
pre-commit install
```

## Python formatting

Python source code in this project is formatted using [Black](https://black.readthedocs.io/en/stable/)
You should enable the pre-commit git hook to make sure it's being run before commiting your changes, it's
also the easiest way to run Black.  
Otherwise, to only update the files that you've changed, simply run:
```
pre-commit run black
```
If you want to run it on the whole project, you can add `--all`:
```
pre-commit run black --all
```
You can also [install Black traditionally](https://black.readthedocs.io/en/stable/installation_and_usage.html)
But bare in mind, it requires python 3.6+ to run.

## C++ formatting

Our pre-commit hooks are running [Clang-format](https://releases.llvm.org/6.0.0/tools/clang/docs/ClangFormat.html) to format our c++ codebase. You'll need version 6.0 in order to pass our CI.
```sh
sudo apt install clang-format-6.0
```

In case you might want to run Clang-format on the entire codebase, you can do:
```sh
pre-commit run clang-format-6.0 --all
```
