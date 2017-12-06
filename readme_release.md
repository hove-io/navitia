# HOWTO Release navitia

You should first have a proper python env:
```
pip install -r requirements_release.txt -U
```

Then for a "Normal" release (minor):
```
cd <path/to/repo/navitia>
# to be sure to launch the correct version of the release script
git fetch <canaltp_distant_repo_name> && git rebase <canaltp_distant_repo_name>/dev dev
python ./script_release.py minor <canaltp_distant_repo_name>
```
Then follow the instructions given by the script, and also:
* pay attention to the changelog, remove useless PR (small doc) and check that every important PR is there
* don't forget to make `git submodule update` when necessary
* check that `release` branch COMPILES and TESTS (unit, docker and tyr) before pushing it!

## Other releases

For a major release, same as minor, but major:
```
python ./script_release.py major <canaltp_distant_repo_name>
```

For hotfix:
```
python ./script_release.py hotfix <canaltp_distant_repo_name>
```
Then the process is less automated (but still, instructions are given):
* you have to populate the changelog
* and the TAG yourself
* also cherry-pick the commits you want to release


# Troubleshooting
If you run into github's daily limitation, you can easily provide your login/token into the script.
Search for "rate limit exceeded" in script.
