# coding=utf8
from conf import base_url
from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render
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

    def journey(self, obj, region_name, details):
        result = {
                'duration': obj.duration,
                'nb_transfers': obj.nb_transfers,
                'departure_date_time': obj.departure_date_time,
                'arrival_date_time': obj.arrival_date_time,
                'href': 'bliii'
                }
        if obj.HasField('origin'):
            result['origin'] = self.place(obj.origin, region_name)
        if obj.HasField('destination'):
            result['destination'] = self.place(obj.destination, region_name)
        return result

    def place(self, obj, region_name):
        if obj.object_type == type_pb2.STOP_AREA:
            return self.stop_area(obj.stop_area, region_name, False)
        elif obj.object_type == type_pb2.STOP_POINT:
            return self.stop_point(obj.stop_point, region_name, False)

    def region(self, obj, region_id, details=False):
        result = {
                'id': region_id,
                'href': self.base_url + region_id,
                'start_production_date': obj.start_production_date,
                'end_production_date': obj.end_production_date,
                'status': 'running',
                'shape': obj.shape
                }
        if details:
            links = {}
            for key in collections_to_resource_type:
                links[key] = result['href'] + '/' + key
            result['links'] = links
        return result


    def generic_type(self, type, obj, region, details):
        result = {
                'name': obj.name,
                'id': region + '/' + type + '/' + obj.uri
                }
        result['href'] = self.base_url + result['id'] 
        try: # si jamais y’a pas de champs coord dans le .proto, HasField gère une exception
            if obj.HasField('coord'):
                result['coord'] = {'lon': obj.coord.lon, 'lat': obj.coord.lat}
        except:
            pass

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

def region(region_name, details):
    renderer = json_renderer(base_url + '/v1/')
    response = {}
    req = request_pb2.Request()
    req.requested_api = type_pb2.METADATAS
    try:
        resp = NavitiaManager().send_and_receive(req, region_name)
        return renderer.region(resp.metadatas, region_name, details)
    except DeadSocketException :
        return {"id" : region, "status" : "not running", "href": base_url + "/v1/" + region_name}
    except RegionNotFound:
         return {"id" : region, "status" : "not found", "href": base_url + "/v1/" + region_name}

def regions(request, format='json'):
    response = {'regions': []}
    for region_name in NavitiaManager().instances.keys() :
        response['regions'].append(region(region_name, False))
    return render(response, format,  request.args.get('callback'))

def region_metadata(region_name, format, callback):
    tmp_resp = region(region_name, True)
    if tmp_resp['status'] == 'not found':
        return generate_error("Unknown region: " + region, status=404)
    else:
        return render({'regions': tmp_resp}, format, callback)

def isochrone(path, uri, response, format, callback):
    renderer = json_renderer(base_url + '/v1/')
    response_dict = {'journeys': []}
    for journey in response.journeys:
        response_dict['journeys'].append(renderer.journey(journey, uri.region(), False))
#        journey['href'] = base_url + path + '/' + uri.region() + '/stop_points/' +journey['destination']['stop_point']['uri']
    return render(response_dict, format, callback)


