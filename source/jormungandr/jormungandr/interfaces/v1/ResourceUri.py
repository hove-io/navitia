# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from flask_restful import abort
from jormungandr.interfaces.v1.converters_collection_type import collections_to_resource_type
from jormungandr.interfaces.v1.converters_collection_type import resource_type_to_collection
from jormungandr.interfaces.v1.StatedResource import StatedResource
from jormungandr.interfaces.v1.make_links import add_id_links, clean_links, add_pagination_links
from functools import wraps
from collections import deque
from flask import url_for
from flask_restful.utils import unpack
from jormungandr.authentication import authentication_required
from six.moves import map


def protect(uri):
    """
    we protect the uri so there can be special character in them
    """
    return '"' + uri.replace('"', '\\"') + '"'


class ResourceUri(StatedResource):
    def __init__(self, authentication=True, links=True, *args, **kwargs):
        StatedResource.__init__(self, *args, **kwargs)
        self.region = None
        if links:
            self.get_decorators.append(add_id_links())
            self.get_decorators.append(add_computed_resources(self))
            self.get_decorators.append(add_pagination_links())
            self.get_decorators.append(clean_links())

        if authentication:
            # Some rare endpoints (eg journey) must handle the authentication by themselves, thus deactivate it
            # by default ALWAYS use authentication=True
            self.get_decorators.append(authentication_required)

    def get_filter(self, items, args):

        # handle headsign
        if args.get("headsign"):
            f = u"vehicle_journey.has_headsign({})".format(protect(args["headsign"]))
            if args.get("filter"):
                args["filter"] = '({}) and {}'.format(args["filter"], f)
            else:
                args["filter"] = f

        filter_list = ['({})'.format(args["filter"])] if args.get("filter") else []
        type_ = None

        for item in items:
            if not type_:
                if item != "coord":
                    if item == "calendars":
                        type_ = 'calendar'
                    else:
                        if item not in collections_to_resource_type:
                            abort(400, message="unknown type: {}".format(item))
                        type_ = collections_to_resource_type[item]
                else:
                    type_ = "coord"
            else:
                if type_ == "coord" or type_ == "address":
                    splitted_coord = item.split(";")
                    if len(splitted_coord) == 2:
                        lon, lat = splitted_coord
                        object_type = "stop_point"
                        if self.collection == "pois":
                            object_type = "poi"
                        filter_ = '{obj}.coord DWITHIN({lon},{lat},{distance})'.format(
                            obj=object_type, lon=lon, lat=lat, distance=args.get('distance', 200)
                        )
                        filter_list.append(filter_)
                    else:
                        filter_list.append(type_ + ".uri=" + protect(item))
                else:
                    filter_list.append(type_ + ".uri=" + protect(item))
                type_ = None
        # handle tags
        tags = args.get("tags[]", [])
        if tags:
            filter_list.append('disruption.tags({})'.format(' ,'.join([protect(t) for t in tags])))
        return " and ".join(filter_list)


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
            if 'links' not in data:
                return response
            collection = None
            kwargs["_external"] = True
            templated = True
            for key in data:
                if key == 'disruptions' and collection is not None:
                    # disruption is a special case since it can be present in all responses
                    continue
                if key in collections_to_resource_type:
                    collection = key
                if key in resource_type_to_collection:
                    collection = resource_type_to_collection[key]
            if collection is None:
                return response
            kwargs["uri"] = collection + '/'
            if "id" in kwargs:
                kwargs["uri"] += kwargs["id"]
                del kwargs["id"]
                templated = False
            else:
                kwargs["uri"] += '{' + collection + ".id}"
            if (
                collection in ['stop_areas', 'stop_points', 'lines', 'routes', 'addresses']
                and "region" in kwargs
            ):
                for api in ['route_schedules', 'stop_schedules', 'arrivals', 'departures', "places_nearby"]:
                    data['links'].append(
                        {"href": url_for("v1." + api, **kwargs), "rel": api, "type": api, "templated": templated}
                    )
            if collection in ['stop_areas', 'stop_points', 'addresses']:
                data['links'].append(
                    {
                        "href": url_for("v1.journeys", **kwargs),
                        "rel": "journeys",
                        "type": "journey",
                        "templated": templated,
                    }
                )
            # for lines we add the link to the calendars
            if 'region' in kwargs:
                if collection == 'lines':
                    data['links'].append(
                        {
                            "href": url_for("v1.calendars", **kwargs),
                            "rel": "calendars",
                            "type": "calendar",
                            "templated": templated,
                        }
                    )
                if collection in ['stop_areas', 'lines', 'networks']:
                    data['links'].append(
                        {
                            "href": url_for("v1.traffic_reports", **kwargs),
                            "rel": "disruptions",
                            "type": "disruption",
                            "templated": templated,
                        }
                    )
            if isinstance(response, tuple):
                return data, code, header
            else:
                return data

        return wrapper


class complete_links(object):
    # This list should not change
    EXPECTED_ITEMS = set(['category', 'id', 'internal', 'rel', 'type', 'comment_type'])

    def __init__(self, resource):
        self.resource = resource

    def make_and_get_link(self, elem, collect):
        if collect == "notes":
            note = {"id": elem['id'], "category": elem['category'], "value": elem['value'], "type": collect}
            if 'comment_type' in elem:
                note['comment_type'] = elem['comment_type']
            return note
        type_ = "Add" if elem['except_type'] == 0 else "Remove"
        return {"id": elem['id'], "date": elem['date'], "type": type_}

    def get_links(self, data):
        queue = deque()
        result = {"notes": [], "exceptions": []}
        queue.extend(list(data.values()))
        while queue:
            elem = queue.pop()
            if isinstance(elem, (list, tuple)):
                queue.extend(elem)
            elif hasattr(elem, 'keys'):
                collect = elem.get('type')
                if collect in result:
                    link = self.make_and_get_link(elem, collect)
                    if link.get('id') not in [l.get('id') for l in result[collect]]:
                        result[collect].append(link)
                    # Delete all items from link not in expected_keys
                    del_keys = set(elem.keys()).difference(self.EXPECTED_ITEMS)
                    if len(del_keys):
                        list(map(elem.pop, del_keys))
                else:
                    queue.extend(list(elem.values()))
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
                # Add notes and exceptions
                data.update(self.get_links(data))
            if isinstance(objects, tuple):
                return data, code, header
            else:
                return data

        return wrapper
