#include <iostream>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <utility>                   // for std::pair
#include <math.h>
#include <algorithm>                 // for std::for_each
#include <chrono>
#include <sstream>
#include <string>
#include <list>
#include <fstream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/gregorian_calendar.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/depth_first_search.hpp>


#define BOOST_GRAPH_USE_NEW_CSR_INTERFACE
#include <boost/graph/compressed_sparse_row_graph.hpp>

#include "data.h"
#include "type.pb.h"
#include "network.h"


#include "main.h"


#include "proto_sortie.pb.h"

using namespace boost;
using namespace network;
navitia::type::Data data;


std::list<vertex_t> in_vertices(vertex_t v, NW g);



struct get_edge_property{
    NW & nw;
    get_edge_property(NW & nw) : nw(nw) {}
    EdgeDesc operator()(edge_t i){
        return nw[i];
    }
};





unsigned int day;


int main(int /*argc*/, char** argv){




    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    std::chrono::duration<double> sec;
    //Chargement des données
    std::cout << "Chargement des données..." << std::flush;
    //                data.load_flz("/home/vlara/navitia/jeu/PdL/PdL.nav");
    data.load_flz("/home/vlara/navitia/jeu/IdF/IdF.nav");
    //        data.load_flz("/home/vlara/navitia/jeu/poitiers/poitiers.nav");
//    data.load_flz("/home/vlara/navitia/jeu/passeminuit/passeminuit.nav");
    sec = std::chrono::system_clock::now() - start;
    std::cout << " effectué en " << sec.count() << std::endl << std::endl;

    std::cout << "Nb mode types : " << data.pt_data.modes.size() << std::endl;

    //Initialisation du graph
    start = std::chrono::system_clock::now();
    std::cout << "Initialisation du graph ..." << std::flush;
    //    NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + (data.pt_data.stop_times.size()*3));

    //    NW g(10);
    //    std::vector<vertex_t> vertices(10);

    //Initialisations de variables pour bosot graph
    //    idxv idx_v = get(&VertexDesc::idx, g);
    //    ntt nt_t = get(&VertexDesc::nt, g);
    //    intp debut_t = get(&EdgeDesc::debut, g);





    //        charger_graph2(data, g);
    //    if((atoi(argv[5]) == 5) || (atoi(argv[5]) == 6) || (atoi(argv[5]) == 8)|| (atoi(argv[5]) == 9)) {
    if(atoi(argv[5]) != 13) {
        NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()*2));
        charger_graph4(data, g);
        sec = std::chrono::system_clock::now() - start;
        std::cout << " effectuée en " << sec.count() << std::endl << std::endl;


        std::cout << "Fin chargement" << std::endl;
        std::cout << "Nb stop areas : " << data.pt_data.stop_areas.size() << std::endl;
        std::cout << "Nb stop points : " << data.pt_data.stop_points.size() << std::endl;
        std::cout << "Nb route points : " << data.pt_data.route_points.size() << std::endl;
        std::cout << "Nb stop times : " << data.pt_data.stop_times.size() << std::endl;
        std::cout << "Nb modes : " << data.pt_data.modes.size() << std::endl;
        std::cout << "Num vertices : " << num_vertices(g) << std::endl;
        std::cout << "Num edges  : " << num_edges(g) << std::endl;

        int count = 0;//Recherche du plus court chemin des arguments passes en parametes
        std::vector<vertex_t> predecessors(boost::num_vertices(g));
        std::vector<etiquette> distances(boost::num_vertices(g));

        vertex_t v1, v2;
        v1 = atoi(argv[1]);
        v2 = atoi(argv[2]);
        boost::graph_traits<NW>::out_edge_iterator e, e1, e_end, e_end1;
        for (tie(e, e_end) = out_edges(v1, g); e != e_end; ++e)  {
            for (tie(e1, e_end1) = out_edges(target(*e, g), g); e1 != e_end1; ++e1)
                count ++;
        }

        std::cout << "Num horaires au depart : " <<  count << std::endl;

        std::cout << v1 << " : "<<  data.pt_data.stop_areas.at(v1).name << "  v2 : " << v2 << " : " << data.pt_data.stop_areas.at(v2).name << std::endl;




        //    day = (gregorian::from_string(argv[4]) - data.pt_data.validity_patterns.at(1).beginning_date).days();
        //    std::cout << "d : " << day << std::endl;
        //    bool t = false;
        //    BOOST_FOREACH(navitia::type::ValidityPattern vp, data.pt_data.validity_patterns) {
        //        t = t || vp.check(day);
        //    }
        //    std::cout << "vp : " << data.pt_data.validity_patterns.size() << std::endl;

        //debug2(g, vertices); navitia::type::Data &data


        std::cout << "Recherche de l'itineraire ..." << std::flush;

        //    sdt::cout << atoi(argv[3]) << std::endl;
        etiquette etdebut, etmax;
        etdebut.temps = 0;
        etdebut.arrivee = atoi(argv[3]);
        etdebut.commence = false;
        etdebut.cible = v2;
        etmax.temps = 3600*12 + 1;
        etmax.arrivee = 3600*12 + 1;
        etmax.commence = true;

        if(atoi(argv[5]) == 1 /*|| atoi(argv[5]) == 5*/) {
            std::cout << "Sans CSR" << std::endl;
            start = std::chrono::system_clock::now();
            try {
                boost::dijkstra_shortest_paths(g, v1,
                                               boost::predecessor_map(&predecessors[0])
                                               .weight_map(boost::get(edge_bundle, g))
                                               .distance_map(&distances[0])
                                               .distance_combine(&calc_fin2)
                                               .distance_zero(etdebut)
                                               .distance_inf(etmax)
                                               .distance_compare(edge_less())
                                               .visitor(dijkstra_goal_visitor(v2))
                                               );
            } catch(found_goal) {std::cout << "...FOUND..." << std::endl;}
        } else if(atoi(argv[5]) == 2){
            std::cout << "Avec CSR" << std::endl;
            NWCSR g2(g);
            BOOST_FOREACH(vertex_t v, vertices(g))
                    g2[v] = g[v];
            edge_t ed;
            bool bo;
            BOOST_FOREACH(auto e4, edges(g2)) {
                boost::tie(ed, bo)=boost::edge(source(e4, g2), target(e4, g2), g);
                g2[e4] = g[ed];
            }

            start = std::chrono::system_clock::now();
            try {
                boost::dijkstra_shortest_paths_no_color_map(g2, v1,
                                                            boost::predecessor_map(&predecessors[0])
                                                            .weight_map(boost::get(edge_bundle, g2))
                                                            .distance_map(&distances[0])
                                                            .distance_combine(&calc_fin2)
                                                            .distance_zero(etdebut)
                                                            .distance_inf(etmax)
                                                            .distance_compare(edge_less())
                                                            //.visitor(dijkstra_goal_visitor(v2))
                                                            );
            } catch(found_goal) {}
        } else if(atoi(argv[5]) == 3) {
            std::cout << "Sans couleur" << std::endl;
            start = std::chrono::system_clock::now();
            try {
                try {
                    boost::dijkstra_shortest_paths_no_color_map(g, v1,
                                                                boost::predecessor_map(&predecessors[0])
                                                                .weight_map(boost::get(edge_bundle, g))
                                                                .distance_map(&distances[0])
                                                                .distance_combine(&calc_fin3)
                                                                .distance_zero(etdebut)
                                                                .distance_inf(etmax)
                                                                .distance_compare(edge_less())
                                                                .visitor(dijkstra_goal_visitor2(v2, data))
                                                                );
                } catch(found_goal f) { v2 = f.v; std::cout << "FOUND " << f.v << " " << std::endl;afficher_horaire(data.pt_data.stop_times.at(g[f.v].idx).arrival_time); }

            } catch(found_goal) {}
        }else if(atoi(argv[5]) == 4) {
            std::cout << "Sans visitor" << std::endl;
            start = std::chrono::system_clock::now();
            boost::dijkstra_shortest_paths(g, v1,
                                           boost::predecessor_map(&predecessors[0])
                                           .weight_map(boost::get(edge_bundle, g))
                                           .distance_map(&distances[0])
                                           .distance_combine(&calc_fin2)
                                           .distance_zero(etdebut)
                                           .distance_inf(etmax)
                                           .distance_compare(edge_less())
                                           //                                       .visitor(dijkstra_goal_visitor(v2))
                                           );
        } else if(atoi(argv[5]) == 5) {
            std::cout << "DAG" << std::flush;
            start = std::chrono::system_clock::now();

            try {
                boost::dijkstra_shortest_paths(g, v1,
                                               boost::predecessor_map(&predecessors[0])
                                               .weight_map(boost::get(edge_bundle, g))
                                               .distance_map(&distances[0])
                                               .distance_combine(&calc_fin3)
                                               .distance_zero(etdebut)
                                               .distance_inf(etmax)
                                               .distance_compare(edge_less())
                                               .visitor(dijkstra_goal_visitor2(v2, data))
                                               );
            } catch(found_goal f) { v2 = f.v; std::cout << "FOUND " << f.v << " " << std::endl;afficher_horaire(data.pt_data.stop_times.at(g[f.v].idx).arrival_time); }



            //        boost::depth_first_search(g, visitor(detect_loops(data)));
        }  else if(atoi(argv[5]) == 6) {
            std::cout << "DAG sans visiteur" << std::flush;
            start = std::chrono::system_clock::now();


            boost::dijkstra_shortest_paths(g, v1,
                                           boost::predecessor_map(&predecessors[0])
                                           .weight_map(boost::get(edge_bundle, g))
                                           .distance_map(&distances[0])
                                           .distance_combine(&calc_fin2)
                                           .distance_zero(etdebut)
                                           .distance_inf(etmax)
                                           .distance_compare(edge_less())
                                           );


        } else if(atoi(argv[5]) == 7) {
            std::cout << "Sans CSR" << std::endl;
            start = std::chrono::system_clock::now();
            try {
                boost::dijkstra_shortest_paths(g, v1,
                                               boost::predecessor_map(&predecessors[0])
                                               .weight_map(boost::get(edge_bundle, g))
                                               .distance_map(&distances[0])
                                               .distance_combine(&calc_fin2)
                                               .distance_zero(etdebut)
                                               .distance_inf(etmax)
                                               .distance_compare(edge_less())
                                               .visitor(dijkstra_goal_visitor(v2))
                                               );
            } catch(found_goal) {std::cout << " I caught it because I can" << std::endl;}
            FILE *file = fopen( "test.txt", "w" );
            vertex_t v;
            edge_t e;
            bool bo;
            for( v = v2 ; v != v1;v = predecessors[v]) {
                tie(e, bo) = edge(predecessors[v], v, g);
                fprintf(file, "%ld %ld %d\n",predecessors[v], v, g[e].e_type);
            }

            fclose(file);

        } else if(atoi(argv[5]) == 8) {
            std::ifstream file( "test.txt" );


            std::string ligne;

            int a,b,c;
            edge_t e;
            bool bo, t2 = false;
            vertex_t v1, v2;
            while ( std::getline( file, ligne ) ) {
                sscanf(ligne.c_str(), "%d %d %d", &a, &b, &c);
                tie(e, bo) = edge(a, b, g);
                std::cout << a << " - > " << b << " type : " << c << std::endl;
                if(bo)
                    if( g[e].e_type == hoho)
                        v2 = a;

                if(!bo & !t2)
                    t2 = true;
                if(bo & t2) {
                    if(g[e].e_type == hoho)
                        v1 = b;
                    try {
                        boost::dijkstra_shortest_paths(g, v1,
                                                       boost::predecessor_map(&predecessors[0])
                                                       .weight_map(boost::get(edge_bundle, g))
                                                       .distance_map(&distances[0])
                                                       .distance_combine(&calc_fin2)
                                                       .distance_zero(etdebut)
                                                       .distance_inf(etmax)
                                                       .distance_compare(edge_less())
                                                       .visitor(dijkstra_goal_visitor(v2))
                                                       );
                    } catch(found_goal) {std::cout << " I caught it because I can" << std::endl;}
                    if(predecessors[v2] == v2) {
                        std::cout << "Fail" << std::endl;
                    } else {
                        afficher_chemin(v1, v2, predecessors, distances, g);
                        std::cout << "duree : " << distances[v2].temps << " arrivee : " << distances[v2].arrivee <<  std::endl;
                    }
                    t2 = false;
                }

            }
        } else if (atoi(argv[5])==9) {
            try {
                boost::dag_shortest_paths(g, v1,
                                          boost::predecessor_map(&predecessors[0])
                                          .weight_map(boost::get(edge_bundle, g))
                                          .distance_map(&distances[0])
                                          .distance_combine(&calc_fin3)
                                          .distance_zero(etdebut)
                                          .distance_inf(etmax)
                                          .distance_compare(edge_less())
                                          .visitor(dijkstra_goal_visitor2(v2, data))
                                          );
            } catch(found_goal f) { v2 = f.v; std::cout << "FOUND " << f.v << " " << std::endl;afficher_horaire(data.pt_data.stop_times.at(g[f.v].idx).arrival_time); }

        } else if(atoi(argv[5]) == 10) {
            //PAS FINI DU TOUT
            std::cout << "DAG no init" << std::flush;
            std::vector<vertex_t> indexes(boost::num_vertices(g));
            init_distances_predecessors_map(etmax, distances, predecessors, indexes);
            property_map<NW, vertex_index_t>::type indexmap = get(vertex_index, g);
            start = std::chrono::system_clock::now();
            try {
                dijkstra_shortest_paths_no_init(g, v1, &predecessors[0], &distances[0], boost::get(edge_bundle, g), indexmap, edge_less(), &calc_fin3, etdebut,dijkstra_goal_visitor2(v2, data));

            } catch(found_goal f) { v2 = f.v; std::cout << "FOUND " << f.v << " " << std::endl;afficher_horaire(data.pt_data.stop_times.at(g[f.v].idx).arrival_time); }



            //        boost::depth_first_search(g, visitor(detect_loops(data)));
        }else if(atoi(argv[5]) == 11) {
            debug_prochains_horaires(g, v1, atoi(argv[3]));
        } else if(atoi(argv[5]) == 12) {
            std::cout << "DAG ++" << std::endl;
            start = std::chrono::system_clock::now();
            try {
                boost::dijkstra_shortest_paths(g, v1,
                                               boost::predecessor_map(&predecessors[0])
                                               .weight_map(boost::get(edge_bundle, g))
                                               .distance_map(&distances[0])
                                               .distance_combine(&calc_fin4)
                                               .distance_zero(etdebut)
                                               .distance_inf(etmax)
                                               .distance_compare(edge_less())
                                               .visitor(dijkstra_goal_visitor2(v2, data))
                                               );
            } catch(found_goal f) { v2 = f.v; std::cout << "FOUND " << f.v <<std::endl;afficher_horaire(data.pt_data.stop_times.at(g[f.v].idx).arrival_time); }
            //        boost::depth_first_search(g, visitor(detect_loops(data)));
        }


        sec = std::chrono::system_clock::now() - start;
        std::cout << " effectuée en " << sec.count() << std::endl << std::endl;

        //        debug2(g);

        //Affichage du chemin
        if(predecessors[v2] == v2) {
            std::cout << "Fail" << std::endl;
        } else {
            afficher_chemin(v1, v2, predecessors, distances, g);
            std::cout << "duree : " << distances[v2].temps << " arrivee : " << distances[v2].arrivee <<  std::endl;
        }

        if(atoi(argv[5]) == 6) {
            std::multimap<unsigned int, etiquette> retourHoraires;
            std::map<unsigned int, navitia::type::GeographicalCoord> retourGeo;
            for(unsigned int tdi = data.pt_data.stop_areas.size() + data.pt_data.stop_points.size(); tdi < data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.stop_times.size();++tdi) {
                if((g[tdi].saidx != v1) & (predecessors[tdi] != tdi)) {
                    retourHoraires.insert(std::pair<unsigned int, etiquette>(tdi, distances[tdi]));
                    if(retourGeo.find(g[tdi].saidx) == retourGeo.end())
                        retourGeo.insert(std::pair<unsigned int, navitia::type::GeographicalCoord>(g[tdi].saidx, data.pt_data.stop_areas.at(g[tdi].saidx).coord));
                }
            }

            std::cout << "Size horaires : " << retourHoraires.size() << " | size retourGeo : " << retourGeo.size() << std::endl;
            std::cout << "Un horaire : " << sizeof(etiquette) << " | une coordonnee : " << sizeof(navitia::type::GeographicalCoord) << std::endl;

            std::ofstream fichier("test.csv", std::ios::out | std::ios::trunc);

            if(fichier) {
                for(std::multimap<unsigned int, etiquette>::iterator it = retourHoraires.begin(); it!=retourHoraires.end();++it ) {
                    fichier << (*it).first << ";" << (*it).second.temps << ";" << ";" << (*it).second.arrivee << std::endl ;
                }

                fichier.close();
            }

        }
    } else if(atoi(argv[5]) == 13) {
        std::cout << "Num Validity Patterns  : " << data.pt_data.validity_patterns.size() << std::endl;
        NW2 g2(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()*2));
        charger_graph5(data, g2);
        sec = std::chrono::system_clock::now() - start;
        std::cout << " effectuée en " << sec.count() << std::endl << std::endl;


        std::cout << "Fin chargement" << std::endl;
        std::cout << "Nb stop areas : " << data.pt_data.stop_areas.size() << std::endl;
        std::cout << "Nb stop points : " << data.pt_data.stop_points.size() << std::endl;
        std::cout << "Nb route points : " << data.pt_data.route_points.size() << std::endl;
        std::cout << "Nb stop times : " << data.pt_data.stop_times.size() << std::endl;
        std::cout << "Nb modes : " << data.pt_data.modes.size() << std::endl;
        std::cout << "Num vertices : " << num_vertices(g2) << std::endl;
        std::cout << "Num edges  : " << num_edges(g2) << std::endl;
        std::cout << "Num Validity Patterns  : " << data.pt_data.validity_patterns.size() << std::endl;


        std::vector<vertex_t2> predecessors(boost::num_vertices(g2));
        std::vector<etiquette2> distances(boost::num_vertices(g2));

        vertex_t2 v1, v2;
        v1 = atoi(argv[1]);
        v2 = atoi(argv[2]);


        std::cout << "DAG (++)²" << std::endl;
        start = std::chrono::system_clock::now();

        etiquette2 etdebut;
        etdebut.temps = 0;
        etdebut.date_arrivee = data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string(argv[4]));
        etdebut.heure_arrivee = atoi(argv[3]);
        etdebut.commence = false;



        std::cout << "v1 : " << data.pt_data.stop_areas.at(v1).name << " v2 : " << data.pt_data.stop_areas.at(v2).name << "predecessors size : " << predecessors.size() << " distances size : "  << distances.size() << std::endl;
        std::cout << "date arrivee : " << etdebut.date_arrivee << std::endl;

                try {
                    boost::dijkstra_shortest_paths(g2, v1,
                                                   boost::predecessor_map(&predecessors[0])
                                                   .weight_map(boost::get(edge_bundle, g2))
                                                   .distance_map(&distances[0])
                                                   .distance_combine(combine2(data, g2, v2, etdebut.date_arrivee))
                                                   .distance_zero(etdebut)
                                                   .distance_inf(etiquette2::max())
                                                   .distance_compare(edge_less2())
                                                   .visitor(dijkstra_goal_visitor4(v2, data))
                                                   );
                } catch(found_goal fg) { v2 = fg.v; std::cout << "FOUND " << fg.v <<std::endl;afficher_horaire(data.pt_data.stop_times.at(g2[fg.v].idx).arrival_time); }

                sec = std::chrono::system_clock::now() - start;
                std::cout << " effectuée en " << sec.count() << std::endl << std::endl;


                //Affichage du chemin
                if(predecessors[v2] == v2) {
                    std::cout << "Fail " << v2 << std::endl;
                } else {
                    afficher_chemin(v1, v2, predecessors, distances, g2);
                    std::cout << "duree : " << distances[v2].temps << " arrivee : " << distances[v2].heure_arrivee << " " << distances[v2].date_arrivee << " " << etdebut.date_arrivee<<  std::endl;
                }

        std::ofstream myfile;
        myfile.open ("graph.dot");

        label_name ln(g2, data);
        edge_namer en(g2);

        write_graphviz(myfile, g2, ln, en);

        myfile.close();
    }











    //Afficher toutes les heures d'arrivees possible


    //    vertex_t tcv, tav;
    //    unsigned int stid;

    //    Liste des sp

    //            std::cout  << "Num vertices : " << num_vertices(g) << "Num vj : " << data.pt_data.vehicle_journeys.size() <<  std::endl;
    //            BOOST_FOREACH(vertex_t spv, in_vertices(v2, g)) {
    //                //Liste des tc
    //                for (tie(e, e_end) = out_edges(spv, g); e != e_end; ++e) {
    //                    tcv = target(*e, g);
    //                    if(get(nt_t, tcv) == ho) {
    //                        stid = get(idx_v, tcv);

    //                        tav = data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + stid;
    //                        if(get(nt_t, tav) == ho) {
    //                            //On recupere l'heure d'arrivee
    //                            //                    if(distances[tav].second < (3600*24+1)) {
    //                            if(predecessors[tav] != tav) {
    //                                std::cout << "Il existe une arrivee a : "  << std::flush ;
    //                                afficher_horaire(distances[tav].arrivee);
    //                                std::cout << " qui dure : " << distances[tav].temps <<std::endl;
    //                                afficher_chemin(v1, tav, predecessors, distances, g);
    //                                std::cout << std::endl;
    //                            }

    //                            //                    std::cout << get(idx_v, tcv) << " " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(get(idx_v, tcv)).vehicle_journey_idx).route_idx).line_idx).name << std::endl;
    //                        }


    //                    }

    //                }
    //            }


    //    iterator_t e1, e_end1;
    //    vertex_t spv, tdv;

    //    for(tie(e1, e_end1) = out_edges(v1, g); e1!=e_end1; ++e1) {
    //        spv = target(*e1, g);
    //        for (tie(e, e_end) = out_edges(spv, g); e != e_end; ++e) {
    //            tcv = target(*e, g);
    //            stid = get(idx_v, tcv);
    //            tdv = data.pt_data.stop_points.size() + data.pt_data.stop_areas.size() + data.pt_data.stop_times.size() + stid ;
    //            if(get(nt_t, tdv) == td) {
    //                //On recupere l'heure d'arrivee
    //                //                    if(distances[tav].second < (3600*24+1)) {
    //                std::cout << "Il existe un depart  a : "  << std::flush ;
    //                afficher_horaire(distances[tdv].second);
    //                std::cout << " qui dure : " << distances[tdv].first <<std::endl;
    //            }
    //        }hoho
    //    }



    //    std::ofstream myfile;
    //    myfile.open ("graph.z");

    //    label_name ln(g);


    //    write_graphviz(myfile, g, ln);









    return 0;
}


