import argparse
import csv
import json
import logging


def poi_to_excluded_zones(poi_file, output_dir, instance_name):
    logger = logging.getLogger(__name__)

    tmp_path = "tmp/poi_{}".format(instance_name)
    import zipfile

    with zipfile.ZipFile(poi_file, 'r') as zip_ref:
        zip_ref.extractall(tmp_path)

    excluded_zones = {}
    excluded_geometries_ids = {}

    # get excluded zones
    with open(tmp_path + "/poi_properties.txt") as csvfile:
        reader = csv.reader(csvfile, delimiter=';', quotechar='"')
        for row in reader:
            if row[1].lower() != "excluded_zones":
                continue
            excluded_zones[row[0]] = json.loads(row[2])

    # find geometry id
    with open(tmp_path + "/poi.txt") as csvfile:
        reader = csv.reader(csvfile, delimiter=';', quotechar='"')
        for row in reader:
            if row[0] not in excluded_zones:
                continue
            excluded_geometries_ids[row[0]] = row[7]

    if excluded_geometries_ids.keys() != excluded_zones.keys():
        logger.warning("not all excluded zone's pois are found in poi.txt")
        logger.warning("excluded_geometries_ids: {}".format(excluded_geometries_ids.keys()))
        logger.warning("excluded_zones: {}".format(excluded_zones.keys()))

    # read geometries
    geometries_shapes = {}
    with open(tmp_path + "/geometries.txt") as csvfile:
        reader = csv.reader(csvfile, delimiter=';', quotechar='"')
        for row in reader:
            geometries_shapes[row[0]] = row[1]

    for poi_id, zones in excluded_zones.items():
        geometry_id = excluded_geometries_ids.get(poi_id)
        if not geometry_id:
            logger.error("{} could not be found in poi.txt".format(row[0]))
            continue

        shape = geometries_shapes.get(geometry_id)
        if not shape:
            logger.error("{} could not be found in geometries.txt".format(geometry_id))
            continue

        for i, zone in enumerate(zones):
            output_id = "{}_{}_{}".format(poi_id, i, instance_name)
            output = {'id': output_id, 'instance': instance_name, 'poi': poi_id}
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
