#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>

using namespace std;

void serv_log(string s);

class http_message
{
  string raw;
  string start_line;
  multimap<string, string> header_lines;
  vector<char> body;
private:
  //private methods
  string get_token(string &str) const;
  string get_field_value(string &str) const;
  string get_obs_fold(string &s) const; 
  string get_field_content(string &s) const;  
public:
  http_message();
  http_message(string s);
  void parse_raw_data(string s);
  class invalid_state: public exception {
  public:    
    const char* what() const throw()
    {
      return "Invalid  HTTP message";
    }
  };
  void print() const;
};

#endif
