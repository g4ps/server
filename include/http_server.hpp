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
#include "http_location.hpp"
#include "http_utils.hpp"

using namespace std;

class http_server
{
private:
  string name;
  deque<int> sock_fds;
  list<http_location> locations;
  deque<int> active_fds;
private:
  //private methods
  void check_location_for_correctness(http_location);
public:
  class init_error: public exception {
    const char* what() const throw()
    {
      return "Initialization error";
    }
  };
  class internal_error: public exception {
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
  class invalid_target: public exception {
    const char* what() const throw()
    {
      return "Cannot process target";
    }
  };
  class method_not_allowed: public exception {
    const char* what() const throw()
    {
      return "Requested method is not allowed in this location";
    }
  };
  void add_socket(string addr, unsigned short port);
  void add_socket(sockaddr_in*);
  //  void start();
  int serve(int fd, sockaddr_in addr);
  size_t num_of_sockets() const;
  void add_socket_from_hostname(string host, unsigned short port);
  void process_request(http_request& msg, sockaddr_in addr);
  void process_get_request(http_request &msg, sockaddr_in addr);
  void process_post_request(http_request &msg);

  //Should be removed
  // void process_not_found(http_request &req);
  // void send_status_code(http_request&, int code);
  // void send_timeout(int fd);
  bool has_socket(int fd);
  deque<int> get_sockets() const;
  string get_header_string(int fd);
  void process_error(http_request &fd, int status);
  void process_error(int fd, int status);
  string get_error_target_name(string target);
  string get_default_err_page(int status);
  bool is_cgi_request(string target_name);
  void process_cgi(http_request &req, sockaddr_in addr);
  http_location& get_location_from_target(string s);
  void add_location(http_location in);
  void process_redirect(http_request &in, int status, string target);
  const char** compose_cgi_envp(http_request& req, sockaddr_in addr);
  bool is_active_connection(int fd) const;
  void add_active_connection(int fd);
  void remove_active_connection(int fd);
};

string test_page();

#endif
