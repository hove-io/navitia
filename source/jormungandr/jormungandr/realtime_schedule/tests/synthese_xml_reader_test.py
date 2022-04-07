# coding=utf-8
# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

from __future__ import absolute_import, print_function, division
import xml.etree.ElementTree as et
import pytest
from jormungandr.realtime_schedule.synthese import Synthese, SyntheseRoutePoint
from navitiacommon.parser_args_type import DateTimeFormat
import pytz


def get_xml_parser():
    return (
        '<timeTable xsi:noNamespaceSchemaLocation="http://synthese.rcsmobility.com/include/54_departures_table/DisplayScreenContentFunction.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" type="departure">'
        '<journey routeId="2533412229452279" dateTime="2016-Mar-21 12:07:37" blink="0" realTime="yes" waiting_time="00:02:59">'
        '<stop id="3377704015495922" operatorCode="MURQB,MNLP_**_21MUA,MNLP_**_22MUR,MNLP_**_31MUA,MNLP_**_33MUR,MNLP_**_A1MUA,MNLP_**_MUSER" name="Musée d\'Art Roger Quilliot"/>'
        '<line id="11821953316814877" creatorId="20" name="Ligne 20" shortName="20" longName="" color="(94,184,62)" xmlColor="#5eb83e" style="vertclair" image="ligne-20.jpg" direction="AULNAT St Exup. via Aéro." wayback="1">'
        '<network id="6192453782601729" name="T2C" image=""/>'
        '</line>'
        '<origin id="1970329131942197" name="GERZAT Champfleuri" cityName="Gerzat"/>'
        '<destination id="1970329131942194" name="AULNAT St Exupéry" cityName="Aulnat"/>'
        '<stopArea id="1970329131942296" name="Musée d\'Art Roger Quilliot" cityId="1688849860563049" cityName="Clermont-Ferrand" directionAlias=""/>'
        '</journey>'
        '<journey routeId="2533412229399934" dateTime="2016-Mar-21 12:15:00" blink="0" realTime="yes" waiting_time="00:10:22">'
        '<stop id="3377704015495922" operatorCode="MURQB,MNLP_**_21MUA,MNLP_**_22MUR,MNLP_**_31MUA,MNLP_**_33MUR,MNLP_**_A1MUA,MNLP_**_MUSER" name="Musée d\'Art Roger Quilliot"/>'
        '<line id="11821953316814878" creatorId="21" name="Ligne 21" shortName="21" longName="" color="(217,232,106)" xmlColor="#d9e86a" style="jaune" image="ligne-21.jpg" direction="Quartier Chambon" wayback="0">'
        '<network id="6192453782601729" name="T2C" image=""/>'
        '</line>'
        '<origin id="1970329131942531" name="Stade G. Montpied" cityName="Clermont-Ferrand"/>'
        '<destination id="1970329131941974" name="Quartier Chambon" cityName="Aubière"/>'
        '<stopArea id="1970329131942296" name="Musée d\'Art Roger Quilliot" cityId="1688849860563049" cityName="Clermont-Ferrand" directionAlias=""/>'
        '</journey>'
        '<journey routeId="2533412229399934" dateTime="2016-Mar-22 12:15:00" blink="0" realTime="yes" waiting_time="00:10:22">'
        '<stop id="3377704015495922" operatorCode="MURQB,MNLP_**_21MUA,MNLP_**_22MUR,MNLP_**_31MUA,MNLP_**_33MUR,MNLP_**_A1MUA,MNLP_**_MUSER" name="Musée d\'Art Roger Quilliot"/>'
        '<line id="11821953316814878" creatorId="21" name="Ligne 21" shortName="21" longName="" color="(217,232,106)" xmlColor="#d9e86a" style="jaune" image="ligne-21.jpg" direction="Quartier Chambon" wayback="0">'
        '<network id="6192453782601729" name="T2C" image=""/>'
        '</line>'
        '<origin id="1970329131942531" name="Stade G. Montpied" cityName="Clermont-Ferrand"/>'
        '<destination id="1970329131941974" name="Quartier Chambon" cityName="Aubière"/>'
        '<stopArea id="1970329131942296" name="Musée d\'Art Roger Quilliot" cityId="1688849860563049" cityName="Clermont-Ferrand" directionAlias=""/>'
        '</journey>'
        '</timeTable>'
    )


def make_dt(str):
    tz = pytz.timezone("Europe/Paris")
    dt = DateTimeFormat()(str)
    return tz.normalize(tz.localize(dt)).astimezone(pytz.utc)


def xml_valid_test():
    builder = Synthese("id_synthese", "http://fake.url/", "Europe/Paris")
    result = builder._get_synthese_passages(get_xml_parser())
    route_point = SyntheseRoutePoint('2533412229452279', '3377704015495922')

    assert route_point in result
    assert len(result[route_point]) == 1
    assert result[route_point][0].is_real_time == True
    assert result[route_point][0].datetime == make_dt("2016-Mar-21 12:07:37")

    route_point = SyntheseRoutePoint('2533412229399934', '3377704015495922')
    assert route_point in result
    assert len(result[route_point]) == 2
    assert result[route_point][0].is_real_time == True
    assert result[route_point][0].datetime == make_dt("2016-Mar-21 12:15:00")

    assert result[route_point][1].is_real_time == True
    assert result[route_point][1].datetime == make_dt("2016-Mar-22 12:15:00")


def xml_date_time_invalid_test():
    builder = Synthese("id_synthese", "http://fake.url/", "Europe/Paris")
    xml = get_xml_parser().replace(str("2016-Mar-21 12:07:37"), str("2016-Mar-41 12:07:37"), 1)

    with pytest.raises(ValueError):
        builder._get_synthese_passages(xml)


def xml_invalid_test():
    builder = Synthese("id_synthese", "http://fake.url/", "Europe/Paris")
    xml = get_xml_parser().replace(str("</journey>"), str(""), 1)
    with pytest.raises(et.ParseError):
        builder._get_synthese_passages(xml)
