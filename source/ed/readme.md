# Various data import components

As a reminder here is a simplified version of the architecture:

.. image:: documentation/diagrams/simple_archi_data_view.png

Nearly all data used by a kraken (the c++ core) are extracted from one database called `ed`

For each data format, one executable can be found in this module to import those data to the `ed` database and there is one component, `ed2nav` that aggregate all data found in `ed` and build the kraken input file.

The kraken input file is a binary blob file created using boost::serialize

#TODO schema

## common stuff
All executables offer a ```--help``` option to list the different option

Every executables need a ```--connection-string``` option to be able to be linked to the database

This connection string must contains:
 * host: database host
 * user: database user
 * password: database password
 * dbname: name of the database

example: ```--connection-sting="host=localhost user=navitia password=my_pwd dbname=my_db"```

## ed2nav


## osm2ed

## gtfs2ed

## fusio2ed

## nav2rt

## fare2ed

## poi2ed

## geopal2ed

## synomyme2ed
