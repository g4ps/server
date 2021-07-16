#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <cstdio>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <unistd.h>

using namespace std;

class http_server;

class http_message
{
protected:
  http_server *serv;
  vector<char> raw;
  string start_line;
  multimap<string, string> header_lines;
  int sock_fd;
  vector<char> body;
  
  //protected methods
  string get_token(string &str) const;
  string get_field_value(string &str) const;
  string get_obs_fold(string &s) const; 
  string get_field_content(string &s) const;
  void parse_header_fields(string &inp);
  ssize_t read_block(size_t size = BUFSIZ, int fd = -1);
  ssize_t read_nb_block(size_t size = BUFSIZ, int fd = -1);
  size_t msg_body_position() const;
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
  class req_timeout: public exception {
  public:    
    const char* what() const throw()
    {
      return "Request holded for too long";
    }
  };
  virtual void print() const;
  ///////////////////////////////////////////////////
  // Supposed to be in different class  
  ///////////////////////////////////////////////////

  //////////////////////////////////////////////////
  void set_socket(int fd);
  int get_socket();
  void set_start_line(string l);
  virtual string& get_start_line();
  void add_header_field(string name, string val);
  string compose_header_fields();
  //maybe i should rename it
  pair<bool, string> get_header_value(string name) const;
  size_t get_body_size() const;
  void print_body_into_fd(int fd);
  void set_server(http_server *s);
  list<string> get_cgi_header_values();
};

#endif
