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
  void write_responce();
  void set_body(string s);  
};




#endif
