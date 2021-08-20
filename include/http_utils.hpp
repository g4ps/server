#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP


#include <string>
#include <vector>
#include <list>
#include <unistd.h>

using namespace std;

void set_log(string s);
void serv_log(string s);
string convert_to_string(int);
string convert_to_string(size_t);
string convert_to_string(short);
string convert_to_string(off_t);
const char ** make_argument_vector(list<string> s);
bool is_directory(string target);
string str_to_upper(string req);
bool does_exist(string s);
void create_file(string filename, vector<char> &content);
string uri_to_string(string s);

#endif
