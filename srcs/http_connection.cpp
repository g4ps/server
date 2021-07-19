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

http_connection::http_connection(int in)
{
  set_fd(in);
}

void http_connection::set_fd(int in)
{
  fd = in;
}

int http_connection::get_fd() const
{
  return fd;
}

// void http_connection::add_buffer()
// {
//   char *buf = new char[BUFSIZ];
//   int ret = read(fd, buf, BUFSIZ);
//   if (ret < 0) {
//     serv_log(string("read error on socket ") + convert_to_string(fd));
//     throw read_error();
//   }
//   if (ret == 0)
//     return ;
//   read_buffer.insert(read_buffer.end(), buf, buf + ret);  
// }

// bool http_connection::has_full_header() const
// {
//   if (
// }

// void http_connection::process_header