void charger_graph2(navitia::type::Data &data, NW &g) {
    idxv idx_v = get(&VertexDesc::idx, g);

    unsigned int debut = 0;

    //Creation des stop areas
    BOOST_FOREACH(navitia::type::StopArea sai, data.pt_data.stop_areas) {
        g[sai.idx] = VertexDesc(sai.idx, sa);
    }

    //Creation des stop points et creations lien SA<->SP
    debut = data.pt_data.stop_areas.size();
    unsigned int i = 0;
    BOOST_FOREACH(navitia::type::StopPoint spi, data.pt_data.stop_points) {
        g[get_sp_idx(spi.idx, data)] = VertexDesc(spi.idx, sp);
        std::ostringstream oss;
        oss << g[spi.stop_area_idx].idx;
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx, data), EdgeDesc(1, /*"SA->SP "+oss.str(), */sasp), g);
        add_edge(get_sp_idx(spi.idx, data), spi.stop_area_idx, EdgeDesc(1, /*"SP->SA "+oss.str(), */spsa), g);
        ++i;
    }


    vertex_t tcv, tav, tdv, spv;

    debut += data.pt_data.stop_points.size();
    BOOST_FOREACH(navitia::type::VehicleJourney vj,  data.pt_data.vehicle_journeys) {
        tdv = 0;
        BOOST_FOREACH(unsigned int stid, vj.stop_time_list) {
            spv = get_sp_idx(data.pt_data.route_points.at(data.pt_data.stop_times.at(stid).route_point_idx).stop_point_idx, data);

            //ta

            g[get_ta_idx(stid, data)] = VertexDesc(stid, ta);
            tav = get_ta_idx(stid, data) ;//add_vertex(g[get_ta_idx(stid)] , g);

            if(tdv != 0) {
                add_edge(tdv, tav, EdgeDesc(vj.idx, /*"TD->TA",*/ data.pt_data.stop_times.at(get(idx_v, tdv)).departure_time, data.pt_data.stop_times.at(stid).arrival_time, tdta), g);
            }

            //td
            g[get_td_idx(stid, data)] = VertexDesc(stid, td);
            tdv = get_td_idx(stid, data);//add_vertex(g[get_td_idx(stid)], g);

            //tc
            g[get_tc_idx(stid, data)] = VertexDesc(stid, tc);
            tcv = get_tc_idx(stid, data);//add_vertex(g[get_tc_idx(stid)], g);

            //edges

            add_edge(tav, tcv, EdgeDesc(vj.idx,/* "TA->TC", */data.pt_data.stop_times.at(stid).arrival_time + 1, data.pt_data.stop_times.at(stid).arrival_time + 2, tatc ), g);
            add_edge(tcv, tdv, EdgeDesc(vj.idx, /*"TC->TD", */data.pt_data.stop_times.at(stid).departure_time - 2, data.pt_data.stop_times.at(stid).departure_time - 1, tctd), g);
            add_edge(tav, tdv, EdgeDesc(vj.idx, /*"TA->TD", */data.pt_data.stop_times.at(stid).arrival_time, data.pt_data.stop_times.at(stid).departure_time, tatd), g);
            add_edge(tcv, spv, EdgeDesc(vj.idx, /*"TC->SP", */data.pt_data.stop_times.at(stid).arrival_time + 2, data.pt_data.stop_times.at(stid).arrival_time + 3,tcsp) , g);
            add_edge(spv, tcv, EdgeDesc(vj.idx, /*"SP->TC", */data.pt_data.stop_times.at(stid).departure_time - 3, data.pt_data.stop_times.at(stid).departure_time - 2, sptc) ,  g);
        }
    }
}


