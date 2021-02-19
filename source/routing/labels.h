
#pragma once

#include "type/fwd_type.h"
#include "type/stop_point.h"
#include "utils/idx_map.h"
#include "routing/raptor_utils.h"

#include <vector>

namespace navitia {
namespace routing {

struct Labels {
    using Map = IdxMap<type::StopPoint, DateTime>;

    Labels();
    Labels(const std::vector<type::StopPoint*> stop_points);
    Labels(Map dt_pts, Map dt_transfers, Map walkings, Map walking_transfers);

    // initialize the structure according to the number of jpp
    void init_inf(const std::vector<type::StopPoint*>& stops) { init(stops, DateTimeUtils::inf); }
    // initialize the structure according to the number of jpp
    void init_min(const std::vector<type::StopPoint*>& stops) { init(stops, DateTimeUtils::min); }

    // clear the structure according to a given structure. Same as a
    void clear(const Labels& clean);

    // copy without touching the boarding_jpp fields
    const DateTime& dt_transfer(SpIdx sp_idx) const { return dt_transfers[sp_idx]; }
    const DateTime& dt_pt(SpIdx sp_idx) const { return dt_pts[sp_idx]; }
    DateTime& mut_dt_transfer(SpIdx sp_idx) { return dt_transfers[sp_idx]; }
    DateTime& mut_dt_pt(SpIdx sp_idx) { return dt_pts[sp_idx]; }

    const DateTime& walking_duration_pt(SpIdx sp_idx) const { return walking_duration_pts[sp_idx]; }
    const DateTime& walking_duration_transfer(SpIdx sp_idx) const { return walking_duration_transfers[sp_idx]; }
    DateTime& mut_walking_duration_pt(SpIdx sp_idx) { return walking_duration_pts[sp_idx]; }
    DateTime& mut_walking_duration_transfer(SpIdx sp_idx) { return walking_duration_transfers[sp_idx]; }

    bool pt_is_initialized(SpIdx sp_idx) const { return is_dt_initialized(dt_pt(sp_idx)); }
    bool transfer_is_initialized(SpIdx sp_idx) const { return is_dt_initialized(dt_transfer(sp_idx)); }

    Map& get_dt_pts() { return dt_pts; }
    Map& get_dt_transfers() { return dt_transfers; }
    Map& get_walking_duration_pts() { return walking_duration_pts; }
    Map& get_walking_duration_transfers() { return walking_duration_transfers; }

    void fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert);

private:
    void init(const std::vector<type::StopPoint*>& stops, DateTime val);

    // All these vectors are indexed by sp_idx
    //
    // dt_pts[stop_point] stores the earliest arrival time to stop_point.
    // More precisely, at time dt_pts[stop_point], we just alighted from
    // a vehicle going through stop_point, but we need to do a transfer
    // before being able to board a new vehicle.
    Map dt_pts;

    // dt_transfers[stop_point] stores the earliest time at which we are able
    // to board a vehicle leaving from stop_point (i.e. a transfer to stop_point has been done).
    Map dt_transfers;

    // walking_duration_pts[stop_point] stores the total walking duration (fallback + transfers)  of a
    // journey that alight to stop_point at DateTime dt_pts[stop_point]
    Map walking_duration_pts;

    // waling_duration_transfers[stop_point] stores the total walking duration (fallback + transfers) of a
    // journey that allows to board a vehicle at stop_point at DateTime transfers_pts[stop_point]
    Map walking_duration_transfers;
};

}  // namespace routing
}  // namespace navitia
