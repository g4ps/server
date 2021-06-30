#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <cstdio>

#include "http_message.hpp"

using namespace std;

class http_request: public http_message
{
protected:
  string method;
  string request_target;
  string http_version;
  bool req_is_complete;
  ssize_t read_block(size_t size = BUFSIZ);
  size_t msg_body_position() const;
public:
  http_request();
  http_request(int fd);
  void parse_head();
  class invalid_head: public exception {
  public:    
    const char* what() const throw()
    {
      return "Invalid HTTP header in request";
    }
  };
  class process_error: public exception {
    const char* what() const throw()
    {
      return "Something went terribly wrong";
    }
  };
  class invalid_function_call: public exception {
    const char* what() const throw()
    {
      return "Function, which requires prior initialization"
	" was called without said initialization";
    }
  };
  void get_header();
  bool is_fd_valid() const;
  void recieve();
  bool is_complete() const;
  void get_msg_body();
  void print() const;
  string get_method() const;
  string get_request_target() const;
  string get_http_version() const;
};

#endif
