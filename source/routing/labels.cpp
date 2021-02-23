
#include "routing/labels.h"
#include <boost/range/combine.hpp>

namespace navitia {
namespace routing {

Labels::Labels() {}
Labels::Labels(const std::vector<type::StopPoint*> stop_points) : labels(stop_points) {}

Labels::Labels(Map dt_pts, Map dt_transfers, Map walkings, Map walking_transfers) {
    const auto& zip = boost::combine(dt_pts, dt_transfers.values(), walkings.values(), walking_transfers.values());
    labels.resize(zip.size());
    for (const auto& z : zip) {
        const auto sp_idx = z.get<0>().first;
        labels[sp_idx] = Label{z.get<0>().second, z.get<1>(), z.get<2>(), z.get<3>()};
    }
}

std::array<Labels::Map, 4> Labels::inrow_labels() {
    std::array<Map, 4> row_labels;

    for (auto& row_label : row_labels)
        row_label.resize(labels.size());

    for (const auto& l : labels) {
        const auto& sp_idx = l.first;
        const auto& label = l.second;

        row_labels[0][sp_idx] = label.dt_pts;
        row_labels[1][sp_idx] = label.dt_transfers;
        row_labels[2][sp_idx] = label.walking_duration_pts;
        row_labels[3][sp_idx] = label.walking_duration_transfers;
    }

    return row_labels;
}

void Labels::fill_values(DateTime pts, DateTime transfert, DateTime walking, DateTime walking_transfert) {
    Label default_label{pts, transfert, walking, walking_transfert};
    boost::fill(labels.values(), default_label);
}

void Labels::init(const std::vector<type::StopPoint*>& stops, DateTime val) {
    Label init_label{val, val, DateTimeUtils::not_valid, DateTimeUtils::not_valid};
    labels.assign(stops, init_label);
}

}  // namespace routing
}  // namespace navitia
