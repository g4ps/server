#include <string>
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

#include "http_message.hpp"
#include "http_responce.hpp"
#include "http_utils.hpp"


http_responce::http_responce()
{
  http_version = "HTTP/1.1";
};

http_responce::http_responce(int st)
{
  http_version = "HTTP/1.1";
  status_code = st;
  switch(st) {
  case 200:
    reason_phrase = "OK";
    break;
  case 404:
    reason_phrase = "Not Found";
    break;
  case 400:
    reason_phrase = "Bad Request";
    break;
  case 501:
    reason_phrase = "Not Implemented";
    break;
  default:
    serv_log(string("Cannot set status ") + convert_to_string(st));
    throw bad_status();
    break;
  }
};

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
    add_header_field("Connection", "closed");
    if (body.size() != 0)
      add_header_field("Content-length", convert_to_string(body.size()));
    // out << http_version << " " << status_code << " " << reason_phrase << "\r\n";
    // out << compose_header_fields();
    // string head = out.str() + "\r\n";
    // //make it non-block and with polling
    // write(sock_fd, head.c_str(), head.length());
    write_head();
    char *buf = new char[BUFSIZ];
    while (body.size() != 0) {
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
  //CGI Should be processed somewhere here
  else {
    add_header_field("Connection", "closed");
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
      add_header_field("Connection", "closed");
      add_header_field("Content-length", 0);
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
