# HOW TO Release navitia

## Versioning

Our versionning is based on [Semantic Versionning](nhttps://semver.org/)
* the **major** version is bumped in case of an API/interface change or **when a binarisation is needed**.
* the **minor** version is bumped when functionnalities are backward compatible.
* the **patch** version is bumped on a bug fix.

## "Regular" release

This lets the script decide if it's major or minor release.
The decision is based on the data_version located in source/type/data.cpp
See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how data_version is managed.

First have a look on github's repo at PRs about to be released https://github.com/hove-io/navitia/pulls?q=is%3Apr+is%3Aclosed+sort%3Aupdated-desc
* Check that `not_in_changelog` and `hotfix` labels are correct and none is missing on PRs that are gonna be released
* Check that titles are correct (clear, with the component impacted)

Then the script should take over:
```sh
cd <path/to/repo/navitia>
# to be sure to trigger the correct version of the release script
git fetch <hove_git_remote_name> && git rebase <hove_git_remote_name>/dev dev
```
At this point, you may build and run tests to check that everything is OK. If you're confident, proceed with:
```sh
./release_navitia.sh <hove_git_remote_name>
```
Note: this script uses `python2.7`, `pipenv` and `vim`, make sure it's installed on your machine.

Then follow the instructions given by the script, and also (see below for troubleshooting):
* pay attention to the changelog, remove useless PR (small doc) and check that every important PR is there
* don't forget to make `git submodule update --recursive` when necessary
* check that `release` branch COMPILES and TESTS (unit, docker and tyr) before pushing it!
* you might want to disable `pre-commit` since `black` requires a recent version of `pre-commit` which is not available on Python 2.7
	* `pre-commit uninstall`
	* comment the line that installs `pre-commit` in `release_navitia.sh`

Nota: `major` and `minor` invocations are possible but deprecated.

## For hotfix

Note: It is preferable but not mandatory to merge the hotfix PR before.
```sh
./release_navitia.sh <hove_git_remote_name>
```
A new branch has been created <release_x.yy.z> and the changelog is opened.
Then the process is less automated (but still, instructions are given):
* Populate the changelog :
	Add the hotfix PR name and link to github (as for the automated changelog in the release process)
* Cherry-pick the commits you want to release:
	```
	git cherry-pick <commit_id> # Each commit_id of the hotfix PR
	```
* Merge the content of the new release branch with the hotfix commits to the 'release' branch:
	```
	git checkout release
	git merge --no-ff <release_x.yy.z>
	```
* Tag the new release:
	```
	git tag -a vx.yy.z
	```
    _Minor_: You will have to populate the tag with correct blank lines if you want a nice github changelog:
    ```
    Version 2.57.0

        * Kraken: Add ptref shortcut between physical_mode and jpps  <https://github.com/hove-io/navitia/pull/2417>
    ```

## And Finally

* Merge the 'release' branch to the 'dev' branch:
	```
	git checkout dev
	git merge --ff release
	```
* Push the release and dev branches to the repo
	```
	git push <hove_git_remote_name> release dev --tags
	```

# Troubleshooting
Note that if the release process fails in the middle, here are a few tips that might help before launching it again:

- check you're on `dev` branch
- check the branch `release_X.X.X` doesn't exist (might have been created by a previous execution); **or else** remove it
- check that no files is modified; **or else** `git restore --staged --worktree .`
- if you run into Github's daily limitation, you can easily provide your login/token into the script, search for "rate limit exceeded" in script.
