
# How do you run Navitia's tests suite:

## Requirements
 * You have already setup your environment and built Navitia following the [installation documentation](https://github.com/hove-io/navitia/blob/dev/install.rst).

## How to run the tests

From the your build directory, run the command:

```sh
$ make test
```

This will trigger all unit and integration tests for the different modules (c++ and python).


## What's happening during a `make test`

To understand what's going on under the hood, enable cmake's verbose output:

```sh
$ VERBOSE=1 make test
```

The first few lines should tell you that `ctest` is being invoked

```sh
Running tests...
/usr/bin/ctest --force-new-ctest-process
Test project /home/user/build/navitia/release
      Start  1: utils_test
```

This means that `make test` is an alias to `ctest`.


## How to run an individual test ?

`ctest` has some interesting options:
 * `-V` to enable verbose output
 * `-N` to disable the actual tests execution
 * `-R <regex>` to only run tests matching the regex

To get the actual command line used for `jormungandr_serpy` test for instance, you can run:

```sh
$ ctest -V -N -R serpy$
```

This should give you:
 * the binary being executed
 * the options being used with the binary
 * the environment variables you need to set

