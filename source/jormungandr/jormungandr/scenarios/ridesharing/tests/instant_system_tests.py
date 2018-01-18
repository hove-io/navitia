# -*- coding: utf-8 -*-
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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


from __future__ import absolute_import, print_function, unicode_literals, division

from jormungandr.scenarios.ridesharing.instant_system import InstantSystem
import mock
from jormungandr.tests import utils_test
from jormungandr import utils
import json


fake_response ="""
{
  "total": 2,
  "journeys": [
    {
      "id": "4bcd0b9d-2c9d-42a2-8ffb-4508c952f4fb",
      "departureDate": "2017-12-25T08:07:59+01:00",
      "arrivalDate": "2017-12-25T08:25:36+01:00",
      "duration": 1057,
      "distance": 12650,
      "url": "https://jky8k.app.goo.gl/?efr=1&apn=com.is.android.rennes&ibi=&isi=&utm_campaign=KISIO&link=https%3A%2F%2Fwww.star.fr%2Fsearch%2F%3FfeatureName%3DsearchResultDetail%26networkId%3D33%26journeyId%3D4bcd0b9d-2c9d-42a2-8ffb-4508c952f4fb",
      "paths": [
        {
          "mode": "RIDESHARINGAD",
          "from": {
            "name": "",
            "lat": 48.1102,
            "lon": -1.68623
          },
          "to": {
            "name": "",
            "lat": 48.02479,
            "lon": -1.74673
          },
          "departureDate": "2017-12-25T08:07:59+01:00",
          "arrivalDate": "2017-12-25T08:25:36+01:00",
          "shape": "wosdH|ihIRVDTFDzBjPNhADJ\\`C?TJt@Hj@h@tDp@bFR?bAFRBZDR@JCL@~AJl@Df@DfBNv@B~@DjAFh@HXH~@VbEfANDh@PdAl@\\RdAZnBHpADvBDf@@d@Gv@S\\OlAOl@EbAHjAVNDd@Dd@Mt@u@FGrE{EtBaBr@zCp@dDd@~BRtAHj@X`BFXlAjDLd@v@dDXlAh@TVl@hBtIB`ANpAh@nBf@xATf@Xd@JFPD@JHRLBLKDBbCbBbBbBjApA?VHPPBL`@\\^|BrBDHJ`@AP?PDRFL\\TRAJGRD`Al@jBhA~BbBx@VfALl@PHVDHPFNCVNdCnBpHzDdB|AfAjAj@h@^d@jAhBhAvA?^BNFJPHPCFGVNpBhApBt@ZL|B^dCJfDAZFLRHBNEJQZIdUa@b@JJ`@TXTFTAPKNUH]nBGtOb@vDd@`C`ArAp@zAjAnBnBJJh@h@`_@l`@fIvIfMhNl@t@dAzBnAnDx@xDh@jFfBbRdAnMdBnSjB|JbDbIhMj[rN`_@nEfJzCxDrCtDl@pBDtE^Bn@?h@?t@IdAe@XUFIvBaBvBaBf@Wl@OdAEfAJJXJHJBLCbAbAx@j@fBn@p@X`HfDdAd@NB\\CBLJDFCBI?OGILYn@gDb@uAVe@\\_@jEgDlFgARElBa@|G}AxFwA`AWv@YNI~AaArAg@bEw@pA[t@Y`B{@~BmAtAo@fAk@TYBBH?DGBKTEd@U^QlBcA^QvEcCP@Le@Cm@Eo@Ia@AI",
          "rideSharingAd": {
            "id": "24bab9de-653c-4cc4-a947-389c59cf0423",
            "type": "DRIVER",
            "from": {
              "name": "9 Allée Rochester, Rennes",
              "lat": 48.127905,
              "lon": -1.652393
            },
            "to": {
              "name": "2 Avenue Alphonse Legault, Bruz",
              "lat": 48.024714,
              "lon": -1.746711
            },
            "user": {
              "alias": "Jean P.",
              "gender": "MALE",
              "imageUrl": "https://dummyimage.com/128x128/C8E6C9/000.png&text=JP",
              "rating": {
                "rate": 0,
                "count": 0
              }
            },
            "price": {
              "amount": 170,
              "currency": "EUR"
            },
            "vehicle": {
              "availableSeats": 4
            }
          }
        }
      ]
    },
    {
      "id": "05223c04-834d-4710-905f-aa3796da5837",
      "departureDate": "2017-12-25T08:35:42+01:00",
      "arrivalDate": "2017-12-25T08:53:09+01:00",
      "duration": 1047,
      "distance": 11686,
      "url": "https://jky8k.app.goo.gl/?efr=1&apn=com.is.android.rennes&ibi=&isi=&utm_campaign=KISIO&link=https%3A%2F%2Fwww.star.fr%2Fsearch%2F%3FfeatureName%3DsearchResultDetail%26networkId%3D33%26journeyId%3D05223c04-834d-4710-905f-aa3796da5837",
      "paths": [
        {
          "mode": "RIDESHARINGAD",
          "from": {
            "name": "",
            "lat": 48.1102,
            "lon": -1.68623
          },
          "to": {
            "name": "",
            "lat": 48.03193,
            "lon": -1.74635
          },
          "departureDate": "2017-12-25T08:35:42+01:00",
          "arrivalDate": "2017-12-25T08:53:09+01:00",
          "shape": "wosdH|ihIRVDTFDzBjPNhADJ\\`C?TJt@Hj@h@tDp@bFR?bAFRBZDR@JCL@~AJl@Df@DfBNv@B~@DjAFh@HXH~@VbEfANDh@PdAl@\\RdAZnBHpADvBDf@@d@Gv@S\\OlAOl@EbAHjAVNDd@Dd@Mt@u@FGrE{EtBaB`CeB|AeAr@cA`@_BNgCf@RTJ`@PjBr@fBz@~@d@~Av@`Af@d@TzBfAZPVLRJf@TbAj@zIpE|J`F\\PNHpBbAp@Z~Ax@PHdA\\ANFPH@DCDMxDpBt@fABTLXJFL@JEJI\\A`Bb@fABpAY|Aq@tCsAjBy@d@StHuDxAm@t@Qf@GX@VZTDTId@Fr@\\z@\\`Dz@|PrD`FbA|HjBNBZDdB`@`QfDnGfAxJhBvBd@vF`B~CzAhGvDnD|BxBtAbBnAd@f@b@h@BRP^VNZALId@?bBbAnMhIfH~D~@`@t@NjA@`@?VFVXJZAZBXLf@HNLJLFT@RERMz@~@~@r@b@d@Rd@`@zAGj@BZL`@HJBl@BzJFh@FbAL~@Pv@dAjDl@bBlBhFvBhFdCtCrGdE`IhFt@d@HF@?DBLJNL`EnClEpCpCfBp@^bBv@hAb@hBf@xBb@rIpAVNPL@NFNFHJDBXbA|GJt@bAzGt@dFl@vDZpCLhC@pBCxBYpDC`@GAODKRALD\\JLD@G^GVw@lHSjCK|B@vCPpMCnAKf@OHEJEVF\\FHNDHLH\\BZH|KA`E@N\\x@bBtBpGpGZV",
          "rideSharingAd": {
            "id": "4eb9a4ba-fb10-4cf0-bf4c-31ada02a2c91",
            "type": "DRIVER",
            "from": {
              "name": "1 Boulevard Volney, Rennes",
              "lat": 48.1247,
              "lon": -1.666796
            },
            "to": {
              "name": "9012 Rue du 8 Mai 1944, Bruz",
              "lat": 48.031951,
              "lon": -1.74641
            },
            "user": {
              "alias": "Alice M.",
              "gender": "FEMALE",
              "imageUrl": "https://dummyimage.com/128x128/B2EBF2/000.png&text=AM",
              "rating": {
                "rate": 0,
                "count": 0
              }
            },
            "price": {
              "amount": 0,
              "currency": "EUR"
            },
            "vehicle": {
              "availableSeats": 4
            }
          }
        }
      ]
    }
  ],
  "url": "https://jky8k.app.goo.gl/?efr=1&apn=com.is.android.rennes&ibi=&isi=&utm_campaign=KISIO&link=https%3A%2F%2Fwww.star.fr%2Fsearch%2F%3FfeatureName%3DsearchResults%26networkId%3D33%26from%3D48.109377%252C-1.682103%26to%3D48.020335%252C-1.743929%26multimodal%3Dfalse%26departureDate%3D2017-12-25T08%253A00%253A00%252B01%253A00"
}

"""

