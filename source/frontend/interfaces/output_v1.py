# coding=utf8
from conf import base_url
from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render
from protobuf_to_dict import protobuf_to_dict
import request_pb2, type_pb2, response_pb2
from uri import collections_to_resource_type
from error import generate_error

def add_uri(collection_name, region, collection):
    for item in collection:
        item.uri = region+"/"+collection_name+"/"+item.uri
    return collection

#Objets à partir desquels on peut demander un nearby ou initier un itinéraire
nearbyable_types = ['stop_points', 'stop_areas', 'coords', 'pois']

def render_ptref(response, region, resource_type, uid, format, callback):
    resp_dict = protobuf_to_dict(response)
    if 'error' in resp_dict and resp_dict['error'] == '404':
        return generate_error("Object not found", status=404)

    if not resource_type in resp_dict:
        resp_dict[resource_type] = []
    for item in resp_dict[resource_type]:
        item['id'] = region +"/"+resource_type+"/"+item['uri']
        item['href'] = base_url + '/v1/' + item['id']
        if uid: #On a demandé un objet spécifique, donc on file plein des liens
            item['links'] = {
                    'schedules' : item['href'] + '/schedules',
                    }
            if resource_type in nearbyable_types:
                item['links']['nearby'] = item['href'] + '/nearby'
                item['links']['journeys'] = item['href'] + '/journeys'
        # On ajoute les liens vers tous les objets, à part vers lui-même
            for key in collections_to_resource_type:
                if key != resource_type:
                    item['links'][key] = item['href'] + '/' + key

        del(item['uri'])

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


