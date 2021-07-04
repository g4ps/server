#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP


#include <string>
#include <list>
#include <unistd.h>

using namespace std;

void serv_log(string s);
string convert_to_string(int);
string convert_to_string(size_t);
string convert_to_string(short);
string convert_to_string(off_t);
const char ** make_argument_vector(list<string> s);

#endif
