#ifndef	SERVER_HPP
#define	SERVER_HPP

#include <map>
#include <string>
#include <vector>

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
	// int			port = 80;
	std::string		port;
	// std::string	host = "localhost";
	std::string	host;
	std::vector<std::string>	serverNames;
	std::map<int, std::string>	errorPages;
	int	client_max_body_size = 0;
	std::string		root;
	std::vector<Route> routes;
};

#endif
