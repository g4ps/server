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
#include <algorithm>

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


void http_server::add_socket(string saddr, unsigned short port)
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

void http_server::add_socket_from_hostname(string host, unsigned short port)
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
    serv_log(string("ERROR: getaddrinfo ") + gai_strerror(err));
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


size_t http_server::num_of_sockets() const
{
  return sock_fds.size();
}

int http_server::serve(int fd, sockaddr_in addr)
{
  //get time;
  http_request req;
  try {
    req.set_socket(fd);
    req.recieve();
    process_request(req, addr);
  }
  catch (method_not_allowed &e) {
    serv_log("ERROR: Method is not allowed");
    process_error(fd, 405);
    return 0;
  }
  catch (http_request::invalid_head &e) {
    serv_log("ERROR: Invalid header form");
    process_error(fd, 400);
    return 0;
    // cout<<"ERROR: "<<e.what()<<endl;
  }
  catch (exception &e) {
    serv_log(string("ERROR: Internal server error: ") + e.what());
    process_error(fd, 500);
    return 0;
  }
  if (req.keep_alive())
    active_fds.push_back(fd);
  return 0;
}

void http_server::process_request(http_request& msg, sockaddr_in addr)
{
  string type = msg.get_method();
  http_location &r = get_location_from_target(msg.get_request_path());
  if (!r.is_method_accepted(msg.get_method())) {
    throw method_not_allowed();
  }
  if (r.is_cgi_request(msg.get_request_path())) {
    process_cgi(msg, addr);
    return ;
  }
  else if (type == "GET") {
    process_get_request(msg, addr);
  }
  else if (type == "POST") {
    process_post_request(msg);
  }
  else {    
    process_error(msg, 501);
  }
}

void http_server::process_get_request(http_request& req, sockaddr_in addr)
{
  try {
    http_location &r = get_location_from_target(req.get_request_path());
    string file_name = r.get_file_name(req.get_request_path());
    serv_log(string("Path of location: '") + r.get_path() + "'");
    serv_log(string("Requested file: '") + file_name + "'");
    http_responce resp(200);
    resp.set_target_name(file_name);
    resp.set_socket(req.get_socket());
    resp.write_responce();
  }
  catch(http_location::directory_uri &e) {
    string t = req.get_header_value("host").second;
    if (t[t.length() - 1] == '/')
      t = t.substr(0, t.length() - 1);
    t += req.get_request_path();
    t += "/";
    t = string("http://") + t;
    process_redirect(req, 301, t);
  }
  catch(http_location::not_found &e) {
    process_error(req, 404);
  }
  catch(http_responce::target_not_found &e) {
    process_error(req, 404);
  }
  catch(http_responce::target_denied &e) {
    process_error(req, 403);
  }
  catch(exception &e) {
    serv_log(string("Something went terribly wrong during get request: ") + e.what());
  }
}

const char** http_server::compose_cgi_envp(http_request& req, sockaddr_in addr)
{
  http_location &r = get_location_from_target(req.get_request_path());
  string script_name = r.get_uri_full_path(req.get_request_path());
  list<string> args;
  args = req.get_cgi_header_values();
  args.push_back(string("DOCUMENT_ROOT=") + r.get_root());
  if (req.get_body_size() != 0) {
    args.push_back(string("CONTENT_LENGTH=")
		   + convert_to_string(req.get_body_size()));
  }
  args.push_back(string ("QUERY_STRING=") + req.get_request_query());
  args.push_back("SERVER_SOFTWARE=Eugene_server 0.1");
  args.push_back(string("REQUEST_METHOD=") + req.get_method());
  args.push_back("SERVER_PROTOCOL=HTTP/1.1");
  //TODO: add adequate path info handler
  //  args.push_back(string("PATH_INFO"));// + req.get_request_target());
  // char pbuf[100];
  // getcwd(pbuf, 100);
  
  // args.push_back(string("PATH_TRANSLATED=") + pbuf + r.get_root() + req.get_request_target());
  const char *fsn = realpath(strdup(r.get_file_name(req.get_request_path()).c_str()), NULL);
  string full_script_name = fsn;
  args.push_back(string("PATH_TRANSLATED=") + full_script_name);
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
  args.push_back(string("SCRIPT_NAME=") + script_name);
  // args.push_back(string("SCRIPT_NAME=/"));//+ req.get_request_target());
  args.push_back("GATEWAY_INTERFACE=CGI/1.1");
  if (req.get_header_value("Content-Type").first) {
    args.push_back(string("CONTENT_TYPE=") +
		   req.get_header_value("Content-Type").second);
  }
  const char **vv = make_argument_vector(args);
  return vv;
}

