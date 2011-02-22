#include "http.h"
#include <boost/algorithm/string/replace.hpp>

using boost::asio::ip::tcp;


/* Base commune entre GET et POST
 * Ne devrait pas être appelé directement
 */
std::string send_http(const std::string & server, const std::string & req, const std::string & port, const std::string & method, const std::string & data)
{
    if(method != "POST" && method != "GET")
        throw http_error(500, "Méthode invalide");

    boost::asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(server, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);   
    tcp::resolver::iterator end;
    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;

    while (error && endpoint_iterator != end)
    {
        socket.close();
        socket.connect(*endpoint_iterator++, error);
    }
    if (error)
    {
        throw http_error(error.value(), "Erreur boost : " + error.message());
    }

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << method << " " << req << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n";

    if(method == "POST") {
        request_stream << "Content-Type: application/x-www-form-urlencoded\r\n";
        request_stream << "Content-Length: " << data.size() << "\r\n\r\n";
        boost::asio::write(socket, request);
        request_stream << data;
    }
    else
        request_stream << "\r\n";


    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
        throw http_error(500, "La réponse ne semble pas être du http valable");
    }


    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r")
    {
    }

    std::stringstream content;
    // Write whatever content we already have to output.
    if (response.size() > 0)
        content << &response;

    // Read until EOF, writing data to output as we go.
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
    {
        content << &response;
    }

    if (error != boost::asio::error::eof)
        throw http_error(error.value(), "Erreur boost : " + error.message());

    if(status_code != 200){
        throw http_error(status_code, content.str());
    }

    return content.str();
}

std::string get_http(const std::string & server, const std::string & request, const std::string & port)
{
	return send_http(server, boost::replace_all_copy(request, " ", "%20"), port, "GET", "");
}

std::string post_http(const std::string & server, const std::string & request, const std::string & data, const std::string & port)
{
    return send_http(server, request, port, "POST", data);
}

