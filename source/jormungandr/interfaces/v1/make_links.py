from flask import request, url_for, Flask, Request
import collections
from collections import OrderedDict
from functools import wraps
from converters_collection_type import resource_type_to_collection,\
                                       collections_to_resource_type
from flask import Request
from flask.ext.restful.utils import unpack

class generate_links(object):
    def prepare_objetcs(self, objects, hasCollections=False):
        if type(objects) is tuple:
            objects = objects[0]
        if not "links" in objects.keys():
            objects["links"] = []
        elif hasattr(self, "collections") :
            for link in objects["links"]:
                if "type" in link.keys():
                    self.collections.remove(link["type"])
        return objects

    def prepare_kwargs(self, kwargs, objects):
        if not "region" in kwargs.keys() and not "lon" in kwargs.keys()\
           and "regions" in objects.keys():
            kwargs["region"] = "{regions.id}"

        if "uri" in kwargs:
            del kwargs["uri"]
        return kwargs

class add_pagination_links(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            endpoint = None
            pagination = None
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects
            for key, value in data.iteritems():
                if key == "regions":
                    endpoint = "v1.coverage"
                elif key == "pagination":
                    pagination = value
                elif key in collections_to_resource_type.keys():
                    endpoint = "v1."+key+"."
                    endpoint += "id" if "id" in kwargs.keys() else "collection"
                elif key in ["journeys", "stop_schedules", "route_schedules",
                             "departures", "arrivals"]:
                    endpoint = "v1."+key
            if pagination and endpoint:
                pagination = data["pagination"]
                if "start_page" in pagination.keys() and \
                    "items_on_page" in pagination.keys() and \
                    "items_per_page" in pagination.keys() and \
                    "total_result" in pagination.keys() :
                        if not "links" in data.keys():
                            data["links"] = []
                        start_page = int(pagination["start_page"])
                        items_per_page = int(pagination["items_per_page"])
                        items_on_page = int(pagination["items_on_page"])
                        total_result = int(pagination["total_result"])
                        kwargs["_external"] = True

                        if start_page > 0:
                            kwargs["start_page"] = start_page - 1
                            data["links"].append({
                                "href": url_for(endpoint, **kwargs),
                                "type": "previous",
                                "templated" : False
                                })

                        if total_result > (items_per_page * start_page + items_on_page):
                            kwargs["start_page"] = start_page + 1
                            data["links"].append({
                                "href": url_for(endpoint, **kwargs),
                                "type": "next",
                                "templated" : False
                            })
                        kwargs["start_page"] = 0 if items_per_page == 0 or total_result == 0 \
                                              else (total_result-1) / items_per_page
                        data["links"].append({
                            "href": url_for(endpoint, **kwargs),
                            "type": "last",
                            "templated" : False
                        })

                        del kwargs["start_page"]
                        data["links"].append({
                            "href": url_for(endpoint, **kwargs),
                            "type": "first",
                            "templated" : False
                        })
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data
        return wrapper





class add_coverage_link(generate_links):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects
            if isinstance(data, OrderedDict):
                data = self.prepare_objetcs(data)
                kwargs  = self.prepare_kwargs(kwargs, data)
                url = url_for("v1.coverage", _external=True, **kwargs)
                data["links"].append({"href":url, "rel" : "related",
                                             "templated":True})
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data
        return wrapper


class add_collection_links(generate_links):
    def __init__(self, collections):
        self.collections = collections

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects
            if isinstance(data, OrderedDict):
                data = self.prepare_objetcs(objects, True)
                kwargs  = self.prepare_kwargs(kwargs, data)
                for collection in self.collections:
                    url = url_for("v1."+collection+".collection",
                                    _external=True, **kwargs)
                    data["links"].append({"href":url, "rel" : collection,
                                             "templated":True})
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data
        return wrapper


class add_id_links(generate_links):
    def __init__(self, *args, **kwargs):
        self.data = set()

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            if isinstance(objects, tuple):
                data, code, header = unpack(objects)
            else:
                data = objects
            self.get_objets(data)
            data = self.prepare_objetcs(objects, True)
            kwargs  = self.prepare_kwargs(kwargs, data)
            uri_id = None
            if "id" in kwargs.keys() and\
               "collection" in kwargs.keys() and \
               kwargs["collection"] in data.keys():
                   uri_id = kwargs["id"]
            for obj in self.data:
                if obj in resource_type_to_collection.keys():
                    kwargs["collection"] = resource_type_to_collection[obj]
                else:
                    kwargs["collection"] = obj
                if kwargs["collection"] in collections_to_resource_type.keys():
                    if not uri_id:
                        kwargs["id"] = "{"+obj+".id}"
                    endpoint = "v1."+kwargs["collection"]+"."
                    endpoint += "id" if "region" in kwargs.keys()\
                                        else "redirect"
                    del kwargs["collection"]
                    url = url_for(endpoint, _external=True, **kwargs)
                    data["links"].append({"href":url, "rel" : obj,
                                             "templated":True})
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data
        return wrapper

    def get_objets(self, data, collection_name=None):
        if isinstance(data, dict)or \
           isinstance(data, OrderedDict):
            if "id" in data.keys() \
               and (not "href" in data.keys()) \
               and collection_name:
                   self.data.add(collection_name)
            for key, value in data.iteritems():
                c_name = key
                self.get_objets(value, c_name)
        if isinstance(data, tuple) or isinstance(data, list):
            for item in data:
                self.get_objets(item, collection_name)


class clean_links(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response = f(*args, **kwargs)
            if isinstance(response, tuple):
                data, code, header = unpack(response)
                if code!=200:
                    return data, code, header
            else:
                data = response
            if isinstance(data, OrderedDict) and "links" in data.keys():
                for link in data['links']:
                    link['href'] = link['href'].replace("%7B", "{")\
                                               .replace("%7D", "}")
            if isinstance(response, tuple):
                return data, code, header
            else:
                return data
        return wrapper
