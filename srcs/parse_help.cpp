#include "parse_help.hpp"
#include "http_utils.hpp"
#include <string>
#include <cstring>
#include <cstdlib>

using namespace std;

/*
  function: is_field_content, is_obs_fold

  returns: 1 if string starts accordingly with 
  BNF-specified values , 0 otherwise

  comment: part of BNF in field-value;
  mid-level functions
 */


// FORM:  field-char [ 1*( SP / HTAB ) field-char ]
int is_field_content(const string &s) {
  size_t t_pos = 0;
  if (s.length() == 0)
    return 0;
  if (is_field_char(s[t_pos])) {
    t_pos++;
    if (s[t_pos] == ' ' || s[t_pos] == '\t') {
      while (t_pos < s.length() && (s[t_pos] == ' ' || s[t_pos] == '\t')) {
	t_pos++;
      }
      if (t_pos == s.length())
	return 0;
      if (!is_field_char(s[t_pos]))
	return 0;
    }
  }
  else
    return 0;
  return 1;  
}

//FORM: CRLF 1*( SP / HTAB )
int is_obs_fold(const string &s)
{
  if (s.length() >= 3 && s[0] == '\r' &&
      s[1] == '\n' && (s[2] == ' ' || s[2] == '\t'))
    return 1;
  return 0;
}



/*
  functions: is_tchar , is_obstext, is_fieldchar

  returns: 0 if char is not in the set of chars,
  specified in acconding BNF's;
  1 otherwise;

  comment: lowest-level BNF definitions; required
  by highter-level BNF parsers
 */

//FORM: VCHAR minus "(),/:;<=>?@[\]{}" and dquote
int is_tchar(char c)
{
  static string s = "(),/:;<=>?@[\\]{}\"";
  return s.find(c) == string::npos && isprint(c);
}

//FORM: 0x80 - 0xFF
int is_obstext(char c) {
  if (static_cast<int>(c) >= 0x80 && static_cast<int>(c) <= 0xff)
    return 1;
  return 0;
}

//FORM: VCHAR / obs-text
int is_field_char(char c)
{
  if (isprint(c) || is_obstext(c))
    return 1;
  return 0;
}

/*
  functions: skip_ows;

  returns: void
  
  comment: skips optional white spaces in string
  i.e. *( HTAB / SP ); white spaces are discarded
 */

void skip_ows(string &str)
{
  size_t pos = 0;
  while ((str[pos] == ' ' || str[pos] == '\t') && pos < str.length())
    pos++;
  str.erase(0, pos);
}

/*
  function: str_to_lower

  returns: void

  comment: because all the field values are supposed to be
  case-insensitive, we just convert them all into lower-case
 */

void str_to_lower(string &s)
{
  for (size_t i = 0; i < s.length(); i++) {
    s[i] = tolower(s[i]);
  }  
}
