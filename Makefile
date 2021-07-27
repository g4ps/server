NAME = webserv

HEAD_DIR = include/
HEAD = http_message.hpp parse_help.hpp http_server.hpp http_responce.hpp http_webserv.hpp http_utils.hpp http_location.hpp http_connection.hpp
HEAD := $(HEAD:%$(HEAD_DIR):%)

SRCS_DIR = srcs/
SRCS = main.cpp parse_help.cpp http_message.cpp http_server.cpp http_responce.cpp \
	http_request.cpp http_webserv.cpp http_utils.cpp http_location.cpp http_connection.cpp
SRCS := $(SRCS:%=$(SRCS_DIR)%)

OBJS := $(SRCS:.cpp=.o)

#CPPFLAGS += -Wall -Werror -Wextra
CPPFLAGS += -std=c++98 -pthread

#includes
CPPFLAGS += -I. -I$(HEAD_DIR)

CPP = clang++

$(NAME): $(OBJS) $(HEAD)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	-rm $(OBJS)

fclean: clean
	-rm $(NAME)
