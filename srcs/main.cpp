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
#include "http_server.hpp"
#include "http_webserv.hpp"
#include "http_utils.hpp"

using namespace std;


/*
  function: get_address_from_hostname returns sockaddr_in (ipv4) for 
  hostname, supplied in the host argument

  returns: NULL in case of an error and free-able sockaddr_info if
  successful

  comment: I know, that we have gataddrinfo for the sole purpose
  of getting adderess from hostname, in this function i just
  add all the neccesary hints, bells and whistles in it;
 */
// struct sockaddr_in*
// get_address_from_hostname(const char *host, unsigned short port)
// {
//   struct addrinfo hints;
//   memset(&hints, 0, sizeof(addrinfo));
//   hints.ai_family = AF_INET;
//   struct addrinfo *res = NULL;
//   struct addrinfo *i = NULL;
//   struct addrinfo *tmp = NULL;
//   int err = getaddrinfo(host, NULL, &hints, &res);
//   if (err != 0) {
//     serv_log(gai_strerror(err));
//     return NULL;
//   }
//   struct sockaddr_in *p = NULL;
//   for (i = res; i != NULL; i++) {
//     if (i->ai_family == AF_INET) {
//       p = (sockaddr_in *)res->ai_addr;
//       break;
//     }
//   }
//   if (p == NULL) {
//     return NULL;
//   }
//   struct sockaddr_in* ret = new sockaddr_in;
//   memcpy(ret, p, sizeof(sockaddr_in));
//   ret->sin_port = htons(port);
//   freeaddrinfo(res);
//   return ret;
//   //С++ maybe exceptions
// }

/*
  function: init_serv

  returns: fd for socket or -1

  comment: returns socket fd with all the neccesary options
  and already listening
 */

// int init_serv(struct sockaddr_in *addr)
// {
//   int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//   if (sock_fd < 0) {
//     serv_log("ERROR: socket shat the bed");
//     serv_log(strerror(errno));
//     return -1;
//   }
//   int reuse = 1;
//   if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
//     serv_log("ERROR: setsockopt fucked up");
//     serv_log(strerror(errno));
//     return -1;
//   }
//   if (bind(sock_fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) == -1) {
//     serv_log("ERROR: bind fucked up");
//     serv_log(strerror(errno));
//     return -1;
//   }
//   if (listen(sock_fd, 10) == -1) {
//     serv_log("ERROR: listen fucked up");
//     serv_log(strerror(errno));
//     return -1;
//   }
//   serv_log("Success");
//   return sock_fd;
//   //C++ exceptions
// }

int
main(int argc, char **argv)
{
	http_webserv n;
	n.parse_config("config.ang");
	n.start();
	exit(0);
  serv_log("Eugene's HTTP server v0.1");
  unsigned short port = 8001;
  std::string host = "localhost";
  if (argc == 2) {
    host = argv[1];
  }
  http_webserv w1;
  serv_log("Initializing server:");
  serv_log("----------------------------------------");
  try {
    http_server s1;
    s1.add_socket("127.0.0.1", 8001);
    http_server s2;
    http_location l1;
    l1.add_method("GET");
    l1.add_method("POST");
    l1.set_path("/");
    //    l1.set_root("/home/eugene/my_quiz");
    l1.set_root("/home/eugene/nquiz/");
    l1.add_index("index.html");
    l1.add_index("index.php");
    l1.add_cgi(".php", "/usr/bin/php-cgi");
    http_location l2;
    l2.set_root("html/");
    l2.set_path("/another/");
    l2.add_index("index.html");
    l2.add_method("GET");
    l2.add_method("POST");
    http_location l3;
    l3.set_root("/usr/share/webapps/phpMyAdmin/");
    l3.add_method("GET");
    l3.add_method("POST");    
    l3.add_index("index.php");
    l3.add_cgi(".php", "/usr/bin/php-cgi");
    l3.set_path("/phpmyadmin/");
    s2.add_socket("0.0.0.0", 8002);
    s2.add_location(l2);
    s2.add_location(l1);
    s1.add_location(l1);
    s1.add_location(l2);
    s2.add_location(l3);
    w1.add_server(s1);
    w1.add_server(s2);
  }
  catch(exception &e) {
    serv_log(string("INIT ERROR: ") + e.what());
    exit(1);
  }
  serv_log("----------------------------------------");
  serv_log("STARTING");
  try {
    w1.start();
  }
  catch(exception &e) {
    serv_log(string("RUNTIME ERROR: ") + e.what());
    exit(1);
  }
  return 0;
}
