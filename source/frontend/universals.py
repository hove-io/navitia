from apis import Apis


def universal_journeys(api, request, version, format):
    origin = request.args.get("origin", "")
    origin_re = re.match('^coord:(.+):(.+)$', origin)
    region = None
    if origin_re:
        try:
            lon = float(origin_re.group(1))
            lat = float(origin_re.group(2))
            region = NavitiaManager().key_of_coord(lon, lat)
        except:
            return Response("Unable to parse coordinates", status=400)
        if region:
            return Apis().on_api(request, version, region, api, format)
        else:
            return Response("No region found at given coordinates", status=404)
    else:
        return Response("Journeys without specifying a region only accept coordinates as origin or destination", status=400)

def on_universal_journeys(api):
    return lambda request, version, format: universal_journeys(api, request, version, format)

def on_universal_proximity_list(request, version, format):
    try:
        region = NavitiaManager().key_of_coord(float(request.args.get("lon")), float(request.args.get("lat")))
        if region:
            return Apis().on_api(request, version, region, "proximity_list", format)
        else:
            return Response("No region found at given coordinates", status=404)
    except:
        return Response("Invalid coordinates", status=400)
   
