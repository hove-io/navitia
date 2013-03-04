from validate import *
from apis_functions import *
from singleton import singleton
from instance_manager import DeadSocketException, RegionNotFound


class Arguments:
    scheduleArguments = {
        "filter" : Argument("Filter to have the times you want", filter, True,
                            False, order=0),
        "from_datetime" : Argument("The date from which you want the times",
                              datetime_validator, True, False, order=10),
        "duration" : Argument("Maximum duration between the datetime and the last  retrieved stop time",
                                  int, False, False, defaultValue=86400, order=20 ),        
        "wheelchair" : Argument("true if you want the times to have accessibility", boolean, False, False, defaultValue=False, order=50),
        "depth" : Argument("Maximal depth of the returned objects", int, False,
                           False, defaultValue = 1, order=100)
        }
    stopsScheduleArguments = {
        "from_datetime" : Argument("The date from which you want the times",
                              datetime_validator, True, False, order=10),
        "duration" : Argument("Maximum duration between the datetime and the last  retrieved stop time",
                                  int, False, False, defaultValue=86400, order=20 ),        
        "wheelchair" : Argument("true if you want the times to have accessibility", boolean, False, False, defaultValue=False, order=50),
        "depth" : Argument("Maximal depth of the returned objects", int, False,
                           False, defaultValue = 1, order=100),
        "departure_filter" : Argument("The filter of your departure point", filter,
                                                      True, False,order=0),
        "arrival_filter" : Argument("The filter of your arrival point", filter,
                                                      True, False,order=1)

    }

    nextTimesArguments = {
        "filter" : Argument("Filter to have the times you want", filter, False,
                            False, order=0, defaultValue = ""),
        "from_datetime" : Argument("The date from which you want the times",
                              datetime_validator, True, False, order=10),
        "duration" : Argument("Maximum duration between the datetime and the last  retrieved stop time",
                                  int, False, False, defaultValue=86400, order=20 ),        
        "wheelchair" : Argument("true if you want the times to have accessibility", boolean, False, False, defaultValue=False, order=50),
        "depth" : Argument("Maximal depth of the returned objects", int, False,
                           False, defaultValue = 1, order=100),
        "nb_stoptimes": Argument("The maximum number of stop_times, defaults to 20", int , False, False, 20, order=30)
        }

    ptrefArguments = {
        "filter" : Argument("Conditions to filter the returned objects", filter,
                            False, False, "", order=0),
        "depth" : Argument("Maximum depth on objects", int, False, False, 1,
                           order = 50)
        }
    journeyArguments = {
        "origin" : Argument("Departure Point", entrypoint, True, False, order = 0),
        "destination" : Argument("Destination Point" , entrypoint, True, False, order = 1),
        "datetime" : Argument("The time from which you want to arrive (or arrive before depending on the value of clockwise)", datetime_validator, True, False, order = 2),
        "clockwise" : Argument("true if you want to have a journey that starts after datetime, false if you a journey that arrives before datetime", boolean, False, False, True, order = 3),
        #"forbiddenline" : Argument("Forbidden lines identified by their external codes",  str, False, True, ""),
        #"forbiddenmode" : Argument("Forbidden modes identified by their external codes", str, False, True, ""),
        #"forbiddenroute" : Argument("Forbidden routes identified by their external codes", str, False, True, ""),
        "walking_speed" : Argument("Walking speed in m/s", float, False, False, 1.38),
        "walking_distance" : Argument("Maximum walking distance in meters", int,
                                      False, False, 1000),
        "wheelchair" : Argument("Does the journey has to be accessible?",
                                boolean, False, False, False)
        }

    isochroneArguments ={
        "origin" : Argument("Departure Point", entrypoint, True, False, order = 0),
        "datetime" : Argument("The time from which you want to arrive (or arrive before depending on the value of clockwise)", datetime_validator, True, False, order = 2),
        "clockwise" : Argument("true if you want to have a journey that starts after datetime, false if you a journey that arrives before datetime", boolean, False, False, True, order = 3),
        #"forbiddenline" : Argument("Forbidden lines identified by their external codes",  str, False, True, ""),
        #"forbiddenmode" : Argument("Forbidden modes identified by their external codes", str, False, True, ""),
        #"forbiddenroute" : Argument("Forbidden routes identified by their external codes", str, False, True, ""),
        "walking_speed" : Argument("Walking speed in m/s", float, False, False, 1.38),
        "walking_distance" : Argument("Maximum walking distance in meters", int,
                                      False, False, 1000),
        "wheelchair" : Argument("Does the journey has to be accessible?",
                                boolean, False, False, False)
        }

