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
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>

#include "parse_help.hpp"
#include "http_message.hpp"
#include "server.hpp"

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

void http_server::start()
{
  size_t num = num_of_sockets();
  pollfd *fdarr = new pollfd[num];
  while (1) {
    for (size_t i = 0; i < num; i++) {
      fdarr[i].fd = sock_fds[i];
      fdarr[i].events = 0;
      fdarr[i].events |= POLLRDNORM;
    }
    if (poll(fdarr, num, -1) == -1) {
      serv_log(string("poll ERROR: ") + strerror(errno));
      throw process_error();
    }
    for (size_t i = 0; i < num; i++) {
      if (fdarr[i].revents & POLLERR) {
	serv_log(string("poll ERROR: ") + strerror(errno));
	throw process_error();
      }
      if (fdarr[i].revents & POLLRDNORM) {
	int ns = accept(fdarr[i].fd, NULL, NULL);
	if (fcntl(ns, F_SETFL, O_NONBLOCK) == -1) {
	  serv_log(string("fcntl error: ") + strerror(errno));
	  throw process_error();
	}
	if (ns == -1) {
	  serv_log(string("accept error: ") + strerror(errno));
	  throw process_error();
	}
	serv_log("Connection established");
	http_server::serve(ns);
      }
    }
  }
}


void http_server::serve(int fd)
{
  string inp;
  char buf[BUFSIZ];
  bzero(buf, BUFSIZ);
  ssize_t n;
  pollfd fdarr;
  fdarr.fd = fd;
  fdarr.events = 0;
  fdarr.events |= POLLIN;
  while (1) {
    if (poll(&fdarr, 1, 5000) <= 0)
      break;
    if (fdarr.revents & POLLERR) {
      serv_log(string("poll ERROR: ") + strerror(errno));
      throw process_error();
    }
    else if (!(fdarr.revents & POLLIN)) {
      serv_log(string("poll ERROR: ") + strerror(errno));
      throw process_error();
    }
    n = read(fd, buf, 4);
    if (n < 0) {
      serv_log("Read error: ");
      #ifdef NOT_SHIT
      serv_log(strerror(errno));
      #endif
      throw process_error();
      return ;
    }
    inp.append(buf, n);
    if (inp.find("\r\n\r\n") != string::npos)
      break;
  }
  try {
    http_message req(inp);
    req.print();
    string kk = test_page();
    kk.length();
    write(fd, kk.c_str(), kk.length());
  }
  catch (exception &e) {
    cout<<"ERROR: "<<e.what()<<endl;
  }
  close(fd);
  serv_log("Connection closed");
}
