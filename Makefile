NAME				:= webserv

CXX					:= c++
FLAGS				:= -g -Wall -Wextra -Werror -std=c++17

INCDIRS				:= -I./includes/

#directories of source files
VPATH				:= srcs/config srcs/http srcs/main srcs/network	\
					   srcs/server srcs/CGI							\

#source files
SRCS_CONFIG			:= ConfigParser.cpp 							\

SRCS_HTTP			:= Request.cpp Response.cpp 					\

SRCS_MAIN			:= main.cpp utils.cpp							\

SRCS_NETWORK		:= Connection.cpp Poller.cpp Sockets.cpp		\

SRCS_CGI			:= CGI_Handler.cpp								\


SRCS				:= $(SRCS_CONFIG) $(SRCS_HTTP) $(SRCS_MAIN)		\
					   $(SRCS_NETWORK) $(SRCS_CGI)							\

BUILDDIR			:= ./build
ODIR				:= $(BUILDDIR)/obj
DDIR				:= $(BUILDDIR)/deps
OBJS				:= $(patsubst %.cpp,$(ODIR)/%.o,$(SRCS))
DEPFILES			:= $(patsubst %.cpp,$(DDIR)/%.d,$(SRCS))
DEPFLAGS			=  -MMD -MP -MF $(DDIR)/$*.d

all: $(BUILDDIR) $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME)

$(ODIR)/%.o: %.cpp
	$(CXX) $(FLAGS)  $(INCDIRS) $(DEPFLAGS) -c -o $@ $<

$(BUILDDIR):
	@mkdir -p $(BUILDDIR) $(ODIR) $(DDIR)

clean:
	rm -f $(OBJS) $(DEPFILES)

fclean: clean
	rm -f $(NAME)
	rm -rf $(BUILDDIR)

re: fclean all

-include $(DEPFILES)

.PHONY: all clean fclean re
