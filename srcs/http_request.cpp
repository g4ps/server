#include <string>
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

#include "http_message.hpp"
#include "http_request.hpp"
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
    throw invalid_method();
  get_header();
  parse_head();
  if (!get_header_value("Content-Length").first &&
      !get_header_value("Transfer-Encoding").first) {
    get_msg_body();
  }
  else if (raw.find("\r\n\r\n") + 2 != raw.length()) {
    //error here
  }
}

void http_request::parse_head()
{
  //CRLF as string for simple usage
  string crlf = "\r\n";

  string inp = raw;
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
  char buf[BUFSIZ];
  bzero(buf, BUFSIZ);
  ssize_t n;
  pollfd fdarr;
  fdarr.fd = sock_fd;
  fdarr.events = 0;
  fdarr.events |= POLLIN;
  while (raw.find("\r\n\r\n") == string::npos) {
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
    raw.append(buf, n);
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
  //TODO
}