void charger_graph3(navitia::type::Data &data, NW &g) {
    idxv idx_v = get(&VertexDesc::idx, g);

    unsigned int debut = 0;

    //Creation des stop areas
    BOOST_FOREACH(navitia::type::StopArea sai, data.pt_data.stop_areas) {
        g[sai.idx] = VertexDesc(sai.idx, sa);
    }

    //Creation des stop points et creations lien SA<->SP
    debut = data.pt_data.stop_areas.size();
    BOOST_FOREACH(navitia::type::StopPoint spi, data.pt_data.stop_points) {
        g[get_sp_idx(spi.idx, data)] = VertexDesc(spi.idx, sp);
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx, data), EdgeDesc(1, /*"SA->SP "+oss.str(), */sasp), g);
        add_edge(get_sp_idx(spi.idx, data), spi.stop_area_idx, EdgeDesc(1, /*"SP->SA "+oss.str(), */spsa), g);
    }


    vertex_t ho1, hop, spv;
    debut += data.pt_data.stop_points.size();
    BOOST_FOREACH(navitia::type::VehicleJourney vj,  data.pt_data.vehicle_journeys) {
        hop = num_vertices(g) + 1 ;

        BOOST_FOREACH(unsigned int stid, vj.stop_time_list) {
            spv = get_sp_idx(data.pt_data.route_points.at(data.pt_data.stop_times.at(stid).route_point_idx).stop_point_idx, data);

            ho1 = get_ho_idx(stid, data) ;
            g[ho1] = VertexDesc(stid, ho);


            if(hop != num_vertices(g) + 1) {
                add_edge(hop, ho1, EdgeDesc(vj.idx, data.pt_data.stop_times.at(get(idx_v, hop)).departure_time, data.pt_data.stop_times.at(stid).arrival_time, hoho), g);
            }

            //edges
            add_edge(spv, ho1, EdgeDesc(vj.idx, data.pt_data.stop_times.at(stid).departure_time - 2, data.pt_data.stop_times.at(stid).departure_time - 1, spho) ,  g);
            add_edge(ho1, spv, EdgeDesc(vj.idx, data.pt_data.stop_times.at(stid).arrival_time + 1, data.pt_data.stop_times.at(stid).arrival_time + 2,hosp) , g);

            hop = ho1;
        }
    }

}





