from werkzeug.wrappers import Response
import os, re
from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from error import generate_error
import request_pb2, type_pb2
from renderers import protobuf_to_dict, render, render_from_protobuf
from apis import validate_pb_request, InvalidArguments, ApiNotFound

def on_index(request, region = None ):
    path = os.path.join(os.path.dirname(__file__), 'static','lost.html')
    file = open(path, 'r')
    return Response(file.read(), mimetype='text/html;charset=utf-8')


def on_regions(request, format):
    response = {'regions': []}
    for region in NavitiaManager().instances.keys() : 
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        try:
            resp = NavitiaManager().send_and_receive(req, region)
            resp_dict = protobuf_to_dict(resp) 
            if 'metadatas' in resp_dict.keys():
                resp_dict['metadatas']['region_id'] = region                
                response['regions'].append(resp_dict['metadatas'])
        except DeadSocketException :
            response['regions'].append({"region_id" : region, "status" : "not running"})
        except RegionNotFound:
            response['regions'].append({"region_id" : region, "status" : "not found"})

    return render(response, format,  request.args.get('callback'))

def find_region(uri):
    uri_re = re.match('^coord:(.+):(.+)$', uri)
    region = None
    if uri_re:
        try:
            lon = float(uri_re.group(1))
            lat = float(uri_re.group(2))
            region = NavitiaManager().key_of_coord(lon, lat)
        except:
            pass
    return region

def universal_journeys(api, request, format):
    region = find_region(request.args.get("origin", ""))
    if region:
        return on_api(request, region, api, format)
    else:
        return generate_error("Unable to deduce the region from the uri. Is it a valid coordinate?", status=404)

def on_universal_journeys(api):
    return lambda request, format: universal_journeys(api, request, format)

def on_universal_places_nearby(request, format):
    region = find_region(request.args.get("uri", ""))
    if region:
        return on_api(request, region, "places_nearby", format)
    else:
        return generate_error("Unable to deduce the region from the uri. Is it a valid coordinate?", status=404)
   
def on_api(request, region, api, format):
    arguments = None
    response = None
    try:
        arguments = validate_pb_request(api, request)
    except InvalidArguments, e:
        return generate_error(e.message)
    except ApiNotFound, e:
        return generate_error("Unknown api : " + api, 404)
    response = NavitiaManager().dispatch(arguments, region, api)
    return render_from_protobuf(response, format, request.args.get("callback"))
