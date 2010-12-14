/** Fonctions pour effectuer des requêtes HTTP
 *
 * Pour avoir un contrôle bas niveau, nous ré-implémentons le protocole en nous basant sur boost::asio
 */

#include <iostream>
#include <boost/asio.hpp>

#ifndef HTTP_H
#define HTTP_H

/// Exception levée en cas d'erreur
struct http_error{
    int code; ///< Code d'erreur (par exemple 400)
    std::string message; ///< Message d'erreur
    /// Constructeur simple
    http_error(int code, const std::string & message) : code(code), message(message) {}
};

/** Effectue une requête GET */
std::string get_http(const std::string & server, const std::string & request, const std::string & port = "80");

/** Effectue une requête POST */
std::string post_http(const std::string & server, const std::string & request, const std::string & data, const std::string & port = "80");
#endif