etiquette calc_fin(etiquette debut, EdgeDesc ed) {
    if(debut == etiquette::max())
        return debut;
    else
        if(!debut.commence)
            if(ed.e_type == sptc)
                if(debut.arrivee <= ed.debut) {
                    etiquette retour;
                    retour.commence = true;
                    retour.temps = debut.temps + ed.fin - ed.debut;
                    retour.arrivee = ed.fin;
                    return retour;
                }

                else
                    return etiquette::max();

            else
                return debut;
        else
            if(ed.e_type == spsa || ed.e_type == sasp || ed.e_type == sptc || ed.e_type == tcsp|| ed.e_type == hosp || ed.e_type == spho)
                return debut;
            else
                if(debut.arrivee <= ed.debut) {
                    if(data.pt_data.vehicle_journeys.at(ed.idx).validity_pattern_idx > data.pt_data.validity_patterns.size()) {
                        etiquette retour;
                        retour.commence = true;
                        retour.temps = debut.temps + ed.fin - ed.debut;
                        retour.arrivee = ed.fin;
                        return retour;
                    }

                    if(data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(ed.idx).validity_pattern_idx).check(day)) {
                        etiquette retour;
                        retour.commence = true;
                        retour.temps = debut.temps + ed.fin - ed.debut;
                        retour.arrivee = ed.fin;
                        return retour;
                    }
                    else
                        return etiquette::max();

                }

                else
                    return etiquette::max();
}


