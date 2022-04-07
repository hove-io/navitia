# Contributing


## Overview on how-to contribute

Dev branch: [![Last build](https://img.shields.io/github/workflow/status/hove-io/navitia/Build%20Navitia%20Packages%20For%20Dev?logo=github&style=flat-square)](https://github.com/hove-io/navitia/actions?query=workflow%3A%22Build+Navitia+Packages+For+Dev%22)

Fork the github repo, create a new branch from dev, and submit your pull request!

Make sure to run the tests before submitting the pull request (`make test` in the build directory,
you may also run `make docker_test` for important contributions), and have a wee look at what follows.

### Acceptance process and merge

For a proposed PR, we require 2 approvals by repository owners.
If you have the rights, once your PR has 2 approvals, you are encouraged to merge it yourself.


## Build Navitia

If you want to build navitia, please refer to the
[installation documentation](https://github.com/hove-io/navitia/blob/dev/install.rst).

You can install a full development environment with helping
[scripts](https://github.com/hove-io/navitia/tree/dev/scripts), depending on your OS.

Therefore, an
[automated navitia script](https://github.com/hove-io/navitia/blob/dev/scripts/build_setup_and_run_navitia_demo.sh)
is available to build, setup and run a navitia demo.
It's needed as a prerequisite for a dev environment setup.
Kraken is built into navitia_dir/build_release and all demo files are available in navitia_dir/run:
```
navitia_dir/run/data.nav.lz4
navitia_dir/run/kraken/kraken
navitia_dir/run/kraken/kraken.init
navitia_dir/run/kraken/kraken.log
navitia_dir/run/jormungandr/default.jon
navitia_dir/run/jormungandr/jormungandr_settings.jon
navitia_dir/run/jormungandr/venv_jormungandr/
```


## Code Organisation

At the root of the repository, several directories can be found:

1. source: contains the navitia source code (c++ and python)
2. documentation: all the navitia documentation
3. release: contains [script_release.py](https://github.com/hove-io/navitia/blob/dev/release/script_release.py) to run the release process
4. scripts: different useful scripts

### `data_version` management

The data_version stored into [source/type/data.cpp](source/type/data.cpp) is used to know if a given `nav` file
is readable using a given kraken (both contain their version).

So this number **must** be incremented when data serialization changes.<br>In case multiple changes occur between 2 releases, only one increment is enough.

This number is also used to tag major versions of Navitia.<br>Major version is tracked externally to know if nav-files must be regenerated.<br>So please update this `data_version` to indicate a need for re-binarization.


## Tools

* Gcc (or clang) as the C++ compiler (g++)
* CMake for the build system
* Python for the api


## Release a new version

Please follow instructions from [readme_release.md](readme_release.md).

## Git hooks

The project provides a few git hooks that you should use to prevent any issue.
The hooks are managed by [pre-commit](https://pre-commit.com/) that you need to
[install](https://pre-commit.com/#install).
Then, install the hooks with:
```
pre-commit install
```
Alternatively, you can configure a [template directory](https://pre-commit.com/#automatically-enabling-pre-commit-on-repositories). Whenever you clone a pre-commit enabled repository, the hooks will already be set up and you won't need to install pre-commit:
```
git config --global init.templateDir ~/.git-template
pre-commit init-templatedir ~/.git-template
```

### Python formatting

Python source code in this project is formatted using [Black](https://black.readthedocs.io/en/stable/)
You should enable the pre-commit git hook to make sure it's being run before commiting your changes, it's
also the easiest way to run Black.<br>Otherwise, to only update the files that you've changed, simply run:
```
pre-commit run black
```
If you want to run it on the whole project, you can add `--all`:
```
pre-commit run black --all
```
You can also [install Black traditionally](https://black.readthedocs.io/en/stable/installation_and_usage.html)
But bare in mind, it requires python 3.6+ to run.

### C++ formatting

Our pre-commit hooks are running [Clang-format](https://releases.llvm.org/6.0.0/tools/clang/docs/ClangFormat.html) to format our c++ codebase. You'll need version 6.0 in order to pass our CI.
```sh
sudo apt install clang-format-6.0
```

In case you might want to run Clang-format on the entire codebase, you can do:
```sh
pre-commit run clang-format-6.0 --all
```
