# Running Navitia suite

To get a full fledged Navitia suite, you'll need some of the following services:
* Tyr-web (Tyr Web API)
* Tyr-beat (Tyr Orchestrator)
* Tyr-worker (Tyr worker)
* Jormungandr (Navitia Web API)
* Kraken (Navitia Public Transport Engine)
* Postgresql (database shared between Tyr and Jormungandr)
* Postgresql (Cities database for Tyr admin regions)
* RabbitMq (Message key between Tyr and Kraken)
* Optionnal : Instance configurator (Helper to setup coverages, not a real service)

## Kraken Builder

1. Build docker image Kraken-builder
```sh
$ docker build -f docker/debian11/Dockerfile-builder-kraken  -t navitia/builder-kraken .
```
2. Build Kraken and its associated binaries (fusio2ed, poi2ed, ed2nav, etc...) by mounting your local repository 
```sh
$ docker run -it -v /your/github/navitia:/opt/navitia navitia/builder-kraken /opt/navitia
```

Once, this is built, you can use the docker-compose that follow.

## Docker-compose

To help you setup all those services together, one can use the docker-compose file that aggregate and configure a default coverage for you. 

To build and run the docker-compose instances, from Navitia root source directory:

First, edit the `docker/.env` file with your Github user/access-token

Build and run the docker-compose :

```sh
$ docker container prune --force  && docker-compose --env-file ./docker/.env -f ./docker/docker-compose.yml up --build
```

Once built, you should have 2 services available : 
 * Port `9898` : Tyr
 * Port `80` : Jormungandr

## Data ingestion

Send Data to Tyr (using [httpie](https://httpie.io/docs/cli/main-features)):

```sh
http -f POST :9898/v0/jobs/default file@/my/NTFS/fusio.zip
```

## Instance Configuration

Set configuration to Tyr :

```sh
http PUT :9898/v0/instances/default pt_planners_configurations:='{ "loki": { "args": { "timeout": 5 }, "klass": "jormungandr.pt_planners.loki.Loki", "data_source": "minio" } }'
```

## Jormungandr

Query Jormungandr :

```sh 
http :80/v1/coverage
```

Or use the Playground, targeting your local machine : https://playground.navitia.io/play.html?request=http%3A%2F%2Flocalhost%2Fv1%2Fcoverage%2Fdefault%2F%3F


