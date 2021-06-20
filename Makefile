NAME = webserv

HEAD = http_message.hpp parse_help.hpp server.hpp http_responce.hpp

SRCS = main.cpp parse_help.cpp http_message.cpp server.cpp http_responce.cpp

OBJS := $(SRCS:.cpp=.o)

#CPPFLAGS += -Wall -Werror -Wextra
CPPFLAGS += -g -std=c++98

#includes
CPPFLAGS += -I.

CPPFLAGS += -DNOT_SHIT

CPP = g++

$(NAME): $(OBJS) $(HEAD)
	$(CPP) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	-rm $(OBJS)

fclean: clean
	-rm $(NAME)
