#include <string>
#include <list>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <cstring>
#include <sys/wait.h>

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
#include "http_utils.hpp"

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

void http_server::serve(int fd, sockaddr_in addr)
{
  //get time;
  try {
    http_request req(fd);
    req.print();
    process_request(req, addr);
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

void http_server::process_request(http_request& msg, sockaddr_in addr)
{
  if (is_cgi_request(msg.get_request_target())) {
    process_cgi(msg, addr);
  }
  string type = msg.get_method();
  if (type == "GET") {
    process_get_request(msg);  
  }
  else if (type == "POST") {
    process_post_request(msg);
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

void http_server::process_cgi(http_request& req, sockaddr_in addr)
{
  serv_log("Processing cgi");
  int fd1[2];
  int fd2[2];
  pipe(fd1);
  pipe(fd2);
  pid_t pid = fork();
  if (pid == 0) {
    close(fd1[1]);
    close(fd2[0]);
    dup2(fd1[0], 0);
    dup2(fd2[1], 1);
    //All of the shit, concerning setting variables should exist in
    //other function
    const char **vars = new const char*[100];
    for (int i = 0; i < 100; i++) {
      vars[i] = NULL;
    }
    list<string> args;
    args.push_back(string("CONTENT_LENGTH=")
		   + convert_to_string(req.get_body_size()));
    args.push_back("QUERY_STRING=");
    args.push_back("SERVER_SOFTWARE=Eugene_server 0.1");
    args.push_back(string("REQUEST_METHOD=") + req.get_method());
    args.push_back("SERVER_PROTOCOL=HTTP/1.1");
    args.push_back(string("PATH_INFO=") + req.get_request_target());
    char pbuf[100];
    getcwd(pbuf, 100);
    args.push_back(string("PATH_TRANSLATED=") + pbuf + "/html" + req.get_request_target());
    char cad[100];
    inet_ntop(AF_INET, &(addr.sin_addr.s_addr), cad, sizeof(sockaddr_in));
    args.push_back(string("REMOTE_PORT=") + convert_to_string(ntohs(addr.sin_port)));
    args.push_back(string("REMOTE_ADDR=") + cad);
    sockaddr_in serv_addr;
    socklen_t ssize = sizeof(sockaddr_in);
    getsockname(req.get_socket(), (sockaddr*)&serv_addr, &ssize);
    char cbd[100];
    inet_ntop(AF_INET, &(serv_addr.sin_addr.s_addr), cbd, sizeof(sockaddr_in));
    args.push_back(string("SERVER_NAME=") + cbd);
    args.push_back(string("SERVER_PORT=") + convert_to_string(htons(serv_addr.sin_port)));
    args.push_back(string("SCRIPT_NAME=") + req.get_request_target());
    //args.push_back("SCRIPT_NAME=html/test.php");
    //args.push_back("SCRIPT_FILENAME=/home/eugene/school/server/html/test.php");
    args.push_back("GATEWAY_INTERFACE=CGI/1.1");
    if (req.get_header_value("Content-Type").first) {
      args.push_back(string("CONTENT_TYPE=") +
		     req.get_header_value("Content-Type").second);
      fprintf(stderr, "GOT HERE FOAN ONDSAO NDOSA NOIDASNO DSAIO \n");
    }
    const char **vv = make_argument_vector(args);
    for (size_t i = 0; i < args.size(); i++) {
      fprintf(stderr, "%s\n", vv[i]);
    }
    const char *k[10];
    k[0] = "/usr/bin/php-cgi";
    k[1] = "-f";
    k[2] = "/home/eugene/school/server/html/test.php";
    k[3] = NULL;
    execve("/usr/bin/php-cgi", (char* const *)k,  (char* const *)vv);
    exit(0);
  }
  else {
    close(fd1[0]);
    close(fd2[1]);
    if (req.get_body_size() != 0) {
      req.print_body_into_fd(fd1[1]);
    }
    close(fd1[1]);
    char buf[512];
    int n = 0;
    while ((n = read(fd2[0], buf, 512)) != 0) {
      write(1, buf, n);
    }
    int status;
    wait(&status);
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

bool http_server::is_cgi_request(string target_name)
{
  //change later for config-defined parameters;
  //maybe it even shouldn't be here, i'm just trying some code exec
  if (target_name.find(".php") == target_name.length() - 4)
    return true;
  return false;
}

void http_server::process_post_request(http_request &req)
{
  // if (is_cgi_request(req.get_request_target())) {
  //   process_cgi(req);
  // }
}