etiquette calc_fin2(etiquette debut, EdgeDesc ed) {
    if(debut == etiquette::max())
        return debut;
    else
        if(!debut.commence)
            if(ed.e_type == spho)
                if(debut.arrivee <= ed.debut) {
                    debut.commence = true;
                    return debut;
                }
                else
                    return etiquette::max();

            else
                return debut;
        else
            if(ed.e_type == spsa || ed.e_type == sasp || ed.e_type == hosp || ed.e_type == spho) {
                debut.arrivee += 60;
                debut.temps += 60;
                return debut;
            }
            else
                if(debut.arrivee <= ed.debut) {
                    //                    if(data.pt_data.vehicle_journeys.at(ed.idx).validity_pattern_idx > data.pt_data.validity_patterns.size()
                    //                       || data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(ed.idx).validity_pattern_idx).check(day)) {
                    etiquette retour;
                    retour.commence = true;
                    retour.temps = debut.temps + ed.fin - ed.debut;
                    retour.arrivee = ed.fin;
                    return retour;
                    //                    }
                    //                    else
                    //                        return etiquette::max();
                }

                else
                    return etiquette::max();
}




void afficher_horaire(int tps_secs){
    std::cout << (tps_secs / 3600) << ":" << (tps_secs % 3600) / 60 << ":" << (tps_secs % 60);
}


