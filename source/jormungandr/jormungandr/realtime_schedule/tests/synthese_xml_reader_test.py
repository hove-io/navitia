# coding=utf-8
# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

from datetime import datetime
from nose.tools.nontrivial import raises
from jormungandr.realtime_schedule.synthese_xml_reader import SyntheseXmlReader
from jormungandr.interfaces.parsers import date_time_format
from aniso8601 import parse_time


def get_xml_parser():
    return '<timeTable xsi:noNamespaceSchemaLocation="http://synthese.rcsmobility.com/include/54_departures_table/DisplayScreenContentFunction.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" type="departure">' \
           '<journey routeId="2533412229452279" dateTime="2016-Mar-21 12:07:37" blink="0" realTime="yes" waiting_time="00:02:59">' \
           '<stop id="3377704015495922" operatorCode="MURQB,MNLP_**_21MUA,MNLP_**_22MUR,MNLP_**_31MUA,MNLP_**_33MUR,MNLP_**_A1MUA,MNLP_**_MUSER" name="Musée d\'Art Roger Quilliot"/>' \
           '<line id="11821953316814877" creatorId="20" name="Ligne 20" shortName="20" longName="" color="(94,184,62)" xmlColor="#5eb83e" style="vertclair" image="ligne-20.jpg" direction="AULNAT St Exup. via Aéro." wayback="1">' \
           '<network id="6192453782601729" name="T2C" image=""/>' \
           '</line>' \
           '<origin id="1970329131942197" name="GERZAT Champfleuri" cityName="Gerzat"/>' \
           '<destination id="1970329131942194" name="AULNAT St Exupéry" cityName="Aulnat"/>' \
           '<stopArea id="1970329131942296" name="Musée d\'Art Roger Quilliot" cityId="1688849860563049" cityName="Clermont-Ferrand" directionAlias=""/>' \
           '</journey>' \
           '<journey routeId="2533412229399934" dateTime="2016-Mar-21 12:15:00" blink="0" realTime="yes" waiting_time="00:10:22">' \
           '<stop id="3377704015495922" operatorCode="MURQB,MNLP_**_21MUA,MNLP_**_22MUR,MNLP_**_31MUA,MNLP_**_33MUR,MNLP_**_A1MUA,MNLP_**_MUSER" name="Musée d\'Art Roger Quilliot"/>' \
           '<line id="11821953316814878" creatorId="21" name="Ligne 21" shortName="21" longName="" color="(217,232,106)" xmlColor="#d9e86a" style="jaune" image="ligne-21.jpg" direction="Quartier Chambon" wayback="0">' \
           '<network id="6192453782601729" name="T2C" image=""/>' \
           '</line>' \
           '<origin id="1970329131942531" name="Stade G. Montpied" cityName="Clermont-Ferrand"/>' \
           '<destination id="1970329131941974" name="Quartier Chambon" cityName="Aubière"/>' \
           '<stopArea id="1970329131942296" name="Musée d\'Art Roger Quilliot" cityId="1688849860563049" cityName="Clermont-Ferrand" directionAlias=""/>' \
           '</journey>' \
           '</timeTable>'


def xml_valid_test():
    builder = SyntheseXmlReader()
    result = builder.get_synthese_passages(get_xml_parser())

    route_point = builder.find_route_point('2533412229452279', '3377704015495922', result)
    assert route_point.syn_route_id == '2533412229452279'
    assert route_point.syn_stop_point_id == '3377704015495922'

    assert len(result[route_point]) == 1
    assert result[route_point][0].real_time == True
    assert result[route_point][0].date_time == date_time_format("2016-Mar-21 12:07:37")
    assert result[route_point][0].waiting_time == parse_time("00:02:59")

    route_point = builder.find_route_point('2533412229399934', '3377704015495922', result)
    assert route_point.syn_route_id == '2533412229399934'
    assert route_point.syn_stop_point_id == '3377704015495922'

    assert len(result[route_point]) == 1
    assert result[route_point][0].real_time == True
    assert result[route_point][0].date_time == date_time_format("2016-Mar-21 12:15:00")
    assert result[route_point][0].waiting_time == parse_time("00:10:22")

@raises(ValueError)
def xml_date_time_invalid_test():
    builder = SyntheseXmlReader()
    xml = get_xml_parser().replace("2016-Mar-21 12:07:37", "2016-Mar-41 12:07:37", 1)
    builder.get_synthese_passages(xml)

@raises(ValueError)
def xml_time_invalid_test():
    builder = SyntheseXmlReader()
    xml = get_xml_parser().replace("00:02:59", "00:81:59", 1)
    builder.get_synthese_passages(xml)
