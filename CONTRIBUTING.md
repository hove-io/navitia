# Contributing

## Python formatting
Python source code in this project is formatted using [Black](https://black.readthedocs.io/en/stable/)
You should enable the pre-commit git hook to make sure it's being run before commiting your changes, it's also the easiest way to run Black. Otherwise, to only update the files that you've changed, simply run:
```
pre-commit run black
```
If you want to run it on the whole project, you can add `--all`:
```
pre-commit run black --all
```
You can also [install Black traditionally](https://black.readthedocs.io/en/stable/installation_and_usage.html)
But bare in mind, it requires python 3.6+ to run.


## Git hooks
The project provides a few git hooks that you should use to prevent any issue.
The hooks are managed by [pre-commit](https://pre-commit.com/) that you need to [install](https://pre-commit.com/#install).
Then, install the hooks with:
```
pre-commit install
```
