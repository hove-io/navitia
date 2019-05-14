#!/bin/sh

cd release
pipenv --bare install
pipenv --bare run pre-commit install
echo ""
echo '### Installation Completed - Running release script ! ###'
pipenv --bare run python ./script_release.py $*
pipenv --rm
