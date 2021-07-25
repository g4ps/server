#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <string>
#include <list>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <cstring>
#include <sys/wait.h>
#include <algorithm>

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
#include <ctime>

#include "parse_help.hpp"
#include "http_message.hpp"
#include "http_responce.hpp"
#include "http_request.hpp"
#include "http_server.hpp"
#include "http_utils.hpp"

using namespace std;

class http_connection
{
  time_t init_time;
  int fd;
  sockaddr_in addr;
  vector<char> buffer;
  bool kind;
  ssize_t left_to_process;
  http_request *temp_request;
  http_responce *temp_responce;
public:
  http_connection();
  http_connection(int fd, sockaddr_in addr);
  ~http_connection();
  void set_fd(int fd);
  int get_fd() const;
  void set_addr(sockaddr_in a);
  sockaddr_in get_addr() const;
  void expand_buffer();
  bool has_header() const;
  void form_request_head();
  void form_request_msg();
  http_request get_request();
};


#endif
