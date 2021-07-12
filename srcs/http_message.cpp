#include <string>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>
#include <list>

#include "http_message.hpp"
#include "http_utils.hpp"
#include "parse_help.hpp"

using namespace std;

http_message::http_message()
{
}

// http_message::http_message(string s, int inp_fd)
// {
//   parse_raw_head(s, inp_fd);
// }

/*
  needed for quick initialization of the class by easy stuff
 */
// void http_message::parse_raw_head(string inp, int inp_fd)
// {
//   //CRLF as string for simple usage
//   string crlf = "\r\n";

//   sock_fd = inp_fd;

//   //getting start_line
//   //start line BNF: request-line / status-line
//   if (inp.empty()) {
//     serv_log("Empty input");
//     throw invalid_state();
//   }
//   raw = inp;
//   size_t pos = 0;
//   size_t tpos = inp.find(crlf);
//   if (tpos == string::npos) {
//     serv_log("Invalid START LINE in header: expected CRLF, got \"" + inp + "\"");
//     throw invalid_state();
//   }
//   //TODO: add checher for correct request-line syntax
//   tpos += crlf.length();
//   start_line = inp.substr(pos, tpos);
//   inp.erase(pos, tpos - pos);

//   //getting all the header-fields
//   //header-field BNF is field-name ":" OWS field-value OWS
//   while (inp.compare(0, crlf.length(), crlf) != 0) {
//     pos = 0;
//     tpos = pos;
//     //field-name is token, so
//     string field_name = get_token(inp);

//     //checking for sanity
//     if (inp.empty()) {
//       serv_log("Invalid header-field: eof when expected \":\" OWS field-value");
//     }

//     //checking ":" for existence and acting accordingly
//     if (inp[0] != ':') {
//       serv_log("Invalid header-field: expected ':', got " + inp);
//       throw invalid_state();
//     }
//     inp.erase(0, 1);
//     skip_ows(inp);

//     //getting field value
//     string field_value = get_field_value(inp);

//     if (inp.empty()) {
//       serv_log("Invalid header-field: expected CRLF, got EOF");
//       throw invalid_state();
//     }

//     if (inp.compare(0, crlf.length(), crlf) != 0)  {
//       serv_log("Invalid header-field: expected CRLF, got " + inp);
//       throw invalid_state();
//     }
//     inp.erase(0, 2);
//     //str_to_lower(field_name);
//     //str_to_lower(field_value);
//     header_lines.insert(make_pair(field_name, field_value));
//   }
//   serv_log("Header parsing was successfull");
// }

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
  cout << "Oh, hi there\n";
}

void http_message::set_socket(int fd)
{
  sock_fd = fd;
}

void http_message::add_header_field(string name, string val)
{
  header_lines.insert(make_pair(name, val));
}

string& http_message::get_start_line() //maybe i should make it const
{
  return start_line;
}

string http_message::compose_header_fields()
{
  string ret;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    ret +=  i->first  + ": " +  i->second +  "\r\n";
  }
  return ret;
}


int http_message::get_socket()
{
  return sock_fd;
}

pair<bool, string> http_message::get_header_value(string name)
{
  str_to_lower(name);
  map<string, string>::iterator it;
  it = header_lines.find(name);
  if (it == header_lines.end())
    return pair<bool, string>(false, "");
  return pair<bool, string>(true, it->second);
}

size_t http_message::get_body_size() const
{
  return body.size();
}

void http_message::print_body_into_fd(int fd)
{
  char buf[4096];
  copy(body.begin(), body.end(), buf);
  write(fd, buf, body.size());
}

void http_message::set_server(http_server *l)
{
  serv = l;
}


void http_message::parse_header_fields(string &inp)
{
  string crlf = "\r\n";
  size_t pos = 0;
  size_t tpos = inp.find(crlf);
  while (inp.compare(0, crlf.length(), crlf) != 0) {
    pos = 0;
    tpos = pos;
    //field-name is token, so
    string field_name = get_token(inp);

    //checking for sanity
    if (inp.empty()) {
      serv_log("Invalid header-field: eof when expected \":\" OWS field-value");
      throw invalid_state();
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
    //str_to_lower(field_value);
    header_lines.insert(make_pair(field_name, field_value));
  }
}

ssize_t http_message::read_block(size_t size, int fd)
{
  int ret;
  if (fd < 0)
    fd = sock_fd;
  char buf[size];
  bzero(buf, size);
  ssize_t n;
  pollfd fdarr;
  fdarr.fd = fd;
  fdarr.events = 0;
  fdarr.events |= POLLIN;
  if ((ret = poll(&fdarr, 1, 5000)) <= 0) {
    close(fd);
    if (ret == 0) {
      throw req_timeout();
    }
    throw invalid_state();
  }
  if (fdarr.revents & POLLERR) {
    serv_log(string("poll ERROR: ") + strerror(errno));
    close(fd);
    throw invalid_state();
  }
  else if (!(fdarr.revents & POLLIN)) {
    serv_log(string("poll ERROR: ") + strerror(errno));
    close(fd);
    throw invalid_state();
  }
  n = read(fd, buf, BUFSIZ);
  if (n < 0) {
    serv_log("Read error: ");
#ifdef NOT_SHIT
    serv_log(strerror(errno));
#endif
    close(fd);
    throw invalid_state();
  }
  if (memchr(buf, '\0', n) != NULL) {
    serv_log("Got \\0 in header");
    close(fd);
    throw invalid_state();
  }
  raw.insert(raw.end(), buf, buf + n);
  return n;
}

ssize_t http_message::read_nb_block(size_t size, int fd)
{
  if (fd < 0)
    fd = sock_fd;
  char buf[size];
  bzero(buf, size);
  ssize_t n;
  pollfd fdarr;
  fdarr.fd = fd;
  fdarr.events = 0;
  fdarr.events |= POLLIN;
  if (poll(&fdarr, 1, 5000) <= 0) {
    close(fd);
    throw invalid_state();
  }
  if (fdarr.revents & POLLERR) {
    serv_log(string("poll ERROR: ") + strerror(errno));
    close(fd);
    throw invalid_state();
  }
  // else if (!(fdarr.revents & POLLIN)) {
  //   serv_log(string("poll ERROR: ") + strerror(errno));
  //   close(fd);
  //   throw invalid_state();
  // }
  n = read(fd, buf, BUFSIZ);
  if (n < 0) {
    serv_log("Read error: ");
#ifdef NOT_SHIT
    serv_log(strerror(errno));
#endif
    close(fd);
    throw invalid_state();
    return 0;
  }
  raw.insert(raw.end(), buf, buf + n);
  return n;
}

size_t http_message::msg_body_position() const
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
    throw invalid_state();
  }
  it += head_end.length();
  return it - raw.begin();
}


list<string>
http_message::get_cgi_header_values()
{
  list<string> ret;
  multimap<string, string>::const_iterator it;
  for (it = header_lines.begin(); it != header_lines.end(); it++){
    ret.push_back(string("HTTP_") + str_to_upper(it->first)
		  + "=" + str_to_upper(it->second));
  }
  return ret;
}
