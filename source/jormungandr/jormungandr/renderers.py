# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

import json
import dict2xml
from jormungandr.protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Response
from .error import generate_error


def render(dico, formats, callback, status=200):
    if 'application/json' in formats or 'json' in formats:
        json_str = json.dumps(dico, ensure_ascii=False)
        if callback == '' or callback is None:
            result = Response(json_str,
                              mimetype='application/json;charset=utf-8',
                              status=status)
        else:
            result = Response(callback + '(' + json_str + ')',
                              mimetype='application/json;charset=utf-8',
                              status=status)
    elif 'text/html' in formats or 'text/plain' in formats:
        json_str = json.dumps(add_links(dico), ensure_ascii=False, indent=4)
        result = Response('<html><pre>' + json_str + '</pre></html>',
                          mimetype='text/html;charser=utf8', status=status)
    elif 'txt' in formats:
        result = Response(json.dumps(dico, ensure_ascii=False, indent=4),
                          mimetype='text/plain;charset=utf-8', status=status)
    elif 'xml' in formats:
        response = '<?xml version="1.0" encoding="UTF-8"?>\n'
        response += dict2xml.dict2xml(dico, wrap="Response")
        result = Response(response,
                          mimetype='application/xml;charset=utf-8',
                          status=status)
    elif 'pb' in formats:
        r = generate_error('Protocol buffer not supported for this request',
                           status=404)
        result = render_from_protobuf(r, 'json')
    else:
        error = "Unknown file format format('" + unicode(formats) + "')"
        error += ". Please choose .json, .txt, .xml or .pb"
        r = generate_error(error, status=404)
        result = render_from_protobuf(r, 'json')
    result.headers.add('Access-Control-Allow-Origin', '*')
    return result


def search_links(dico):
    result = {}
    if "links" in dico:
        for link in dico['links']:
            if 'templated' in link and link['templated']:
                if link['rel'] == "related":
                    for key, val in dico.iteritems():
                        if key != "links" and key != "pagination":
                            result[key] = link['href']
                else:
                    result[link['rel']] = link['href']
    return result


def add_a(obj, links, last_type):
    if last_type in links:
        result = "<a href='"
        result += links[last_type].replace("{" + last_type + ".id}", obj)
        result += "'>" + obj + "</a>"
        return result
    else:
        return obj


def add_links(obj):
    links = search_links(obj)
    result = add_links_recc(obj, links)

    # if obj.has_key("links"):
    #    for link in obj["links"]:
    #        if 'templated' in link and not link['templated']:
    #         link['href'] = "<a href='"+link['href']+"'>"+link['href']+'</a>'
    return result


def add_links_recc(obj, links, last_type=None):
    if isinstance(obj, type({})):
        for key, value in obj.iteritems():
            object_type = last_type
            if key == "id":
                obj[key] = add_a(value, links, object_type)
            if key == "href" and last_type != "links":
                obj[key] = "<a href='" + obj[key] + "'>" + obj[key] + '</a>'
            if key == "links":
                for link in obj["links"]:
                    if 'templated' in link and not link['templated']:
                        new_link = "<a href='" + link['href'] + "'>"
                        new_link += link['href'] + '</a>'
                        link['href'] = new_link
            add_links_recc(obj[key], links, key)
    elif isinstance(obj, type([])):
        for value in obj:
            add_links_recc(value, links, last_type)
    return obj


def render_from_protobuf(pb_resp, format, callback, status=200):
    if pb_resp.status_code:
        status = pb_resp.status_code
        pb_resp.ClearField("status_code")
    if format == 'pb':
        return Response(pb_resp.SerializeToString(),
                        mimetype='application/octet-stream', status=status)
    else:
        return render(protobuf_to_dict(pb_resp, use_enum_labels=True), format,
                      callback, status)
