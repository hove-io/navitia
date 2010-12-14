#include "gateway.h"
#include "http.h"
#include "threadpool.h"
#include "boost/lexical_cast.hpp"

Navitia::Navitia(const std::string & server, const std::string & path) : server(server), path(path), error_count(0){
}

std::string Navitia::query(const std::string & request){
    return get_http(server, path + request);
}

void NavitiaPool::add(const Navitia & n){
    navitias.push_back(n);
    next_navitia = navitias.begin();
}

/// Classe associée à chaque thread
struct Worker {
    /// Fonction appelée à chaque requête
    webservice::ResponseData operator()(const webservice::RequestData & data, NavitiaPool & pool){
        webservice::ResponseData resp;
        resp.status_code = 200;
        resp.content_type = "text/xml";
        if(data.path == "/status"){
            resp.response = "<info>Passerelle en FastCGI</info>\n";
        }
        else if(data.path == "/api" || data.path == "/const") {
            try{
                resp.response = pool.query( data.path + "?" + data.params );
            }
            catch(http_error e){
                resp.response = "<error>"
                    "<http code=\"";
                resp.response += boost::lexical_cast<std::string>(e.code)+ "\">";
                    resp.response += e.message;
                    resp.response += "</http>"
                    "</error>";
            }
        }
        else {
            resp.status_code = 400;
            resp.response = "<error>API inconnue</error>";
        }

        return resp;
    }
};

NavitiaPool::NavitiaPool() : nb_threads(16) {
    add(Navitia("10.2.0.16","/navitia/vfe/cgi-bin/navitia_GU.dll")); 
    add(Navitia("10.2.0.16","/navitia/vfe1/cgi-bin/navitia_GU.dll"));
    add(Navitia("10.2.0.16","/navitia/vfe2/cgi-bin/navitia_GU.dll"));
    add(Navitia("10.2.0.16","/navitia/vfe3/cgi-bin/navitia_GU.dll"));
    add(Navitia("10.2.0.16","/navitia/vfe4/cgi-bin/navitia_GU.dll"));
    add(Navitia("10.2.0.16","/navitia/vfe5/cgi-bin/navitia_GU.dll"));
}

std::string NavitiaPool::query(const std::string & query){
    iter_mutex.lock();
    next_navitia++;
    if(next_navitia == navitias.end())
        next_navitia = navitias.begin();
    if(next_navitia == navitias.end())
        return "Aucune instance de NAViTiA présente";
    iter_mutex.unlock();

    return next_navitia->query(query);
}

/*void NavitiaPool::const_stats(std::string &data)
{
    rapidxml::xml_document<> doc;    // character type defaults to char
    char * data_ptr = doc.allocate_string(data.c_str());
    doc.parse<0>(data_ptr);    // 0 means default parse flags

    rapidxml::xml_node<> *node = doc.first_node("const");
    if(node)
    {
        node = node->first_node("Thread");
        if(node)
        {
            node = node->first_node("ActiveThread");
            if (node)
            {
                const_calls++;
                nb_threads_sum += atoi(node->value());
                if(const_calls % 100 == 0){
                    std::cout << "Nombre d'appels : " << const_calls << " moyenne de threads actifs : " << (nb_threads_sum/const_calls) << std::endl;
                }
            }
        }
    }
}*/


MAKE_WEBSERVICE(NavitiaPool, Worker)
