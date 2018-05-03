# Various data import components

As a reminder here is a simplified version of the architecture:

![](../../documentation/diagrams/simple_archi_data_view.png "architecture")

Nearly all data used by a kraken (the c++ routing engine) are extracted from one database called `ed`

For each data format, one executable can be found in this module to import those data to the `ed` database and there is one component, `ed2nav` that aggregate all data found in `ed` and build the kraken input file.

The kraken input file is a binary blob file created using boost::serialize.

## common stuff
All executables offer a ```--help``` option to list the different options

Every executable needs a ```--connection-string``` option to be able to be linked to the database

This connection string must contains:
 * host: database host
 * user: database user
 * password: database password
 * dbname: name of the database

example: ```--connection-string="host=localhost user=my_user password=my_pwd dbname=my_db"```

Note: importing data will remove the data of the specified type (i.e. osm2ed and geopal2ed will remove street network data).

## ed2nav
Component that aggregates all data from `ed` and build the kraken input file.

To run this component, some public transport data *must* be loaded in the database (but other data are not mandatory)

## osm2ed
Component that loads a osm .pbf file into `ed`

## gtfs2ed
Component that loads a gtfs data set into `ed`.

The directory containing all gtfs text files is given with the `-i` option

## fusio2ed
Component that loads a fusio data set into `ed`.

Fusio is a CanalTP custom file format derived from gtfs but with additional files.

The directory containing all fusio text files is given with the `-i` option

## nav2rt
Component that takes a kraken input file in input and loads real time data into it.

## fare2ed
Component that loads fare data into `ed`.

Note: fares can also be loaded with fusio2ed if the fare files are in the fusio data directory

## poi2ed
Component that loads POI (Point Of Interest) into `ed` from csv files.

You can use poi2ed to get POI in addition of osm2ed (that mainly extract street network data) ; If you do so, osm2ed will not extract any POI from the OSM dataset and use the poi2ed dataset instead.
If you want to switch your POI source back to OSM, you will need to set the `parse_pois_from_osm` boolean to `true` by using an api tyr `PUT $HOST/v0/instances/$INSTANCE_NAME/actions/migrate_from_poi_to_osm` and launch again an osm2ed data integration job.

## geopal2ed
Component that loads geopal street network data into `ed`.

## synonym2ed
Component that loads synonym data into `ed`.

Synonyms are used in the autocomplete to find equivalent words. For example it allows user to find "boulevard" by searching "bd".
