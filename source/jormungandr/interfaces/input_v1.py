from werkzeug.wrappers import Response
from uri import Uri, InvalidUriException, collections_to_resource_type, types_not_ptrefable
from apis import validate_and_fill_arguments, InvalidArguments, validate_pb_request
from instance_manager import NavitiaManager
from error import generate_error
import datetime
import output_v1
import logging
def coverage(request):
    return output_v1.coverage(request, format=request.accept_mimetypes)

def coord(request, lon, lat):
    return output_v1.coord(request, lon, lat)

def departures_arrivals(type, request, uri1):
    u = None
    try:
        u = Uri(uri1)
    except InvalidUriException:
        return generate_error("Invalid uri")

    if u.is_region:
        if len(u.objects) == 0:
            return generate_error(type+" are only available from a stop area or a stop point", 501)

        resource_type, uid = u.objects.pop()
        if resource_type not in ['stop_points', 'stop_areas']:
            return generate_error(type+" are only available from a stop area or a stop point", 501)
        filter_ = collections_to_resource_type[resource_type]+".uri="+uid
    else:
        filter_ = "stop_point.coord DWITHIN("+str(u.lon)+","+str(u.lat)+", 200)"

    req = {
            'filter': [filter_],
            'from_datetime': [datetime.datetime.now().strftime("%Y%m%dT1337")]
          }
    arguments = validate_and_fill_arguments("next_"+type, req)
    return NavitiaManager().dispatch(arguments, u.region(), "next_"+type), u.region()

def departures(request, uri1):
    response, region = departures_arrivals("departures", request, uri1)
    return output_v1.departures(response,  region, request.accept_mimetypes, request.args.get("callback"))

def arrivals(request, uri1):
    response, region = departures_arrivals("arrivals", request, uri1)
    return output_v1.arrivals(response,  region, request.accept_mimetypes, request.args.get("callback"))


def uri(request, uri=None):
    u = None
    try:
        u = Uri(uri)
    except InvalidUriException, e:
        return generate_error("Invalid uri")
    if len(u.objects)==0:
        if u.is_region:
            return output_v1.coverage(request, u.region(),
                                      request.accept_mimetypes)
        else:
            return output_v1.coord(request, str(u.lon), str(u.lat))
    resource_type, uid = u.objects.pop()
    if resource_type in types_not_ptrefable:
        return generate_error("Type : " + resource_type + " not consultable yet", 501)
    req = {}
    if 'start_page' in request.args:
        req['startPage'] = [int(request.args.get('start_page'))]

    filters = []
    if uid:
        filters.append(collections_to_resource_type[resource_type]+".uri="+uid)
    else:
        for resource_type2, uid2 in u.objects:
            filters.append(collections_to_resource_type[resource_type2]+".uri="+uid2)

    if 'filter' in request.args:
        filters.append(request.args.get('filter'))

    if len(filters)>0:
        req["filter"] = [" and ".join(filters)]
    arguments = None
    try:
        arguments = validate_and_fill_arguments(resource_type, req)
        response = NavitiaManager().dispatch(arguments, u.region(),
                                             resource_type)
    except InvalidArguments, e:
        return generate_error(e.message)
    return output_v1.render_ptref(response, u.region(), resource_type, uid,
                                  request.accept_mimetypes,
                                  request.args.get("callback"))

def places(request, region):
    arguments = validate_pb_request("places", request)
    if arguments.valid:
        response = NavitiaManager().dispatch(arguments, region, "places")
        return output_v1.places(response, region, request.accept_mimetypes,
                                request.args.get("callback"))
    else:
        return generate_error("Invalid arguments : " + arguments.details)


def route_schedules(request, uri1=None):
    u = None
    req = {"filter" : [], "from_datetime": []}
    region = ""
    if(uri1):
        try:
            u = Uri(uri1)
        except InvalidUriException, e:
            return generate_error("Invalid uri", e.message)

        resource_type, uid = u.objects[-1]
        region = u.region()
        if not uid:
            return generate_error("You cannot ask a route schedule with this object, not implemented", status=501)
        req["filter"] =[collections_to_resource_type[resource_type]+".uri="+uid]
    elif(request.args.get("filter")):
        filter_ = request.args.get("filter", str)
        if filter_.count("=") == 0 :
            return generate_error("Invalid filter")
        region = filter_.split("=")[1].split("/")[0]
        filters = []
        for f in filter_.split("="):
            filters.append(f.replace(f, f.split("/")[-1]))
        tmp = "=".join(filters)
        req["filter"] = [tmp]
    else:
        return generate_error("Invalid request")
    if not request.args.get("from_datetime"):
        req["from_datetime"] = [datetime.datetime.now().strftime("%Y%m%dT1337")]
    else:
        req["from_datetime"] = [request.args.get("from_datetime", str)]

    if request.args.get("duration"):
        req["duration"] = [int(request.args.get("duration"))]
    if request.args.get("depth"):
        req["depth"] = [int(request.args.get("depth"))]

    arguments = validate_and_fill_arguments("route_schedules", req)
    if arguments.valid:
        response = NavitiaManager().dispatch(arguments, region, "route_schedules")
        return output_v1.route_schedules(response, region,
                                         request.accept_mimetypes,
                                         request.args.get("callback"))
    else:
        return generate_error("Invalid arguments : " + arguments.details)

