
#include "routing/labels.h"

namespace navitia {
namespace routing {

Labels::Labels() {}
Labels::Labels(const std::vector<type::StopPoint*> stop_points)
    : dt_pts(stop_points),
      dt_transfers(stop_points),
      walking_duration_pts(stop_points),
      walking_duration_transfers(stop_points) {}

Labels::Labels(Map dt_pts, Map dt_transfers, Map walkings, Map walking_transfers)
    : dt_pts(std::move(dt_pts)),
      dt_transfers(std::move(dt_transfers)),
      walking_duration_pts(std::move(walkings)),
      walking_duration_transfers(std::move(walking_transfers)) {}

void swap(Labels& lhs, Labels& rhs) {
    swap(lhs.dt_pts, rhs.dt_pts);
    swap(lhs.dt_transfers, rhs.dt_transfers);
}

void Labels::clear(const Labels& clean) {
    dt_pts = clean.dt_pts;
    dt_transfers = clean.dt_transfers;

    walking_duration_pts = clean.walking_duration_pts;
    walking_duration_transfers = clean.walking_duration_transfers;
}

void Labels::fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert) {
    boost::fill(dt_pts.values(), pts);
    boost::fill(dt_transfers.values(), transfert);
    boost::fill(walking_duration_pts.values(), walking);
    boost::fill(walking_duration_transfers.values(), walking_transfert);
}

void Labels::init(const std::vector<type::StopPoint*>& stops, DateTime val) {
    dt_pts.assign(stops, val);
    dt_transfers.assign(stops, val);

    walking_duration_pts.assign(stops, DateTimeUtils::not_valid);
    walking_duration_transfers.assign(stops, DateTimeUtils::not_valid);
}
}  // namespace routing
}  // namespace navitia
