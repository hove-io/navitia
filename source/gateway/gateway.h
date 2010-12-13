#pragma once

#include <string>
#include <vector>
#include <boost/thread.hpp>

/** Contient toutes les informations relatives aux instances NAViTiA
 * pilotées par la passerelle
 */
class Navitia{
private:
    /// Serveur utilisé
    std::string server;

    /// Chemin vers dll/fcgi que l'on désire interroger
    std::string path;

    /// Nombre d'erreurs rencontrées
    int error_count;

public:
    /// Constructeur : on passe le serveur, et le chemin vers dll/fcgi que l'on désire utiliser
    Navitia(const std::string & server, const std::string & path);

    /// On interroge dll/fcgi avec la requête passée en paramètre
    std::string query(const std::string & request);
};

/** Contient un ensemble de NAViTiA qui sont interrogeables */
class NavitiaPool {
private:
    /// Iterateur vers le prochain NAViTiA à interroger
    std::vector<Navitia>::iterator next_navitia;

    /// Structure contenant l'ensemble des navitias
    std::vector<Navitia> navitias;

    /// Mutex pour proteger l'itérateur indiquant le prochain NAViTiA à utiliser
    boost::mutex iter_mutex;

public:
    /// Nombre de threads
    int nb_threads;
    /// Constructeur par défaut
    NavitiaPool();
    /// Rajoute un nouveau navitia au Pool. Celui-ci sera copié
    void add(const Navitia & n);

    /// Choisit un NAViTiA et lui fait executer la requête
    std::string query(const std::string & query);
};

