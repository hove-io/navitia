import argparse
import csv
import json
import logging
import zipfile


def parse_file(filename):
    try:
        with open(filename) as csvfile:
            reader = csv.reader(csvfile, delimiter=';', quotechar='"')
            for row in reader:
                yield row
    except Exception as e:
        logging.getLogger(__name__).error(
            "opg_excluded_zones: Unable to read file {}, error ({})".format(filename, str(e))
        )
        raise


def get_excluded_zones(path):
    result = {}
    for row in parse_file(path + "/poi_properties.txt"):
        if row[1].lower() != "excluded_zones":
            continue
        try:
            result[row[0]] = json.loads(row[2])
        except Exception:
            logging.getLogger(__name__).error(
                "opg_excluded_zones: Ignored line, Invalid json ({})".format(row[2])
            )
    return result


def get_geometries_ids(path, excluded_zones):
    result = {}
    for row in parse_file(path + "/poi.txt"):
        if row[0] not in excluded_zones:
            continue
        result[row[0]] = row[7]
    return result


def get_geometries_shapes(path):
    result = {}
    for row in parse_file(path + "/geometries.txt"):
        result[row[0]] = row[1]
    return result


def poi_to_excluded_zones(poi_file, output_dir, instance_name):
    tmp_path = "tmp/poi_{}".format(instance_name)

    with zipfile.ZipFile(poi_file, 'r') as zip_ref:
        zip_ref.extractall(tmp_path)

    # get excluded zones
    excluded_zones = get_excluded_zones(tmp_path)

    # find geometry id
    excluded_geometries_ids = get_geometries_ids(tmp_path, excluded_zones)

    if excluded_geometries_ids.keys() != excluded_zones.keys():
        logging.getLogger(__name__).warning("not all excluded zone's pois are found in poi.txt")
        logging.getLogger(__name__).warning("excluded_geometries_ids: {}".format(excluded_geometries_ids.keys()))
        logging.getLogger(__name__).warning("excluded_zones: {}".format(excluded_zones.keys()))

    # read geometries
    geometries_shapes = get_geometries_shapes(tmp_path)

    for poi_id, zones in excluded_zones.items():
        geometry_id = excluded_geometries_ids.get(poi_id)
        if not geometry_id:
            logging.getLogger(__name__).error("{} could not be found in poi.txt".format(poi_id))
            continue

        shape = geometries_shapes.get(geometry_id)
        if not shape:
            logging.getLogger(__name__).error("{} could not be found in geometries.txt".format(geometry_id))
            continue

        for i, zone in enumerate(zones):
            output_id = "{}_{}_{}".format(poi_id, i, instance_name)
            output = {'id': output_id, 'instance': instance_name, 'poi': "poi:{}".format(poi_id)}
            output.update(zone)
            output["shape"] = shape
            with open(output_dir + "/{}.json".format(output_id), "w") as output_file:
                json.dump(output, output_file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--poi', help='poi zip')
    args = parser.parse_args()
    logger = logging.getLogger(__name__)

    poi_to_excluded_zones(args.poi, "excluded_zones", "dummy_instance")
