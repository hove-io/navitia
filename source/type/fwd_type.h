/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#pragma once

// forward declare
//
template <class DATATYPE, class ELEMTYPE, int NUMDIMS, class ELEMTYPEREAL, int TMAXNODES, int TMINNODES>
class RTree;
namespace navitia {
namespace georef {
struct GeoRef;
struct POI;
struct POIType;
struct Admin;
using AdminRtree = class RTree<Admin*, double, 2, double, 8, 4>;
}  // namespace georef
namespace fare {
struct Fare;
}
namespace routing {
struct dataRAPTOR;
struct JourneyPattern;
struct JourneyPatternPoint;
}  // namespace routing
namespace type {
class MetaData;

struct GeographicalCoord;
struct Line;
struct StopArea;
struct Network;
struct StopPointConnection;
struct ValidityPattern;
struct Route;
struct VehicleJourney;
struct StopTime;
struct Dataset;
struct StopPoint;
struct ExceptionDate;
struct Contributor;
struct Company;
struct CommercialMode;
struct PhysicalMode;
struct LineGroup;
struct MetaVehicleJourney;
struct DiscreteVehicleJourney;
struct FrequencyVehicleJourney;
struct Calendar;
namespace disruption {
struct Impact;
struct Message;
}  // namespace disruption
}  // namespace type
}  // namespace navitia
