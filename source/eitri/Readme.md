![alt text](../../documentation/diagrams/logo_navitia_horizontal_fd_gris_250px.png)
# Eitri

## In Norse Mythology
Eitri is a dwarf and the brother of Brokkr, he builds stuff.

## For Navitia
This software in Python2 builds a `data.nav.lz4` file used as an input for Kraken.
Eitri uses docker to create the temporary database needed to produce the `.nav`.
To do so, it uses the set of ED tools (\*2ed and ed2\*).

Eitri consumes GTFS/NTFS, POI infos, Open Street Map data, etc... to generate a `.nav` file.
If you have several datasets (eg. a GTFS and an OSM), they will need to be in sperarate directories (ie. `/my/data/gtfs` & `/my/data/osm`).

## How to run Eitri
```sh
./eitri.py -d /my/data/ -e /my/folder/with/ed/binaries
```

## Help
```sh
./eitri.py --help
usage: eitri [-h] -d DATA_FOLDER -e ED_FOLDER [-o OUTPUT_FILE]
             [-i CITIES_FILE] [-f CITIES_FOLDER] [-c COLOR]

Eitri uses a set of binaries to extract data, load it into a database and
extract it to a single file. This software is not distributed and only used by
Navitia.

optional arguments:
  -h, --help            show this help message and exit
  -d DATA_FOLDER, --data-folder DATA_FOLDER
                        the directory path with all data. If several datasets
                        ("*.osm", "*.gtfs", ...) are available, they need to
                        be in separate directories
  -e ED_FOLDER, --ed-folder ED_FOLDER
                        the directory path containing all "*2ed" and "ed2nav"
                        binaries
  -o OUTPUT_FILE, --output-file OUTPUT_FILE
                        the output file path (default: './data.nav.lz4')
  -i CITIES_FILE, --cities-file CITIES_FILE
                        the path to the "cities" file to load (usually a
                        *.osm.pbf)
  -f CITIES_FOLDER, --cities_folder CITIES_FOLDER
                        the path to the folder containing the "cities" binary
  -c COLOR, --color COLOR
                        enable or disable colored logs (default: True)

```
