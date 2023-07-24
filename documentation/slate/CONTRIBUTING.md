# Contributing to the documentation

Navitia.io documentation is built using [Slate](https://github.com/slatedocs/slate).

These sources are published to [http://doc.navitia.io/](http://doc.navitia.io/).

## Editing documentation content

All documentation is in `includes` directory, and uses Markdown text format.

## Preview changes

Once some changes have been done, Slate needs to build HTML sources from Markdown files.

### Using GitHub Action locally

This approach allows to test the GitHub Action in the same time.

Install [Act tool](https://github.com/nektos/act), then run

``` shell
act -W .github/workflows/publish_navitia_documentation.yml
```

The last steps will not work, that's normal, the secrets are not given but the website build should work.

### Using docker directly

Run this container sharing your folder (this allows to have the same context than in the GitHub Action)

``` shell
docker run --rm -it -v $(pwd):/work --entrypoint bash slatedocs/slate:latest
```

Then run these commands (same commands are run in the GitHub Action)

``` shell
cp -r /work/documentation/slate/source /srv/slate
./slate.sh build
rm -rf /work/documentation/slate/build
cp -r /srv/slate/build /work/documentation/slate
```

### Result

Both methods will generate a `build/` folder with an `index.html` inside, which is the doc.

Just open `documentation/slate/build/index.html` to see how it will be published.