void http_server::process_cgi(http_request& req, sockaddr_in addr)
{
  serv_log("Processing cgi");
  int fd1[2];
  int fd2[2];
  int r1, r2;
  r1 = pipe(fd1);
  r2 = pipe(fd2);
  if (r1 < 0 && r2 < 0) {
    serv_log(string ("Pipe ERROR ") + strerror(errno));
    throw internal_error();
  }
  pid_t pid = fork();
  if (pid < 0) {
    serv_log(string("Fork error ") + strerror(errno));
    throw internal_error();
  }
  if (pid == 0) {
    close(fd1[1]);
    close(fd2[0]);
    try {
      http_location &r = get_location_from_target(req.get_request_path());
      const char **vv = compose_cgi_envp(req, addr);
      //make it a fucking path
      const char **k = make_argument_vector(r.cgi_path(req.get_request_path()));
      // k[0] = strdup(r.cgi_path(req.get_request_path()).c_str());
      // k[1] = NULL;
      // k[1] = "-f";
      // k[2] = "/home/eugene/school/server/html/test.php";
      // k[3] = NULL;
      const char** arg;
      // cerr << "Argv:\n";
      // for (arg = k; *arg != NULL; arg++) {
      // 	cerr << *arg << endl;
      // }
      // cerr << "Envp:\n";
      // for (arg = vv; *arg != NULL; arg++) {
      // 	cerr << *arg << endl;
      // }
      // const char *fn = strdup(r.cgi_path(req.get_request_path()).c_str());
      dup2(fd1[0], 0);
      dup2(fd2[1], 1);
      if (chdir(r.get_root().c_str()) < 0) {
	cerr << "Cannot change directory to " << r.get_root() <<
	  "'" << strerror(errno) << "'" << endl;
	exit(1);
      }
      cerr << "PHP: EXECUTING\n";
      if (execve(k[0], (char* const *)k,  (char* const*)vv) < 0) {
	cerr << "PHP: EXECVE_FAIL\n";
	cerr << "Error happened during execve: " << strerror(errno) << endl;
	exit(1);
      }
    }
    catch(exception &e) {
      cerr << string("Something went wrong during execution of cgi script: ") + e.what() << endl;
      exit(1);
    }
    exit(0);
  }
  else {
    close(fd1[0]);
    close(fd2[1]);
    try {
      if (req.get_body_size() != 0) {
	req.print_body_into_fd(fd1[1]);
      }
      close(fd1[1]);
      http_responce resp;
      resp.set_socket(req.get_socket());
      resp.set_cgi_fd(fd2[0]);
      resp.handle_cgi();
    }
    catch(exception &e) {
      close(fd1[1]);
      close(fd2[0]);
      serv_log("Something bad happened while executing CGI\n");
    }
    close(fd2[0]);
    int status;
    wait(&status);
    if (WIFEXITED(status)) {
      serv_log("CGI process exited normally");
    }
    else {
      serv_log("CGI process exited abnormaly");
      if (WIFSIGNALED(status)) {
	int sign = WTERMSIG(status);
	serv_log(string("Cause of exit: SIGNAL ")
		 + convert_to_string(sign)
		 + " (" + strsignal(sign) + ")");
      }
      throw internal_error();
    }
  }
}

void http_server::process_redirect(http_request &in, int status, string target)
{
  serv_log(string("Processing redirect from '") + in.get_request_path()
	   + "' to '" + target + "'");
  try {
    http_responce resp(status);
    resp.set_socket(in.get_socket());
    resp.add_header_field("Location", target);
    resp.write_responce();
  }
  catch (exception &e) {
    serv_log(string("Something went wrong: ") + e.what());    
  }
}

void http_server::process_error(http_request &in, int status)
{
  serv_log(string("Processing error '") + convert_to_string(status) + "' on target '"
	   + in.get_request_target() + "'");
  try {    
    http_responce resp(status);
    resp.set_socket(in.get_socket());
    if (status == 405) {
      http_location &r = get_location_from_target(in.get_request_path());
      string t = r.compose_allowed_methods();
      resp.add_header_field("Accept", t);
    }
    string err_target = get_error_target_name(in.get_request_path());    
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

//Should be removed

// void http_server::process_not_found(http_request &req)
// {
//   http_responce resp(404);
//   resp.set_socket(req.get_socket());
//   resp.write_responce();
// }

// void http_server::send_status_code(http_request &req, int code)
// {
//   if (code == 404)
//     process_not_found(req);
// }

// void http_server::send_timeout(int fd)
// {
// }

bool http_server::has_socket(int fd)
{
  deque<int>::const_iterator it;
  for (it = sock_fds.begin(); it != sock_fds.end(); it++) {
    if (*it == fd)
      return true;
  }
  for (it = active_fds.begin(); it != active_fds.end(); it++) {
    if (*it == fd)
      return true;
  }
  return false;
}

deque<int> http_server::get_sockets() const
{
  deque<int> ret;
  ret.insert(ret.begin(), sock_fds.begin(), sock_fds.end());
  ret.insert(ret.begin(), active_fds.begin(), active_fds.end());
  return ret;
}

bool http_server::is_cgi_request(string target_name)
{
  http_location &r = get_location_from_target(target_name);
  if (r.is_cgi_request(target_name))
    return true;
  return false;
}

void http_server::process_post_request(http_request &req)
{
  // if (is_cgi_request(req.get_request_target())) {
  //   process_cgi(req);
  // }
}

http_location& http_server::get_location_from_target(string s)
{
  static string inp;
  static list<http_location>::iterator ret;
  if (inp == s)
    return *ret;
  inp = s;
  size_t max = 0;
  list<http_location>::iterator it;
  ret = locations.end();
  for (it = locations.begin(); it != locations.end(); it++) {
    if (it->is_located(s)) {
      size_t tm = it->get_path_depth();
      if (tm > max) {
	max = tm;
	ret = it;
      }
    }
  }
  if (ret == locations.end()) {
    serv_log(string("Cannot find appropriate path for target ") + s);
    throw invalid_target();
  }
  return *ret;
}

void http_server::check_location_for_correctness(http_location in)
{
  //TODO
}

void http_server::add_location(http_location in)
{
  locations.push_back(in);
}


