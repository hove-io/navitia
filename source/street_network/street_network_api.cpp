#include "street_network_api.h"
#include "type/data.h"
#include "types.h"

namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {

    void create_pb(const ng::Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork* sn){
        sn->set_length(path.length);
        for(auto item : path.path_items){
            if(item.way_idx < data.geo_ref.ways.size()){
                pbnavitia::PathItem * path_item = sn->add_path_item();
                path_item->set_name(data.geo_ref.ways[item.way_idx].name);
                path_item->set_length(item.length);
            }else{
                std::cout << "Way étrange : " << item.way_idx << std::endl;
            }

        }
        for(auto coord : path.coordinates){
            pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinate();
            pb_coord->set_x(coord.x);
            pb_coord->set_y(coord.y);
        }
    }

    pbnavitia::Response street_network(const navitia::type::GeographicalCoord &origin, 
            const navitia::type::GeographicalCoord& destination, const navitia::type::Data &data){

        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::STREET_NETWORK);

        std::vector<ng::vertex_t> start = {data.geo_ref.pl.find_nearest(origin)};
        std::vector<ng::vertex_t> dest = {data.geo_ref.pl.find_nearest(destination)};
        
        ng::Path path = data.geo_ref.compute(start, dest);

        create_pb(path, data, pb_response.mutable_street_network());

        return pb_response;
    }
}}
