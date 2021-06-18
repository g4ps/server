NAME = webserv

HEAD = http_message.hpp parse_help.hpp

SRCS = main.cpp parse_help.cpp http_message.cpp

OBJS := $(SRCS:.cpp=.o)

#CPPFLAGS += -Wall -Werror -Wextra
CPPFLAGS += -g -std=c++98
CPP = g++

$(NAME): $(OBJS) $(HEAD)
	$(CPP) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	-rm $(OBJS)

fclean: clean
	-rm $(NAME)