void debug1(vertex_t &v, NW &g, navitia::type::Data &data) {
    //Affiche tous les tc possible pour un sa et un jour donne

    vertex_t sp;
    //On liste les sp
    intp debut_t = get(&EdgeDesc::debut, g);
    idxp line_t = get(&EdgeDesc::idx, g);


    boost::graph_traits<NW>::out_edge_iterator e, e1, e_end, e_end1;
    for (tie(e, e_end) = out_edges(v, g); e != e_end; ++e) {
        //On liste les tc
        sp = target(*e, g);

        for (tie(e1, e_end1) = out_edges(sp, g); e1 != e_end1; ++e1) {
            if(data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(get(line_t, *e1)).validity_pattern_idx).check(day)) {
                afficher_horaire(get(debut_t, *e1));
                std::cout << std::endl;
            }
        }
    }

}

void debug2(NW &g) {
    ntt nt_t = get(&VertexDesc::nt, g);
    ett et_t = get(&EdgeDesc::e_type, g);



    BOOST_FOREACH(edge_t e, edges(g)) {
        switch(get(et_t, e)) {
        case tatd :
            if(!((get(nt_t, source(e, g)) == ta )& (get(nt_t, target(e, g)) == td))) {
                std::cout << "wrong 1" << " "<< get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g));
                std::cout <<" "<< source(e, g)<< " " << target(e, g) << std::endl;
            }
            break;
        case tdta :
            if(!((get(nt_t, source(e, g)) ==  td)& (get(nt_t, target(e, g)) == ta))) {
                std::cout << "wrong 2 " << get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g));
                std::cout <<" "<< g[source(e, g)].idx<< " " << g[target(e, g)].idx << std::endl;
            }
            break;
        case tatc :
            if(!((get(nt_t, source(e, g)) ==  ta)& (get(nt_t, target(e, g)) == tc)))
                std::cout << "wrong 3" << std::endl;
            break;
        case tctd :
            if(!((get(nt_t, source(e, g)) ==  tc)& (get(nt_t, target(e, g)) == td)))
                std::cout << "wrong 4" << std::endl;
            break;
        case sptc :
            if(!((get(nt_t, source(e, g)) ==  sp)& (get(nt_t, target(e, g)) == tc)))
                std::cout << "wrong 5" << " " <<  get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g)) << std::endl;
            break;
        case tcsp :
            if(!((get(nt_t, source(e, g)) == tc )&( get(nt_t, target(e, g)) == sp)))
                std::cout << "wrong 6 " << std::endl;
            break;
        case spsa :
            if(!((get(nt_t, source(e, g)) == sp )& (get(nt_t, target(e, g)) == sa)))
                std::cout << "wrong 7" <<  get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g)) << std::endl;
            break;
        case sasp :
            if(!((get(nt_t, source(e, g)) == sa )& (get(nt_t, target(e, g)) == sp)))
                std::cout << "wrong 8" << std::endl;
            break;
        case spho :
            if(!((get(nt_t, source(e, g)) == sp )& (get(nt_t, target(e, g)) == ho)))
                std::cout << "wrong 9" << std::endl;
            break;
        case hosp :
            if(!((get(nt_t, source(e, g)) == ho )& (get(nt_t, target(e, g)) == sp)))
                std::cout << "wrong 10" << std::endl;
            break;
        case hoho :
        case hoi :
            if(!((get(nt_t, source(e, g)) == ho )& (get(nt_t, target(e, g)) == ho)))
                std::cout << "wrong 11" << std::endl;
            break;
        case spta:
            if(!((get(nt_t, source(e, g)) == sp )& (get(nt_t, target(e, g)) == ta)))
                std::cout << "wrong 12" << std::endl;
            break;
        case tdtd:
            if(!((get(nt_t, source(e, g)) == td )& (get(nt_t, target(e, g)) == td)))
                std::cout << "wrong 13 " << g[source(e, g)].nt << " " << g[target(e, g)].nt << std::endl;
            break;
        case tatdc:
            if(!((get(nt_t, source(e, g)) == ta )& (get(nt_t, target(e, g)) == td))) {
                std::cout << "wrong 14" << " " <<get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g));
                std::cout <<" "<< g[source(e, g)].idx<< " " << g[target(e, g)].idx << std::endl;
            }
            break;
        case sprp:
            if(!((get(nt_t, source(e, g)) == sp )& (get(nt_t, target(e, g)) == rp))) {
                std::cout << "wrong 15" << " " <<get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g));
                std::cout <<" "<< g[source(e, g)].idx<< " " << g[target(e, g)].idx << std::endl;
            }
            break;
        case rpta:
            if(!((get(nt_t, source(e, g)) == rp )& (get(nt_t, target(e, g)) == ta))) {
                std::cout << "wrong 16" << " " <<get(nt_t, source(e, g)) << " " << get(nt_t, target(e, g));
                std::cout <<" "<< g[source(e, g)].idx<< " " << g[target(e, g)].idx << std::endl;
            }
            break;
        case autre :
            std::cout << "autre" << std::endl;
        }
    }
    unsigned int debut, fin;
    debut = 0;
    fin = data.pt_data.stop_areas.size();
    for(unsigned int i=debut; i<fin;++i)
        if(g[i].nt!=sa)
            std::cout << "Wrong 1 " << i << " " << g[i].nt << std::endl;


    debut = fin;
    fin = fin + data.pt_data.stop_points.size();
    for(unsigned int i=debut; i<fin;++i)
        if(g[i].nt!=sp)
            std::cout << "Wrong 2 " << i << " " << g[i].nt << std::endl;

    debut = fin;
    fin = fin + data.pt_data.route_points.size();
    for(unsigned int i=debut; i<fin;++i)
        if(g[i].nt!=rp)
            std::cout << "Wrong 3 " << i << " " << g[i].nt << std::endl;

    debut = fin;
    fin = fin + data.pt_data.stop_times.size();
    for(unsigned int i=debut; i<fin;++i)
        if(g[i].nt!=ta)
            std::cout << "Wrong 4 " << i << " " << g[i].nt << std::endl;


    debut = fin;
    fin = fin+ data.pt_data.stop_times.size();
    for(unsigned int i=debut; i<fin;++i)
        if(g[i].nt!=td)
            std::cout << "Wrong 5 " << i << " " << g[i].nt << std::endl;

    //    for(unsigned int i=0;i<data.pt_data.stop_times.size();++i) {
    //        if(get(nt_t, get_ta_idx(i, data))!=ta)
    //            std::cout << "Wrong 3 " << i << std::endl;

    //        if(get(nt_t, get_td_idx(i, data))!=td)
    //            std::cout << "Wrong " << td << " " <<get(nt_t, get_td_idx(i, data)) << std::endl;
    //        if(get(nt_t, get_tc_idx(i, data))!=tc)
    //            std::cout << "Wrong " << tc << " " << get(nt_t, get_tc_idx(i, data)) << std::endl;
    //    }


}


