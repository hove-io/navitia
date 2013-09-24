from flask.ext.restful import Resource
from converters_collection_type import collections_to_resource_type
from make_links import add_id_links, clean_links, add_pagination_links
from functools import wraps
from collections import OrderedDict
from flask import url_for
from flask.ext.restful.utils import unpack


class ResourceUri(Resource):
    def __init__(self, *args, **kwargs):
        Resource.__init__(self, *args, **kwargs)
        self.region = None
        self.method_decorators = []
        self.method_decorators.append(add_id_links())
        self.method_decorators.append(add_address_region(self))
        self.method_decorators.append(add_computed_resources(self))
        self.method_decorators.append(add_pagination_links())
        self.method_decorators.append(clean_links())
        self.method_decorators.append(add_notes(self))


    def get_filter(self, items):
        filters = []
        if len(items) % 2 != 0:
            items = items[:-1]
        type_ = None
        for item in items:
            if not type_:
                type_ = collections_to_resource_type[item]
            else:
                if type_ == "coord":
                    splitted_coord = item.split(";")
                    if len(splitted_coord) == 2:
                        lon, lat = splitted_coord
                        filters.append("stop_point.coord DWITHIN("+lon+","+lat+",200)")
                    else:
                        filters.append(type_+".uri="+item)
                else :
                    filters.append(type_+".uri="+item)
                type_ = None
        return " and ".join(filters)

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
                if key != 'links' and key != 'pagination':
                    collection = key
            if collection is None:
                return response
            kwargs["uri"] = collection + '/'
            if "id" in kwargs.keys():
                kwargs["uri"] += kwargs["id"]
                del kwargs["id"]
                templated = False
            else:
                kwargs["uri"] += '{' + collection + ".id}"
            if collection in ['stop_areas', 'stop_points', 'lines', 'routes']:
                for api in ['route_schedules', 'stop_schedules',
                            'arrivals', 'departures']:
                    data['links'].append({
                        "href" : url_for("v1."+api, **kwargs),
                        "rel" : api,
                        "templated" : templated
                            })
            if collection in ['stop_areas', 'stop_points']:
                data['links'].append({
                    "href" : url_for("v1.journeys", **kwargs),
                    "rel" : "journeys",
                    "templated" : templated
                    })
            if isinstance(response, tuple):
                return data, code, header
            else:
                return data
        return wrapper

class add_address_region(object):
    def __init__(self, resource):
        self.resource = resource

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            def add_region(objects):
                if isinstance(objects, list) or isinstance(objects, tuple):
                    for item in objects:
                        add_region(item)
                elif isinstance(objects, dict) or\
                     isinstance(objects, OrderedDict):
                         if 'embedded_type' in objects.keys() and\
                            objects['embedded_type'] == 'address':
                            tmp = 'address:'+self.resource.region+':'
                            objects['id'] = objects['id'].replace('address:', tmp)
                            objects['address']['id'] = objects['id']
                         else :
                             for v in objects.items():
                                 add_region(v)
            if self.resource.region:
                add_region(objects)
            return objects
        return wrapper



class add_notes(object):
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

            def add_note(data):
                result = []
                if isinstance(data, list) or isinstance(data, tuple):
                    for item in data:
                        result.extend(add_note(item))

                elif isinstance(data, dict) or\
                     isinstance(data, OrderedDict):
                         if 'type' in data.keys() and data['type'] == 'notes':
                            result.append({"id" : data['id'], "value" :  data['value']})
                            del data["value"]
                         else :
                             for v in data.items():
                                 result.extend(add_note(v))
                return result
            if self.resource.region:
                if not "notes" in data.keys() or  not isinstance(data["notes"], list):
                    data["notes"] = []
                    result = []
                    result_note = add_note(data)
                    [result.append(item) for item in result_note if not item in result]
                    data["notes"].extend(result)

            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data

        return wrapper
