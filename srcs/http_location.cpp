#include <string>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <list>
#include <deque>
#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>

#include "parse_help.hpp"
#include "http_message.hpp"
#include "http_request.hpp"
#include "http_utils.hpp"
#include "http_location.hpp"

using namespace std;

bool http_location::is_path(string s) const
{
  if (s == path)
    return true;
  s += "/";
  return s == path;
}

http_location::http_location()
{
}

http_location::http_location(string s)
{
  set_path(s);
}

void http_location::add_method(string s)
{
  methods.push_back(s);
}

bool http_location::is_method_accepted(string s)
{
  list<string>::const_iterator it;
  for (it = methods.begin(); it != methods.end(); it++) {
    if (*it == s)
      return true;
  }
  return false;
}

void http_location::add_redirect(int status, string dir)
{
  if (!redirection.insert(pair<int, string>(status, dir)).second){
    serv_log(string("Invalid redirect '")
	     + convert_to_string(status)
	     + "' on target '" + dir + "': already exists");
    throw invalid_state();
  }  
}

void http_location::set_path(string s)
{
  //Adding trailing slash
  //Needed  for consistency (i.e. every functions,
  //that interacts with path assumes that it has a
  //slash at the end
  if (s[s.length() - 1] != '/')
    s += "/";
  path = s;
}

string http_location::get_path() const
{
  return path;
}

void http_location::set_root(string s)
{
  if (!is_directory(s)) {
    serv_log(string("Couln't open directory ") + s);
    throw invalid_state();
  }
  if (s[s.length() - 1] != '/')
    s += "/";
  root = s;
}

string http_location::get_root() const
{
  return root;
}

void http_location::set_autoindex()
{
  auto_index = true;  
}

void http_location::unset_autoindex()
{
  auto_index = false;
}

bool http_location::get_autoindex() const
{
  return auto_index;
}

void http_location::add_error_page(int status, string path)
{
  if (!error_pages.insert(pair<int, string>(status, path)).second){
    serv_log(string("Cannot add error page for status '")
	     + convert_to_string(status) +
	     "' on target '" + path + "': already exists");
    throw invalid_state();
  }
}

void http_location::add_index(string def)
{
  index.push_back(def);
}

string http_location::get_index_page(string path)
{
  if (path[path.length() - 1] != '/' && is_directory(path))
    throw directory_uri();
  list<string>::const_iterator it;
  for (it = index.begin(); it != index.end(); it++) {
    string temp = path + *it;
    if (does_exist(temp))
      return temp;
  }
  serv_log(string("Couldn't find index in path '") + path + "'");
  throw not_found();
}

string http_location::get_file_name(string s)
{
  if (is_path(s)) {
    if (s[s.length() - 1] != '/')
      throw directory_uri();    
    return get_index_page(root);
  }
  s.erase(s.begin(), s.begin() + path.length());  
  s = root + s;
  if (is_directory(s))
    return get_index_page(s);
  if (!does_exist(s)) {
    serv_log(string("File '") + s + "' doesn't exist");
    throw not_found();
  }
  return s;
}

bool http_location::is_located(string target) const
{
  if (is_path(target))
    return true;
  target.resize(path.size());
  if (target == path)
    return true;
  return false;
}

size_t http_location::get_path_depth() const
{
  return count(path.begin(), path.end(), '/');
}

void http_location::add_cgi(string ext, string filename)
{
  http_cgi n;
  n.extention = ext;
  n.path = filename;
  cgi.push_back(n);
}

string http_location::compose_allowed_methods() const
{
  string ret;
  list<string>::const_iterator it;
  for (it = methods.begin(); it != methods.end(); it++) {
    if (ret.length() != 0)
      ret += " ";
    ret += *it;
  }
  return ret;
}

bool http_location::is_cgi_request(string target) const
{
  list<http_cgi>::const_iterator it;
  for (it = cgi.begin(); it != cgi.end(); it++) {
    string curr = it->extention;
    string temp = target;
    if (curr.length() < temp.length()) {
      temp = temp.substr(target.length() - curr.length(), curr.length());
      if (temp == curr)
	return true;
    }
  }
  return false;
}

string http_location::cgi_path(string target) const
{
  list<http_cgi>::const_iterator it;
  for (it = cgi.begin(); it != cgi.end(); it++) {
    string curr = it->extention;
    string temp = target;
    if (curr.length() < temp.length()) {
      temp = temp.substr(target.length() - curr.length(), curr.length());
      if (temp == curr)
	return it->path;
    }
  }
  return "";
}
