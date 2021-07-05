#include <string>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <list>
#include <deque>

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

void http_location::add_default_page(string def)
{
  default_pages.push_back(def);
}

string http_location::process_file_name(string s)
{
  string ret = root;
  if (ret[ret.length() - 1] == '/')
    ret.resize(ret.length() - 1); //removing trailing slash
  //Removing path
  size_t len = 0;
  while (s[len] == path[len] && len < s.length())
    len++;
  s = s.substr(len);
  return ret + s;
}
