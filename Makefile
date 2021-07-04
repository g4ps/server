NAME = webserv

HEAD_DIR = include/
HEAD = http_message.hpp parse_help.hpp http_server.hpp http_responce.hpp http_webserv.hpp http_utils.hpp http_location.hpp
HEAD := $(HEAD:%$(HEAD_DIR):%)

SRCS_DIR = srcs/
SRCS = main.cpp parse_help.cpp http_message.cpp http_server.cpp http_responce.cpp \
	http_request.cpp http_webserv.cpp http_utils.cpp http_location.cpp
SRCS := $(SRCS:%=$(SRCS_DIR)%)

OBJS := $(SRCS:.cpp=.o)

#CPPFLAGS += -Wall -Werror -Wextra
CPPFLAGS += -g -std=c++98

#includes
CPPFLAGS += -I. -I$(HEAD_DIR)

CPPFLAGS += -DNOT_SHIT

CPP = g++

$(NAME): $(OBJS) $(HEAD)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	-rm $(OBJS)

fclean: clean
	-rm $(NAME)
