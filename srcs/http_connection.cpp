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

#include "parse_help.hpp"
#include "http_message.hpp"
#include "http_responce.hpp"
#include "http_request.hpp"
#include "http_server.hpp"
#include "http_utils.hpp"
#include "http_connection.hpp"

using namespace std;

http_connection::http_connection()
{
}

http_connection::http_connection(int in, sockaddr_in a)
{
  set_fd(in);
  set_addr(a);
}

http_connection::~http_connection()
{
}

void http_connection::set_fd(int in)
{
  fd = in;
}

int http_connection::get_fd() const
{
  return fd;
}

void http_connection::set_addr(sockaddr_in a)
{
  addr = a;
}

sockaddr_in http_connection::get_addr() const
{
  return addr;
}
