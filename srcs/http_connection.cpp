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
  init_time = time(NULL);
  temp_request = NULL;
  temp_responce = NULL;
  left_to_process = 0;
}

http_connection::http_connection(int in, sockaddr_in a)
{
  init_time = time(NULL);
  temp_request = NULL;
  temp_responce = NULL;
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

void http_connection::expand_buffer()
{
  char *buf = new char[BUFSIZ];
  int ret = read(fd, buf, BUFSIZ);
  if (ret < 0) {
    //TODO process error
    return ;
  }
  buffer.insert(buffer.begin(), buf, buf + ret);
}

bool http_connection::has_header() const
{
  string head_end = "\r\n\r\n";
  return search(buffer.begin(), buffer.end(),
		head_end.begin(), head_end.end()) != buffer.end();
}

void http_connection::form_request_head()
{
  string head_end = "\r\n\r\n";
  if (temp_request != NULL)
    delete temp_request;
  temp_request = new http_request;
  vector<char> b;
  vector<char>::iterator it;
  it = search(buffer.begin(), buffer.end(),
	      head_end.begin(), head_end.end());
  it = it + head_end.length();
  b.insert(b.begin(), buffer.begin(), it);
  temp_request->set_raw(b);
  buffer.erase(buffer.begin(), it);
  temp_request->parse_head();
  if (temp_request->get_header_value("Content-Length").first) {
    string sleft = temp_request->get_header_value("Content-length").second;
    left_to_process = atoi(sleft.c_str()) - buffer.size();
    if (left_to_process == 0) {
      temp_request->append_raw(buffer);
      temp_request->append_body(buffer);
    }
  }
  else if (temp_request->get_header_value("Transfer-Encoding").first) {
    left_to_process = -1;
  }
}

void http_connection::form_request_msg()
{
  char buf[BUFSIZ];
  ssize_t n;
  if (left_to_process != 0) {
    if ((n = read(fd, buf, BUFSIZ)) < 0) {
      //TODo Process error
      return ;
    }
    buffer.insert(buffer.end(), buf, buf + n);
  }
  if (left_to_process > 0) {
    if (n > left_to_process) {
      // TODO error
      return;
    }
    left_to_process -= n;
    if (left_to_process == 0) {
      temp_request->append_raw(buffer);
      temp_request->append_body(buffer);
    }
  }
  else if (left_to_process < 0) {
    string crlf = "\r\n";
    // ssize_t next_chunk_position = msg_body_position();
    while (search(buffer.begin(), buffer.end(),
	       crlf.begin(), crlf.end()) != buffer.end()) {
      //getting position of end of the chunk size length
      ssize_t chunk_head_end = search(buffer.begin(), buffer.end(),
				      crlf.begin(), crlf.end()) - buffer.begin();
      string chunk_head;
      chunk_head.insert(chunk_head.begin(), buffer.begin(),
			buffer.begin() + chunk_head_end);
      if (chunk_head.length() == 0) {
	//TODO error
	return ;
      }
      string str_chunk_size = chunk_head.substr(0, chunk_head.find(";"));
      if (!is_hex_string(str_chunk_size)) {
	return;
	//TODO error
      }
      long chunk_size = strtol(str_chunk_size.c_str(), NULL, 16);
      if (chunk_size + chunk_head_end + 2 < buffer.size())
	return ;
      if (chunk_size == 0) {
	temp_request->append_raw(buffer);
	left_to_process = 0;
	return ;
      }
      //getting the whole chunk
      //2 is the size of crlf
      // while (raw.size() < next_chunk_position + chunk_head_end + 2 + chunk_size)
      //   read_block();
      vector<char> body;
      body.insert(body.begin(), buffer.begin() + chunk_head_end + 2,
		  buffer.begin() + chunk_head_end + 2 + chunk_size);
      temp_request->append_raw(buffer);
      temp_request->append_body(body);
      buffer.erase(body.begin(), body.begin() + chunk_head_end + 2 + chunk_size);
    }
  }
}

http_request http_connection::get_request()
{
  return *temp_request;
}
