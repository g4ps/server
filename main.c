#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/*
  funciton: log

  returns: void

  comment: used for united logging of the whole server;
  the idea, is that if i will use some other log, then i will
  need to change only this function;
 */

void serv_log(const char *str)
{
  //С++
  printf("%s\n", str);
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
  memset(&hints, 0, sizeof(struct addrinfo));
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
      p = (struct sockaddr_in *)res->ai_addr;
      break;
    }
  }
  if (p == NULL) {
    return NULL;
  }
  struct sockaddr_in* ret = (struct sockaddr_in*)calloc(sizeof(struct sockaddr_in), 1);
  memcpy(ret, p, sizeof(struct sockaddr_in));
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

char* init_page()
{
  char *ret = "HTTP/1.1 404 Not Found"
    "Date: Thu, 17 Jun 2021 16:36:52 GMT"
    "Server: Apache/2.4.48 (Unix) OpenSSL/1.1.1k PHP/8.0.7"
    "Vary: accept-language,accept-charset"
    "Accept-Ranges: bytes"
    "Transfer-Encoding: chunked"
    "Content-Type: text/html; charset=utf-8"
    "Content-Language: en"
    "<html>"
    "<head>"
    "<title>Object not found!</title>"
    "</head>"
    "<body>"
    "<h1>Objectnot found!</h1>"
    "<p>"
    "   The requested URL was not found on this server."
    "   If you entered the URL manually please check your"
    "   spelling and try again."
    "</p>"
    "</body>"
    "</html>";
  char *r = malloc(1000);
  bzero(r, 1000);
  strcpy(r, ret);
  return r;
}

int
main(int argc, char **argv)
{
  unsigned short port = 8001;
  const char *host = "localhost";
  if (argc == 2) {
    host = argv[1];
  }
  struct sockaddr_in* my_addr;
  my_addr = get_address_from_hostname(host, port);

  if (my_addr == NULL) {
    printf("No such address\n");
    exit(0);
  }
  char str[BUFSIZ];
  char res[BUFSIZ];
  sprintf(res, "Initializing server on address %s (name: %s, port %d)",
	  inet_ntop(AF_INET, &my_addr->sin_addr, str, INET_ADDRSTRLEN), host,
	  port);
  serv_log(res);  
  int sock_fd = init_serv(my_addr);
  if (sock_fd < 0)
    return 1;
  int new_sock = accept(sock_fd, NULL, NULL);
  char *msg = "Fuck\n";
  char *r = init_page();
  char buf[512];
  int n;
  while ((n = read(new_sock, buf, 512)) != 0) {
    write(1, buf, n);
  }
  write(new_sock, r, strlen(r) + 1);
  free(my_addr);
  return 0;
}
