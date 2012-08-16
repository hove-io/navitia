#include "street_network_api.h"
#include "type/data.h"
#include "types.h"


namespace navitia { namespace streetnetwork {



    void create_pb(const Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork& sn){
        sn.set_length(path.length);
        BOOST_FOREACH(auto item, path.path_items){
            if(item.way_idx < data.street_network.ways.size()){
                pbnavitia::PathItem * path_item = sn.add_path_item_list();
                path_item->set_name(data.street_network.ways[item.way_idx].name);
                path_item->set_length(item.length);
            }else{
                std::cout << "Way étrange : " << item.way_idx << std::endl;
            }

        }
        BOOST_FOREACH(auto coord, path.coordinates){
            pbnavitia::GeographicalCoord * pb_coord = sn.add_coordinate_list();
            pb_coord->set_x(coord.x);
            pb_coord->set_y(coord.y);
        }
    }

    pbnavitia::Response street_network(const navitia::type::GeographicalCoord &origin, 
            const navitia::type::GeographicalCoord& destination, const navitia::type::Data &data){

        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::STREET_NETWORK);

        std::vector<navitia::streetnetwork::vertex_t> start = {data.street_network.pl.find_nearest(origin)};
        std::vector<navitia::streetnetwork::vertex_t> dest = {data.street_network.pl.find_nearest(destination)};
        Path path = data.street_network.compute(start, dest);

        create_pb(path, data, *pb_response.mutable_street_network());

        return pb_response;
    }
}}
