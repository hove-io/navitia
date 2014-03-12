#include "calendar.h"
#include "ptreferential/ptreferential.h"
#include "type/data.h"

namespace navitia { namespace calendar {

std::vector<type::idx_t> Calendar::get_calendars(const std::string& filter,
                const std::vector<std::string>& forbidden_uris,
                const type::Data &d,
                const boost::gregorian::date_period filter_period,
                const boost::posix_time::ptime){

    std::vector<type::idx_t> to_return;
    to_return  = ptref::make_query(type::Type_e::Calendar, filter, forbidden_uris, d);
    if(!to_return.empty() && (!filter_period.begin().is_not_a_date())){
        std::vector<type::idx_t> indexes;
        for(type::idx_t idx : to_return){
            navitia::type::Calendar* cal = d.pt_data.calendars[idx];
            for(const boost::gregorian::date_period per : cal->active_periods){
                if(filter_period.begin() == filter_period.end()){
                    if (per.contains(filter_period.begin())){
                        indexes.push_back(cal->idx);
                        break;
                    }
                }else{
                    if (filter_period.intersects(per)){
                        indexes.push_back(cal->idx);
                        break;
                    }
                }
            }
        }
        std::sort(indexes.begin(), indexes.end());
        std::unique(indexes.begin(), indexes.end());
        std::vector<type::idx_t> tmp_indexes;
        std::back_insert_iterator< std::vector<type::idx_t> > it(tmp_indexes);
        std::set_intersection(to_return.begin(), to_return.end(), indexes.begin(), indexes.end(), it);
        to_return = tmp_indexes;
    }
        return to_return;
}
}
}
