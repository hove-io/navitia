# Eitri

Eitri is the brother of Brokkr. He build stuff.
 
In navitia it's a module to build a data.nav.lz4 (the file needed by kraken) without the need of a specific database

Eitri use docker to create the temporary database needed to create the data.nav.lz4

To run eitri:

PYTHONPATH=.:../navitiacommon eitri.py ~/run/navitia/fr-bou/data/fusio