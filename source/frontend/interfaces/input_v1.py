from werkzeug.wrappers import Response
from uri import Uri, InvalidUriException
from apis import validate_and_fill_arguments, InvalidArguments, validate_pb_request
from instance_manager import NavitiaManager
from renderers import render_from_protobuf
from error import generate_error
import datetime
from output_v1 import add_uri

def regions(request):
    return Response('{"apiname":"regions"}', mimetype='text/plain;charset=utf-8')


def collections_to_resource_type(collection):
    m = {"stop_points": "stop_point", "connections":"connection",
         "networks":"network", "commercial_modes":"commercial_mode",
         "physical_modes":"physical_mode", "companies":"company",
         "stop_areas":"stop_area", "lines":"line"}
    if collection in m:
        return m[collection]
    else:
        return ""

def uri(request, uri):
    u = None
    try:
        u = Uri(uri)
    except InvalidUriException, e:
        return generate_error("Invalid uri")
    resource_type, uid = u.objects.pop()
    req = {}
    if uid:
        req["filter"] = [collections_to_resource_type(resource_type)+".uri="+uid]
    else:
        filters = []
        for resource_type2, uid2 in u.objects:
            filters.append(collections_to_resource_type(resource_type2)+".uri="+uid2)
        if len(filters)>0:
            req["filter"] = [" and ".join(filters)]
    arguments = None
    try : 
        arguments = validate_and_fill_arguments(resource_type, req)
        response = NavitiaManager().dispatch(arguments.arguments, u.region(), resource_type)
    except InvalidArguments, e:
        return generate_error(e.message)
    add_uri(resource_type, u.region(), response.__getattribute__(resource_type ))
    return render_from_protobuf(response, "json", request.args.get("callback"))

def places(request, uri):
    u = None
    try:
        u = Uri(uri)
    except InvalidUriException, e:
        return generate_error("Invalid uri", e.message)

    if len(u.objects) > 0:
        return generate_error("You cannot search places within this object", status=501)

    arguments = validate_pb_request("places", request)
    if arguments.valid:
        response = NavitiaManager().dispatch(arguments.arguments, u.region(), "places")
        return render_from_protobuf(response, "json", request.args.get("callback"))
    else:
        return generate_error("Invalid arguments : " + arguments.details)
    

def schedules(request, uri1, uri2=None):
    return_ = '{"apiname":"schedules", uri1:"'+uri1+'"'
    if uri2:
        return_ += ', uri2:"'+uri2+'"'
    return Response(return_ +'}', mimetype='text/plain;charset=utf-8')

def journeys(request, uri1, uri2=None, datetime=None):    
    acceptable_types = ["stop_areas", "stop_points", "address", "coord", "poi"]
    u1 = None
    try:
        u1 = Uri(uri1)
    except InvalidUriException, e:
        return generate_error("Invalid uri" + e.message)
   
    resource_type1, uid1 = u1.objects.pop()
    if not uid1 or not resource_type1 in acceptable_types:
        return generate_error("Unsupported uri : " + uri1, status=501)
    req = {}
    req["origin"] = [uid1]
    req["datetime"] = [datetime.datetime.now().strftime("%Y%m%dT%H%M%S")]
    if uri2:
        u2 = None
        try:
            u2 = Uri(uri2)
        except InvalidUriException, e:
            return generate_error("Invalid uri" + e.message)
        resource_type2, uid2 = u2.objects.pop()
        if not uid2 or not resource_type2 in acceptable_types:
            return generate_error("Unsupported uri : " + uri2, status=501)
        if u1.region() != u2.region():
            return generate_error("The regions has to be the same", status=501)
        req["destination"] = [uid2]
        try:
            arguments = validate_and_fill_arguments("journeys", req)
        except InvalidArguments, e:
            return generate_error("Invalid Arguments : " + str(e.message))
        if arguments.valid:
            response = NavitiaManager().dispatch(arguments.arguments, u1.region(), "journeys")
            return render_from_protobuf(response, "json", request.args.get("callback"))
        else:
            return generate_error("Invalid arguments : " + arguments.details)
    else:
        try:
            arguments = validate_and_fill_arguments("isochrone", req)
        except InvalidArguments, e:
            return generate_error("Invalid Arguments : " + e.message)
        if arguments.valid:
            response = NavitiaManager().dispatch(arguments.arguments, u1.region(), "isochrone")
            return render_from_protobuf(response, "json", request.args.get("callback"))
        else:
            return generate_error("Invalid arguments : " + arguments.details)


def nearby(request, uri1, uri2=None):
    u = None
    try:
        u = Uri(uri1)
    except InvalidUriException, e:
        return generate_error("Invalid uri" + e.message)
    resource_type, uid = u.objects.pop()
    req = {}
    if uid:
        req["uri"] = [uid]
    else:
        return generate_error("You cannot search places within this object", status=501)
    
    try:
        arguments = validate_and_fill_arguments("places_nearby", req)
    except InvalidArguments, e:
        return generate_error("Invalid Arguments : " + e.message)
    if arguments.valid:
        response = NavitiaManager().dispatch(arguments.arguments, u.region(), "places_nearby")
        return render_from_protobuf(response, "json", request.args.get("callback"))
    else:
        return generate_error("Invalid arguments : " + arguments.details)

