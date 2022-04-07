# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

import sys
import json
import logging
import argparse
from kombu.connection import BrokerConnection
from kombu.entity import Exchange
import kombu
import uuid
import time
import datetime, calendar
import gtfs_realtime_pb2
import chaos_pb2


def to_timestamp(date):
    """
    convert a datatime objet to a posix timestamp (number of seconds from 1070/1/1)
    """
    return int(calendar.timegm(date.utctimetuple()))


class PtObject(object):
    def __init__(self, uri, type):
        self.uri = uri
        self.type = type


class Line(PtObject):
    def __init__(self, uri):
        self.uri = uri
        self.type = 'line'


class StopPoint(PtObject):
    def __init__(self, uri):
        self.uri = uri
        self.type = 'stop_point'


class StopArea(PtObject):
    def __init__(self, uri):
        self.uri = uri
        self.type = 'stop_area'


class Route(PtObject):
    def __init__(self, uri):
        self.uri = uri
        self.type = 'route'


class Network(PtObject):
    def __init__(self, uri):
        self.uri = uri
        self.type = 'network'


class LineSection(Line):
    def __init__(self, uri, start, end):
        self.uri = uri
        self.start = start
        self.end = end
        self.type = 'line_section'


class RailSection(Line):
    def __init__(self, uri, start, end, routes=[], blocked_stop_areas=[]):
        self.uri = uri
        self.start = start
        self.end = end
        self.routes = routes
        self.blocked_stop_areas = blocked_stop_areas
        self.type = 'rail_section'


class Disruption(object):
    def __init__(self, impacted_obj, start, end, logger, impact_type="NO_SERVICE"):
        self.id = str(uuid.uuid4())
        self.impacted_obj = impacted_obj
        self.start = start
        self.end = end
        self.is_deleted = False
        self.impact_type = impact_type

        self.cause = "disruption cause test"
        self.message = "disruption test sample"
        self.logger = logger
        if impacted_obj == None:
            self.logger.info(
                "new empty disruption id: {} - start: {} - end: {} - without impact".format(
                    self.id, self.start, self.end
                )
            )
        else:
            self.logger.info(
                "new disruption type: {} - uri: {} - id: {} - start: {} - end: {} - impact type: {}".format(
                    self.impacted_obj.type,
                    self.impacted_obj.uri,
                    self.id,
                    self.start,
                    self.end,
                    self.impact_type,
                )
            )

    def to_pb(self):
        feed_message = gtfs_realtime_pb2.FeedMessage()
        feed_message.header.gtfs_realtime_version = '1.0'
        feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
        feed_message.header.timestamp = 0

        feed_entity = feed_message.entity.add()
        feed_entity.id = self.id
        feed_entity.is_deleted = self.is_deleted

        disruption = feed_entity.Extensions[chaos_pb2.disruption]

        disruption.id = self.id
        disruption.cause.id = self.cause
        disruption.cause.wording = self.cause
        disruption.reference = "DisruptionTest"
        disruption.publication_period.start = to_timestamp(self.start)
        disruption.publication_period.end = to_timestamp(self.end)

        if not self.impacted_obj:
            return feed_message.SerializeToString()

        # Impacts
        impact = disruption.impacts.add()
        impact.id = "impact_" + self.id + "_1"
        enums_impact = gtfs_realtime_pb2.Alert.DESCRIPTOR.enum_values_by_name
        impact.created_at = to_timestamp(datetime.datetime.utcnow())
        impact.updated_at = impact.created_at
        impact.severity.effect = enums_impact[self.impact_type].number
        impact.severity.id = "impact id for " + self.impact_type
        impact.severity.wording = "impact wording for " + self.impact_type
        if self.impact_type == "NO_SERVICE":
            impact.severity.priority = 10
            impact.severity.color = "#FF0000"
        else:
            impact.severity.priority = 1
            impact.severity.color = "#FFFF00"

        # Application periods
        application_period = impact.application_periods.add()
        application_period.start = to_timestamp(self.start)
        application_period.end = to_timestamp(self.end)

        # PT object
        type_col = {
            "network": chaos_pb2.PtObject.network,
            "stop_area": chaos_pb2.PtObject.stop_area,
            "line": chaos_pb2.PtObject.line,
            "line_section": chaos_pb2.PtObject.line_section,
            "rail_section": chaos_pb2.PtObject.rail_section,
            "route": chaos_pb2.PtObject.route,
            "stop_point": chaos_pb2.PtObject.stop_point,
        }

        ptobject = impact.informed_entities.add()
        ptobject.uri = self.impacted_obj.uri
        ptobject.pt_object_type = type_col.get(self.impacted_obj.type, chaos_pb2.PtObject.unkown_type)
        if ptobject.pt_object_type == chaos_pb2.PtObject.line_section:
            line_section = ptobject.pt_line_section
            line_section.line.uri = self.impacted_obj.uri
            line_section.line.pt_object_type = chaos_pb2.PtObject.line
            pb_start = line_section.start_point
            pb_start.uri = self.impacted_obj.start
            pb_start.pt_object_type = chaos_pb2.PtObject.stop_area
            pb_end = line_section.end_point
            pb_end.uri = self.impacted_obj.end
            pb_end.pt_object_type = chaos_pb2.PtObject.stop_area
        if ptobject.pt_object_type == chaos_pb2.PtObject.rail_section:
            rail_section = ptobject.pt_rail_section
            rail_section.line.uri = self.impacted_obj.uri
            rail_section.line.pt_object_type = chaos_pb2.PtObject.line
            pb_start = rail_section.start_point
            pb_start.uri = self.impacted_obj.start
            pb_start.pt_object_type = chaos_pb2.PtObject.stop_area
            pb_end = rail_section.end_point
            pb_end.uri = self.impacted_obj.end
            pb_end.pt_object_type = chaos_pb2.PtObject.stop_area
            for route_uri in self.impacted_obj.routes:
                ptobject_rs = impact.informed_entities.add()
                ptobject_rs.uri = route_uri
                ptobject_rs.pt_object_type = chaos_pb2.PtObject.route
                rail_section.routes.append(ptobject_rs)
            for bsa in self.impacted_obj.blocked_stop_areas:
                oPtObj = rail_section.blocked_stop_areas.add()
                oPtObj.uri = bsa[0]
                oPtObj.order = bsa[1]

        # Message with one channel and two channel types: web
        message = impact.messages.add()
        message.text = self.message
        message.channel.name = "web"
        message.channel.id = "web"
        message.channel.max_size = 250
        message.channel.content_type = "html"
        message.channel.types.append(chaos_pb2.Channel.web)

        return feed_message.SerializeToString()

    def publish(self, producer):
        producer.publish(self.to_pb())

    def delete(self, producer):
        self.is_deleted = True
        producer.publish(self.to_pb())