bool check_parcours(vertex_t v1, vertex_t v2, std::vector<vertex_t> &predecessors,  std::vector<double> distances) {
    for(vertex_t v = v2 ; v != v1 ;v = predecessors[v]) {
        if(distances[v] == (3600*24 + 1))
            return false;

    }
    return true;


}

std::list<vertex_t> in_vertices(vertex_t v, NW g) {

    boost::graph_traits<NW>::edge_iterator e, e_end;
    std::list<vertex_t> result;

    for(tie(e, e_end) = edges(g);e!=e_end;++e) {
        if(target(*e, g) == v)
            result.push_back(source(*e, g));
    }

    return result;
}

void afficher_chemin(vertex_t debut, vertex_t fin, std::vector<vertex_t>& predecessors, std::vector<etiquette> & distances, NW &g) {
    vertex_t v, pred = fin;
    bool bo;
    edge_t ed/*, edh*/;

    //    intp fin_t = get(&EdgeDesc::fin, g);
    intp debut_t = get(&EdgeDesc::fin, g);
    //    ett et_t = get(&EdgeDesc::e_type, g);
    idxp line_t = get(&EdgeDesc::idx, g);
    idxv idx_v = get(&VertexDesc::idx, g);



    for( v = fin ; v != debut  ;v = predecessors[v]) {
        //        if(pred != fin) {
        //            boost::tie(ed, bo)=edge(v, pred, g);
        //            if(get(et_t, ed) == hosp) {
        //                std::cout << "Arret : " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(get(idx_v, target(ed, g))).stop_area_idx).name << "\t"  << " " << data.pt_data.stop_points.at(get(idx_v, target(ed, g))).stop_area_idx << " ";
        //                afficher_horaire(distances[v].arrivee );
        //                std::cout << "\t Ligne : " << data.pt_data.vehicle_journeys.at(get(line_t, ed)).name;
        //                std::cout << "\t" <<  data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(get(line_t, ed)).route_idx).line_idx).name << " " <<  distances[v].temps<< std::endl;
        //            } else if(get(et_t, ed) == spho)
        //                edh = ed;
        //        } else {
        if(g[v].nt == ta || g[v].nt == td) {
            std::cout << "Arret : " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(g[v].idx).route_point_idx).stop_point_idx).stop_area_idx).name << "\t";
            std::cout << "arrivee : ";
            afficher_horaire(data.pt_data.stop_times.at(g[v].idx).arrival_time);
            std::cout << "\t Ligne : " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(g[v].idx).vehicle_journey_idx).route_idx).line_idx).name;
            std::cout << " distance : " << distances[v].arrivee << std::flush;
            std::cout << "\t" << g[v].saidx;
            std::cout << "\t" << data.pt_data.stop_times.at(g[v].idx).vehicle_journey_idx;
            std::cout << "\t" << g[v].nt;
            std::cout << "\t" << v;

            std::cout << std::endl;
        }
        //        }
        pred = v;
    }
    boost::tie(ed, bo)=edge(v, pred, g);
    if(bo) {
        std::cout << "Arret : " << data.pt_data.stop_areas.at(get(idx_v, source(ed, g))).name << "\t";
        afficher_horaire(get(debut_t, ed) + 2);
        std::cout << "\t\t Ligne : " << data.pt_data.vehicle_journeys.at(get(line_t, ed)).name;
        std::cout << "\t" <<  data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(get(line_t, ed)).route_idx).line_idx).name<< " " <<  distances[v].temps << std::endl;
    }

}


