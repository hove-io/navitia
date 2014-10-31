# Source directory

Navitia is a mix of C++ components and python modules

Here is a quick overview of the different components

## Main components

### routing
The core of navitia, contains all public transport routing algorithm

### ed
Collection of public transport data import tools

This module contains tool to read GTFS, Fusio (format used by CanalTP, derived from GTFS), Osm, ...

It also contains ed2nav, the tool to build all those data together for Kraken

### georef
Handle the street network in kraken.

Contains the shortest path algorithms and some tools explore the street network data (address, house numbers, ...)

### jormungandr
Web service around the c++ core(s).

This is the navitia api front end, based over flask

:snake: **python module**

### kraken
Kraken is the c++ module behind navitia.

### fare
Used to handle fares in kraken.

Fares are not taken into account during journey computation but as a postprocess.

### type
Contains the navitia public transport c++ object model.

Also contains all protobuff definition.

### tyr
Here stand the law and nothing but the law.

This module is the conductor of Navitia.

In a running environment, Tyr is responsible for detecting new data and integrating them in the system.

:snake: **python module**

### ptreferential
Module used to explore the public transport data.

It is this module that allow to query for all stops for a given lines and a given network

### disruption
Chaos connector.

Chaos is the python web service which implements the realtime aspect of Navitia.
This connector loads Chaos data in navitia.

### autocomplete
Tools for autocomplete

### navitiacommon
navitia python model.

Shared by all python modules

:snake: **python module**


# Misc
### chaos-proto
Submodule containing chaos (real time python modules) protobuff definitions

### cities
Module used to fill a database with some OSM administrative regions.

The database will be used in ed2nav to find the administrative region of public transport objects outside the streetnetwork of the insances

### calendar
Small module used to have to query the calendars.
Calendars are objects used in timetables.

### cmake_modules
Directory with cmake modules

### connectors
Alerte Traffic (AT) connector

Alerte Traffic are databases filled with public transport disruptions. This connector import the disruption in Navitia.

:snake: **python module**

### linenoise
submodule with a lib made to create prompt.

Used to a a debug cli for kraken

### lz4_filter
Module to handle lz4 compresion format in boost::serialize

Kraken use data serialized with boost::serialize (and compressed with lz4)

### monitor
Module to monitor kraken.

Kraken is monitored via zmq and this module is a web service to display the status

:snake: **python module**

### proximity_list
Contains tools for proximity list

proximity list are tools used to get objets around a location

It is for example used to get all stops around a geographical point

### scripts
Various bash scripts

### sindri
Alerte traffic (AT) connector, used to write in the Alerte traffic database

:snake: **python module**

### sql
Contains alembic migrations for the Ed databases

### stat_persistor
Statistics module.

Persists Jormungandr calls in a database

:snake: **python module**

### tests
Contains kraken mock excecutable used in Jormungandr integration tests

### third_party
Various third party libraries

### time_tables
Module to handle timetable

With this module you can query for the next departure from a stop point or for the schedule of a given route.

### utils
Submodule containing some code helpers

### validator
Public transport data validator

### vptranslator
Work in progress

Module to transform a ValidityPattern (a year bitset) to a nice string like "valid from the 1st of march to the 31st of may, all working day of the week but the third webdnesday of april"