@singleton
class Apis:
    apis = {
        "autocomplete" : {"endpoint" : on_autocomplete, "arguments" : {"name" : Argument("The data to search", str, True, False, order = 1),
                                                                       "object_type[]" : Argument("The type of datas you want in return", str, False, True, ["stop_area", "stop_point", "address"], 2,["stop_area", "stop_point", "address"])},
                          "description" : "Retrieves the objects which contains in their name the \"name\"",
                          "order":2},
        "next_departures" : {"endpoint" : on_next_departures, "arguments" :
                             Arguments.nextTimesArguments, 
                             "description" : "Retrieves the departures after datetime at the stop points filtered with filter",
                          "order":3},
        "next_arrivals" : {"endpoint" : on_next_arrivals, "arguments" :
                            Arguments.nextTimesArguments,
                           "description" : "Retrieves the departures after datetime at the stop points filtered with filter",
                          "order":3},
        "line_schedule" : {"endpoint" : on_line_schedule, "arguments" :
                           Arguments.scheduleArguments,
                           "description" : "Retrieves the schedule of line at the day datetime",
                          "order":4},
        "stops_schedule" : {"endpoint" : on_stops_schedule, "arguments" :
                            Arguments.stopsScheduleArguments,
                            "description" : "Retrieves the schedule for 2 stops points",
                          "order":4},
        "departure_board" : {"endpoint" : on_departure_board,
                             "arguments":Arguments.scheduleArguments,
                             "description" : "Give all the departures of filter at datetime",
                          "order":4},
        "stop_areas" : {"endpoint" : on_ptref(type_pb2.STOP_AREA), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the stop areas filtered with filter",
                          "order":5},
        "stop_points" : {"endpoint" : on_ptref(type_pb2.STOP_POINT), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the stop points filtered with filter",
                          "order":5},
        "lines" : {"endpoint" : on_ptref(type_pb2.LINE), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the stop_areas filtered with filter",
                          "order":5},
        "routes" : {"endpoint" : on_ptref(type_pb2.ROUTE), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the routes filtered with filter",
                          "order":5},
        "networks" : {"endpoint" : on_ptref(type_pb2.NETWORK), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the networks filtered with filter",
                          "order":5},
        "physical_modes" : {"endpoint" : on_ptref(type_pb2.PHYSICAL_MODE), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the physical modes filtered with filter",
                          "order":5},
        "commercial_modes" : {"endpoint" : on_ptref(type_pb2.COMMERCIAL_MODE), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the commercial modes filtered with filter",
                          "order":5},
        "connections" : {"endpoint" : on_ptref(type_pb2.CONNECTION), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the connections points filtered with filter",
                          "order":5},
        "journey_pattern_points" : {"endpoint" : on_ptref(type_pb2.JOURNEY_PATTERN_POINT), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the journey pattern points filtered with filter",
                        "order":5, "hidden": True},
        "journey_patterns" : {"endpoint" : on_ptref(type_pb2.JOURNEY_PATTERN), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the journey pattern filtered with filter",
                        "order":5, "hidden": True},
        "companies" : {"endpoint" : on_ptref(type_pb2.COMPANY), "arguments" :
                        Arguments.ptrefArguments,
                        "description" : "Retrieves all the companies filtered with filter",
                          "order":5},
        "vehicle_journeys" : {"endpoint" : on_ptref(type_pb2.VEHICLE_JOURNEY),
                              "arguments" : Arguments.ptrefArguments,
                              "description" :"Retrieves all the vehicle journeys filtered with filter" ,
                              "order" : 5},
        "journeys" : {"endpoint" :  on_journeys(type_pb2.PLANNER), "arguments" :
                      Arguments.journeyArguments,
                      "description" : "Computes and retrieves a journey",
                      "order":1, "universal" : True},
        "isochrone" : {"endpoint" : on_journeys(type_pb2.ISOCHRONE), "arguments" : Arguments.isochroneArguments,
                       "description" : "Computes and retrieves an isochrone",
                          "order":1, "universal" : True},
        "proximity_list" : {"endpoint" : on_proximity_list, "arguments" : {
                "lon" : Argument("Longitude of the point from where you want objects", float, True, False, order=0),
                "lat" : Argument("Latitude of the point from where you want objects", float, True, False, order=1),
                "distance" : Argument("Distance range of the query", int, False, False, 1000, order=3),
                "object_type[]" : Argument("Type of the objects you want to have in return", str, False, False, ["stop_area", "stop_point"], order=4)
                },
            "description" : "Retrieves all the objects around a point within the given distance",
            "order" : 1.1, "universal" : True},
        "metadatas" : {"endpoint" : on_metadatas, "arguments" : {}, "description" : "Retrieves the metadatas of a region"},
        "status" : {"endpoint" : on_status, "arguments" : {}, "description" : "Retrieves the status of a region", "hidden" : True},
        "load" : {"endpoint" : on_load, "arguments" : {},  "hidden" : True}
        }

    def __init__(self):
        self.apis_all = copy.copy(self.apis)
        self.apis_all["regions"] = {"arguments" : {}, "description" : "Retrieves the list of available regions", "regions" : False,
                          "order":0}


    def on_api(self, request, version, region, api, format):
        if version != "v0":
            return Response("Unknown version: " + version, status=404)
        if api in self.apis:
            v = validate_arguments(request, self.apis[api]["arguments"])
            if v.valid:
                try:
                    return render_from_protobuf(self.apis[api]["endpoint"](v.arguments, version, region), format, request.args.get("callback"))
                except DeadSocketException, e:
                    return Response(e, status=503)
                except RegionNotFound, e:
                    return Response(e, status=404)
            else:
                return Response("Invalid arguments: " + str(v.details), status=400)
        else:
            return Response("Unknown api: " + api, status=404)