void afficher_chemin(vertex_t2 debut, vertex_t2 fin, std::vector<vertex_t2>& predecessors, std::vector<etiquette2> & distances, NW2 &g) {
    vertex_t2 v, pred = fin;
    bool bo;
    edge_t2 ed/*, edh*/;



    for( v = fin ; v != debut  ;v = predecessors[v]) {
        if(get_n_type(v, data) == TA || get_n_type(v, data) == TD) {
            std::cout << "Arret : " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data)).route_point_idx).stop_point_idx).stop_area_idx).name << "\t";
            std::cout << "arrivee : ";
            afficher_horaire(data.pt_data.stop_times.at(g[v].idx).arrival_time);
            std::cout << "\t Ligne : " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(get_idx(v, data)).vehicle_journey_idx).route_idx).line_idx).name;
            std::cout << " distance : " << distances[v].temps << std::flush;
            std::cout << " validity pattern : " << data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(get_idx(v, data)).vehicle_journey_idx).validity_pattern_idx).id << std::flush;
            std::cout << std::endl;
        }
        //        }
        pred = v;
    }
    /*boost::tie(ed, bo)=edge(v, pred, g);
    if(bo) {
        std::cout << "Arret : " << data.pt_data.stop_areas.at(get(idx_v, source(ed, g))).name << "\t";
        afficher_horaire(get(debut_t, ed) + 2);
        std::cout << "\t\t Ligne : " << data.pt_data.vehicle_journeys.at(get(line_t, ed)).name;
        std::cout << "\t" <<  data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(get(line_t, ed)).route_idx).line_idx).name<< " " <<  distances[v].temps << std::endl;
    }*/

}

void init_distances_predecessors_map(etiquette debut, std::vector<etiquette> &distances, std::vector<vertex_t> &predecessors,std::vector<vertex_t> &indexes) {
    for(unsigned int i=0; i<distances.size();++i) {
        distances[i] = debut;
        predecessors[i] = i;
        indexes[i] = i;
    }

    std::cout << "Init size : " << distances.size() << " " << predecessors.size() << std::endl;

}


void debug_prochains_horaires(NW &g, vertex_t v, unsigned int heure) {
    vertex_t sp = data.pt_data.route_points.at(data.pt_data.stop_times.at(g[v].idx).route_point_idx).stop_point_idx;
    vertex_t h = v;
    BOOST_FOREACH(edge_t e1, out_edges(v, g)) {
        if(g[e1].e_type == tatd) {
            h = target(e1, g);
        }
    }
    bool bo = true;
    std::cout << " SP : " << sp << "type : " << g[h].nt <<  std::endl;
    while(bo) {
        bo = false;
        BOOST_FOREACH(edge_t e1, out_edges(h, g)) {
            if(g[e1].e_type == tdtd) {
                bo = data.pt_data.stop_times.at(g[h].idx).departure_time < (heure +600);
                if(bo) {
                    h = target(e1, g);
                    std::cout  << "Idx : " << g[h].idx << "Ligne : " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(g[h].idx).vehicle_journey_idx).route_idx).line_idx).name << std::endl ;
                }
            } else {
                std::cout << "ELSE Type : " << g[e1].e_type << " idx : " << g[h].idx;
            }
        }
    }
}





