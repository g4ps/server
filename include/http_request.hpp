#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>

#include "http_message.hpp"

using namespace std;

class http_request: public http_message
{
protected:
  string method;
  string request_target;
  string http_version;
  bool req_is_complete;
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
  class invalid_method: public exception {
  public:    
    const char* what() const throw()
    {
      return "Incorrect method used";
    }
  };
  void get_header();
  bool is_fd_valid() const;
  void recieve();
  bool is_complete() const;
  void get_msg_body();
};

#endif
