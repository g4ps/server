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



int
main(int argc, char **argv)
{
  // string k="DOOR%20STUCK!%20DOOR%20STUCK!-VqB1uoDTdKM.mp4";
  // cout << uri_to_string(k) << endl;
  // return 0;
  //set_log("serv_log");
  string conf_name = "conf/config.ang";
  if (argc == 2) {
    conf_name = argv[1];
  }
  serv_log("Eugene's HTTP server v0.1");
  http_webserv w1;
  serv_log("Initializing server:");
  serv_log("----------------------------------------");
  try {
    w1.parse_config(conf_name);
  }
  catch(exception &e) {
    serv_log(string("INIT ERROR: ") + e.what());
    exit(1);
  }
  serv_log("----------------------------------------");
  serv_log("STARTING");
  try {
    w1.start();
  }
  catch(exception &e) {
    serv_log(string("RUNTIME ERROR: ") + e.what());
    exit(1);
  }
  return 0;
}