def parse_args(logger):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-b",
        "--broker_connection",
        dest="broker_connection",
        help="AMQ broker connection string (mandatory). " "Example: -b pyamqp://guest:guest@localhost:5672",
    )
    parser.add_argument(
        "-e", "--exchange", dest="exchange", help="AMQ exhange (mandatory). " "Example: -e navitia"
    )
    parser.add_argument(
        "-t", "--topic", dest="topic", help="AMQ topic (mandatory). " "Example: -t shortterm.coverage"
    )
    parser.add_argument(
        "-p",
        "--pt_object",
        dest="pt_object",
        help='The public transport object that you want to impact '
        '(mandatory except with --empty_disruption parameter). Allows one shot disruption. '
        'Example : -p "Line(\'line:DUA:800853022\')". '
        "List of possibility: "
        'Line("line_uri"), '
        'LineSection("line_uri", "stop_area_uri", "stop_area_uri"), '
        'RailSection("line_uri", "stop_area_uri", "stop_area_uri"), '
        'StopArea("stop_area_uri"), '
        'StopPoints("stop_point_uri"), '
        'Route("route_uri"), '
        'Network(network_uri)',
    )
    parser.add_argument(
        "-i",
        "--impact_type",
        dest="impact_type",
        help="Impact type for disruption : "
        "NO_SERVICE, "
        "REDUCED_SERVICE, "
        "SIGNIFICANT_DELAYS, "
        "DETOUR, "
        "ADDITIONAL_SERVICE, "
        "MODIFIED_SERVICE, "
        "OTHER_EFFECT, "
        "UNKNOWN_EFFECT. "
        "By default = NO_SERVICE",
        default="NO_SERVICE",
    )
    parser.add_argument(
        "-f",
        "--file",
        dest="file",
        help='File to import an impacts list from json. It is a simple python list of PT object. '
        'Here an example of what the file can contain: '
        '{"impacts": [{"pt_object": "Line("line:DUA:800853022"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": LineSection("line:DUA:800803071", "stop_area:DUA:SA:8778543", "stop_area:DUA:SA:8732832"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": RailSection("line:DUA:800803071", "stop_area:DUA:SA:8778543", "stop_area:DUA:SA:8732832"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": StopArea("stop_area:DUA:SA:93:1316"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": StopPoint("stop_point:DUA:SP:93:1317"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": Route("route:DUA:8008555486001"),"impact_type":"NO_SERVICE"}, '
        '{"pt_object": Network(network:DUA855),"impact_type":"NO_SERVICE"}]}',
    )
    parser.add_argument(
        "--empty_disruption",
        action='store_true',
        help="create only empty disruption without impact. Allows one shot disruption",
    )
    parser.add_argument(
        "-s",
        "--sleep",
        help="sleep time between 2 disruptions in file mode (secondes). default=10s",
        default=10,
        type=int,
    )
    parser.add_argument(
        "-d",
        "--disruption_duration",
        help="Duration of disruption (minutes). default=10min",
        default=10,
        type=int,
    )
    parser.add_argument("-l", "--loop_forever", action='store_true', help="loop forever in file mode")
    args = parser.parse_args()

    if not args.broker_connection:
        logger.error("AMQ broker connection is mandatory")
        logger.error("Please fill -b parameter")
        sys.exit("AMQ broker connection is mandatory")
    if not args.exchange:
        logger.error("AMQ exchange is mandatory")
        logger.error("Please fill -e parameter")
        sys.exit("AMQ exchange is mandatory")
    if not args.topic:
        logger.error("AMQ topic is mandatory")
        logger.error("Please fill -t parameter")
        sys.exit("AMQ topic is mandatory")
    if not args.pt_object and not args.empty_disruption and not args.file:
        logger.error("pt_object is mandatory except if the --empty_disruption or --file option is activated")
        logger.error("Please fill --pt_object paramter or active --empty_disruption or fill --file option")
        sys.exit("pt_object is mandatory except if the --empty_disruption or --file option is activated")

    return args


