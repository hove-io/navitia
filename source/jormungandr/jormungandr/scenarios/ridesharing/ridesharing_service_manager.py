# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from jormungandr.scenarios.ridesharing.ridesharing_service import AbstractRidesharingService
import logging, datetime
from typing import Dict
from jormungandr import utils
import six
from jormungandr import new_relic
from jormungandr import app
from jormungandr.scenarios import journey_filter
from jormungandr.scenarios.ridesharing.ridesharing_journey import Gender
from jormungandr.utils import get_pt_object_coord, generate_id
from jormungandr.street_network.utils import crowfly_distance_between
from navitiacommon import response_pb2
from jormungandr.utils import PeriodExtremity
from jormungandr.scenarios.journey_filter import to_be_deleted
from jormungandr.scenarios.helper_classes.helper_future import FutureManager
from importlib import import_module
from jormungandr.utils import can_connect_to_database
import pytz


class RidesharingServiceManager(object):
    class InstanceParams(object):
        greenlet_pool_for_ridesharing_services = True
        ridesharing_greenlet_pool_size = 1
        walking_speed = 1.11
        timezone = pytz.timezone("UTC")

        @classmethod
        def make_params(cls, instance):
            param = cls()
            param.greenlet_pool_for_ridesharing_services = instance.greenlet_pool_for_ridesharing_services
            param.ridesharing_greenlet_pool_size = instance.ridesharing_greenlet_pool_size
            param.walking_speed = instance.walking_speed
            param.timezone = pytz.timezone(instance.timezone)
            return param

    def __init__(
        self, instance, ridesharing_services_configuration, rs_services_getter=None, update_interval=60
    ):
        self.logger = logging.getLogger(__name__)
        self.ridesharing_services_configuration = ridesharing_services_configuration
        self._ridesharing_services_legacy = []
        self._rs_services_getter = rs_services_getter
        self._ridesharing_services = {}  # type: Dict[str, AbstractRidesharingService]
        self._ridesharing_services_last_update = {}  # type: Dict[str, datetime]
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = update_interval
        self.instance = instance

    def init_ridesharing_services(self):
        # Init legacy ridesharing from config file
        for config in self.ridesharing_services_configuration:
            # Set default arguments
            if 'args' not in config:
                config['args'] = {}
            if 'service_url' not in config['args']:
                config['args'].update({'service_url': None})
            try:
                service = utils.create_object(config)
            except KeyError as e:
                raise KeyError(
                    'impossible to build a ridesharing service for {}, '
                    'missing mandatory field in configuration: {}'.format(self.instance.name, e.message)
                )
            self.logger.info(
                '** Ridesharing: {} used for instance: {} **'.format(type(service).__name__, self.instance.name)
            )
            self._ridesharing_services_legacy.append(service)

    def _init_class(self, klass, arguments):
        """
        Create an instance of a provider according to config
        :param klass: name of the class configured in the database (Kros, Klaxit, Blablalines, ...)
        :param arguments: parameters from the database required to create the Ridesharing class
        :return: instance of Ridesharing service
        """
        try:
            if '.' not in klass:
                self.logger.warning('impossible to build, wrongly formated class: {}'.format(klass))

            module_path, name = klass.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
            return attr(**arguments)
        except ImportError:
            self.logger.warning('impossible to build, cannot find class: {}'.format(klass))

    def _update_ridesharing_service(self, service):
        self.logger.info('updating/adding {} ridesharing service'.format(service.id))
        try:
            self._ridesharing_services[service.id] = self._init_class(service.klass, service.args)
            self._ridesharing_services_last_update[service.id] = service.last_update()
        except Exception:
            self.logger.exception('impossible to initialize ridesharing service')

        # If the provider added in db is also defined in legacy, delete it.
        services_from_db = list(self._ridesharing_services.values())
        services_legacy = self._ridesharing_services_legacy
        if services_legacy and services_from_db:
            new_services_legacy = []
            for srv in services_legacy:
                if srv not in services_from_db:
                    new_services_legacy.append(srv)
            self._ridesharing_services_legacy = new_services_legacy

    def update_config(self):
        """
        Update list of ridesharing services from db
        """
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._rs_services_getter
        ):
            return

        # If database is not accessible we update the value of self._last_update and exit
        if not can_connect_to_database():
            self.logger.debug('Database is not accessible')
            self._last_update = datetime.datetime.utcnow()
            return

        self.logger.debug('Updating ridesharing services from db')
        self._last_update = datetime.datetime.utcnow()

        try:
            services = self._rs_services_getter()
        except Exception as e:
            self.logger.exception('Failure to retrieve ridesharing services configuration (error: {})'.format(e))
            # database is not accessible, so let's use the values already present in self._ridesharing_services_legacy
            return

        if not services:
            self.logger.debug('No ridesharing services available in db')
            self._ridesharing_services = {}
            self._ridesharing_services_last_update = {}

            return

        for service in services:
            if (
                service.id not in self._ridesharing_services_last_update
                or service.last_update() > self._ridesharing_services_last_update[service.id]
            ):
                self._update_ridesharing_service(service)

    def ridesharing_services_activated(self):
        # Make sure we update the ridesharing services list from the database before returning them
        self.update_config()
        return any((self._ridesharing_services, self._ridesharing_services_legacy))

    def get_all_ridesharing_services(self):
        # Make sure we update the ridesharing services list from the database before returning them
        self.update_config()
        return self._ridesharing_services_legacy + list(self._ridesharing_services.values())

    def decorate_journeys_with_ridesharing_offers(self, response, request, instance):
        greenlet_pool_actived = instance.greenlet_pool_for_ridesharing_services
        if greenlet_pool_actived:
            logging.info('ridesharing is called in async mode')
        else:
            logging.info('ridesharing is called in sequential mode')

        with FutureManager(instance.ridesharing_greenlet_pool_size) as future_manager:
            futures = {}
            for journey_idx, journey in enumerate(response.journeys):
                if 'ridesharing' not in journey.tags or to_be_deleted(journey):
                    continue
                futures[journey_idx] = {}
                for section_idx, section in enumerate(journey.sections):
                    if section.street_network.mode == response_pb2.Ridesharing:
                        section.additional_informations.append(response_pb2.HAS_DATETIME_ESTIMATED)
                        period_extremity = None
                        if len(journey.sections) == 1:  # direct path, we use the user input
                            period_extremity = PeriodExtremity(request['datetime'], request['clockwise'])
                        elif (
                            section_idx == 0
                        ):  # ridesharing on first section we want to arrive before the start of the pt
                            period_extremity = PeriodExtremity(section.end_date_time, False)
                        else:  # ridesharing at the end, we search for solution starting after the end of the pt sections
                            period_extremity = PeriodExtremity(section.begin_date_time, True)
                        instance_params = self.InstanceParams.make_params(instance)
                        if greenlet_pool_actived:
                            futures[journey_idx][section_idx] = future_manager.create_future(
                                self.build_ridesharing_journeys,
                                section.origin,
                                section.destination,
                                period_extremity,
                                instance_params,
                            )
                        else:
                            pb_rsjs, pb_tickets, pb_fps = self.build_ridesharing_journeys(
                                section.origin, section.destination, period_extremity, instance_params
                            )
                            self.add_new_ridesharing_results(
                                pb_rsjs, pb_tickets, pb_fps, response, journey_idx, section_idx
                            )

            if greenlet_pool_actived:
                for journey_idx in futures:
                    for section_idx in futures[journey_idx]:
                        pb_rsjs, pb_tickets, pb_fps = futures[journey_idx][section_idx].wait_and_get()
                        self.add_new_ridesharing_results(
                            pb_rsjs, pb_tickets, pb_fps, response, journey_idx, section_idx
                        )

    def add_new_ridesharing_results(self, pb_rsjs, pb_tickets, pb_fps, response, journey_idx, section_idx):
        if not pb_rsjs:
            journey_filter.mark_as_dead(response.journeys[journey_idx], 'no_matching_ridesharing_found')
        else:
            response.journeys[journey_idx].sections[section_idx].ridesharing_journeys.extend(pb_rsjs)
            response.tickets.extend(pb_tickets)

        response.feed_publishers.extend((fp for fp in pb_fps if fp not in response.feed_publishers))

    def get_ridesharing_journeys_with_feed_publishers(
        self, from_coord, to_coord, period_extremity, instance_params, limit=None
    ):
        calls = []
        res = []
        fps = set()

        for service in self.get_all_ridesharing_services():

            def _call(s=service):
                return s.request_journeys_with_feed_publisher(
                    from_coord, to_coord, period_extremity, instance_params, limit
                )

            calls.append(_call)

        call_res = []
        if instance_params.greenlet_pool_for_ridesharing_services:
            with FutureManager(instance_params.ridesharing_greenlet_pool_size) as future_manager:
                futures = [future_manager.create_future(call) for call in calls]
            call_res = (f.wait_and_get() for f in futures)
        else:
            call_res = (c() for c in calls)

        for rsjs, fp in call_res:
            res.extend(rsjs)
            fps.add(fp)

        return res, fps

    def build_ridesharing_journeys(self, from_pt_obj, to_pt_obj, period_extremity, instance_params):
        from_coord = get_pt_object_coord(from_pt_obj)
        to_coord = get_pt_object_coord(to_pt_obj)
        from_str = "{},{}".format(from_coord.lat, from_coord.lon)
        to_str = "{},{}".format(to_coord.lat, to_coord.lon)
        try:
            rsjs, fps = self.get_ridesharing_journeys_with_feed_publishers(
                from_str, to_str, period_extremity, instance_params
            )
        except Exception as e:
            self.logger.exception(
                'Error while retrieving ridesharing ads and feed_publishers from %s to %s: {}', from_str, to_str
            )
            new_relic.record_custom_event('ridesharing_internal_failure', {'message': str(e)})
            rsjs = []
            fps = []

        pb_rsjs = []
        pb_tickets = []
        pb_feed_publishers = [self._make_pb_fp(fp) for fp in fps if fp is not None]
        request_id = None

        for rsj in rsjs:
            pb_rsj = response_pb2.Journey()
            pb_rsj_pickup = self.instance.georef.place(
                "{};{}".format(rsj.pickup_place.lon, rsj.pickup_place.lat), request_id
            )
            pb_rsj_dropoff = self.instance.georef.place(
                "{};{}".format(rsj.dropoff_place.lon, rsj.dropoff_place.lat), request_id
            )
            pickup_coord = get_pt_object_coord(pb_rsj_pickup)
            dropoff_coord = get_pt_object_coord(pb_rsj_dropoff)

            pb_rsj.requested_date_time = period_extremity.datetime
            if rsj.departure_date_time:
                pb_rsj.departure_date_time = rsj.departure_date_time
            if rsj.arrival_date_time:
                pb_rsj.arrival_date_time = rsj.arrival_date_time
            pb_rsj.tags.append('ridesharing')

            # start teleport section
            start_teleport_section = pb_rsj.sections.add()
            start_teleport_section.id = "section_{}".format(six.text_type(generate_id()))
            start_teleport_section.street_network.mode = response_pb2.Walking
            start_teleport_section.origin.CopyFrom(from_pt_obj)
            start_teleport_section.destination.CopyFrom(pb_rsj_pickup)
            if rsj.origin_pickup_shape:
                # If stree_network shape in the offer contains only two coords, we use default CROW_FLY section"
                if len(rsj.origin_pickup_shape) == 2:
                    start_teleport_section.type = response_pb2.CROW_FLY
                    start_teleport_section.length = int(rsj.origin_pickup_distance)
                    start_teleport_section.shape.extend(rsj.origin_pickup_shape)
                else:
                    start_teleport_section.type = response_pb2.STREET_NETWORK
                    start_teleport_section.length = int(rsj.origin_pickup_distance)
                    start_teleport_section.street_network.length = start_teleport_section.length
                    start_teleport_section.street_network.duration = start_teleport_section.duration
                    start_teleport_section.street_network.mode = response_pb2.Walking
                    for sh in rsj.origin_pickup_shape:
                        coord = start_teleport_section.street_network.coordinates.add()
                        coord.lon = float(sh.lon)
                        coord.lat = float(sh.lat)
            else:
                start_teleport_section.type = response_pb2.CROW_FLY
                start_teleport_section.length = int(crowfly_distance_between(from_coord, pickup_coord))
                start_teleport_section.shape.extend([from_coord, pickup_coord])

            # We take departure to pickup duration
            if rsj.origin_pickup_duration:
                start_teleport_section.duration = int(rsj.origin_pickup_duration)
                pb_rsj.durations.walking += start_teleport_section.duration
                pb_rsj.durations.total += start_teleport_section.duration
                pb_rsj.duration = start_teleport_section.duration
            if rsj.departure_date_time:
                start_teleport_section.begin_date_time = rsj.departure_date_time
            if rsj.pickup_date_time:
                start_teleport_section.end_date_time = rsj.pickup_date_time
            # report value to journey
            pb_rsj.distances.walking += start_teleport_section.length

            # real ridesharing section
            rs_section = pb_rsj.sections.add()
            rs_section.id = "section_{}".format(six.text_type(generate_id()))
            rs_section.type = response_pb2.RIDESHARING
            rs_section.origin.CopyFrom(pb_rsj_pickup)
            rs_section.destination.CopyFrom(pb_rsj_dropoff)
            rs_section.additional_informations.append(response_pb2.HAS_DATETIME_ESTIMATED)

            rs_section.ridesharing_information.operator = rsj.metadata.system_id
            rs_section.ridesharing_information.network = rsj.metadata.network
            if rsj.available_seats is not None:
                rs_section.ridesharing_information.seats.available = rsj.available_seats
            if rsj.total_seats is not None:
                rs_section.ridesharing_information.seats.total = rsj.total_seats
            if rsj.driver.alias:
                rs_section.ridesharing_information.driver.alias = rsj.driver.alias
            if rsj.driver.image:
                rs_section.ridesharing_information.driver.image = rsj.driver.image
            if rsj.driver.gender is not None:
                if rsj.driver.gender == Gender.MALE:
                    rs_section.ridesharing_information.driver.gender = response_pb2.MALE
                elif rsj.driver.gender == Gender.FEMALE:
                    rs_section.ridesharing_information.driver.gender = response_pb2.FEMALE
            if rsj.driver.rate is not None:
                rs_section.ridesharing_information.driver.rating.value = rsj.driver.rate
            if rsj.driver.rate_count:
                rs_section.ridesharing_information.driver.rating.count = rsj.driver.rate_count
            if rsj.metadata.rating_scale_min is not None and rsj.metadata.rating_scale_max is not None:
                rs_section.ridesharing_information.driver.rating.scale_min = rsj.metadata.rating_scale_min
                rs_section.ridesharing_information.driver.rating.scale_max = rsj.metadata.rating_scale_max

            if rsj.ridesharing_ad:
                l = rs_section.ridesharing_information.links.add()
                l.key = "ridesharing_ad"
                l.href = rsj.ridesharing_ad

            # TODO CO2 = length * coeffCar / (totalSeats  + 1)
            rs_section.length = int(rsj.distance)

            if rsj.shape:
                rs_section.shape.extend(rsj.shape)

            if rsj.pickup_date_time:
                rs_section.begin_date_time = rsj.pickup_date_time
            if rsj.dropoff_date_time:
                rs_section.end_date_time = rsj.dropoff_date_time
            if rsj.duration:
                rs_section.duration = int(rsj.duration)
            # report values to journey
            pb_rsj.distances.ridesharing += rs_section.length
            pb_rsj.durations.total += rs_section.duration
            pb_rsj.durations.ridesharing += rs_section.duration
            pb_rsj.duration += rs_section.duration

            # end teleport section
            end_teleport_section = pb_rsj.sections.add()
            end_teleport_section.id = "section_{}".format(six.text_type(generate_id()))
            end_teleport_section.street_network.mode = response_pb2.Walking
            end_teleport_section.origin.CopyFrom(pb_rsj_dropoff)
            end_teleport_section.destination.CopyFrom(to_pt_obj)
            if rsj.dropoff_dest_shape:
                # If stree_network shape in the offer contains only two coords, we use default CROW_FLY section"
                if len(rsj.dropoff_dest_shape) == 2:
                    end_teleport_section.type = response_pb2.CROW_FLY
                    end_teleport_section.length = int(rsj.dropoff_dest_distance)
                    end_teleport_section.shape.extend(rsj.dropoff_dest_shape)
                else:
                    end_teleport_section.type = response_pb2.STREET_NETWORK
                    end_teleport_section.length = int(rsj.dropoff_dest_distance)
                    end_teleport_section.street_network.length = end_teleport_section.length
                    end_teleport_section.street_network.duration = end_teleport_section.duration
                    end_teleport_section.street_network.mode = response_pb2.Walking
                    for sh in rsj.dropoff_dest_shape:
                        coord = end_teleport_section.street_network.coordinates.add()
                        coord.lon = float(sh.lon)
                        coord.lat = float(sh.lat)
            else:
                end_teleport_section.type = response_pb2.CROW_FLY
                end_teleport_section.length = int(crowfly_distance_between(dropoff_coord, to_coord))
                end_teleport_section.shape.extend([dropoff_coord, to_coord])

            # We take dropoff to destination duration
            if rsj.dropoff_dest_duration:
                end_teleport_section.duration = int(rsj.dropoff_dest_duration)
                pb_rsj.durations.walking += end_teleport_section.duration
                pb_rsj.durations.total += end_teleport_section.duration
                pb_rsj.duration += end_teleport_section.duration

            if rsj.dropoff_date_time:
                end_teleport_section.begin_date_time = rsj.dropoff_date_time
            if rsj.arrival_date_time:
                end_teleport_section.end_date_time = rsj.arrival_date_time
            # report value to journey
            pb_rsj.distances.walking += end_teleport_section.length

            # create ticket associated
            ticket = response_pb2.Ticket()
            ticket.id = "ticket_{}".format(six.text_type(generate_id()))
            ticket.name = "ridesharing_price_{}".format(ticket.id)
            ticket.found = True
            ticket.comment = "Ridesharing price for section {}".format(rs_section.id)
            ticket.section_id.extend([rs_section.id])
            # also add fare to journey
            ticket.cost.value = rsj.price
            pb_rsj.fare.total.value = ticket.cost.value
            ticket.cost.currency = rsj.currency
            pb_rsj.fare.total.currency = rsj.currency
            pb_rsj.fare.found = True
            pb_rsj.fare.ticket_id.extend([ticket.id])

            pb_tickets.append(ticket)
            pb_rsjs.append(pb_rsj)

        return pb_rsjs, pb_tickets, pb_feed_publishers

    def _make_pb_fp(self, fp):
        pb_fp = response_pb2.FeedPublisher()
        pb_fp.id = fp.id
        pb_fp.name = fp.name
        pb_fp.url = fp.url
        pb_fp.license = fp.license
        return pb_fp
