# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from flask import url_for
from collections import OrderedDict
from functools import wraps
from sqlalchemy.sql.elements import _type_from_args
from converters_collection_type import resource_type_to_collection,\
    collections_to_resource_type
from flask.ext.restful.utils import unpack


def create_external_link(url, rel, _type=None, templated=False, description=None, **kwargs):
    """
    :param url: url forwarded to flask's url_for
    :param rel: relation of the link to the current object
    :param _type: type of linked object
    :param templated: if the link is templated ({} is the url)
    :param description: description of the link
    :param kwargs: args forwarded to url_for
    :return: a dict representing a link
    """
    #if no type, type is rel
    if not _type:
        _type = rel

    d = {
        "href": url_for(url, _external=True, **kwargs),
        "templated": templated,
        "rel": rel,
        "type": _type
    }
    if description:
        d['title'] = description

    return d


def create_internal_link(rel, _type, id, templated=False, description=None):
    """
    :param rel: relation of the link to the current object
    :param _type: type of linked object
    :param id: id of the link
    :param templated: if the link is templated ({} is the url)
    :return: a dict representing a link
    """
    #if no type, type is rel
    if not _type:
        _type = rel

    d = {
        "templated": templated,
        "rel": rel,
        "internal": True,
        "type": _type
    }
    if description:
        d['title'] = description
    if id:
        d['id'] = id

    return d


class generate_links(object):

    def prepare_objetcs(self, objects, hasCollections=False):
        if isinstance(objects, tuple):
            objects = objects[0]
        if not "links" in objects.keys():
            objects["links"] = []
        elif hasattr(self, "collections"):
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
                    endpoint = "v1." + key + "."
                    endpoint += "id" if "id" in kwargs.keys() else "collection"
                elif key in ["journeys", "stop_schedules", "route_schedules",
                             "departures", "arrivals", "places_nearby", "calendars"]:
                    endpoint = "v1." + key
            if pagination and endpoint and "region" in kwargs:
                pagination = data["pagination"]
                if "start_page" in pagination.keys() and \
                        "items_on_page" in pagination.keys() and \
                        "items_per_page" in pagination.keys() and \
                        "total_result" in pagination.keys():
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
                            "templated": False
                        })
                    nb_next_page = items_per_page * start_page
                    nb_next_page += items_on_page
                    if total_result > nb_next_page:
                        kwargs["start_page"] = start_page + 1
                        data["links"].append({
                            "href": url_for(endpoint, **kwargs),
                            "type": "next",
                            "templated": False
                        })
                        if items_per_page == 0 or total_result == 0:
                            kwargs["start_page"] = 0
                        else:
                            nb_last_page = total_result - 1
                            nb_last_page = nb_last_page / items_per_page
                            kwargs["start_page"] = nb_last_page
                            data["links"].append({
                                "href": url_for(endpoint, **kwargs),
                                "type": "last",
                                "templated": False
                            })

                        del kwargs["start_page"]
                    data["links"].append({
                        "href": url_for(endpoint, **kwargs),
                        "type": "first",
                        "templated": False
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
                kwargs = self.prepare_kwargs(kwargs, data)
                data["links"].append(create_external_link("v1.coverage", rel='related', templated=True, **kwargs))
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
                kwargs = self.prepare_kwargs(kwargs, data)
                for collection in self.collections:
                    data["links"].append(create_external_link("v1.{c}.collection".format(c=collection),
                                                     rel=collection, templated=True, **kwargs))
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
            kwargs = self.prepare_kwargs(kwargs, data)
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
                        kwargs["id"] = "{" + obj + ".id}"
                    endpoint = "v1." + kwargs["collection"] + "."
                    endpoint += "id" if "region" in kwargs.keys() or\
                        "lon" in kwargs.keys()\
                                        else "redirect"
                    collection = kwargs["collection"]
                    to_pass = {k:v for k,v in kwargs.iteritems() if k != "collection"}
                    data["links"].append(create_external_link(url=endpoint, rel=collection,
                                                              _type=obj, templated=True,
                                                             **to_pass))
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data
        return wrapper

    def get_objets(self, data, collection_name=None):
        if hasattr(data, 'keys'):
            if "id" in data.keys() \
               and (not "href" in data.keys()) \
               and collection_name:
                self.data.add(collection_name)
            for key, value in data.iteritems():
                self.get_objets(value, key)
        if isinstance(data, (list, tuple)):
            for item in data:
                self.get_objets(item, collection_name)


class clean_links(object):

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response = f(*args, **kwargs)
            if isinstance(response, tuple):
                data, code, header = unpack(response)
                if code != 200:
                    return data, code, header
            else:
                data = response
            if isinstance(data, OrderedDict) and "links" in data.keys():
                for link in data['links']:
                    link['href'] = link['href'].replace("%7B", "{")\
                                               .replace("%7D", "}")\
                                               .replace("%3B", ";")
            if isinstance(response, tuple):
                return data, code, header
            else:
                return data
        return wrapper
