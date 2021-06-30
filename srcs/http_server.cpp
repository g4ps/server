#include <string>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <cstring>

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
#include "http_responce.hpp"
#include "http_request.hpp"
#include "http_server.hpp"

using namespace std;

void http_server::add_socket(string saddr, short port)
{
  stringstream s;
  s << "Initializing server on host " << saddr
    << " (port: " <<  port << ")";
  serv_log(s.str());
  sockaddr_in addr;
  if ((addr.sin_addr.s_addr = inet_addr(saddr.c_str())) == INADDR_NONE) {
    serv_log("Address \"" + saddr + "\" is malformed");
    throw init_error();
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  add_socket(&addr);
}

void http_server::add_socket(sockaddr_in *addr)
{  
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    serv_log("ERROR: socket shat the bed");
    serv_log(strerror(errno));
    throw init_error();
  }
  int reuse = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
    serv_log("ERROR: setsockopt fucked up");
    serv_log(strerror(errno));
    throw init_error();
  }
  if (bind(sock_fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) == -1) {
    serv_log("ERROR: bind fucked up");
    serv_log(strerror(errno));
    throw init_error();
  }
  if (listen(sock_fd, 10) == -1) {
    serv_log("ERROR: listen fucked up");
    serv_log(strerror(errno));
    throw init_error();
  }
  if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) == -1) {
    serv_log("ERROR: fcntl fucked up");
    serv_log(strerror(errno));
    throw init_error();
  }
  sock_fds.push_back(sock_fd);
  char b[512];
  bzero(b, 512);
  stringstream ss;
  ss << "Successfully added socket for host " <<
    inet_ntop(AF_INET, (void*) &(addr->sin_addr), b, INET_ADDRSTRLEN) <<
    " on port " << htons(addr->sin_port);
  serv_log(ss.str());
}

#ifdef NOT_SHIT

void http_server::add_socket_from_hostname(string host, short port)
{
  stringstream ss;
  ss << "Initializing server on host " << host
    << " (port: " <<  port << ")";
  serv_log(ss.str());
  struct addrinfo hints;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_INET;
  struct addrinfo *res = NULL;
  struct addrinfo *i = NULL;
  struct addrinfo *tmp = NULL;
  int err = getaddrinfo(host.c_str(), NULL, &hints, &res);
  if (err != 0) {
    serv_log(string("ERROR: getaddrinfo") + gai_strerror(err));
    throw init_error();
  }
  struct sockaddr_in *p = NULL;
  for (i = res; i != NULL; i++) {
    if (i->ai_family == AF_INET) {
      p = (sockaddr_in *)res->ai_addr;
      break;
    }
  }
  if (p == NULL) {
    serv_log("ERROR: could not find address in AF_INET");
    throw init_error();
  }
  struct sockaddr_in* ret = new sockaddr_in;
  memcpy(ret, p, sizeof(sockaddr_in));
  ret->sin_port = htons(port);
  freeaddrinfo(res);
  add_socket(ret);
  delete ret;
}

#endif

size_t http_server::num_of_sockets() const
{
  return sock_fds.size();
}

void http_server::serve(int fd)
{
  //get time;
  try {
    http_request req(fd);
    req.print();
    process_request(req);
  }
  catch (exception &e) {
    process_error(fd, 400);
    cout<<"ERROR: "<<e.what()<<endl;
  }
  close(fd);
  stringstream s;
  s << "Connection closed on socket (" << fd << ")";
  serv_log(s.str());
}

void http_server::process_request(http_request& msg)
{
  string type = msg.get_method();
  if (type == "GET") {
    process_get_request(msg);  
  }
  else {    
    process_error(msg, 501);
  }
}

void http_server::process_get_request(http_request& req)
{
  //replace this thing by normal search
  string file_name = "html" + req.get_request_target();
  if (file_name[file_name.length() - 1] == '/')
    file_name += "index.html";
  try {
    http_responce resp(200);
    resp.set_target_name(file_name);
    resp.set_socket(req.get_socket());
    resp.write_responce();
  }
  catch(http_responce::target_not_found &e) {
    process_error(req, 404);
  }
  catch(http_responce::target_denied &e) {
    process_error(req, 403);
  }
  catch(exception &e) {
    serv_log("Something went terribly wrong during get request");
  }
}

void http_server::process_error(http_request &in, int status)
{
  serv_log(string("Processing error '") + convert_to_string(status) + "' on target '"
	   + in.get_request_target() + "'");
  try {
    
    http_responce resp(status);
    resp.set_socket(in.get_socket());
    string err_target = get_error_target_name(in.get_request_target());    
    if (err_target.length() == 0) {
      resp.set_body(get_default_err_page(status));      
    }
    else {
      resp.set_target_name(err_target);
    }
    resp.write_responce();
  }
  //Maybe i should make exception handling a little bit more sophisticated
  catch(exception &e) {
    serv_log("Something went wrong with writing error page");
  }
}

void http_server::process_error(int in, int status)
{
  serv_log(string("Processing error '") + convert_to_string(status) + "' on this socket");
  try {
    http_responce resp(status);
    resp.set_socket(in);
    resp.set_body(get_default_err_page(status));
    resp.write_responce();
  }
  //Maybe i should make exception handling a little bit more sophisticated
  catch(exception &e) {
    serv_log("Something went wrong with writing error page");
  }
}

string http_server::get_default_err_page(int status)
{
  //If i'll ever get to making this project myself,
  //then i should look into making this function respond differently
  //to different errors;
  //Also, disabling of this particular page should be configured
  string ret;
  ret += "<html>\n";
  ret += "  <head>\n";
  ret += "    <title>";
  ret += convert_to_string(status);
  //Maybe should add here reasoning
  ret += " Error</title>\n";
  ret += "    <meta charset='utf-8'>    \n";
  ret += "  </head>\n";
  ret += "  <body>\n";
  ret += "    <center>\n";
  ret += "      <h1>Server couldn't proceess your request</h1>\n";
  ret += "      <h2> Error:";
  ret += convert_to_string(status);
  ret += "      </h2>\n";
  ret += "      <hr>\n";
  ret += "    </center>\n";
  ret += "  </body>\n";
  ret += "</html>\n";
  return ret;
}

string http_server::get_error_target_name(string target)
{
  return "";
}

void http_server::process_not_found(http_request &req)
{
  http_responce resp(404);
  resp.set_socket(req.get_socket());
  resp.write_responce();
}

void http_server::send_status_code(http_request &req, int code)
{
  if (code == 404)
    process_not_found(req);
}

void http_server::send_timeout(int fd)
{
}

bool http_server::has_socket(int fd)
{
  deque<int>::const_iterator it;
  for (it = sock_fds.begin(); it != sock_fds.end(); it++) {
    if (*it == fd)
      return true;
  }
  return false;
}

deque<int>& http_server::get_sockets()
{
  return sock_fds;
}

bool is_cgi_request(string target_name)
{
  //change later for config-defined parameters;
  //maybe it even shouldn't be here, i'm just trying some code exec
  if (target_name.find(".php") == target_name.length() - 4)
    return true;
  return false;
}
