#ifndef	REQUEST_HPP
#define	REQUEST_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <regex>
#include <set>
#include <poll.h>
#include "../network/Poller.hpp"

struct serverConfig;

struct HttpRequest {
	std::string	method;
	std::string	path;
	std::string	httpVersion;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool	isValid; //just don't set it yet
	bool	isComplete = false;
	bool	headersParsed = false;
	struct	pollfd	poll_fd;
	std::map<int, std::string>	errorCodes;
};

serverConfig &selectServer(int fd, std::vector<serverConfig>& servers, std::string hostValue);
HttpRequest	parseHttpRequestFromBuffer(std::string &buffer, int fd, std::vector<serverConfig>& servers);
void 		timeOutCheck(int curr_nfds, std::unordered_map<int, connectionState>& connectionStates, Poller& poller);
void		testParseHttpRequest(std::vector<serverConfig>& servers);

#endif
