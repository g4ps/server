#include <string>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "parse_help.hpp"

using namespace std;

void serv_log(std::string out)
{
  std::cout<<out<<std::endl;
}

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

http_message::http_message()
{
}

http_message::http_message(string s)
{
  parse_raw_data(s);
}
				      

/*
  needed for quick initialization of the class by easy stuff
 */
void http_message::parse_raw_data(string inp)
{
  //CRLF as string for simple usage
  string crlf = "\r\n";

  //getting start_line
  //start line BNF: request-line / status-line
  if (inp.empty()) {
    serv_log("Empty input");
    throw invalid_state();
  }
  raw = inp;
  size_t pos = 0;
  size_t tpos = inp.find(crlf);
  if (tpos == string::npos) {
    serv_log("Invalid START LINE in header: expected CRLF, got \"" + inp + "\"");
    throw invalid_state();
  }
  //TODO: add checher for correct request-line syntax
  tpos += crlf.length();
  start_line = inp.substr(pos, tpos);
  inp.erase(pos, tpos - pos);

  //getting all the header-fields
  //header-field BNF is field-name ":" OWS field-value OWS
  while (inp.compare(0, crlf.length(), crlf) != 0) {
    pos = 0;
    tpos = pos;
    //field-name is token, so
    string field_name = get_token(inp);

    //checking for sanity
    if (inp.empty()) {
      serv_log("Invalid header-field: eof when expected \":\" OWS field-value");
    }

    //checking ":" for existence and acting accordingly
    if (inp[0] != ':') {
      serv_log("Invalid header-field: expected ':', got " + inp);
      throw invalid_state();
    }
    inp.erase(0, 1);
    skip_ows(inp);

    //getting field value
    string field_value = get_field_value(inp);

    if (inp.empty()) {
      serv_log("Invalid header-field: expected CRLF, got EOF");
      throw invalid_state();
    }

    if (inp.compare(0, crlf.length(), crlf) != 0)  {
      serv_log("Invalid header-field: expected CRLF, got " + inp);
      throw invalid_state();
    }
    inp.erase(0, 2);
    str_to_lower(field_name);
    str_to_lower(field_value);
    header_lines.insert(make_pair(field_name, field_value));
  }
}

/*
  function: get_token;

  returns: token;

  comment: if token is not present, then 
 */

string http_message::get_token(string &str) const
{
  size_t pos = 0;
  while (is_tchar(str[pos]) && pos < str.length())
    pos++;
  if (pos == 0) {
    serv_log("Expected token, got " + str);
    throw invalid_state();
  }
  string ret = str.substr(0, pos);
  str.erase(0, pos);
  return ret;
}

string http_message::get_field_value(string &str) const
{
  string ret;
  while (is_obs_fold(str) || is_field_content(str)) {
    if (is_obs_fold(str))
      ret += get_obs_fold(str);
    else
      ret += get_field_content(str);
  }
  return ret;
}

string http_message::get_field_content(string &str) const
{
  string ret;
  size_t t_pos = 0;

  if (str.length() < 1 || !is_field_char(str[t_pos])) {
    serv_log("Something just got absolutely fucked up with get_field_content");
    throw invalid_state();
  }
  t_pos++;
  if (str[t_pos] == ' ' || str[t_pos] == '\t') {
    while (str[t_pos] == ' ' || str[t_pos] == '\t')
      t_pos++;
    if (!is_field_char(str[t_pos])) {
      serv_log("Something just got absolutely fucked up with get_field_content");
      throw invalid_state();
    }
    t_pos++;
  }
  ret = str.substr(0, t_pos);
  str.erase(0, t_pos);
  return ret;
}

string http_message::get_obs_fold(string &s) const
{
  if (is_obs_fold(s)) {
    serv_log("Something just got absolutely fucked up with obs shit");
    throw invalid_state();
  }
  size_t t_pos = 2;
  while (s[t_pos] == ' ' || s[t_pos] == '\t')
    t_pos++;
  string ret = s.substr(0, t_pos);
  s.erase(0, t_pos);
  return ret;
}

