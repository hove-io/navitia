#include "georef.h"

#include "utils/logger.h"
#include "utils/functions.h"
#include "utils/csv.h"
#include "utils/configuration.h"

#include <unordered_map>
#include <boost/foreach.hpp>
#include <array>
#include <boost/math/constants/constants.hpp>


using navitia::type::idx_t;

namespace navitia{ namespace georef{

/** Ajout d'une adresse dans la liste des adresses d'une rue
  * les adresses avec un numéro pair sont dans la liste "house_number_right"
  * les adresses avec un numéro impair sont dans la liste "house_number_left"
  * Après l'ajout, la liste est trié dans l'ordre croissant des numéros
*/

void Way::add_house_number(const HouseNumber& house_number){
    if (house_number.number % 2 == 0){
            this->house_number_right.push_back(house_number);
            std::sort(this->house_number_right.begin(),this->house_number_right.end());
    } else{
        this->house_number_left.push_back(house_number);
        std::sort(this->house_number_left.begin(),this->house_number_left.end());
    }
}

/** Recherche des coordonnées les plus proches à un un numéro
    * les coordonnées par extrapolation
*/
nt::GeographicalCoord Way::extrapol_geographical_coord(int number){
    HouseNumber hn_upper, hn_lower;
    nt::GeographicalCoord to_return;

    if (number % 2 == 0){ // pair
        for(auto it=this->house_number_right.begin(); it != this->house_number_right.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }else{
        for(auto it=this->house_number_left.begin(); it != this->house_number_left.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }

    // Extrapolation des coordonnées:
    int diff_house_number = hn_upper.number - hn_lower.number;
    int diff_number = number - hn_lower.number;

    double x_step = (hn_upper.coord.lon() - hn_lower.coord.lon()) /diff_house_number;
    to_return.set_lon(hn_lower.coord.lon() + x_step*diff_number);

    double y_step = (hn_upper.coord.lat() - hn_lower.coord.lat()) /diff_house_number;
    to_return.set_lat(hn_lower.coord.lat() + y_step*diff_number);

    return to_return;
}

/**
    * Si le numéro est plus grand que les numéros, on renvoie les coordonées du plus grand de la rue
    * Si le numéro est plus petit que les numéros, on renvoie les coordonées du plus petit de la rue
    * Si le numéro existe, on renvoie ses coordonnées
    * Sinon, les coordonnées par extrapolation
*/

nt::GeographicalCoord Way::get_geographical_coord(const std::vector< HouseNumber>& house_number_list, const int number){
    if (!house_number_list.empty()){

        /// Dans le cas où le numéro recherché est plus grand que tous les numéros de liste
        if (house_number_list.back().number <= number){
            return house_number_list.back().coord;
        }

        /// Dans le cas où le numéro recherché est plus petit que tous les numéros de liste
        if (house_number_list.front().number >= number){
            return house_number_list.front().coord;
        }

        /// Dans le cas où le numéro recherché est dans la liste = à un numéro dans la liste
        for(auto it=house_number_list.begin(); it != house_number_list.end(); ++it){
            if ((*it).number  == number){
                return (*it).coord;
             }
        }

        /// Dans le cas où le numéro recherché est dans la liste et <> à tous les numéros
        return extrapol_geographical_coord(number);
    }
    nt::GeographicalCoord to_return;
    return to_return;
}

/** Recherche des coordonnées les plus proches à un numéro
    * Si la rue n'a pas de numéro, on renvoie son barycentre
*/
nt::GeographicalCoord Way::nearest_coord(const int number, const Graph& graph){
    /// Attention la liste :
    /// "house_number_right" doit contenir les numéros pairs
    /// "house_number_left" doit contenir les numéros impairs
    /// et les deux listes doivent être trier par numéro croissant

    if (( this->house_number_right.empty() && this->house_number_left.empty() )
            || (this->house_number_right.empty() && number % 2 == 0)
            || (this->house_number_left.empty() && number % 2 != 0)
            || number <= 0)
        return barycentre(graph);

    if (number % 2 == 0) // Pair
        return get_geographical_coord(this->house_number_right, number);
    else // Impair
        return get_geographical_coord(this->house_number_left, number);
}

// Calcul du barycentre de la rue
nt::GeographicalCoord Way::barycentre(const Graph& graph){
    std::vector<nt::GeographicalCoord> line;
    nt::GeographicalCoord centroid;

    std::pair<vertex_t, vertex_t> previous(type::invalid_idx, type::invalid_idx);
    for(auto edge : this->edges){
        if(edge.first != previous.second || edge.second != previous.first ){
            line.push_back(graph[edge.first].coord);
            line.push_back(graph[edge.second].coord);
        }
        previous = edge;
    }
    try{
        boost::geometry::centroid(line, centroid);
    }catch(...){
      LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log") ,"Impossible de trouver le barycentre de la rue :  " + this->name);
    }

    return centroid;
}

/** Recherche du némuro le plus proche à des coordonnées
    * On récupère le numéro se trouvant à une distance la plus petite par rapport aux coordonnées passées en paramètre
*/
int Way::nearest_number(const nt::GeographicalCoord& coord){

    int to_return = -1;
    double distance, distance_temp;
    distance = std::numeric_limits<double>::max();
    for(auto house_number : this->house_number_left){
        distance_temp = coord.distance_to(house_number.coord);
        if (distance  > distance_temp){
            to_return = house_number.number;
            distance = distance_temp;
        }
    }
    for(auto house_number : this->house_number_right){
        distance_temp = coord.distance_to(house_number.coord);
        if (distance  > distance_temp){
            to_return = house_number.number;
            distance = distance_temp;
        }
    }
    return to_return;
}

/**
  * Compute the angle between the last segment of the path and the next point
  *
  * A-------B
  *  \)
  *   \
  *    \
  *     C
  *
  *                       l(AB)² + l(AC)² - l(BC)²
  *the angle ABC = cos-1(_________________________)
  *                          2 * l(AB) * l(AC)
  *
  * with l(AB) = length OF AB
  *
  * The computed angle is 180 - the angle ABC, ie we compute the turn angle of the path
 */
int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord) {
    if (path.path_items.empty()) {
        return 0;
    }
    nt::GeographicalCoord b_coord, a_coord;
    const PathItem& last_item = path.path_items.back();
    a_coord = last_item.coordinates.back();
    if (last_item.coordinates.size() > 1) {
        b_coord = last_item.coordinates[last_item.coordinates.size() - 2];
    } else {
        if ( path.path_items.size() < 2 ) {
            return 0; //we don't have 2 previous coordinate, we can't compute an angle
        }
        const PathItem& previous_item = *(++(path.path_items.rbegin()));
        b_coord = previous_item.coordinates.back();
    }
    if (a_coord == b_coord || b_coord == c_coord) {
        return 0;
    }

    double len_ab = a_coord.distance_to(b_coord);
    double len_bc = b_coord.distance_to(c_coord);
    double len_ac = a_coord.distance_to(c_coord);

    double numerator = pow(len_ab, 2) + pow(len_ac, 2) - pow(len_bc, 2);
    double ab_lon = b_coord.lon() - a_coord.lon();
    double bc_lat = c_coord.lat() - b_coord.lat();
    double ab_lat = b_coord.lat() - a_coord.lat();
    double bc_lon = c_coord.lon() - b_coord.lon();

    double denominator =  2 * len_ab * len_ac;
    double raw_angle = acos(numerator / denominator);

    double det = ab_lon * bc_lat - ab_lat * bc_lon;

    //conversion into angle
    raw_angle *= 360 / (2 * boost::math::constants::pi<double>());

    int rounded_angle = static_cast<int>(raw_angle);

    rounded_angle = 180 - rounded_angle;
    if ( det < 0 )
        rounded_angle *= -1.0;

//    std::cout << "angle : " << rounded_angle << std::endl;

    return rounded_angle;
}


Path GeoRef::build_path(vertex_t best_destination, std::vector<vertex_t> preds, bool add_one_elt) const {
    Path p;
    std::vector<vertex_t> reverse_path;
    while (best_destination != preds[best_destination]){
        reverse_path.push_back(best_destination);
        best_destination = preds[best_destination];
    }
    reverse_path.push_back(best_destination);

    // On reparcourt tout dans le bon ordre
    nt::idx_t last_way = type::invalid_idx;
    PathItem path_item;
    path_item.coordinates.push_back(graph[reverse_path.back()].coord);
    p.length = 0;
    for (size_t i = reverse_path.size(); i > 1; --i) {
        bool path_item_changed = false;
        vertex_t v = reverse_path[i-2];
        vertex_t u = reverse_path[i-1];

        edge_t e = boost::edge(u, v, graph).first;
        Edge edge = graph[e];
        if (edge.way_idx != last_way && last_way != type::invalid_idx) {
            p.path_items.push_back(path_item);
            path_item = PathItem();
            path_item_changed = true;
        }

        nt::GeographicalCoord coord = graph[v].coord;
        path_item.coordinates.push_back(coord);
        last_way = edge.way_idx;
        path_item.way_idx = edge.way_idx;
//        path_item.segments.push_back(e);
        path_item.length += edge.length;
        p.length += edge.length;
        if (path_item_changed) {
            //we update the last path item
            p.path_items.back().angle = compute_directions(p, coord);
        }
    }
    //in some case we want to add even if we have only one vertex (which means there is no valid edge)
    size_t min_nb_elt_to_add = add_one_elt ? 1 : 2;
    if (reverse_path.size() >= min_nb_elt_to_add)
        p.path_items.push_back(path_item);

    return p;
}


void GeoRef::add_way(const Way& w){
    Way* to_add = new Way;
    to_add->name =w.name;
    to_add->idx = w.idx;
    to_add->id = w.id;
    to_add->uri = w.uri;
    ways.push_back(to_add);
}

ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const GeoRef & sn, const proximitylist::ProximityList<vertex_t> &prox){

    edge_t edge;
    found = true;
    try {
        edge = sn.nearest_edge(coord, prox);
    } catch(proximitylist::NotFound) {
        found = false;
        this->source = std::numeric_limits<vertex_t>::max();
        this->target = std::numeric_limits<vertex_t>::max();
    }

    if(found) {
        init(coord, sn, edge);
    }
}

ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const GeoRef & sn, type::idx_t offset, const proximitylist::ProximityList<vertex_t> &prox){
    edge_t edge;
    found = true;
    try {
        edge = sn.nearest_edge(coord, offset, prox);
    } catch(proximitylist::NotFound) {
        found = false;
        this->source = std::numeric_limits<vertex_t>::max();
        this->target = std::numeric_limits<vertex_t>::max();
    }

    if(found) {
        init(coord, sn, edge);
    }
}

void ProjectionData::init(const type::GeographicalCoord & coord, const GeoRef & sn, edge_t nearest_edge) {
    // On cherche les coordonnées des extrémités de ce segment
    this->source = boost::source(nearest_edge, sn.graph);
    this->target = boost::target(nearest_edge, sn.graph);
    type::GeographicalCoord vertex1_coord = sn.graph[this->source].coord;
    type::GeographicalCoord vertex2_coord = sn.graph[this->target].coord;
    // On projette le nœud sur le segment
    this->projected = coord.project(vertex1_coord, vertex2_coord).first;
    // On calcule la distance « initiale » déjà parcourue avant d'atteindre ces extrémité d'où on effectue le calcul d'itinéraire
    this->source_distance = projected.distance_to(vertex1_coord);
    this->target_distance = projected.distance_to(vertex2_coord);
}


//Path GeoRef::compute(const type::GeographicalCoord & start_coord, const type::GeographicalCoord & dest_coord) const{
//    ProjectionData start(start_coord, *this, this->pl);
//    ProjectionData dest(dest_coord, *this, this->pl);

//    if (start.found && dest.found) {
//        Path p = compute({start.source, start.target},
//                     {dest.source, dest.target},
//                     {start.source_distance, start.target_distance},
//                     {dest.source_distance, dest.target_distance});

//        //we add the missing segment to and from the projected departure and arrival
//        add_projections(p, start, dest);
//        return p;
//    } else {
//        throw proximitylist::NotFound();
//    }
//}


std::vector<navitia::type::idx_t> GeoRef::find_admins(const type::GeographicalCoord &coord){
    std::vector<navitia::type::idx_t> to_return;
    navitia::georef::Rect search_rect(coord);

    std::vector<idx_t> result;
    auto callback = [](idx_t id, void* vec)->bool{reinterpret_cast<std::vector<idx_t>*>(vec)->push_back(id); return true;};
    this->rtree.Search(search_rect.min, search_rect.max, callback, &result);
    for(idx_t admin_idx : result) {
        if (boost::geometry::within(coord, admins[admin_idx]->boundary)){
            to_return.push_back(admin_idx);
        }
    }
    return to_return;
}

void GeoRef::init_offset(nt::idx_t value){
    //TODO ? with something like boost::enum we could even handle loops and only define the different transport modes in the enum
    offsets[nt::Mode_e::Walking] = 0;
    offsets[nt::Mode_e::Vls] = value;
    offsets[nt::Mode_e::Bike] = 2 * value;
    offsets[nt::Mode_e::Car] = 3 * value;

    /// Pour la gestion du vls
    for(vertex_t v = 0; v<value; ++v){
        boost::add_vertex(graph[v], graph);
    }

    /// Pour la gestion du vélo
    for(vertex_t v = 0; v<value; ++v){
        boost::add_vertex(graph[v], graph);
    }

    /// Pour la gestion du voiture
    for(vertex_t v = 0; v<value; ++v){
        boost::add_vertex(graph[v], graph);
    }
}

void GeoRef::build_proximity_list(){
    pl.clear(); // vider avant de reconstruire

    if(this->offsets[navitia::type::Mode_e::Vls] == 0){
        BOOST_FOREACH(vertex_t u, boost::vertices(this->graph)){
            pl.add(graph[u].coord, u);
        }
    }else{
        // Ne pas construire le proximitylist avec les noeuds utilisés par les arcs pour la recherche vélo, voiture
        for(vertex_t v = 0; v < this->offsets[navitia::type::Mode_e::Vls]; ++v){
            pl.add(graph[v].coord, v);
        }
    }
    pl.build();

    poi_proximity_list.clear();

    for(const POI *poi : pois) {
        poi_proximity_list.add(poi->coord, poi->idx);
    }
    poi_proximity_list.build();
}

void GeoRef::build_autocomplete_list(){
    int pos = 0;
    fl_way.clear();
    for(Way* way : ways){
        // A ne pas ajouter dans le disctionnaire si pas ne nom
        if (!way->name.empty()) {
            std::string key="";
            for(Admin* admin : way->admin_list){
                //Ajout du nom de l'admin de niveau 8
                if (admin->level == 8) {
                    key+= " " + admin->name;
                }
                //Ajoute le code postal si ça existe
                if ((!admin->post_code.empty()) && (admin->level == 8))
                {
                    key += " "+ admin->post_code;
                }
            }
            fl_way.add_string(way->way_type +" "+ way->name + " " + key, pos,alias, synonymes);
        }
        pos++;
    }
    fl_way.build();

    fl_poi.clear();
    //Remplir les poi dans la liste autocompletion
    for(const POI* poi : pois){
        // A ne pas ajouter dans le disctionnaire si pas ne nom ou n'a pas d'admin
        //if ((!poi->name.empty()) && (poi->admin_list.size() > 0)){
        if (!poi->name.empty()) {
            std::string key="";
            if (poi->visible){
                for(Admin* admin : poi->admin_list){
                    if (admin->level == 8)
                    {
                        key += " " + admin->name;
                    }
                }
                fl_poi.add_string(poi->name + " " + key, poi->idx ,alias, synonymes);
            }
        }
    }
    fl_poi.build();

    fl_admin.clear();
    for(Admin* admin : admins){
        std::string key="";

        if (!admin->post_code.empty())
        {
            key = admin->post_code;
        }
        fl_admin.add_string(admin->name + " " + key, admin->idx ,alias, synonymes);
    }
    fl_admin.build();
}


/** Chargement de la liste poitype_map : mappage entre codes externes et idx des POITypes*/
void GeoRef::build_poitypes(){
   this->poitype_map.clear();
   for(const POIType* ptype : poitypes){
       this->poitype_map[ptype->uri] = ptype->idx;
   }
}

/** Chargement de la liste poi_map : mappage entre codes externes et idx des POIs*/
void GeoRef::build_pois(){
    this->poi_map.clear();
   for(const POI* poi : pois){
       this->poi_map[poi->uri] = poi->idx;
   }
}

void GeoRef::build_rtree() {
    typedef boost::geometry::model::box<type::GeographicalCoord> box;
    for(const Admin* admin : this->admins){
        auto envelope = boost::geometry::return_envelope<box>(admin->boundary);
        Rect r(envelope.min_corner().lon(), envelope.min_corner().lat(), envelope.max_corner().lon(), envelope.max_corner().lat());
        this->rtree.Insert(r.min, r.max, admin->idx);
    }
}

/** Normalisation des codes externes des rues*/
void GeoRef::normalize_extcode_way(){
    this->way_map.clear();
    for(Way* way : ways){
        way->uri = "address:"+ way->uri;
        this->way_map[way->uri] = way->idx;
    }
}


void GeoRef::normalize_extcode_admin(){
    this->admin_map.clear();
    for(Admin* admin : admins){
        admin->uri = "admin:" + admin->uri;
        this->admin_map[admin->uri] = admin->idx;
    }
}

/**
    * Recherche les voies avec le nom, ce dernier peut contenir : [Numéro de rue] + [Type de la voie ] + [Nom de la voie] + [Nom de la commune]
        * Exemple : 108 rue victor hugo reims
    * Si le numéro est rensigné, on renvoie les coordonnées les plus proches
    * Sinon le barycentre de la rue
*/
std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> GeoRef::find_ways(const std::string & str, const int nbmax, const int search_type, std::function<bool(nt::idx_t)> keep_element) const{
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> to_return;
    boost::tokenizer<> tokens(str);

    int search_number = str_to_int(*tokens.begin());
    std::string search_str;

    //Si un numero existe au début de la chaine alors il faut l'exclure.
    if (search_number != -1){
        search_str = "";
        int i = 0;
        for(auto token : tokens){
            if (i != 0){
                search_str = search_str + " " + token;
            }
            ++i;
           }
    }else{
        search_str = str;
    }
    if (search_type==0){to_return = fl_way.find_complete(search_str, alias, synonymes, word_weight, nbmax, keep_element);}
    else {to_return = fl_way.find_partial_with_pattern(search_str, alias, synonymes, word_weight, nbmax, keep_element);}

    /// récupération des coordonnées du numéro recherché pour chaque rue
    for(auto &result_item  : to_return){
       Way * way = this->ways[result_item.idx];
       result_item.coord = way->nearest_coord(search_number, this->graph);
       result_item.house_number = search_number;
    }

    return to_return;
}

int GeoRef::project_stop_points(const std::vector<type::StopPoint*> &stop_points){
    int matched = 0;
    this->projected_stop_points.clear();
    this->projected_stop_points.reserve(stop_points.size());

    for(const type::StopPoint* stop_point : stop_points) {
        std::pair<GeoRef::ProjectionByMode, bool> pair = project_stop_point(stop_point);

        this->projected_stop_points.push_back(pair.first);
        if(pair.second)
            matched++;
    }
    return matched;
}

std::pair<GeoRef::ProjectionByMode, bool> GeoRef::project_stop_point(const type::StopPoint* stop_point) const {
    bool one_proj_found = false;
    ProjectionByMode projections;

    for (const auto& pair : offsets) {
        type::idx_t offset = pair.second;
        type::Mode_e transportation_mode = pair.first;

        ProjectionData proj(stop_point->coord, *this, offset, this->pl);
        projections[transportation_mode] = proj;
        if(proj.found)
            one_proj_found = true;
    }
    return {projections, one_proj_found};
}

edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates) const {
    return this->nearest_edge(coordinates, this->pl);
}

