#ifndef HTTP_WEBSERV_HPP
#define HTTP_WEBSERV_HPP

#include <list>
#include <map>
#include <string>
#include "http_server.hpp"

using namespace std;

class http_webserv
{
  list<http_server> servers;
public:
  http_webserv();  
  void start();
  void add_server(http_server s);
  class process_error: public exception {
    const char* what() const throw()
    {
      return "Something went terribly wrong";
    }
  };
  http_server& find_server_with_socket(int fd);
};

#endif