void http_message::print() const
{
  cout<<start_line;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    cout << i->first <<": " << i->second<< "\r\n";
  }
  cout<<"\r\n";
}




/*
  function: get_address_from_hostname returns sockaddr_in (ipv4) for 
  hostname, supplied in the host argument

  returns: NULL in case of an error and free-able sockaddr_info if
  successful

  comment: I know, that we have gataddrinfo for the sole purpose
  of getting adderess from hostname, in this function i just
  add all the neccesary hints, bells and whistles in it;
 */
struct sockaddr_in*
get_address_from_hostname(const char *host, unsigned short port)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_INET;
  struct addrinfo *res = NULL;
  struct addrinfo *i = NULL;
  struct addrinfo *tmp = NULL;
  int err = getaddrinfo(host, NULL, &hints, &res);
  if (err != 0) {
    serv_log(gai_strerror(err));
    return NULL;
  }
  struct sockaddr_in *p = NULL;
  for (i = res; i != NULL; i++) {
    if (i->ai_family == AF_INET) {
      p = (sockaddr_in *)res->ai_addr;
      break;
    }
  }
  if (p == NULL) {
    return NULL;
  }
  struct sockaddr_in* ret = new sockaddr_in;
  memcpy(ret, p, sizeof(sockaddr_in));
  ret->sin_port = htons(port);
  freeaddrinfo(res);
  return ret;
  //С++ maybe exceptions
}

/*
  function: init_serv

  returns: fd for socket or -1

  comment: returns socket fd with all the neccesary options
  and already listening
 */



int init_serv(struct sockaddr_in *addr)
{
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    serv_log("ERROR: socket shat the bed");
    serv_log(strerror(errno));
    return -1;
  }
  int reuse = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
    serv_log("ERROR: setsockopt fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  if (bind(sock_fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) == -1) {
    serv_log("ERROR: bind fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  if (listen(sock_fd, 10) == -1) {
    serv_log("ERROR: listen fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  serv_log("Success");
  return sock_fd;
  //C++ exceptions
}

std::string
init_page()
{
  string head =
    string("HTTP/1.1 200 OK\r\n") +
    "Server: nginx/1.20.0\r\n" +
    "Date: Thu, 17 Jun 2021 20:30:59 GMT\r\n" +
    "Content-Type: text/html\r\n" +
    //"Transfer-Encoding: chunked\r\n" +
    "Connection: keep-alive\r\n";

  string body = string("<html>\r\n") +
    "<head><title>My very own html resopnce</title></head>\r\n" +
    "<body>\r\n" +
    "<h1>  YEAH BABY</h1>\r\n" +
    "<hr></body>\r\n" +
    "</html>\r\n";
  stringstream ss;
  ss << "Content-Length: " << body.length() << "\r\n\r\n";
  head += ss.str();
  return head + body;
}

int
main(int argc, char **argv)
{
  unsigned short port = 8001;
  std::string host = "localhost";
  if (argc == 2) {
    host = argv[1];
  }
  struct sockaddr_in* my_addr;
  my_addr = get_address_from_hostname(host.c_str(), port);
  if (my_addr == NULL) {
    printf("No such address\n");
    exit(0);
  }
  char str[500];
  stringstream s;
  s << "Initializing server on address " << inet_ntop(AF_INET, &my_addr->sin_addr, str, INET_ADDRSTRLEN)
       << " (name: \"" << host << "\" port: " <<  port << ")";
  serv_log(s.str());  
  int sock_fd = init_serv(my_addr);
  if (sock_fd < 0)
    return 1;
  //  fcntl(sock_fd, F_SETFL, O_NONBLOCK);
  int new_sock;
  new_sock = accept(sock_fd, NULL, NULL);
  string r = init_page();
  char buf[512];
  bzero(buf, 512);
  int n;
  while ((n = read(new_sock, buf, 512)) != 0) {
    //    write(1, buf, n);
    break;
  }
  try {
    http_message test;
    test.parse_raw_data(buf);
    test.print();
  }
  catch(exception& e) {
    cout<<e.what()<<endl;
  }
  write(new_sock, r.c_str(), r.length());
  close(new_sock);
  delete my_addr;
  return 0;
}
