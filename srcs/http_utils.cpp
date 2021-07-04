#include <string>
#include <list>
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
    const char *str = it->c_str();
    char *in = new char[it->length() + 1];
    strcpy(in, str);
    ret[i] = in;
    i++;
  }
  return ret;
}
