# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

from jormungandr.interfaces.v1.serializer.base import PbNestedSerializer
from jormungandr.interfaces.v1.serializer import pt
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.fields import LinkSchema


class PassageSerializer(PbNestedSerializer):
    route = pt.RouteSerializer()
    stop_point = pt.StopPointSerializer()
    stop_date_time = pt.StopDateTimeSerializer()
    display_informations = pt.DisplayInformationSerializer(attr='pt_display_informations')
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)

    def get_links(self, obj):
        display_info = obj.pt_display_informations
        uris = display_info.uris
        l = [("line", uris.line),
             ("company", uris.company),
             ("vehicle_journey", uris.vehicle_journey),
             ("route", uris.route),
             ("commercial_mode", uris.commercial_mode),
             ("physical_mode", uris.physical_mode),
             ("network", uris.network),
             ("note", uris.note)]
        return [{"type": v[0], "id": v[1]} for v in l if v[1] != ""] + \
            [create_internal_link(id=value, rel="notes", _type="notes") for value in display_info.notes]
