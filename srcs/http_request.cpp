#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>

#include "http_message.hpp"
#include "http_request.hpp"
#include "http_utils.hpp"
#include "parse_help.hpp"

using namespace std;

http_request::http_request()
{
}

http_request::http_request(int fd)
{
  sock_fd = fd;
  recieve();
}


void http_request::recieve()
{
  if (!is_fd_valid())
    throw invalid_function_call();
  get_header();
  // print_raw();
  parse_head();
  if (get_header_value("Content-Length").first ||
      get_header_value("Transfer-Encoding").first) {
    // serv_log("Requset has a body; Parsing it...");
    get_msg_body();
  }
  serv_log("Request was parsed successfully");
  // else if (raw.find("\r\n\r\n") + 2 != raw.length()) {
  //   //error here
  // }
  print();
}

void http_request::print() const
{
  cout << "Method: \'" << get_method() << "\'" <<  endl;
  cout<<"Request target: \'" << get_request_target() << "\'" << endl;
  cout << "Scheme: '" << scheme << "'" << endl;
  cout << "Path: '" << request_path << "'" << endl;
  cout << "Query: '" << query << "'" << endl;
  cout << "Fragment: '" << fragment << "'" << endl;
  cout<<"HTTP version: \'" << get_http_version() << "\'" << endl;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    cout << i->first <<": " << i->second<< "\r\n";
  }
  cout<<"\r\n";
  string msg_body(body.begin(), body.end());
  if (msg_body.length() != 0) 
    cout<<msg_body<<std::endl;
}

string http_request::get_method() const 
{
  return method;
}

string http_request::get_request_target() const
{
  return request_target;
}

string http_request::get_http_version() const
{
  return http_version;
}

void http_request::parse_start_line(string &inp)
{
  //CRLF as string for simple usage
  string crlf = "\r\n";

  //getting start_line
  //start line BNF: request-line / status-line
  if (inp.empty()) {
    serv_log("Empty input");
    throw invalid_head();
  }
  size_t pos = 0;
  size_t tpos = inp.find(crlf);
  if (tpos == string::npos) {
    serv_log("Invalid START LINE in header: expected CRLF, got \"" + inp + "\"");
    throw invalid_head();
  }
  //TODO: add checher for correct request-line syntax
  tpos += crlf.length();
  start_line = inp.substr(pos, tpos);
  inp.erase(pos, tpos - pos);

  //Setting all the neccesary inside variables
  pos = start_line.find(' ');  
  tpos = start_line.find(' ', pos + 1);
  if (pos == string::npos || tpos == string::npos) {
    serv_log("Cannot set request_target");
    throw invalid_head();
  }
  method = start_line.substr(0, pos);
  request_target = start_line.substr(pos + 1, tpos - pos - 1);
  pos = tpos;
  tpos = start_line.find("\r\n", pos + 1);
  if (pos == string::npos || tpos == string::npos) {
    serv_log("Invalid call to get_request_target on non-request");
    throw invalid_state();
  }
  http_version = start_line.substr(pos + 1, tpos - pos - 1);

  //Parsing target and setting queries and such

  string temp = request_target;
  if (temp.find("://") != string::npos) {
    scheme = temp.substr(0, temp.find("://"));
    temp.erase(0, scheme.length() + 3);
  }
  if (temp.find("://") != string::npos) {
    serv_log("ERROR: double '://' in target");
    throw invalid_head();
  }
  if (temp.find("/") != string::npos) {
    host = temp.substr(0, temp.find("/"));
    temp.erase(0, temp.find("/"));
  }
  if (temp.find("#") != string::npos) {
    fragment = temp.substr(temp.find("#"));
    temp = temp.substr(0, temp.find("#"));
    if (fragment.find("#") != string::npos) {
      serv_log("ERROR: double '#' in fragment");
      throw invalid_head();
    }
  }
  if (temp.find("?") != string::npos) {
    query = temp.substr(temp.find("?") + 1);
    temp = temp.substr(0, temp.find("?"));
    if (fragment.find("?") != string::npos) {
      serv_log("ERROR: double '#' in query");
      throw invalid_head();      
    }
  }
  request_path = temp;
}

void http_request::parse_head()
{
  string crlf = "\r\n";
  string inp(raw.begin(), raw.end());
  size_t pos = 0;
  size_t tpos = inp.find(crlf);
  parse_start_line(inp);
  parse_header_fields(inp);
}

void http_request::get_header()
{
  string head_end = "\r\n\r\n";
  ssize_t n;
  while (search(raw.begin(), raw.end(), head_end.begin(), head_end.end()) == raw.end()) {
    if (read_block() == 0) {
      return ;
    }
  }
}


bool http_request::is_complete() const
{
  return req_is_complete;
}

bool http_request::is_fd_valid() const
{
  return sock_fd >= 0;
}


void http_request::get_msg_body()
{
  //This method recieves message body of request

  //Here we sould have the check for maximum body length
  if (get_header_value("Content-length").first) {
    //Also, add checks for normal symbols
    serv_log("Got header field 'Content-length'; therefore "
	     "reading non-chuncked request");
    string sleft = get_header_value("Content-length").second;
    size_t alr =  raw.size() - msg_body_position();
    size_t left = atoi(sleft.c_str()) - alr;
    while (left != 0) {
      left -= read_block(left);
    }
    body.assign(raw.begin() + msg_body_position(), raw.end());
  }
  else {
    string crlf = "\r\n";
    ssize_t next_chunk_position = msg_body_position();
    while (1) {
      if (search(raw.begin() + next_chunk_position, raw.end(),
		 crlf.begin(), crlf.end()) != raw.end()) {
	//getting position of end of the chunk size length
	ssize_t chunk_head_end = search(raw.begin() + next_chunk_position, raw.end(),
					crlf.begin(), crlf.end()) - (raw.begin() + next_chunk_position);
	//getting chunk size
	string chunk_head;
	chunk_head.insert(chunk_head.begin(), raw.begin() + next_chunk_position,
			  raw.begin() + chunk_head_end + next_chunk_position);
	if (chunk_head.length() == 0)
	  throw invalid_state();
	string str_chunk_size = chunk_head.substr(0, chunk_head.find(";"));
	if (!is_hex_string(str_chunk_size))
	  throw invalid_state();
	long chunk_size = strtol(str_chunk_size.c_str(), NULL, 16);
	if (chunk_size == 0)
	  return ;
	//getting the whole chunk
	//2 is the size of crlf
	while (raw.size() < next_chunk_position + chunk_head_end + 2 + chunk_size)
	  read_block();
	body.insert(body.end(), raw.begin() + next_chunk_position + chunk_head_end + 2,
		    raw.begin() + next_chunk_position + chunk_head_end + 2 + chunk_size);
	next_chunk_position = next_chunk_position + chunk_head_end + 2 + chunk_size + 2;
    
      }
    }
  }
}

string http_request::get_request_path() const
{
  return request_path;
}

string http_request::get_request_query() const
{
  return query;
}

int http_request::keep_alive() const
{
  pair<bool, string> r  = get_header_value("Connection");
  if (!r.first)
    return 0;
  if (r.second == "close")
    return 0;
  if (get_http_version() == "HTTP/1.1")
    return 1;
  if (r.second == "keep-alive" && get_http_version() == "HTTP/1.0")
    return 1;
  return 0;
}