vertex_t GeoRef::nearest_vertex(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const {
    return prox.find_nearest(coordinates);
}

/// Get the nearest_edge with at least one vertex in the graph corresponding to the offset (walking, bike, ...)
edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, type::idx_t offset, const proximitylist::ProximityList<vertex_t>& prox) const {
    auto vertexes_within = prox.find_within(coordinates);
    for (const auto pair_coord : vertexes_within) {
        //we increment the index to get the vertex in the other graph
        const auto new_vertex = pair_coord.first + offset;

        try {
            edge_t edge_in_graph = nearest_edge(coordinates, new_vertex);
            return edge_in_graph;
        } catch(proximitylist::NotFound) {}
    }
    throw proximitylist::NotFound();
}


edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, const vertex_t & u) const{

    type::GeographicalCoord coord_u, coord_v;
    coord_u = this->graph[u].coord;
    float dist = std::numeric_limits<float>::max();
    edge_t best;
    bool found = false;
    BOOST_FOREACH(edge_t e, boost::out_edges(u, this->graph)){
        vertex_t v = boost::target(e, this->graph);
        coord_v = this->graph[v].coord;
        // Petite approximation de la projection : on ne suit pas le tracé de la voirie !
        auto projected = coordinates.project(coord_u, coord_v);
        if(projected.second < dist){
            found = true;
            dist = projected.second;
            best = e;
        }
    }
    if(!found)
        throw proximitylist::NotFound();
    else
        return best;

}
edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const {
    vertex_t u = nearest_vertex(coordinates, prox);
    return nearest_edge(coordinates, u);
}

GeoRef::~GeoRef() {
    for(POIType* poi_type : poitypes) {
        delete poi_type;
    }
    for(POI* poi: pois) {
        delete poi;
    }
    for(Way* way: ways) {
        delete way;
    }
    for(Admin* admin: admins) {
        delete admin;
    }

}


std::vector<type::idx_t> POI::get(type::Type_e type, const GeoRef &) const {
    switch(type) {
    case type::Type_e::POIType : return {poitype_idx}; break;
    default : return {};
    }
}

std::vector<type::idx_t> POIType::get(type::Type_e type, const GeoRef & data) const {
    std::vector<type::idx_t> result;
    switch(type) {
    case type::Type_e::POI:
        for(const POI* elem : data.pois) {
            if(elem->poitype_idx == idx) {
                result.push_back(elem->idx);
            }
        }
        break;
    default : break;
    }
    return result;
}

}}
