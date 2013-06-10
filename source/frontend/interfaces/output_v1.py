# coding=utf8
from conf import base_url
from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render
from protobuf_to_dict import protobuf_to_dict
import request_pb2, type_pb2, response_pb2
from uri import collections_to_resource_type
from error import generate_error

class json_renderer:
    nearbyable_types = ['stop_points', 'stop_areas', 'coords', 'pois']

    def __init__(self, base_url):
        self.base_url = base_url

    def stop_point(self, obj, region, details=False):
        return self.generic_type('stop_points', obj, region, details)

    def stop_area(self, obj, region, details=False):
        return self.generic_type('stop_areas', obj, region, details)

    def route(self, obj, region, details=False):
        return self.generic_type('routes', obj, region, details)

    def line(self, obj, region, details=False):
        return self.generic_type('lines', obj, region, details)

    def network(self, obj, region, details=False):
        return self.generic_type('networks', obj, region, details)

    def commercial_mode(self, obj, region, details=False):
        return self.generic_type('commercial_modes', obj, region, details)

    def physical_mode(self, obj, region, details=False):
        return self.generic_type('physical_modes', obj, region, details)

    def company(self, obj, region, details=False):
        return self.generic_type('companies', obj, region, details)


    def generic_type(self, type, obj, region, details):
        result = {
                'name': obj.name,
                'id': region + '/' + type + '/' + obj.uri
                }
        result['href'] = self.base_url + '/' + result['id'] 

        if details:
            result['links'] = {}
            if type in self.nearbyable_types:
                result['links']['nearby'] = result['href'] + '/nearby'
                result['links']['journeys'] = result['href'] + '/journeys'
            for key in collections_to_resource_type:
                if key != type:
                    result['links'][key] = result['href'] + '/' + key
        return result



def get_field_by_name(obj, name):
    for field_tuple in obj.ListFields():
        print field_tuple[0].name
        if field_tuple[0].name == name:
            return field_tuple[1] 
    return None

def render_ptref(response, region, resource_type, uid, format, callback):
    if response.error and response.error == '404':
        return generate_error("Object not found", status=404)

    resp_dict = {resource_type: []}
    renderer = json_renderer(base_url + '/v1/')
    items = get_field_by_name(response, resource_type)
    if items:
        for item in items:
            resp_dict[resource_type].append(renderer.generic_type(resource_type, item, region, uid != None))
    else:
        print "Not found :'(", resource_type

    return render(resp_dict, format, callback)



def regions(request, format='json'):
    response = {'regions': []}
    for region in NavitiaManager().instances.keys() :
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        try:
            resp = NavitiaManager().send_and_receive(req, region)
            resp_dict = protobuf_to_dict(resp)
            if 'metadatas' in resp_dict.keys():
                resp_dict['metadatas']['id'] = region
                resp_dict['metadatas']['href'] = base_url + "/v1/" + region
                response['regions'].append(resp_dict['metadatas'])
        except DeadSocketException :
            response['regions'].append({"id" : region, "status" : "not running", "href": base_url + "/v1/" + region})
        except RegionNotFound:
            response['regions'].append({"id" : region, "status" : "not found", "href": base_url + "/v1/" + region})

    return render(response, format,  request.args.get('callback'))

def region_metadata(request, format, parsed_uri):
    response = {'regions': []}
    req = request_pb2.Request()
    req.requested_api = type_pb2.METADATAS
    try:
        resp = NavitiaManager().send_and_receive(req, parsed_uri.region_)
        resp_dict = protobuf_to_dict(resp)
        if 'metadatas' in resp_dict.keys():
            base = base_url + '/v1/' + parsed_uri.region_ + '/'
            response['regions'].append(resp_dict['metadatas'])
            response['regions'][0]['id'] = parsed_uri.region_
            links = {'stop_areas': base + 'stop_areas',
                    'stop_points': base + 'stop_points',
                    'lines': base + 'lines',
                    'routes': base + 'routes',
                    'physical_modes': base + 'physical_modes',
                    'commercial_modes': base + 'commercial_modes',
                    'networks': base + 'networks',
                    'companies': base + 'companies',
                    }
            response['links'] = links
    except DeadSocketException :
        response['regions'].append({"id" : region, "status" : "not running", "href": base})
    except RegionNotFound:
        return generate_error("Unknown region: " + region, status=404)
    return render(response, format, request.args.get('callback'))

def isochrone(path, uri, response, format, callback):
    resp_dict = protobuf_to_dict(response)
    for journey in resp_dict['journeys']:
        journey['href'] = base_url + path + '/' + uri.region() + '/stop_points/' +journey['destination']['stop_point']['uri']
    return render(resp_dict, format, callback)


