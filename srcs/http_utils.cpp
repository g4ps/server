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
