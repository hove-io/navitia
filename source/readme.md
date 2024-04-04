# Source directory

Navitia is a mix of C++ components and python modules

Here is a quick overview of the different components

## Main components

### routing
The core of navitia, contains all public transport routing algorithm

:zap: **cpp module**

### ed
Collection of public transport data import tools

This module contains tool to read GTFS, Fusio (format used by Hove, derived from GTFS), Osm, ...

It also contains ed2nav, the tool to build all those data together for Kraken

:zap: **cpp module**

### georef
Handle the street network in kraken.

Contains the shortest path algorithms and some tools to explore the street network data (addresses, house numbers, ...)

:zap: **cpp module**

### jormungandr
Web service around the c++ core(s).

This is the navitia api front end, based on flask

:snake: **python module**

### kraken
Kraken is the c++ module behind navitia.

:zap: **cpp module**

### fare
Used to handle fares in kraken.

Fares are not taken into account during journey computation but as a postprocess.

:zap: **cpp module**

### type
Contains the navitia public transport c++ object model.

Also contains all protobuff definition.

:zap: **cpp module**

### tyr
Here stand the law and nothing but the law.

This module is the conductor of Navitia.

In a running environment, Tyr is responsible for detecting new data and integrating them in the system.

:snake: **python module**

### ptreferential
Module used to explore the public transport data.

It is this module that allow to query for all stops for a given lines and a given network

:zap: **cpp module**

### disruption
Chaos connector.

Chaos is the python web service which implements the realtime aspect of Navitia.
This connector loads Chaos data in navitia.

:zap: **cpp module**

### autocomplete
Tools for autocomplete

:zap: **cpp module**

### navitiacommon
navitia python model.

The directory will also contain the protobuf files from Navitia (python and cpp).
So you might need to use CMake target protobuf_files to generate them :
```
cd <path/to/build/directory>
cmake <path/to/navitia/repo>/source/
make protobuf_files
```

Shared by all python modules

:snake: **python module**


# Misc
### chaos-proto
Submodule containing chaos (real time python modules) protobuff definitions

### cities
Module used to fill a database with some OSM administrative regions.

The database will be used in ed2nav to find the administrative region of public transport objects outside the streetnetwork of the insances

> :warning: `cities` is very France centric OSM wise (only admins with level 8 are treated as a city etc...). If you plan on integrating admins from other countries, please refer to https://github.com/hove-io/cosmogony2cities.

:zap: **cpp module**

### calendar
Small module used to have to query the calendars.
Calendars are objects used in timetables.

:zap: **cpp module**

### cmake_modules
Directory with cmake modules


### monitor
Module to monitor kraken.

Kraken is monitored via zmq and this module is a web service to display the status

:snake: **python module**

### proximity_list
Contains tools for proximity list

proximity list are tools used to get objets around a location

It is for example used to get all stops around a geographical point

:zap: **cpp module**

### time_tables
Module to handle timetable

With this module you can query for the next departure from a stop point or for the schedule of a given route.

:zap: **cpp module**

### utils
Submodule containing some code helpers

:zap: **cpp module**

### scripts
Various bash scripts

:zap: **cpp module**

### sql
Contains alembic migrations for the Ed databases

### stat_persistor
Statistics module.

Persists Jormungandr calls in a database

:snake: **python module**

### tests
Contains kraken mock executable used in Jormungandr integration tests

### third_party
Various third party libraries

### lz4_filter
Module to handle lz4 compression format in boost::serialize

Kraken uses data serialized with boost::serialize (and compressed with lz4)

### validator
Public transport data validator

:zap: **cpp module**

### vptranslator
Work in progress

Module to transform a ValidityPattern (a year bitset) to a nice string like "valid from the 1st of march to the 31st of may, all working day of the week but the third wednesday of april"

:zap: **cpp module**



# Testing


## Unit tests

To run unit tests, you have to compile all Navitia then run tests :
```
cd <path/to/build/directory>
cmake <path/to/navitia/repo>/source/
make && make test
```
