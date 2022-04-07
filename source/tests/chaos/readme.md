# Chaos Tests

This aims at testing the Navitia's dependency on [Chaos](https://github.com/hove-io/chaos)

## How to run
Those tests are part of `docker_test` test suite, hence depend on the same requirements.
If you haven't set any environment just yet, create a new virtual env with :

```sh
pip install -r ../../tyr/requirements_dev.txt -r ../../eitri/requirements.txt
```

Then invoke from your build directory :
```sh
make chaos_tests
```
