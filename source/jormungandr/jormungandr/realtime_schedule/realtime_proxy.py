# coding=utf-8

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
from __future__ import absolute_import, print_function, unicode_literals, division
from abc import abstractmethod, ABCMeta
from copy import deepcopy
from jormungandr.schedule import RoutePoint
from jormungandr.utils import timestamp_to_datetime, record_external_failure
from jormungandr.utils import date_to_timestamp, pb_del_if
from jormungandr import new_relic
from navitiacommon import type_pb2
import datetime
import hashlib
import logging
import six
import math
from jormungandr import cache, app
from collections import namedtuple


Direction = namedtuple("Direction", "uri, label")


def floor_datetime(dt, step):
    """
    floor the second of a datetime to the nearest multiple of step
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 0), 15)
    datetime.datetime(2018, 1, 1, 10, 10)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 12), 15)
    datetime.datetime(2018, 1, 1, 10, 10)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 29), 15)
    datetime.datetime(2018, 1, 1, 10, 10, 15)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 30), 15)
    datetime.datetime(2018, 1, 1, 10, 10, 30)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 59), 15)
    datetime.datetime(2018, 1, 1, 10, 10, 45)

    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10), 30)
    datetime.datetime(2018, 1, 1, 10, 10)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 59), 30)
    datetime.datetime(2018, 1, 1, 10, 10, 30)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 59), 60)
    datetime.datetime(2018, 1, 1, 10, 10)

    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 28), 1)
    datetime.datetime(2018, 1, 1, 10, 10, 28)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 10, 10, 59), 1)
    datetime.datetime(2018, 1, 1, 10, 10, 59)

    >>> floor_datetime(datetime.datetime(2018, 1, 1, 0, 1, 0), 120)
    datetime.datetime(2018, 1, 1, 0, 0)
    >>> floor_datetime(datetime.datetime(2018, 1, 1, 0, 2, 0), 120)
    datetime.datetime(2018, 1, 1, 0, 2)

    >>> floor_datetime(datetime.datetime(2018, 1, 1, 1, 0, 0), 7*60)
    datetime.datetime(2018, 1, 1, 0, 56)
    """
    dt = dt.replace(microsecond=0)
    base = dt.replace(hour=0, minute=0, second=0)
    delta = dt - base
    return base + datetime.timedelta(seconds=int(math.floor(delta.total_seconds() / float(step)) * step))


class RealtimeProxyError(RuntimeError):
    pass


