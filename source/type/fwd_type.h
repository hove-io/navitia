/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#pragma once

// forward declare
//
template <class DATATYPE, class ELEMTYPE, int NUMDIMS, class ELEMTYPEREAL, int TMAXNODES, int TMINNODES>
class RTree;
namespace navitia {
template <typename T>
struct Rank;

namespace georef {
struct GeoRef;
struct POI;
struct POIType;
struct Admin;
struct Address;
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
class PT_Data;
struct AssociatedCalendar;
struct AccessibiliteParams;
struct Calendar;
struct CommercialMode;
struct Company;
struct Contributor;
struct Dataset;
struct DiscreteVehicleJourney;
struct EntryPoint;
struct ExceptionDate;
struct FrequencyVehicleJourney;
struct GeographicalCoord;
struct Line;
struct LineGroup;
struct MetaData;
struct MetaVehicleJourney;
struct Network;
struct PhysicalMode;
struct Route;
struct StopArea;
struct StopPoint;
struct StopPointConnection;
struct StopTime;
struct ValidityPattern;
struct VehicleJourney;
struct AccessPoint;
using RankStopTime = Rank<StopTime>;
template <class T>
struct MultiPolygonMap;
namespace disruption {
struct Disruption;
struct Impact;
struct Message;
struct ApplicationPattern;
enum class ActiveStatus;
}  // namespace disruption
}  // namespace type
}  // namespace navitia
