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
#include <errno.h>

#include "parse_help.hpp"
#include "http_message.hpp"
#include "http_server.hpp"
#include "http_webserv.hpp"
#include "http_utils.hpp"

using namespace std;

void	parse_conf(string filename)
{
	if (does_exist(filename) )

}