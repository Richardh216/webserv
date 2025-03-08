#ifndef	SERVER_HPP
#define	SERVER_HPP

#include <map>
#include <string>
#include <vector>
#include <netinet/in.h>

struct Route {
	std::string	path;
	std::string	root;
	std::vector<std::string>	allowedMethods;
	bool	autoindex = false;
	std::string	indexFile;
	std::map<int, std::string>	redirection;
	std::string	cgiExtension;
	std::string	cgiPath;
	bool	uploadEnabled = false;
	std::string	uploadPath;
	std::string	alias;
};

struct serverConfig {
	std::string	port; //chnaged to handle multiple listen directives
	std::string	host; //chnaged to handle multiple listen directives
	// std::vector<std::string>	port;
	// std::vector<std::string>	host;
	std::vector<std::string>	serverNames;
	std::map<int, std::string>	errorPages;
	int	client_max_body_size = -1; //to represent default value
	std::string		root;
	std::vector<Route> routes;
	struct sockaddr_in	bind_addr; // for Sockets::bindSocket()
};

#endif
