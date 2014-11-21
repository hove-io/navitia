# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
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

from navitiacommon import response_pb2

def has_walking_first(journey):
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            return True
        elif section.type == response_pb2.CROW_FLY \
                and section.street_network.mode != response_pb2.Walking:
            return False
        elif section.type == response_pb2.STREET_NETWORK \
                and section.street_network.mode != response_pb2.Walking:
            return False
    return True

def has_bike_first(journey):
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            return True
        elif section.type == response_pb2.CROW_FLY \
                and section.street_network.mode != response_pb2.Bike:
            return False
        elif section.type == response_pb2.STREET_NETWORK \
                and section.street_network.mode != response_pb2.Bike:
            return False
    return True

def has_bss_first(journey):
    has_bss = False
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            return False
        elif section.type == response_pb2.BSS_RENT:
            return True
    return False

def has_walking_last(journey):
    has_pt = False
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_pt = True
        elif has_pt \
                and section.type == response_pb2.CROW_FLY \
                and section.street_network.mode != response_pb2.Walking:
            return False
        elif has_pt \
                and section.type == response_pb2.STREET_NETWORK \
                and section.street_network.mode != response_pb2.Walking:
            return False
    return has_pt#we will not be here if there is another fallback mode used after the pt section

def has_bike_last(journey):
    has_pt = False
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_pt = True
        elif has_pt \
                and section.type == response_pb2.CROW_FLY \
                and section.street_network.mode != response_pb2.Bike:
            return False
        elif has_pt \
                and section.type == response_pb2.STREET_NETWORK \
                and section.street_network.mode != response_pb2.Bike:
            return False
    return has_pt#we will not be here if there is another fallback mode used after the pt section

def has_bss_last(journey):
    has_pt = False
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_pt = True
        elif has_pt and section.type == response_pb2.BSS_RENT:
            return True
    return False

def has_bss_first_and_walking_last(journey):
    return has_bss_first(journey) and has_walking_last(journey)

def has_walking_first_and_bss_last(journey):
    return has_walking_first(journey) and has_bss_last(journey)

def has_bss_first_and_bss_last(journey):
    return has_bss_first(journey) and has_bss_last(journey)

def has_bike_first_and_walking_last(journey):
    return has_bike_first(journey) and has_walking_last(journey)

def has_bike_first_and_bss_last(journey):
    return has_bike_first(journey) and has_bss_last(journey)

def bike_duration(journey):
    duration = 0
    in_bss = False
    for section in journey.sections:
        if section.type == response_pb2.BSS_RENT:
            in_bss = True
        if section.type == response_pb2.BSS_PUT_BACK:
            in_bss = False
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Bike \
                and not in_bss:
            duration = duration + section.duration

    return duration

def car_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Car:
            duration = duration + section.duration

    return duration

def pt_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            duration = duration + section.duration

    return duration
