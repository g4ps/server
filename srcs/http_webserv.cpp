#include "http_webserv.hpp"
#include "http_utils.hpp"
#include "http_server.hpp"
#include <fstream>
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

int line_num = 0;

int zero_str(string str)
{
	int i = 0;
	while (str[i] < str.length())
	{
		if (str[i] == '\n' || str[i] == ' ' || str[i] == '\t')
			i++;
		else
			return 0;
	}
	return 1;
}


void trim_wsp(string &str)
{
	ssize_t i = 0;
	while ((str[i] == '\t' || str[i] == ' ') && i < str.length())
		i++;
	if (i == str.length())
	{
		str = "";
		return ;
	}
	str = str.substr(i);
	i = str.length() - 1;
	while ((str[i] == '\t' || str[i] == ' ') && i > 0)
		i--;
	i++;
	str = str.substr(0, i);
}

http_server parse_server(ifstream& file)
{
	string res;
	bool open_brace = true;
	while(res != "}")
	{
		getline(file, res);
		line_num++;
		if (res.find("#") != string::npos)
			res = res.substr(0, res.find("#"));
		if (zero_str(res))
			continue;
		trim_wsp(res);
		if (open_brace)
		{
			if (res != "{")
			{
				serv_log(string("ERROR: expected opened brace at line") +
				convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			open_brace = false;
		}

	}
}

void http_webserv::parse_config(string filename)
{
  ifstream file(filename.c_str());
	string res;
	if (!file.good())
	{
		serv_log(string("ERROR: Incorrect config file: ") + filename);
		throw invalid_config();
	}
	while(!file.eof())
	{
		getline(file, res);
		line_num++;
		if (res.find("#") != string::npos)
			res = res.substr(0, res.find("#"));
		if (zero_str(res))
			continue;
		trim_wsp(res);
		if (res == "server")
			add_server(parse_server(file));

	}
}
