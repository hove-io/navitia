# Copyright (c) 2001-2023, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from jormungandr.pt_journey_fare.pt_journey_fare import AbstractPtJourneyFare
from navitiacommon import response_pb2, request_pb2, type_pb2


class Kraken(AbstractPtJourneyFare):
    def __init__(self, instance):
        self.instance = instance

    @staticmethod
    def _pt_sections(journey):
        return [s for s in journey.sections if s.type == response_pb2.PUBLIC_TRANSPORT]

    def create_fare_reqeust(self, pt_journeys):
        request = request_pb2.Request()
        request.requested_api = type_pb2.pt_fares
        pt_fare_request = request.pt_fares
        for journey in pt_journeys:
            pt_sections = self._pt_sections(journey)
            if not pt_sections:
                continue
            pt_journey_request = pt_fare_request.pt_journeys.add(id=journey.internal_id)
            for s in pt_sections:
                pt_journey_request.pt_sections.add(
                    id=s.id,
                    network_uri=s.uris.network,
                    start_stop_area_uri=s.origin.stop_point.stop_area.uri,
                    end_stop_area_uri=s.destination.stop_point.stop_area.uri,
                    line_uri=s.uris.line,
                    begin_date_time=s.begin_date_time,
                    end_date_time=s.end_date_time,
                    first_stop_point_uri=s.origin.stop_point.uri,
                    last_stop_point_uri=s.destination.stop_point.uri,
                    physical_mode=s.uris.physical_mode,
                )
        return request

    def get_pt_journeys_fare(self, pt_journeys, request_id):
        fare_request = self.create_fare_reqeust(pt_journeys)
        return self.instance.send_and_receive(fare_request, request_id="{}_fare".format(request_id))