class RealtimeProxy(six.with_metaclass(ABCMeta, object)):
    """
    abstract class managing calls to external service providing real-time next passages
    """

    @abstractmethod
    def _get_next_passage_for_route_point(self, route_point, count, from_dt, current_dt, duration):
        """
        method that actually calls the external service to get the next passage for a given route_point
        """
        pass

    def _filter_passages(self, passages, count, from_dt, duration, timezone=None):
        """
        after getting the next passages from the proxy, we might want to filter some

        by default we filter:
        * we keep at most 'count' items
        * we don't want to display datetime after from_dt
        """
        if not passages:
            return passages

        if from_dt:
            # we need to convert from_dt (which is a timestamp) to a datetime
            from_dt = timestamp_to_datetime(from_dt, timezone)
            to_dt = from_dt + datetime.timedelta(seconds=duration)
            passages = [p for p in passages if from_dt <= p.datetime <= to_dt]
            if not passages:
                # if there was some passages and everything was filtered,
                # we return None to keep the base schedule
                return None

        if count:
            del passages[count:]

        return passages

    def next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=86400, timezone=None
    ):
        """
        Main method for the proxy

        returns the next realtime passages
        """
        try:
            next_passages = self._get_next_passage_for_route_point(
                route_point, count, from_dt, current_dt, duration
            )
            filtered_passage = self._filter_passages(next_passages, count, from_dt, duration, timezone)

            self.record_call('ok')

            return filtered_passage
        except RealtimeProxyError as e:
            self.record_call('failure', reason=str(e))
            return None

    # Method used to filter schedules from kraken before merging. By default remove all schedules.
    # Overload this to keep some and mix kraken and proxy datas.
    def _filter_base_stop_schedule(self, date_time):
        return True

    def _is_valid_direction(self, direction_uri, passage_direction_uri, group_by_dest):
        return True

    def _add_datetime(self, stop_schedule, passage, add_direction):
        new_dt = stop_schedule.date_times.add()
        # the midnight is calculated from passage.datetime and it keeps the same timezone as passage.datetime
        midnight = passage.datetime.replace(hour=0, minute=0, second=0, microsecond=0)
        time = (passage.datetime - midnight).total_seconds()
        new_dt.time = int(time)
        new_dt.date = date_to_timestamp(midnight)

        if passage.is_real_time:
            new_dt.realtime_level = type_pb2.REALTIME
        else:
            new_dt.realtime_level = type_pb2.BASE_SCHEDULE

        # we also add the direction in the note
        if add_direction and passage.direction:
            note = type_pb2.Note()
            note.note = passage.direction
            note_uri = hashlib.md5(note.note.encode('utf-8', 'backslashreplace')).hexdigest()
            note.uri = 'note:{md5}'.format(md5=note_uri)  # the id is a md5 of the direction to factorize them
            new_dt.properties.notes.extend([note])

    def _get_first_datetime(self, stop_schedule):
        if not stop_schedule.HasField('first_datetime'):
            return None
        first_datetime = stop_schedule.first_datetime
        if not first_datetime.HasField('date'):
            return None
        return first_datetime.date

    def _update_stop_schedule(self, request, stop_schedule, next_realtime_passages, group_by_dest=False):
        """
        Update the stopschedule response with the new realtime passages

        By default, all base schedule data is removed and replaced with realtime data.
        Each proxy can define its own way to merge passages.

        If next_realtime_passages is None (and not if it's []) it means that the proxy failed,
        so we use the base schedule

        If next_realtime_passages is empty and (now < first_datetime.date or first_datetime = None)
        we return None to use the base schedule
        """
        if next_realtime_passages is None:
            return

        if not next_realtime_passages:
            datetime_now = date_to_timestamp(request['_current_datetime'])
            first_datetime = self._get_first_datetime(stop_schedule)
            if first_datetime is None or datetime_now < first_datetime:
                return

        logging.getLogger(__name__).debug(
            'next passages: : {}'.format(["dt: {}".format(d.datetime) for d in next_realtime_passages])
        )

        # we clean up the old schedule
        pb_del_if(stop_schedule.date_times, self._filter_base_stop_schedule)
        direction_uri = stop_schedule.pt_display_informations.uris.stop_area
        for passage in next_realtime_passages:
            if not self._is_valid_direction(direction_uri, passage.direction_uri, group_by_dest):
                continue
            # If the route direction  doesn't match with departure.direction of forseti then
            # we should add direction name as note
            add_direction = direction_uri != passage.direction_uri
            self._add_datetime(stop_schedule, passage, add_direction)

        stop_schedule.date_times.sort(key=lambda dt: dt.date + dt.time)

    # By default, filter passage if they are on the same route point
    def _filter_base_passage(self, passage, route_point):
        return RoutePoint(passage.route, passage.stop_point) == route_point

    def _update_passages(self, passages, route_point, template, next_realtime_passages):
        if next_realtime_passages is None:
            return

        # filter passages with proxy method
        pb_del_if(passages, lambda passage: self._filter_base_passage(passage, route_point))

        # append the realtime passages
        for rt_passage in next_realtime_passages:
            # As in stop_schedule, we should manage group_by_destination to decide whether we keep
            # realtime passage comparing passage destination with route_point destination
            # https://navitia.atlassian.net/browse/NAV-2893
            direction_uri = route_point.fetch_direction_uri()
            if direction_uri:
                if not self._is_valid_direction(direction_uri, rt_passage.direction_uri, group_by_dest=False):
                    continue
            new_passage = deepcopy(template)
            new_passage.stop_date_time.arrival_date_time = date_to_timestamp(rt_passage.datetime)
            new_passage.stop_date_time.departure_date_time = date_to_timestamp(rt_passage.datetime)

            if rt_passage.is_real_time:
                new_passage.stop_date_time.data_freshness = type_pb2.REALTIME
            else:
                new_passage.stop_date_time.data_freshness = type_pb2.BASE_SCHEDULE

            # we also add the direction in the note
            if rt_passage.direction:
                new_passage.pt_display_informations.direction = rt_passage.direction

            # we add physical mode from route
            if len(route_point.pb_route.physical_modes) > 0:
                new_passage.pt_display_informations.physical_mode = route_point.pb_route.physical_modes[0].name

            passages.extend([new_passage])

    @abstractmethod
    def status(self):
        """
        return a status for the API
        """
        pass

    def record_external_failure(self, message):
        record_external_failure(message, 'realtime', six.text_type(self.rt_system_id))

    def record_internal_failure(self, message, comment=None):
        params = {'realtime_system_id': six.text_type(self.rt_system_id), 'message': message}
        if comment is not None:
            params['comment'] = comment
        new_relic.record_custom_event('realtime_internal_failure', params)

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'realtime_system_id': six.text_type(self.rt_system_id), 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('realtime_status', params)

    def record_additional_info(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'realtime_system_id': six.text_type(self.rt_system_id), 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('realtime_proxy_additional_info', params)

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_PTOBJECTS'), 600))
    def _get_direction(self, line_uri, object_code, default_value):
        if not self.destination_id_tag:
            return Direction(object_code, default_value)

        stop_point = self.instance.ptref.get_stop_point(line_uri, self.destination_id_tag, object_code)

        if (
            stop_point
            and stop_point.HasField('stop_area')
            and stop_point.stop_area.HasField('label')
            and stop_point.stop_area.label != ''
        ):
            return Direction(stop_point.stop_area.uri, stop_point.stop_area.label)
        return Direction(object_code, default_value)
