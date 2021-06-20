#include <string>
#include <map>
#include <vector>
#include <exception>

#include "http_message.hpp"
#include "parse_help.hpp"

using namespace std;

http_message::http_message()
{
}

http_message::http_message(string s, int inp_fd)
{
  parse_raw_head(s, inp_fd);
}

/*
  needed for quick initialization of the class by easy stuff
 */
void http_message::parse_raw_head(string inp, int inp_fd)
{
  //CRLF as string for simple usage
  string crlf = "\r\n";

  sock_fd = inp_fd;

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
    //str_to_lower(field_name);
    //str_to_lower(field_value);
    header_lines.insert(make_pair(field_name, field_value));
  }
  serv_log("Header parsing was successfull");
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
  cout<<"Method: \'" << get_method() << "\'" <<  endl;
  cout<<"Request target: \'" << get_request_target() << "\'" << endl;
  cout<<"HTTP version: \'" << get_http_version() << "\'" << endl;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    cout << i->first <<": " << i->second<< "\r\n";
  }
  cout<<"\r\n";
}

///maybe later i'll need to change exception class

string http_message::get_method() const 
{
  size_t pos;
  pos = start_line.find(' ');
  if (pos == string::npos) {
    serv_log("Invalid call to get_method on non-request");
    throw invalid_state();
  }
  return start_line.substr(0, pos);
}

string http_message::get_request_target() const
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

string http_message::get_http_version() const
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
