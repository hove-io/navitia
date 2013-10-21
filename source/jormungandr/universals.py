import re
from apis import Apis
from error import generate_error
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


def universal_journeys(api, request, format):
    region = find_region(request.args.get("origin", ""))
    if region:
        return NavitiaManager().dispatch(request, region, api, format)
    else:
        return generate_error("Unable to deduce the region from the uri. Is it a valid coordinate?", status=404)

def on_universal_journeys(api):
    return lambda request, format: universal_journeys(api, request, format)

def on_universal_places_nearby(request, format):
    region = find_region(request.args.get("uri", ""))
    if region:
        return NavitiaManager().dispatch(request, region, "places_nearby", format)
    else:
        return generate_error("Unable to deduce the region from the uri. Is it a valid coordinate?", status=404)

