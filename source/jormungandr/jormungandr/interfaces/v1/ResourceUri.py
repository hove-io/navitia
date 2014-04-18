from flask.ext.restful import Resource
from converters_collection_type import collections_to_resource_type
from converters_collection_type import resource_type_to_collection
from make_links import add_id_links, clean_links, add_pagination_links
from functools import wraps
from collections import OrderedDict, deque
from flask import url_for
from flask.ext.restful.utils import unpack
from jormungandr.authentification import authentification_required
import navitiacommon.type_pb2 as type_pb2


class ResourceUri(Resource):

    def __init__(self, *args, **kwargs):
        Resource.__init__(self, *args, **kwargs)
        self.region = None
        self.method_decorators = []
        self.method_decorators.append(add_id_links())
        self.method_decorators.append(add_address_poi_id(self))
        self.method_decorators.append(add_computed_resources(self))
        self.method_decorators.append(add_pagination_links())
        self.method_decorators.append(clean_links())
        self.method_decorators.append(authentification_required)

    def get_filter(self, items):
        filters = []
        if len(items) % 2 != 0:
            items = items[:-1]
        type_ = None

        for item in items:
            if not type_:
                if item != "coord":
                    if(item == "calendars"):
                        type_ = 'calendar'
                    else:
                        type_ = collections_to_resource_type[item]
                else:
                    type_ = "coord"
            else:
                if type_ == "coord" or type_ == "address":
                    splitted_coord = item.split(";")
                    if len(splitted_coord) == 2:
                        lon, lat = splitted_coord
                        object_type = "stop_point"
                        if(self.collection == "pois"):
                            object_type = "poi"
                        filter_ = object_type + ".coord DWITHIN(" + lon + ","
                        filter_ += lat + ",200)"
                        filters.append(filter)
                    else:
                        filters.append(type_ + ".uri=" + item)
                elif type_ == 'poi':
                    filters.append(type_ + '.uri=' + item.split(":")[-1])
                else:
                    filters.append(type_ + ".uri=" + item)
                type_ = None
        return " and ".join(filters)


class add_address_poi_id(object):

    def __init__(self, resource):
        self.resource = resource

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)

            def add_id(objects, region, type_=None):
                if isinstance(objects, (list, tuple)):
                    for item in objects:
                        add_id(item, region, type_)
                elif hasattr(objects, 'keys'):
                    for v in objects.keys():
                        add_id(objects[v], region, v)
                    if 'address' == type_:
                        lon = objects['coord']['lon']
                        lat = objects['coord']['lat']
                        objects['id'] = lon + ';' + lat
                    if 'embedded_type' in objects.keys() and\
                        (objects['embedded_type'] == 'address' or
                         objects['embedded_type'] == 'poi' or
                         objects['embedded_type'] == 'administrative_region'):
                        objects["id"] = objects[objects['embedded_type']]["id"]
            if self.resource.region:
                add_id(objects, self.resource.region)
            return objects
        return wrapper


class add_computed_resources(object):

    def __init__(self, resource):
        self.resource = resource

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response = f(*args, **kwargs)
            if isinstance(response, tuple):
                data, code, header = unpack(response)
            else:
                data = response
            collection = None
            kwargs["_external"] = True
            templated = True
            for key in data.keys():
                if key in collections_to_resource_type.keys():
                    collection = key
                if key in resource_type_to_collection.keys():
                    collection = resource_type_to_collection[key]
            if collection is None:
                return response
            kwargs["uri"] = collection + '/'
            if "id" in kwargs.keys():
                kwargs["uri"] += kwargs["id"]
                del kwargs["id"]
                templated = False
            else:
                kwargs["uri"] += '{' + collection + ".id}"
            if collection in ['stop_areas', 'stop_points', 'lines', 'routes',
                              'addresses'] and "region" in kwargs:
                for api in ['route_schedules', 'stop_schedules',
                            'arrivals', 'departures', "places_nearby"]:
                    data['links'].append({
                        "href": url_for("v1." + api, **kwargs),
                        "rel": api,
                        "templated": templated
                    })
            if collection in ['stop_areas', 'stop_points', 'addresses']:
                data['links'].append({
                    "href": url_for("v1.journeys", **kwargs),
                    "rel": "journeys",
                    "templated": templated
                })
            #for lines we add the link to the calendars
            if 'region' in kwargs:
                if collection == 'lines':
                    data['links'].append({
                        "href": url_for("v1.calendars", **kwargs),
                        "rel": "calendars",
                        "templated": templated
                    })
                if collection in ['stop_areas', 'lines', 'networks']:
                    data['links'].append({
                        "href": url_for("v1.disruptions", **kwargs),
                        "rel": "disruptions",
                        "templated": templated
                    })
            if isinstance(response, tuple):
                return data, code, header
            else:
                return data
        return wrapper

class complete_links(object):
    def __init__(self, resource):
        self.resource = resource
    def complete(self, data, collect):
        queue = deque()
        result = []
        for v in data.itervalues():
            queue.append(v)
        collect_type = collect["type"]
        while queue:
            elem = queue.pop()
            if isinstance(elem, (list, tuple)):
                queue.extend(elem)
            elif hasattr(elem, 'iterkeys'):
                if 'type' in elem and elem['type'] == collect_type:
                    if collect_type == "notes":
                        result.append({"id": elem['id'], "value": elem['value'], "type": collect_type})
                    elif collect_type == "exceptions":
                        type_= "Remove"
                        if elem['except_type'] == 0:
                            type_ = "Add"
                        result.append({"id": elem['id'], "date": elem['date'], "type" : type_})
                    for del_ in collect["del"]:
                        del elem[del_]
                else:
                    queue.extend(elem.itervalues())
        return result

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects

            if self.resource.region:
                collection = {
                    "notes" : {"type" : "notes",
                              "del":["value"]},
                    "exceptions" : {"type" : "exceptions",
                              "del":["date","except_type"]}
                }
                # Add notes exceptions
                for col in collection:
                    if not col in data or not isinstance(data[col], list):
                        data[col] = []
                        res = []
                        note = self.complete(data, collection[col])
                        [res.append(item) for item in note if not item in res]
                        data[col].extend(res)

            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data

        return wrapper

class update_journeys_status(object):

    def __init__(self, resource):
        self.resource = resource

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects

            def update_status(journey, _items):

                if isinstance(_items, list) or isinstance(_items, tuple):
                    for item in _items:
                        update_status(journey, item)
                elif isinstance(_items, dict) or\
                        isinstance(_items, OrderedDict):
                    if 'messages' in _items.keys():
                        for msg in _items["messages"]:
                            if not "status" in journey.keys():
                                journey["status"] = msg["level"]
                            else:
                                desc = type_pb2.Message.DESCRIPTOR
                                fields = desc.fields_by_name
                                f_status = fields['message_status'].enum_type
                                values = f_status.values_by_name
                                status = values[journey["status"]]
                                level = values[msg["level"]]
                                if status < level:
                                    journey["status"] = msg["level"]
                    else:
                        for v in _items.items():
                            update_status(journey, v)

            if self.resource.region and "journeys" in data:
                for journey in data["journeys"]:
                    update_status(journey, journey)

            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data

        return wrapper
