#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct sockaddr_in*
get_address(char *host)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  struct addrinfo *res = NULL;
  struct addrinfo *i = NULL;
  struct addrinfo *tmp = NULL;
  int err = getaddrinfo(host, NULL, &hints, &res);
  if (err != 0) {
    fprintf(stderr, "%s\n", gai_strerror(err));
    return NULL;
  }
  struct sockaddr_in *p = (struct sockaddr_in *)i->ai_addr;
  struct sockaddr_in* ret = calloc(sizeof(struct sockaddr_in), 1);
  memcpy(ret, p, sizeof(struct sockaddr_in));
  freeaddrinfo(res);
  return ret;
}

int
main(int argc, char **argv)
{
  struct sockaddr_in* my_addr = get_address("Localhost");
  free(my_addr);
  return 0;
}