def stop_schedules(request, uri1, uri2=None):
    return_ = '{"apiname":"schedules", uri1:"'+uri1+'"'
    if uri2:
        return_ += ', uri2:"'+uri2+'"'
    return Response(return_ +'}', mimetype='text/plain;charset=utf-8')

def do_entry_point(uri):
    if uri.is_region:
        resource_type, uid = uri.objects.pop()
        return uid
    else:
        return "coord:"+uri.uri.replace(":", ";")

def journeys(request, uri1=None):
    req = {}
    #We catch the from argument
    from_ = ""
    region = ""
    if uri1:
        u = Uri(uri1)
        from_ = do_entry_point(u)
        if not from_:
            return generate_error("Unable to compute journeys from this type")
        region = u.region()
    else:
        from_ = request.args.get("from", str)
        region = NavitiaManager().key_of_id(from_)
    if from_[:len("address")] == "address":
        region_complete = ":" + region
        index1 = from_.find(region_complete)
        from_ = from_[:index1] + from_[index1+len(region_complete):]
    req["origin"] = [from_]
    #We fill the datetime argument
    if not request.args.get("datetime"):
        req["datetime"] = [datetime.datetime.now().strftime("%Y%m%dT1337")]
    else:
        req["datetime"] = [request.args.get("datetime", str)]
    #If it's a journey from one point to another
    if request.args.get("to"):
        to_ = request.args.get("to")
        if region != NavitiaManager().key_of_id(to_):
            error = "The origin and destination are not in the same region"
            error += ", not implemented"
            return generate_error(error, status=501)
        if to_[:len("address")] == "address":
            region_complete = ":" + region
            index1 = to_.find(region_complete)
            to_ = to_[:index1] + to_[index1+len(region_complete):]
        req["destination"] = [to_]
        try:
            arguments = validate_and_fill_arguments("journeys", req)
        except InvalidArguments, exception:
            error = "Invalid Arguments : "
            return generate_error(error + str(exception.message))
        if arguments.valid:
            response = NavitiaManager().dispatch(arguments, region, "journeys")
            return output_v1.journeys(request.path, region, from_, response,
                                      request.accept_mimetypes,
                                      request.args.get("callback"))
        else:
            return generate_error("Invalid arguments : " + arguments.details)
    else:
        #All the journeys from that point
        try:
            arguments = validate_and_fill_arguments("isochrone", req)
        except InvalidArguments, exception:
            error = "Invalid Arguments : "
            return generate_error(error + str(exception.message))
        if arguments.valid:
            response = NavitiaManager().dispatch(arguments, region, "isochrone")
            return output_v1.journeys(arguments, region, from_, response,
                                      request.accept_mimetypes,
                                      request.args.get("callback"), True)
        else:
            return generate_error("Invalid arguments : " + arguments.details)


def nearby(request, uri1=None, uri2=None):
    u = None
    try:
        u = Uri(uri1)
    except InvalidUriException, e:
        return generate_error("Invalid uri" + e.message)
    resource_type, uid = "", ""
    if u.is_region:
        resource_type, uid = u.objects.pop()
    else:
        resource_type = "coord"
        uid = "coord:"+str(u.lon)+":"+str(u.lat)
    req = {}
    if uid:
        req["uri"] = [uid]
    else:
        return generate_error("You cannot search places around this object, not implemented", status=501)
    try:
        arguments = validate_and_fill_arguments("places_nearby", req)
    except InvalidArguments, e:
        return generate_error("Invalid Arguments : " + e.message)
    if arguments.valid:
        response = NavitiaManager().dispatch(arguments, u.region(), "places_nearby")
        return output_v1.nearby(response, u, request.accept_mimetypes, request.args.get("callback"))
    else:
        return generate_error("Invalid arguments : " + arguments.details)


def index(request):
    return output_v1.index(request, format=request.accept_mimetypes)
