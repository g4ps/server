#include "http_webserv.hpp"
#include "http_utils.hpp"
#include "http_server.hpp"
#include <fstream>
#include <string>
#include <cstdlib>
#include <pthread.h>


http_webserv::http_webserv()
{
}

void http_webserv::start()
{
  while (1) {
    deque<int> sockets;
    list<http_server>::iterator it;
    for (it = servers.begin(); it != servers.end(); it++) {
      deque<int> temp = it->get_sockets();
      sockets.insert(sockets.begin(), temp.begin(), temp.end());
    }
    pollfd *fdarr = new pollfd[sockets.size()];
    for (size_t i = 0; i < sockets.size(); i++) {
      fdarr[i].fd = sockets[i];
      fdarr[i].events = 0;
      fdarr[i].events |= POLLRDNORM;
    }    
    if (poll(fdarr, sockets.size(), -1) == -1) {
      char *str = strerror(errno);
      serv_log(string("ERROR: poll return POLLERR: ") + str);
      throw process_error();
    }
    for (size_t i = 0; i < sockets.size(); i++) {
      if (fdarr[i].revents & POLLERR) {
	serv_log(string("ERROR: fd ") + convert_to_string(fdarr[i].fd) + " returned POLLERR");
	http_server& corr = find_server_with_socket(fdarr[i].fd);
	if (corr.is_active_connection(fdarr[i].fd)) {
	  serv_log("It is an active connection; closing...");
	  corr.remove_active_connection(fdarr[i].fd);
	}
      }
      else if (fdarr[i].revents & POLLHUP) {
	serv_log(string("Hungup on descriptor " ) + convert_to_string(fdarr[i].fd));
	http_server& corr = find_server_with_socket(fdarr[i].fd);
	if (corr.is_active_connection(fdarr[i].fd)) {
	  corr.remove_active_connection(fdarr[i].fd);
	  cout << "Closed errored connection: " << fdarr[i].fd << endl;
	}
      }
      else if (fdarr[i].revents & POLLRDNORM) {
	serv_log("----------------------------------------");
	serv_log("New Connection");	
	sockaddr_in addr;
	socklen_t address_size = sizeof(sockaddr_in);
	http_server& corr = find_server_with_socket(fdarr[i].fd);
	int ns;
	if (!corr.is_active_connection(fdarr[i].fd)) {
	  ns = accept(fdarr[i].fd, (sockaddr*)&addr, &address_size);
	  if (ns < 0) {
	    serv_log(string("accept error: ") + strerror(errno));
	    throw process_error();
	  }
	  if (fcntl(ns, F_SETFL, O_NONBLOCK) == -1) {
	    serv_log(string("fcntl error: ") + strerror(errno));
	    throw process_error();
	  }
	  char cbuf[100];
	  stringstream s;
	  s << "Connection established on fd (" << ns << ")";
	  s << " from host "
	    << inet_ntop(AF_INET,
			 &(addr.sin_addr.s_addr), cbuf, address_size)
	    << ":" << ntohs(addr.sin_port);
	  serv_log(s.str());
	  corr.add_active_connection(ns, addr);
	  continue;
	}
	else {
	  http_connection c = corr.get_active_connection(fdarr[i].fd);
	  ns = c.get_fd();
	  addr = c.get_addr();
	}
	int keep_alive = corr.serve(ns, addr);
	if (keep_alive <= 0)
	  corr.remove_active_connection(ns);
	serv_log("----------------------------------------");
      }
      else if (fdarr[i].revents & POLLNVAL) {
	serv_log(string("ERROR: incorrect fd ") + convert_to_string(fdarr[i].fd));
	http_server& corr = find_server_with_socket(fdarr[i].fd);
	corr.remove_active_connection(fdarr[i].fd);
      }

    }
    delete [] fdarr;
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
  serv_log("Addling location");
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
			serv_log("Location path: " + p);
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
			serv_log("Location root: " + p);
		}
		else if (ins == "index")
		{
		  string out;
			string p;
			while ((p = get_conf_token(res)) != "") {
			  out += string(" ") + p;
			  ret.add_index(p);
			}
			serv_log(string("Indexing files:") + out);
		}
		else if (ins == "method")
		{
		  string out;
			string p;
			while ((p = get_conf_token(res)) != "") {
			  res += string(" ") + out;
			  ret.add_method(p);
			}
			serv_log(string("Allowed methods:") + out);
		}
		else if (ins == "cgi")
		{
			string ext;
			ext = get_conf_token(res);
			ret.add_cgi(ext, res);
			serv_log(string("Processing extention ") + ext + " with " + res);
		}
		else if (ins == "upload_folder") {
		  string f = get_conf_token(res);
		  skip_ows(res);
		  if (res != ""){
		    serv_log(string("ERROR: extra information on line: ") +
			     convert_to_string(line_num));
		    throw http_webserv::invalid_config();
		  }
		  ret.set_upload_folder(f);
		}
		else if (ins == "upload") {
		  string f = get_conf_token(res);
		  skip_ows(res);
		  if (res != ""){
		    serv_log(string("ERROR: extra information on line: ") +
			     convert_to_string(line_num));
		    throw http_webserv::invalid_config();
		  }
		  if (f == "on") {
		    ret.set_upload_accept(true);		    
		  }
		  else if (f == "off") {
		    ret.set_upload_accept(false);		    
		  }
		  else {
		    serv_log(string("ERROR: bad upload value at line: ") +
			     convert_to_string(line_num));
		    throw http_webserv::invalid_config();
		  }
		}
		else if (ins == "error_page")
		{
			string num = get_conf_token(res);
			string page = get_conf_token(res);
			if (!is_digit_string(num))
			{
				serv_log(string("ERROR: bad error value at line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			if (!does_exist(page))
			{
				serv_log(string("ERROR: error page does not exist at line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();

			}
			if (num.length() != 3)
			{
				serv_log(string("ERROR: invalid error status at line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}

			int status = atoi(num.c_str());
			ret.add_error_page(status, page);
		}
		else if (ins == "redirect") {
		  string num = get_conf_token(res);
		  string dir = get_conf_token(res);
		  int status = atoi(num.c_str());
		  if (!is_digit_string(num) || num.length() != 3 || status < 300 || status > 307) {
		    serv_log(string("ERROR: bad redirect value '") + num + "' at line "+
			     convert_to_string(line_num));
		    throw http_webserv::invalid_config();
		  }
		  ret.add_redirect(status, dir);
		  
		}
		else if (ins == "autoindex")
		{
			string b = get_conf_token(res);
			skip_ows(res);
			if (res != "")
			{
				serv_log(string("ERROR: extra information on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
			if (b == "off")
				ret.unset_autoindex();
			else if (b == "on")
				ret.set_autoindex();
			else
			{
				serv_log(string("ERROR: extra information on line: ") +
						 convert_to_string(line_num));
				throw http_webserv::invalid_config();
			}
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
  serv_log("Adding new server");
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
		if (res == "server")
			add_server(parse_server(file));

	}
}
