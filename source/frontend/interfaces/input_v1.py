from werkzeug.wrappers import Response
from uri import Uri, InvalidUriException

def regions(request):
    return Response('{apiname:"regions"}', mimetype='text/plain;charset=utf-8')

def uri(request, uri):
    u = None
    try:
        u = Uri(uri)
    except InvalidUriException, e:
        print e

    u.objects.reverse()
    resource_type, uid = u.objects.pop()
    req = "places?type[]="+resource_type
    if uid:
        req = req + "&filter="+resource_type+".uri="+uid+""
    else:
        filters = []
        for resource_type, uid in u.objects:
            filters.append(resource_type+".uri="+uid)
        req = req + " and ".join(filters)
    return Response('{apiname:"one_uri", uri:"'+uri+'"}', mimetype='text/plain;charset=utf-8')

def places(request, uri):
    return Response('{apiname:"places", uri:"'+uri+'"}', mimetype='text/plain;charset=utf-8')

def schedules(request, uri1, uri2=None):
    return_ = '{apiname:"schedules", uri1:"'+uri1+'"'
    if uri2:
        return_ += ', uri2:"'+uri2+'"'
    return Response(return_ +'}', mimetype='text/plain;charset=utf-8')

def journeys(request, uri1, uri2=None):    
    return_ = '{apiname:"journeys", uri1:"'+uri1+'"'
    if uri2:
        return_ += ', uri2:"'+uri2+'"'
    return Response(return_+'}', mimetype='text/plain;charset=utf-8')

def nearby(request, uri1, uri2=None):
    return_ = '{apiname:"nearby", uri1:"'+uri1+'"'
    if uri2:
        return_ += ', uri2:"'+uri2+'"'
    return Response(return_+'}', mimetype='text/plain;charset=utf-8')

