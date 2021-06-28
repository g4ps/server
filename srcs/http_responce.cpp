#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstdio>

#include "http_message.hpp"
#include "http_responce.hpp"


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
  default:
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
  stringstream out;
  if (body.size() != 0)
    add_header_field("Content-length", convert_to_string(body.size()));
  out << http_version << " " << status_code << " " << reason_phrase << "\r\n";
  out << compose_header_fields();
  string head = out.str() + "\r\n";
  //make it non-block and with polling
  write(sock_fd, head.c_str(), head.length());
  char *buf = new char[BUFSIZ];
  while (body.size() != 0) {
    int n;
    for (n = 0; n < BUFSIZ && n < body.size(); n++) {
      buf[n] = body[n];
    }
    write(sock_fd, buf, n);
    body.erase(body.begin(), body.begin() + n);
  }
  delete[] buf;
}
