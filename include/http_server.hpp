#ifndef SERVER_HPP
#define SERVER_HPP

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

using namespace std;


class http_server
{
private:
  deque<int> sock_fds;
  map<string, string> locations;
  string redirect;
  bool auto_index;
  map<int, string> error_pages;
  list<string> default_pages;
private:
  //private methods  
public:
  class init_error: public exception {
    const char* what() const throw()
    {
      return "Initialization error";
    }
  };
  class process_error: public exception {
    const char* what() const throw()
    {
      return "Something went terribly wrong";
    }
  };
  class header_error: public exception {
    const char* what() const throw()
    {
      return "Bad header input";
    }
  };
  void add_socket(string addr, short port);
  void add_socket(sockaddr_in*);
  //  void start();
  void serve(int fd);
  size_t num_of_sockets() const;
#ifdef NOT_SHIT
  //Because god hates french people
  void add_socket_from_hostname(string host, short port);
#endif
  void process_request(http_request& msg);
  void process_get_request(http_request &msg);
  void process_post_request(http_request &msg);
  void process_not_found(http_request &req);
  void send_status_code(http_request&, int code);
  void send_timeout(int fd);
  bool has_socket(int fd);
  deque<int>& get_sockets();
  string get_header_string(int fd);
  void process_error(http_request &fd, int status);
  void process_error(int fd, int status);
  string get_error_target_name(string target);
  string get_default_err_page(int status);
  bool is_cgi_request(string target_name);
  void process_cgi(http_request &req);
};

string test_page();

#endif
