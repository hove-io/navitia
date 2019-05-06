# Scripts description

## Dev environment installation

Aftermath the installation, it will be possible to :
 - Compile kraken
 - Run all integration and Unit tests
 - Run docker tests (tyr)
 - Run binaries to fill ed database (fusio2ed, osm2ed, ...)
 - Run ed2nav binary to generate data.nav.lz4 form the ed database
 - Run eitri (the docker method) to build data.nav.lz4 with your own NTFS and Osm files

Script                                   | OS                | Version                           | Comments
-----------------------------------------|-------------------|-----------------------------------|--------------------------
ubuntu_bionic_dev_env_install.sh         | Ubuntu            | Bionic                            | Complete dev environment installation on Ubuntu bionic LTS.
build_navitia.sh                         | Ubuntu<br>Debian  | Vivid - Xenial<br>Jessie          | Old script. Combine dev environment installation and Navitia demo

## Build, setup and run navitia demo

Do the following actions :
- build with cmake/make
- Download gtfs and osm ech
- Create data.nav.lz4
- Create conf files
- Run Kraken + Jormungandr

Script                                   | OS                | Version                           | Comments
-----------------------------------------|-------------------|-----------------------------------|---------------------------
build_setup_and_run_navitia_demo.sh      | Ubuntu<br>Debian  | Debian >= 8 - Ubuntu >= 15.04     | Needs as a prerequisite, a dev environment setup
build_navitia.sh                         | Ubuntu<br>Debian  | Vivid - Xenial<br>Jessie          | Old script. Combine dev environment installation and Navitia demo
