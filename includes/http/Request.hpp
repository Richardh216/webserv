#ifndef	REQUEST_HPP
#define	REQUEST_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <regex>
#include <set>
#include <poll.h>

struct serverConfig;

struct HttpRequest {
	std::string	method;
	std::string	path;
	std::string	httpVersion;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool	isValid = true;
	std::string	errorMessage; //might be redundant
	bool	headersParsed = false;
	struct	pollfd	poll_fd;
	std::map<int, std::string>	errorCodes;
};

serverConfig &selectServer(int fd, std::vector<serverConfig>& servers, std::string hostValue);
HttpRequest	parseHttpRequest(int clientFd, std::vector<serverConfig>& servers);
void		testParseHttpRequest(std::vector<serverConfig>& servers);

#endif
