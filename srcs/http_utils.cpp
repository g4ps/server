#include <string>
#include <list>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "http_server.hpp"
#include "http_utils.hpp"

void serv_log(std::string out)
{
  std::cout<<out<<std::endl;
}

string convert_to_string(size_t arg)
{
  stringstream s;
  s << arg;
  return s.str();
}

string convert_to_string(off_t arg)
{
  stringstream s;
  s << arg;
  return s.str();
}

string convert_to_string(int arg)
{
  stringstream s;
  s << arg;
  return s.str();
}

string convert_to_string(short arg)
{
  stringstream s;
  s << arg;
  return s.str();
}

const char ** make_argument_vector(list<string> s)
{
  const char **ret = new const char*[s.size() + 1];
  list<string>::const_iterator it;
  int i = 0;
  for (it = s.begin(); it != s.end(); it++) {
    const char *str = strdup(it->c_str());
    ret[i] = str;
    i++;
  }
  ret[i] = NULL;
  return ret;
}

bool is_directory(string target)
{
  struct stat temp;
  if (stat(target.c_str(), &temp) < 0) {
    return false;
    // serv_log(string("Cannot perform stat: ") + strerror(errno));
    //throw http_server::invalid_target();    
  }
  if (temp.st_mode & S_IFDIR) {
    return true;
  }
  return false;
}

bool does_exist(string target)
{
  struct stat temp;
  if (stat(target.c_str(), &temp) < 0) {
    return false;
  }
  return true;
}

string str_to_upper(string req)
{
  string ret = req;
  for (ssize_t i = 0; i < ret.length(); i++) {
    ret[i] = toupper(ret[i]);    
  }
  return ret;
}


void create_file(string name, vector<char> &content)
{
  int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  char buf[BUFSIZ];
  pollfd pfd;
  while (content.size() != 0) {
    pfd.fd = fd;
    pfd.events = 0 | POLLOUT;
    int ret = poll(&pfd, 1, 5000);
    if (ret == 0) {
      serv_log("ERROR: Timeout during writing file");
      throw http_request::timeout_error();
    }
    else if (ret < 0) {
      serv_log(string ("Poll ERROR: ") + strerror(errno));
      throw http_server::internal_error();
    }
    if (pfd.revents & POLLERR) {
      serv_log(string ("Poll ERROR: ") + strerror(errno));
      throw http_server::internal_error();
    }      
    size_t n = content.size();
    if (n > BUFSIZ)
      n = BUFSIZ;
    copy(content.begin(), content.begin() + n, buf);
    write(fd, buf, n);
    content.erase(content.begin(), content.begin() + n);
  }
}

string uri_to_string(string t)
{
  size_t i = 0;
  string ret;
  while (i < t.length()) {
    if (t[i] == '%') {
      string dig;
      dig += t[i + 1];
      dig += t[i + 2];
      char c = strtol(dig.c_str(), NULL, 16);
      ret += c;
      i += 3;
    }
    else {
      ret += t[i];
      i++;      
    }
  }
  return ret;
}
