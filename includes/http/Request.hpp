#ifndef	REQUEST_HPP
#define	REQUEST_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>

struct HttpRequest {
	std::string	method;
	std::string	path;
	std::string	httpVersion;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool	isValid = true;
	std::string	errorMessage;
};

// // class Request
// // {
// // 	public:
// // 	private:
// // };

HttpRequest	parseHttpRequest(const std::string &rawRequest);

#endif
