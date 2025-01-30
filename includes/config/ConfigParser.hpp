#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

//class Server;
//class Location;

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

class	ConfigParser {
	private:
		std::string	trim(const std::string &str);
		std::string	getValue(const std::string &line);
		std::vector<std::string>	split(const std::string &str, char delim);
		std::string	getLocationPath(const std::string &line);
		int			parseSize(const std::string &sizeStr);
		void		getPortHost(const std::string &line, serverConfig &config);
	public:
		std::vector<serverConfig>	servers;
		void	parseConfigFile(const std::string &filename);
		void	tester(const std::string &inFile);
};