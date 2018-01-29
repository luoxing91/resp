#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include "redisProtocol.hpp"
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int main(int argc, char** argv){
    try{
        // Check command line arguments.
        if(argc != 3){
            std::cerr << "Usage: http-client-sync <host> <port> <target> "
                      <<"[<HTTP version: 1.0 or"
                      <<"1.1(default)>]\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 /\n" <<
                "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];

        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        tcp::socket socket{ioc};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(socket, results.begin(), results.end());

        // Set up an HTTP GET request message
        //redis::redis_body req{redis::type::simple};
        
        // Send the HTTP request to the remote host
        // redis::write(socket, req);
        // redis::write(socket, req);


        // // Declare a container to hold the response
        // redis::redis_body res;

        // // Receive the HTTP response
        // redis::read(socket, res);

        // Write the message to standard out
        //std::cout << res << std::endl;

        // Gracefully close the socket
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ec};

        // If we get here then the connection is closed gracefully
    }catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
