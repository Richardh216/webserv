#ifndef	SERVER_HPP
#define	SERVER_HPP

#include <map>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <chrono>
#include <unordered_map>

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
	std::string	port;
	std::string	host;
	std::vector<std::string>	serverNames;
	std::map<int, std::string>	errorPages;
	int	client_max_body_size = -1; //to represent default value
	std::string		root;
	std::vector<Route> routes;
	struct sockaddr_in	bind_addr; // for Sockets::bindSocket()
};

struct connectionState {
	int			fd;
	std::string	buffer; //raw request data, could be partial
	std::chrono::steady_clock::time_point	lastActivity; //time of the last read
	bool		isPending; //if the parsing returned, without throwing an error
};

#endif
