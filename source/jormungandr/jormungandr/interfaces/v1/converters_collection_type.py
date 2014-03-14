collections_to_resource_type = {
    "stop_points": "stop_point", "routes": "route",
    "networks": "network", "commercial_modes": "commercial_mode",
    "physical_modes": "physical_mode", "companies": "company",
    "stop_areas": "stop_area", "lines": "line",
    "addresses": "address", "coords": "coord",
    "journey_pattern_points": "journey_pattern_point",
    "journey_patterns": "journey_pattern",
    "pois": "poi", "poi_types": "poi_type",
    "connections": "connection", "vehicle_journeys": "vehicle_journey"}

resource_type_to_collection = dict((resource_type, collection)
                                   for (collection, resource_type)
                                   in collections_to_resource_type.iteritems())
