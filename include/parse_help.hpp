#ifndef PARSE_HELP_HPP
#define PARSE_HELP_HPP

#include <string>
using namespace std;

int is_field_content(const string &s);
int is_obs_fold(const string &s);
int is_tchar(char c);
int is_obstext(char c);
int is_field_char(char c);
void skip_ows(string &str);
void str_to_lower(string &s);
string get_conf_token(string &in);

#endif
