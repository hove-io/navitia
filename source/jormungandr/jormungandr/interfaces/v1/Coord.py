from flask.ext.restful import marshal_with, abort, marshal
from jormungandr import i_manager
from ResourceUri import ResourceUri
from jormungandr.interfaces.v1.fields import address
from navitiacommon.type_pb2 import _NAVITIATYPE
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
            self.region = i_manager.get_region("", lon, lat)
        else:
            self.region = region
        result.update(regions=[self.region])
        if not lon is None and not lat is None:
            args = {
                "uri": "coord:" + str(lon) + ":" + str(lat),
                "count": 1,
                "distance": 200,
                "type[]": ["address"],
                "depth": 1,
                "start_page": 0,
                "filter": ""
            }
            pb_result = i_manager.dispatch(args, "places_nearby",
                                           instance_name=self.region)
            if len(pb_result.places_nearby) > 0:
                e_type = pb_result.places_nearby[0].embedded_type
                if _NAVITIATYPE.values_by_name["ADDRESS"].number == e_type:
                    new_address = marshal(pb_result.places_nearby[0].address,
                                          address)
                    result.update(address=new_address)
            else:
                abort(404, message="No address for these coords")
        return result, 200
