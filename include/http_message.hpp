#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <unistd.h>

using namespace std;

void serv_log(string s);
string convert_to_string(int);
string convert_to_string(size_t);
string convert_to_string(short);
string convert_to_string(off_t);

class http_message
{
protected:
  string raw;
  string start_line;
  multimap<string, string> header_lines;
  int sock_fd;
  vector<char> body;
  
  //protected methods
  string get_token(string &str) const;
  string get_field_value(string &str) const;
  string get_obs_fold(string &s) const; 
  string get_field_content(string &s) const;  
public:
  http_message();
  //http_message(string s, int inp_fd = -1);
  //  void parse_raw_head(string s, int inp_fd = -1);
  class invalid_state: public exception {
  public:    
    const char* what() const throw()
    {
      return "Invalid HTTP message";
    }
  };
  void print() const;
  ///////////////////////////////////////////////////
  // Supposed to be in different class  
  ///////////////////////////////////////////////////
  string get_method() const;
  string get_request_target() const;
  string get_http_version() const;
  //////////////////////////////////////////////////
  void set_socket(int fd);
  int get_socket();
  void set_start_line(string l);
  virtual string& get_start_line();
  void add_header_field(string name, string val);
  string compose_header_fields();
  //maybe i should rename it
  pair<bool, string> get_header_value(string name);
};

#endif
