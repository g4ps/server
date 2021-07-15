#include "http_webserv.hpp"
#include "http_utils.hpp"
#include "http_server.hpp"
#include <fstream>
#include <string>
#include <cstdlib>


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
	ssize_t i = 0;
	while (i < str.length())
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

unsigned short create_port(string addr)
{
	if (addr.length() < 1 || addr.length() > 5)
	{
		serv_log(string("ERROR: wrong port number at line: ") +
				 convert_to_string(line_num));
		throw http_webserv::invalid_config();
	}
	for (ssize_t j = 0; j < addr.length(); j++)
	{
		if (!isdigit(addr[j]))
		{
			serv_log(string("ERROR: wrong port number at line: ") +
					 convert_to_string(line_num));
			throw http_webserv::invalid_config();
		}
	}
	int port = atoi(addr.c_str());
	if (port >= 65536)
	{
		serv_log(string("ERROR: wrong port number at line: ") +
				 convert_to_string(line_num));
		throw http_webserv::invalid_config();
	}
	return port;
}

void parse_addport(http_server &ret, string res, string ins)
{
	string addr = get_conf_token(res);
	if (addr == "")
	{
		serv_log(string("ERROR: no argument at line: ") +
				 convert_to_string(line_num));
		throw http_webserv::invalid_config();
	}
	skip_ows(res);
	if (res != "")
	{
		serv_log(string("ERROR: extra info at line: ") +
				 convert_to_string(line_num));
		throw http_webserv::invalid_config();
	}
	cout << ins << ": " << addr << endl;
	if (ins == "port")
		ret.add_socket("0.0.0.0", create_port(addr));
	else {
		unsigned short port = 8000;
		ssize_t d = addr.find(":");
		if (d != string::npos)
			port = create_port(addr.substr(d + 1));
		ret.add_socket_from_hostname(addr.substr(0, d), port);
	}
}

http_location parse_location(ifstream &file)
{
	http_location ret;
	bool open_brace = true;
	string res;
	while(!file.eof())
	{
		getline(file, res);
		line_num++;
		if (file.eof() && res == "")
		{
			serv_log(string("ERROR: unexpected end of file on line: ") +
					 convert_to_string(line_num));
			throw http_webserv::invalid_config();
		}
		if (res.find("#") != string::npos)
			res = res.substr(0, res.find("#"));
		if (zero_str(res))
			continue;
		trim_wsp(res);
		if (open_brace)
		{
			if (res != "{")
			{
				serv_log(string("ERROR: expected opened brace at line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			open_brace = false;
			continue;
		}
		if (res == "}")
			return ret;
		string ins = get_conf_token(res);
		if (ins == "path")
		{
			string p = get_conf_token(res);
			skip_ows(res);
			if (res != "")
			{
				serv_log(string("ERROR: extra information on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			if (p == "")
			{
				serv_log(string("ERROR: path is not supplied on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			ret.set_path(p);
		}
		else if (ins == "root")
		{
			string p = get_conf_token(res);
			if (p == "")
			{
				serv_log(string("ERROR: path is not supplied on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			if (res != "")
			{
				serv_log(string("ERROR: extra information on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			ret.set_root(p);
		}
		else if (ins == "index")
		{
			string p;
			while ((p = get_conf_token(res)) != "")
				ret.add_index(p);
		}
		else if (ins == "method")
		{
			string p;
			while ((p = get_conf_token(res)) != "")
				ret.add_method(p);
		}
		else if (ins == "cgi")
		{
			string ext;
			ext = get_conf_token(res);
			ret.add_cgi(ext, res);
		}
		else
		{
			serv_log(string ("ERROR: invalid instruction on line: ") +
			convert_to_string(line_num));
			throw http_webserv::invalid_config();
		}
	}
	return ret;
}

http_server parse_server(ifstream& file)
{
	http_server ret;
	string res;
	bool open_brace = true;
	while(!file.eof())
	{
		getline(file, res);
		line_num++;
		if (file.eof() && res == "")
		{
			serv_log(string("ERROR: unexpected end of file on line: " ) +
					 convert_to_string(line_num));
			throw http_webserv::invalid_config();
		}
		if (res.find("#") != string::npos)
			res = res.substr(0, res.find("#"));
		if (zero_str(res))
			continue;
		trim_wsp(res);
		if (open_brace)
		{
			if (res != "{")
			{
				serv_log(string("ERROR: expected opened brace at line: ") +
				convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			open_brace = false;
		}
		else if (res == "}")
			return ret;
		else
		{
			string ins = get_conf_token(res);
			if (ins == "address" || ins == "port")
				parse_addport(ret, res, ins);
			else if (ins == "location")
			{
				ret.add_location(parse_location(file));
			}
			else
			{
				serv_log(string("ERROR: illegal instruction at line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
		}
	}
	return ret;
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
		cout << res << endl;
		if (res == "server")
			add_server(parse_server(file));

	}
}
