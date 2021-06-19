#include <string>
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

#include "parse_help.hpp"
#include "http_message.hpp"
#include "server.hpp"

using namespace std;

void serv_log(std::string out)
{
  std::cout<<out<<std::endl;
}

/*
  function: get_address_from_hostname returns sockaddr_in (ipv4) for 
  hostname, supplied in the host argument

  returns: NULL in case of an error and free-able sockaddr_info if
  successful

  comment: I know, that we have gataddrinfo for the sole purpose
  of getting adderess from hostname, in this function i just
  add all the neccesary hints, bells and whistles in it;
 */
struct sockaddr_in*
get_address_from_hostname(const char *host, unsigned short port)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_INET;
  struct addrinfo *res = NULL;
  struct addrinfo *i = NULL;
  struct addrinfo *tmp = NULL;
  int err = getaddrinfo(host, NULL, &hints, &res);
  if (err != 0) {
    serv_log(gai_strerror(err));
    return NULL;
  }
  struct sockaddr_in *p = NULL;
  for (i = res; i != NULL; i++) {
    if (i->ai_family == AF_INET) {
      p = (sockaddr_in *)res->ai_addr;
      break;
    }
  }
  if (p == NULL) {
    return NULL;
  }
  struct sockaddr_in* ret = new sockaddr_in;
  memcpy(ret, p, sizeof(sockaddr_in));
  ret->sin_port = htons(port);
  freeaddrinfo(res);
  return ret;
  //С++ maybe exceptions
}

/*
  function: init_serv

  returns: fd for socket or -1

  comment: returns socket fd with all the neccesary options
  and already listening
 */

int init_serv(struct sockaddr_in *addr)
{
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    serv_log("ERROR: socket shat the bed");
    serv_log(strerror(errno));
    return -1;
  }
  int reuse = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
    serv_log("ERROR: setsockopt fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  if (bind(sock_fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) == -1) {
    serv_log("ERROR: bind fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  if (listen(sock_fd, 10) == -1) {
    serv_log("ERROR: listen fucked up");
    serv_log(strerror(errno));
    return -1;
  }
  serv_log("Success");
  return sock_fd;
  //C++ exceptions
}

std::string
test_page()
{
  string head =
    string("HTTP/1.1 200 OK\r\n") +
    "Server: nginx/1.20.0\r\n" +
    "Date: Thu, 17 Jun 2021 20:30:59 GMT\r\n" +
    "Content-Type: text/html\r\n" +
    //"Transfer-Encoding: chunked\r\n" +
    "Connection: closed\r\n";

  string body = string("<html>\r\n") +
    "<head><title>My very own html resopnce</title></head>\r\n" +
    "<body>\r\n" +
    "<h1>  YEAH BABY</h1>\r\n" +
    "<hr></body>\r\n" +
    "</html>\r\n";
  stringstream ss;
  //ss << "Content-Length: " << body.length() << "\r\n\r\n";
  //head += ss.str();
  head += "\r\n";
  return head + body;
}

int
main(int argc, char **argv)
{
  serv_log("Eugene's HTTP server v0.1");
  unsigned short port = 8001;
  std::string host = "localhost";
  if (argc == 2) {
    host = argv[1];
  }
  serv_log("Initializing server:");
  serv_log("----------------------------------------");
  http_server s1;
  try {
    s1.add_socket_from_hostname("vc", 8001);
    s1.add_socket_from_hostname("vc", 8002);
    s1.add_socket("127.0.0.1", 8001);
    s1.add_socket("127.0.0.1", 8002);
  }
  catch(exception &e) {
    serv_log(string("INIT ERROR: ") + e.what());
    exit(1);
  }
  serv_log("----------------------------------------");
  serv_log("STARTING");
  try {
    s1.start();
  }
  catch(exception &e) {
    serv_log(string("RUNTIME ERROR: ") + e.what());
    exit(1);
  }
  return 0;
}
