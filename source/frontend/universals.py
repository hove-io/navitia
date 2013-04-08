import re
from apis import Apis
from werkzeug import Response
from instance_manager import NavitiaManager


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


def universal_journeys(api, request, version, format):
    region = find_region(request.args.get("origin", ""))
    if region:
        return NavitiaManager().dispatch(request, version, region, api, format)
    else:
        return Response("Journeys without specifying a region only accept coordinates as origin and destination", status=400)

def on_universal_journeys(api):
    return lambda request, version, format: universal_journeys(api, request, version, format)

def on_universal_proximity_list(request, version, format):
    region = find_region(request.args.get("uri", ""))
    if region:
        return NavitiaManager().dispatch(request, version, region, "proximity_list", format)
    else:
        return Response("Unable to deduce the region from the uri. Is it a valid coordinate?", status=404)
   
