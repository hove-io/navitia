# Eitri

Eitri is the brother of Brokkr, he builds stuff.
 
For navitia, he builds a `data.nav.lz4` used as an input by Kraken. Eitri uses docker to create the temporary database needed to produce the `.nav`.

Eitri consumes GTFS/NTFS, POI infos, Open Street Map data etc... to generate a `.nav` file. If you have several datasets (eg. a GTFS and an OSM), they will need to be in sperarate directories (ie. `/my/data/gtfs` & `/my/data/osm`).

## How to run Eitri

```sh
PYTHONPATH=.:../navitiacommon python eitri.py /my/data/
```

## Options
```
--output-file       | -o <str> (default='./data.nav.lz4')
--ed-component-path | -e <str> (default='') the path to the folder that containes your binaries ed2nav, fusio2ed, osm2ed etc...
--add-pythonpath    | -a <list of str> (default=[])
--help              | -? print this help
```
