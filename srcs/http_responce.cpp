#include <string>
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>
#include <cstring>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <poll.h>
#include <cerrno>

#include "http_server.hpp"
#include "http_message.hpp"
#include "http_responce.hpp"
#include "http_utils.hpp"



http_responce::http_responce()
{
  cgi_fd = -1;
  http_version = "HTTP/1.1";
};

http_responce::http_responce(int st)
{
  cgi_fd = -1;
  set_status(st);
}

void http_responce::set_status(int st)
{
  http_version = "HTTP/1.1";
  status_code = st;
  switch(st) {
  case 200:
    reason_phrase = "OK";
    break;
  case 201:
    reason_phrase = "Created";
    break;
  case 202:
    reason_phrase = "Accepted";
    break;
  case 204:
    reason_phrase = "No Content";
    break;
  case 301:
    reason_phrase = "Moved Permanently";
    break;
  case 302:
    reason_phrase = "Found";
    break;
  case 303:
    reason_phrase = "See Other";
    break;
  case 305:
    reason_phrase = "Use proxy";
    break;
  case 307:
    reason_phrase = "Temporary Redirect";
    break;
  case 400:
    reason_phrase = "Bad Request";
    break;
  case 403:
    reason_phrase = "Forbidden";
    break;
  case 404:
    reason_phrase = "Not Found";
    break;
  case 405:
    reason_phrase = "Method Not Allowed";
    break;
  case 500:
    reason_phrase = "Internal Server Error";
    break;
  case 501:
    reason_phrase = "Not Implemented";
    break;
  default:
    serv_log(string("Cannot set status ") + convert_to_string(st));
    throw bad_status();
    break;
  }
}

void http_responce::set_body(string s)
{
  body.clear();
  body.insert(body.begin(), s.begin(), s.end());
}

void http_responce::write_responce()
{
  if (target_name.size() == 0) {
    //Some static stuff, that we had put into the body
    //Is used by default error pages and such
    int ret;
    pollfd pfd;
    // add_header_field("Connection", "close");
    // if (body.size() != 0)
      add_header_field("Content-length", convert_to_string(body.size()));
    // out << http_version << " " << status_code << " " << reason_phrase << "\r\n";
    // out << compose_header_fields();
    // string head = out.str() + "\r\n";
    // //make it non-block and with polling
    // write(sock_fd, head.c_str(), head.length());
    write_head();
    char *buf = new char[BUFSIZ];
    while (body.size() != 0) {
      pfd.fd = sock_fd;
      pfd.events = 0 | POLLOUT;
      if ((ret = poll(&pfd, 1, 5000)) == 0) {
	serv_log("ERROR: Timeout during writing responce");
	throw http_request::timeout_error();
      }
      else if (ret < 0) {
	serv_log(string ("Poll ERROR: ") + strerror(errno));
	throw http_server::internal_error();
      }
      if (pfd.revents & POLLERR) {
	serv_log(string ("Poll ERROR: ") + strerror(errno));
	throw http_server::internal_error();
      }
      size_t n;
      n = BUFSIZ;
      if (body.size() < BUFSIZ)
	n = body.size();
      copy(body.begin(), body.begin() + n, buf);
      write(sock_fd, buf, n);
      body.erase(body.begin(), body.begin() + n);
    }
    delete[] buf;
  }
  else {
    // add_header_field("Connection", "close");
    //Just the static pages
    int fd = open(target_name.c_str(), O_RDONLY);
    //Add check, that target_name doesn't point on a directory!!!!!!!!!!
    if (fd < 0) {
      if (errno == EACCES) {
	serv_log(string("Access to target \'") + target_name + "\' was denied");
	throw target_denied();
      }
      else if (errno == ENOENT) {
	serv_log(string("Couldn't find target \'") + target_name + "\'");
	throw target_not_found();
      }
      else {
	serv_log(string("open SERVER ERROR: ") + strerror(errno));
	throw server_error();
      }
    }
    char *buf = new char[BUFSIZ];
    int n;
    pollfd pfd;
    struct stat st;
    stat(target_name.c_str(), &st);
    off_t file_size = st.st_size;
    if (file_size != 0) {
      add_header_field("Content-length", convert_to_string(file_size));
      write_head();
      if (lseek(fd, 0, SEEK_SET) < 0) {
	serv_log(string("lseek SERVER ERROR: ") + strerror(errno));
	close(fd);
	throw server_error();
      }
      off_t ret;
      while ((ret = lseek(fd, 0, SEEK_CUR)) != file_size) {
	if (ret < 0) {
	  serv_log(string("lseek SERVER ERROR: ") + strerror(errno));
	  throw server_error();
	}
	pfd.fd = fd;
	pfd.events = 0 | POLLIN;
	poll(&pfd, 1, 0);
	if (pfd.revents & POLLERR) {
	  serv_log(string("poll SERVER ERROR: ") + strerror(errno));
	  close(fd);
	  throw server_error();
	}
	ssize_t n;
	ssize_t out_pace;
	ssize_t temp = 0;
	n = read(fd, buf, BUFSIZ);
	while (n != 0) {
	  out_pace = 0;
	  pfd.fd = sock_fd;
	  pfd.events = 0 | POLLOUT;
	  poll(&pfd, 1, 5000); //should be replaced by server variable
	  if (pfd.revents & POLLERR) {
	    serv_log(string("poll SERVER ERROR: ") + strerror(errno));
	    close(fd);
	    throw server_error();
	  }
	  out_pace = write(sock_fd, buf + temp, n);
	  if (out_pace < 0) {
	    serv_log(string("write into socket failed: ") + strerror(errno));
	    close(fd);
	    throw server_error();
	  }
	  temp += out_pace;
	  n -= out_pace;
	}
      }
      close(fd);
    }
    else {
      add_header_field("Content-length", "0");
      write_head();
    }
    delete[] buf;
  }
}


