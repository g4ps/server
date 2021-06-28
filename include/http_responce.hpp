#ifndef HTTP_RESPONCE_HPP
#define HTTP_RESPONCE_HPP

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>

#include "http_message.hpp"

using namespace std;

class http_responce: public http_message
{
  string http_version;
  int status_code;
  string reason_phrase;
  string target_name;
private:
  void write_head();
public:
  http_responce();
  http_responce(int st);
  class bad_status: public exception
  {
    const char* what() const throw()
    {
      return "Bad responce code";
    }
  };
  class target_not_found: public exception
  {
    const char* what() const throw()
    {
      return "Couldn't find target";
    }
  };
  class target_denied: public exception
  {
    const char* what() const throw()
    {
      return "Target coudn't be accessed";
    }
  };
  class server_error: public exception
  {
    const char* what() const throw()
    {
      return "Internal server error";
    }
  };
  void write_responce();
  void set_body(string s);
  void set_target_name(string s);
  string get_target_name() const;
};




#endif
