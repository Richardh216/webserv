#ifndef	REQUEST_HPP
#define	REQUEST_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <regex>
#include <set>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

struct HttpRequest {
	std::string	method;
	std::string	path;
	std::string	httpVersion;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool	isValid = true;
	std::string	errorMessage;
};

HttpRequest	parseHttpRequest(int clientFd);
void		testParseHttpRequest(void);

#endif