def publish(logger, args, disruption):
    logger.info("send disruption on {} - {} - {} ".format(args.broker_connection, args.exchange, args.topic))
    p = kombu.Connection(args.broker_connection).Producer()
    p.publish(disruption.to_pb(), exchange=args.exchange, routing_key=args.topic)


def config_logger():
    logger = logging.getLogger('disruptor')
    logger.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger


def main():
    logger = config_logger()

    args = parse_args(logger)

    logger.info("launch disruptor")
    logger.info("create disruption sample for test")
    if args.file:
        logger.info("load {} file".format(args.file))
        try:
            with open(args.file, 'r') as f:
                pt_objects_dict = json.load(f)
                logger.info("file {}".format(pt_objects_dict))
        except ValueError:
            logger.error("error to decode json - {}".format(sys.exc_info()[0]))
            raise
        except IOError:
            logger.error("error to open file - {}".format(sys.exc_info()[0]))
            raise
        try:
            while True:
                for impact in pt_objects_dict['impacts']:
                    logger.info("prepare disruption with {}".format(impact['pt_object']))
                    try:
                        disruption = Disruption(
                            eval(impact['pt_object']),
                            datetime.datetime.utcnow(),
                            datetime.datetime.utcnow() + datetime.timedelta(minutes=args.disruption_duration),
                            logger,
                            impact['impact_type'],
                        )
                    except NameError:
                        logger.error("pt_object is badly formatted - {}".format(sys.exc_info()[0]))
                        raise

                    publish(logger, args, disruption)
                    logger.info("wait {} secs before next disruption".format(args.sleep))
                    time.sleep(args.sleep)
                if not args.loop_forever:
                    break
        except:
            logger.error("error during disruption creation - {}".format(sys.exc_info()[0]))
            raise
    else:
        try:
            if args.empty_disruption:
                disruption = Disruption(
                    None,
                    datetime.datetime.utcnow(),
                    datetime.datetime.utcnow() + datetime.timedelta(minutes=args.disruption_duration),
                    logger,
                    args.impact_type,
                )
            else:
                disruption = Disruption(
                    eval(args.pt_object),
                    datetime.datetime.utcnow(),
                    datetime.datetime.utcnow() + datetime.timedelta(minutes=args.disruption_duration),
                    logger,
                    args.impact_type,
                )
        except NameError:
            logger.error("pt_object is badly formatted - {}".format(sys.exc_info()[0]))
            raise
        publish(logger, args, disruption)
    logger.info("close disruptor")


if __name__ == "__main__":
    main()
