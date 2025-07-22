# HOW TO Release navitia

## Versioning

Our versionning is based on [Semantic Versionning](https://semver.org/)
* The **major** version is bumped in case of an API/interface change or **when a binarisation is needed**.
  If the `data_version` located in `source/type/data.cpp` changes, then binarisation is needed (See
  [CONTRIBUTING.md](CONTRIBUTING.md) for details on how `data_version` is managed).
* the **minor** version is bumped when functionnalities are backward compatible.
* the **patch** version is bumped on a bug fix.

## Releasing

First have a look on github's repo at PRs about to be released, using
[recently merged PRs](https://github.com/hove-io/navitia/pulls?q=is%3Apr+is%3Aclosed+sort%3Aupdated-desc)
and commits merged since `<latest_version>` (replace with [latest tag](https://github.com/hove-io/navitia/releases)):
[https://github.com/hove-io/navitia/compare/`<latest_version>`...dev](https://github.com/hove-io/navitia/compare/<latest_version>...dev)
* Adjust/check that `not_in_changelog` and `hotfix` labels are correct and none is missing on PRs that are gonna be released
* Adjust/check that titles are correct (typo, clarity, with the component impacted)

Then publish the release on Github:
* In [Github's release page](https://github.com/hove-io/navitia/releases), click `Draft a new release`
* Tag to create and version names are going to be the same: `vX.Y.Z`
* Click `Generate release notes` and review the page, then publish.
* :heavy_check_mark: Let the CI do its work, and draft some PRs to deploy
  components of this new version everywhere necessary.
