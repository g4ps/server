#ifndef HTTP_LOCATION_HPP
#define HTTP_LOCATION_HPP

#include <string>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <list>
#include <deque>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>

#include "parse_help.hpp"
#include "http_message.hpp"
#include "http_request.hpp"
#include "http_utils.hpp"

using namespace std;

class http_location
{
  list<string> methods;
  map<int, string> redirection;
  string path;
  string root;
  bool auto_index;
  map<int, string> error_pages;
  list<string> index;
  // list<http_cgi> cgi;
private:
  bool is_path(string s) const;
public:
  class invalid_state: public exception
  {
    const char* what() const throw()
    {
      return "Invalid state";
    }
  };
  class not_found: public exception
  {
    const char* what() const throw()
    {
      return "File was not found";
    }
  };
  http_location();
  http_location(string s);
  void add_method(string s);
  bool is_method_accepted(string s);
  void add_redirect(int status, string dir);
  void set_path(string s);
  string get_path() const;
  void set_root(string s);
  string get_root() const;
  void set_autoindex();
  void unset_autoindex();
  bool get_autoindex() const;
  void add_error_page(int status, string path);
  void add_index(string name);
  string get_file_name(string s);
  bool is_located(string target) const;
  size_t get_path_depth() const;
  string get_index_page(string s);
};

#endif
