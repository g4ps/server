#include "http_webserv.hpp"
#include <string>


http_webserv::http_webserv()
{
}

void http_webserv::start()
{
  vector<int> sockets;
  list<http_server>::iterator it;
  for (it = servers.begin(); it != servers.end(); it++) {
    deque<int> &temp = it->get_sockets();
    sockets.insert(sockets.begin(), temp.begin(), temp.end());
  }
  pollfd *fdarr = new pollfd[sockets.size()];
  while (1) {
    for (size_t i = 0; i < sockets.size(); i++) {
      fdarr[i].fd = sockets[i];
      fdarr[i].events = 0;
      fdarr[i].events |= POLLRDNORM;
    }
    if (poll(fdarr, sockets.size(), -1) == -1) {
      serv_log(string("poll ERROR: ") + strerror(errno));
      throw process_error();
    }
    for (size_t i = 0; i < sockets.size(); i++) {
      if (fdarr[i].revents & POLLERR) {
	serv_log(string("poll ERROR: ") + strerror(errno));
	throw process_error();
      }
      if (fdarr[i].revents & POLLRDNORM) {
	sockaddr_in addr;
	socklen_t address_size = sizeof(sockaddr_in);
	int ns = accept(fdarr[i].fd, (sockaddr*)&addr, &address_size);
	if (fcntl(ns, F_SETFL, O_NONBLOCK) == -1) {
	  serv_log(string("fcntl error: ") + strerror(errno));
	  throw process_error();
	}
	if (ns == -1) {
	  serv_log(string("accept error: ") + strerror(errno));
	  throw process_error();
	}
	http_server& corr = find_server_with_socket(fdarr[i].fd);
	char cbuf[100];
	stringstream s;
	s << "Connection established on fd (" << ns << ")";
	s << " from host "
	  << inet_ntop(AF_INET,
		       &(addr.sin_addr.s_addr), cbuf, address_size)
	  << ":" << ntohs(addr.sin_port);
	serv_log(s.str());	
	corr.serve(ns, addr);
      }
    }
  }
}

void http_webserv::add_server(http_server s)
{
  servers.push_back(s);
}

http_server& http_webserv::find_server_with_socket(int fd)
{
  list<http_server>::iterator it;
  for (it = servers.begin(); it != servers.end(); it++) {
    if (it->has_socket(fd)) {
      return *it;
    }
  }
  throw process_error();
  it--;  
  return *it;
}
