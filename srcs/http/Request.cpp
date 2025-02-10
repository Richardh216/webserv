#include "../../includes/webserv.hpp"

HttpRequest	parseHttpRequest(const std::string &rawRequest) {
	std::istringstream	stream(rawRequest);
	HttpRequest	request;
	std::string	line;

	//check for valid request line
	if (!std::getline(stream, line) || line.empty()) {
		request.isValid = false;
		request.errorMessage = "Invalid or empty request line.";
		return request;
	}

	//get request line
	std::istringstream	lineStream(line);
	if (!(lineStream >> request.method >> request.path >> request.httpVersion)) {
		request.isValid = false;
		requrst.errorMessage = "Invalid request line.";
		return request;
	}

	//check http method
	
}