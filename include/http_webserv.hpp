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
	class invalid_config: public exception {
		const char* what() const throw()
		{
			return "Error: invalid config file";
		}
	};
	void parse_config(string filename);
  http_server& find_server_with_socket(int fd);
};

void trim_wsp(string &s);
#endif
