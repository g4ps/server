#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>

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

using namespace std;

class http_server
{
private:
  vector<int> sock_fds;
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
  void add_socket(string addr, short port);
  void add_socket(sockaddr_in*);
  void start();
  void serve(int fd);
  size_t num_of_sockets() const;
  #ifdef NOT_SHIT
  //Because god hates french people
  void add_socket_from_hostname(string host, short port);
  void process_request(http_message& msg);
  void process_get_request(http_message &msg);
  void process_not_found(http_message &req);
  void send_status_code(http_message&, int code);
  #endif
};

string test_page();

#endif
