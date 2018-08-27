# Contributing

## Python formatting
Python source code in this project is formatted with [Black](https://black.readthedocs.io/en/stable/)
You should enable the pre-commit git hook to be sure. It's also the easier way to run black, you can simply run:
```
pre-commit run black
```
This will only update file that you changed, if you want to run it on whole project you can add `--all`:
```
pre-commit run black --all
```
Obviously you can also [install Black traditionally](https://black.readthedocs.io/en/stable/installation_and_usage.html)
But attention: it require python 3.6+ to run.



## Git hooks
The project provide a few git hook that you should use to prevent any issues.
These hooks are managed by [pre-commit](https://pre-commit.com/)
that you need to [install](https://pre-commit.com/#install).
Then you can install the hooks with:
```
pre-commit install
```