# https://stackoverflow.com/a/9312242/1614576
import re
regex = re.compile(r'\\(?![/u"])')
fixed = regex.sub(r"\\\\", fake_response)

mock_get = mock.MagicMock(return_value=utils_test.MockResponse(json.loads(fixed), 200, '{}'))


def get_ridesharing_service_test():
    from jormungandr.scenarios.ridesharing.ridesharing_service import Ridesharing
    configs = [
        {
            "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
            "args": {
            "service_url": "toto",
            "api_key": "toto key",
            "network": "N",
            }
        },
        {
            "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
            "args": {
            "service_url": "tata",
            "api_key": "tata key",
            "network": "M",
            }
        }
    ]

    # A hack class
    class DummyInstance:
        name = ''

    services = Ridesharing.get_ridesharing_services(DummyInstance(), configs)
    assert len(services) == 2

    assert services[0].service_url == 'toto'
    assert services[0].api_key == 'toto key'
    assert services[0].network == 'N'

    assert services[1].service_url == 'tata'
    assert services[1].api_key == 'tata key'
    assert services[1].network == 'M'


def instant_system_test():
    with mock.patch('requests.get', mock_get):

        instant_system = InstantSystem('dummyInstance', 'dummyUrl', 'dummyApiKey', 'dummyNetwork')
        from_coord = '48.109377,-1.682103'
        to_coord = '48.020335,-1.743929'

        period_extremity = utils.PeriodExtremity(datetime=1514181600,
                                                represents_start=True)
        ridesharing_journeys = instant_system.request_journeys(from_coord=from_coord, to_coord=to_coord,
                                                               period_extremity=period_extremity)

        assert len(ridesharing_journeys) == 2
        assert ridesharing_journeys[0].pickup_place.addr == "9 Allée Rochester, Rennes"
        assert ridesharing_journeys[0].dropoff_place.addr == "2 Avenue Alphonse Legault, Bruz"
        assert ridesharing_journeys[0].pickup_date_time == utils.str_to_time_stamp("20171225T070759")
        assert ridesharing_journeys[0].dropoff_date_time == utils.str_to_time_stamp("20171225T072536")
        assert ridesharing_journeys[0].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[0].metadata.system_id == 'Instant System'

        assert ridesharing_journeys[1].pickup_place.addr == "1 Boulevard Volney, Rennes"
        assert ridesharing_journeys[1].dropoff_place.addr == "9012 Rue du 8 Mai 1944, Bruz"
        assert ridesharing_journeys[1].pickup_date_time == utils.str_to_time_stamp("20171225T073542")
        assert ridesharing_journeys[1].dropoff_date_time == utils.str_to_time_stamp("20171225T075309")
        assert ridesharing_journeys[1].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[1].metadata.system_id == 'Instant System'
