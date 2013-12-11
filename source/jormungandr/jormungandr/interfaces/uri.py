from .instance_manager import InstanceManager

collections_to_resource_type = {
    "stop_points": "stop_point", "routes": "route",
    "networks": "network", "commercial_modes": "commercial_mode",
    "physical_modes": "physical_mode", "companies": "company",
    "stop_areas": "stop_area", "lines": "line",
    "addresses": "address", "coords": "coord"}

resource_type_to_collection = dict((resource_type, collection)\
                                   for (collection, resource_type)\
                                   in collections_to_resource_type.iteritems())

types_not_ptrefable = ["addresses", "administrative_regions"]


class InvalidUriException(Exception):

    def __init__(self, message):
        Exception.__init__(self, message)


class Uri:

    def __init__(self, string):
        self.uri = string
        self.params = None
        self.lon = None
        self.lat = None
        self.is_region = None
        self.objects = []
        self.region_ = None
        self.parse_region_coord()
        self.parse_params()

    def region(self):
        if not self.region_ and self.lon and self.lat:
            # On va chercher la region associee
            self.region_ = InstanceManager().key_of_coord(self.lon, self.lat)
            if not self.region_:
                error = "No region is covering these coordinates"
                raise InvalidUriException(error)
        return self.region_

    def parse_region_coord(self):
        # On caste la premiere partie de l'url qui est soit une region,
        # soit une coordonnee (coord/lon;lat)
        parts = self.uri.split("/")
        parts.reverse()
        self.region_or_coord_part = parts.pop()
        if self.region_or_coord_part.count(";") == 1:
            self.is_region = False
            lonlatsplitted = self.region_or_coord_part.split(";")
            if len(lonlatsplitted) != 2:
                raise InvalidUriException(", unable to parse lon or lat",
                                          lonlat)
            lon = lonlatsplitted[0]
            lat = lonlatsplitted[1]
            try:
                self.lon = float(lon)
                self.lat = float(lat)
            except ValueError:
                error = ", unable to parse lon or lat" + lon + ":" + lat
                raise InvalidUriException(error)
        else:
            self.is_region = True
            self.region_ = self.region_or_coord_part
        parts.reverse()
        self.params = "/".join(parts)

    def parse_params(self):
        parts = self.params.split("/")
        resource_type, uid = None, None
        for par in parts:
            if par != "":
                if not resource_type:
                    if self.valid_resource_type(par):
                        resource_type = par
                    else:
                        error = "Invalid resource type : " + par
                        raise InvalidUriException(error)
                else:
                    uid = par
                    self.objects.append((resource_type, uid))
                    resource_type, uid = None, None
        if resource_type:
            self.objects.append((resource_type, uid))

    def valid_resource_type(self, resource_type):
        resource_types = ["connections", "stop_points", "networks",
                          "commercial_modes", "physical_modes", "companies",
                          "stop_areas", "routes", "lines", "addresses",
                          "administrative_regions", "coords"]

        return resource_type in resource_types


import unittest


class Tests(unittest.TestCase):

    def testOnlyRegionWithoutBeginningSlash(self):
        string = "paris"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")

    def testOnlyRegionWithBeginningSlash(self):
        string = "/paris"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")

    def testOnlyCoordPosWithoutSlash(self):
        string = "coord/1.1;2.3"
        uri = Uri(string)
        self.assertEqual(uri.lon, 1.1)
        self.assertEqual(uri.lat, 2.3)

        string = "coord/.1;2."
        uri = Uri(string)
        self.assertEqual(uri.lon, .1)
        self.assertEqual(uri.lat, 2)

        string = "coord/.111111;22.3"
        uri = Uri(string)
        self.assertEqual(uri.lon, .111111)
        self.assertEqual(uri.lat, 22.3)

    def testOnlyCoordPosWithSlash(self):
        string = "/coord/1.1;2.3"
        uri = Uri(string)
        self.assertEqual(uri.lon, 1.1)
        self.assertEqual(uri.lat, 2.3)

        string = "/coord/.1;2."
        uri = Uri(string)
        self.assertEqual(uri.lon, .1)
        self.assertEqual(uri.lat, 2)

        string = "/coord/.111111;22.3"
        uri = Uri(string)
        self.assertEqual(uri.lon, .111111)
        self.assertEqual(uri.lat, 22.3)

    def testResourceListWithslash(self):
        string = "/paris/stop_areas"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")
        self.assertEqual(uri.params, "stop_areas")
        self.assertEqual(len(uri.objects), 1)
        self.assertEqual(uri.objects[0][0], "stop_areas")
        self.assertEqual(uri.objects[0][1], None)

    def testResourceWithslash(self):
        string = "/paris/stop_areas/1"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")
        self.assertEqual(uri.params, "stop_areas")
        self.assertEqual(len(uri.objects), 1)
        self.assertEqual(uri.objects[0][0], "stop_areas")
        self.assertEqual(uri.objects[0][1], 1)

    def testResourceListWithoutSlash(self):
        string = "paris/stop_areas"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")
        self.assertEqual(uri.params, "stop_areas")
        self.assertEqual(len(uri.objects), 1)
        self.assertEqual(uri.objects[0][0], "stop_areas")
        self.assertEqual(uri.objects[0][1], None)

    def testResourcetWithoutslash(self):
        string = "paris/stop_areas/1"
        uri = Uri(string)
        self.assertEqual(uri.region(), "paris")
        self.assertEqual(uri.params, "stop_areas")
        self.assertEqual(len(uri.objects), 1)
        self.assertEqual(uri.objects[0][0], "stop_areas")
        self.assertEqual(uri.objects[0][1], 1)
