#ifndef	REQUEST_HPP
#define	REQUEST_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <regex>
#include <set>
#include <poll.h>

struct HttpRequest {
	std::string	method;
	std::string	path;
	std::string	httpVersion;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool	isValid = true;
	std::string	errorMessage;
	bool	headersParsed = false;
	struct	pollfd	poll_fd;
};

HttpRequest	parseHttpRequest(int clientFd);
void		testParseHttpRequest(void);

#endif