void http_responce::set_target_name(string s)
{
  target_name = s;  
}

string http_responce::get_target_name() const
{
  return target_name;
}

void http_responce::write_head()
{
  stringstream out;
  out << http_version << " " << status_code << " " << reason_phrase << "\r\n";
  out << compose_header_fields();
  string head = out.str() + "\r\n";
  //make it non-block and with polling
  pollfd pfd;
  ssize_t n;
  while (head.size() != 0) {
    pfd.fd = sock_fd;
    pfd.events = 0 | POLLOUT;
    poll(&pfd, 1, 5000); //should be replaced by server variable
    if (pfd.revents & POLLERR) {
      serv_log(string("poll SERVER ERROR: ") + strerror(errno));
      throw server_error();
    }
    n = write(sock_fd, head.c_str(), head.length());
    if (n < 0) {
      serv_log(string("write into socket error: ") + strerror(errno));
      throw server_error();
    }
    head.erase(head.begin(), head.begin() + n);
  }
}

void http_responce::print() const
{
  cout << "Responce:\n";
  cout << "HTTP version: \'" << http_version << "\'" << endl;
  cout << "Status: '" << status_code << "'" << endl;
  cout <<"Reason: \'" << reason_phrase << "\'" << endl;
  for (multimap<string, string>::const_iterator i = header_lines.begin(); i != header_lines.end(); i++) {
    cout << i->first <<": " << i->second<< "\r\n";
  }
  string msg_body(body.begin(), body.end());
  // if (msg_body.length() != 0) 
  //   cout<<msg_body<<std::endl;
}


void http_responce::set_cgi_fd(int fd)
{
  cgi_fd = fd;
}

void http_responce::handle_cgi()
{
  string head_end = "\r\n\r\n";
  //add poll
  char buf[512];
  ssize_t n;
  while (read_nb_block(BUFSIZ, cgi_fd) > 0)
    ;
  string inp;
  inp.insert(inp.end(), raw.begin(), raw.end());
  // cout << "Cgi return: " << inp << endl;
  parse_header_fields(inp);
  if (get_header_value("status").first) {
    string t = get_header_value("status").second;
    stringstream ss;
    ss << t;
    int st;
    ss >> st;
    set_status(st);
  }
  else {
    set_status(200);
  }
  body.assign(raw.begin() + msg_body_position(), raw.end());  
  // print();
  write_responce();
}
