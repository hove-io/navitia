from flask.ext.restful import marshal_with, abort, marshal
from instance_manager import InstanceManager
from ResourceUri import ResourceUri
from interfaces.v1.fields import address
from type_pb2 import _NAVITIATYPE
from collections import OrderedDict
#from exceptions import RegionNotFound

class Coord(ResourceUri):
    def get(self, region=None, lon=None, lat=None, id=None, *args, **kwargs):
        result = OrderedDict()
        if id:
            splitted = id.split(";")
            if len(splitted) != 2:
                abort(404, message="No region found for these coordinates")
            lon, lat = splitted

        if region is None:
            self.region = InstanceManager().get_region("", lon, lat)
        else:
            self.region = region
        result.update(regions=[self.region])
        if not lon is None and not lat is None:
            args = {
                    "uri" : "coord:"+str(lon)+":"+str(lat),
                    "count" : 1,
                    "distance" : 200,
                    "type[]" : ["address"],
                    "depth" : 1,
                    "start_page": 0
            }
            pb_result = InstanceManager().dispatch(args, self.region, "places_nearby")
            if len(pb_result.places_nearby) > 0:
                if _NAVITIATYPE.values_by_name["ADDRESS"].number == pb_result.places_nearby[0].embedded_type:
                        result.update(address = marshal(pb_result.places_nearby[0].address, address))
        return result, 200
