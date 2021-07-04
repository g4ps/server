#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>

#include "http_message.hpp"
#include "http_request.hpp"
#include "http_utils.hpp"
#include "parse_help.hpp"

using namespace std;

http_request::http_request()
{
}

http_request::http_request(int fd)
{
  sock_fd = fd;
  recieve();
}


void http_request::recieve()
{
  if (!is_fd_valid())
    throw invalid_function_call();
  get_header();
  parse_head();
  if (get_header_value("Content-Length").first ||
      get_header_value("Transfer-Encoding").first) {
    serv_log("Requset has a body; Parsing it...");
    get_msg_body();
  }
  // else if (raw.find("\r\n\r\n") + 2 != raw.length()) {
  //   //error here
  // }
}

void http_request::print() const
{
  cout<<"Method: \'" << get_method() << "\'" <<  endl;
  cout<<"Request target: \'" << get_request_target() << "\'" << endl;
  cout<<"HTTP version: \'" << get_http_version() << "\'" << endl;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    cout << i->first <<": " << i->second<< "\r\n";
  }
  cout<<"\r\n";
  string msg_body(body.begin(), body.end());
  if (msg_body.length() != 0) 
    cout<<msg_body<<std::endl;
}

string http_request::get_method() const 
{
  size_t pos;
  pos = start_line.find(' ');
  if (pos == string::npos) {
    serv_log("Invalid call to get_method on non-request");
    throw invalid_state();
  }
  return start_line.substr(0, pos);
}

string http_request::get_request_target() const
{
  size_t pos;
  size_t tpos;
  pos = start_line.find(' ');
  tpos = start_line.find(' ', pos + 1);
  if (pos == string::npos || tpos == string::npos) {
    serv_log("Invalid call to get_request_target on non-request");
    throw invalid_state();
  }
  return start_line.substr(pos + 1, tpos - pos - 1);
}

string http_request::get_http_version() const
{
  size_t pos;
  size_t tpos;
  pos = start_line.find(' ');
  pos = start_line.find(' ', pos + 1);
  tpos = start_line.find("\r\n", pos + 1);
  if (pos == string::npos || tpos == string::npos) {
    serv_log("Invalid call to get_request_target on non-request");
    throw invalid_state();
  }
  return start_line.substr(pos + 1, tpos - pos - 1);
}


void http_request::parse_head()
{
  //CRLF as string for simple usage
  string crlf = "\r\n";

  string inp(raw.begin(), raw.end());
  //getting start_line
  //start line BNF: request-line / status-line
  if (inp.empty()) {
    serv_log("Empty input");
    throw invalid_head();
  }
  size_t pos = 0;
  size_t tpos = inp.find(crlf);
  if (tpos == string::npos) {
    serv_log("Invalid START LINE in header: expected CRLF, got \"" + inp + "\"");
    throw invalid_head();
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
      throw invalid_head();
    }
    inp.erase(0, 1);
    skip_ows(inp);

    //getting field value
    string field_value = get_field_value(inp);

    if (inp.empty()) {
      serv_log("Invalid header-field: expected CRLF, got EOF");
      throw invalid_head();
    }

    if (inp.compare(0, crlf.length(), crlf) != 0)  {
      serv_log("Invalid header-field: expected CRLF, got " + inp);
      throw invalid_head();
    }
    inp.erase(0, 2);
    str_to_lower(field_name);
    //str_to_lower(field_value);
    header_lines.insert(make_pair(field_name, field_value));
  }
  method = get_method();
  request_target = get_request_target();
  http_version = get_http_version();
  serv_log("Header parsing was successfull");
}

void http_request::get_header()
{
  string head_end = "\r\n\r\n";
  ssize_t n;
  while (search(raw.begin(), raw.end(), head_end.begin(), head_end.end()) == raw.end()) {
    read_block();
  }
}


bool http_request::is_complete() const
{
  return req_is_complete;
}

bool http_request::is_fd_valid() const
{
  return sock_fd >= 0;
}


void http_request::get_msg_body()
{
  //This method recieves message body of request

  //Here we sould have the check for maximum body length
  if (get_header_value("Content-length").first) {
    //TODO: Atoi should be replaced by something more appropriate
    //Also, add checks for normal symbols
    serv_log("Got header field 'Content-length'; therefore "
	     "reading non-chuncked request");
    string sleft = get_header_value("Content-length").second;
    size_t alr =  raw.size() - msg_body_position();
    size_t left = atoi(sleft.c_str()) - alr;
    while (left != 0) {
      left -= read_block(left);
    }
    body.assign(raw.begin() + msg_body_position(), raw.end());
    
  }
  //TODO add chuncked request
}

size_t http_request::msg_body_position() const
{
  //This method returns the position in array 'raw'
  //of the start of the message body
  string head_end = "\r\n\r\n";
  vector<char>::const_iterator it;
  it = search(raw.begin(), raw.end(),
	      head_end.begin(), head_end.end());
  if (it == raw.end()) {
    serv_log("ERROR: http_request;:msg_body_position was "
	     "called on request without header");
    throw invalid_function_call();
  }
  it += head_end.length();
  return it - raw.begin();
}

ssize_t http_request::read_block(size_t size)
{
  char buf[size];
  bzero(buf, size);
  ssize_t n;
  pollfd fdarr;
  fdarr.fd = sock_fd;
  fdarr.events = 0;
  fdarr.events |= POLLIN;
  if (poll(&fdarr, 1, 5000) <= 0) {
    close(sock_fd);
    throw invalid_head();
  }
  if (fdarr.revents & POLLERR) {
    serv_log(string("poll ERROR: ") + strerror(errno));
    close(sock_fd);
    throw process_error();
  }
  else if (!(fdarr.revents & POLLIN)) {
    serv_log(string("poll ERROR: ") + strerror(errno));
    close(sock_fd);
    throw process_error();
  }
  n = read(sock_fd, buf, BUFSIZ);
  if (n < 0) {
    serv_log("Read error: ");
#ifdef NOT_SHIT
    serv_log(strerror(errno));
#endif
    close(sock_fd);
    throw invalid_head();
  }
  if (memchr(buf, '\0', n) != NULL) {
    serv_log("Got \\0 in header");
    close(sock_fd);
    throw invalid_head();
  }
  raw.insert(raw.end(), buf, buf + n);
  return n;
}
