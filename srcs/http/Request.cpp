#include "../../includes/webserv.hpp"

//keep in mind potential timeout
//potential \r\n\r\n missing
//no content length may cause issues

//have to completely rewrite handleClientFd!!!!!!
//this will be a huge change and the engine behind our webserver

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
	static const std::set<std::string>	validMethods = {"GET", "POST", "DELETE"};
	if (validMethods.find(request.method) == validMethods.end()) {
		request.isValid = false;
		request.errorMessage = "Invalid HTTP method: " + request.method;
		return request;
	}

	//check http version									//R: define a string without having to escape special characters like backslashes.
	if (!std::regex_match(request.httpVersion, std::regex(R"(HTTP\/\d\.\d)"))) { //matches strings that start with "HTTP/", then a digit, a dot, and another digit
		request.isValid = false;
		request.errorMessage = "Invalid HTTP version: " + request.httpVersion;
		return request;
	}

	//read headers
	while (std::getline(stream, line) && line != "\r") {
		size_t	colonPos = line.find(':');

		if (colonPos == std::string::npos) {
			request.isValid = false;
			request.errorMessage = "Invalid header: " + line;
			return request;
		}

		std::string	key = line.substr(0, colonPos);
		std::string	val = line.substr(colonPos + 1);

		//trim whitespaces
		key.erase(0, key.find_first_not_of(" \t")); //trim leading whitespaces
		key.erase(key.find_last_not_of(" \t") + 1); // trim trailing whitespaces
		val.erase(0, val.find_first_not_of(" \t"));
		val.erase(0, val.find_last_not_of(" \t") + 1);

		request.headers[key] = val;
	}

	//handle body if there is content-length
	if (request.headers.find("Content-Length") != request.headers.end()) { //checks if "Content-Length" header was found.
		try {
			int	contentLen = std::stoi(request.headers.at("Content-Length")); // check .at!!! returns CL and throws an exception if the key is not found
			if (contentLen == 0) { //may be unnecessary
				request.body = "";
			} else if (contentLen < 0) {
				request.isValid = false;
				request.errorMessage = "Invalid Content Length: " + std::to_string(contentLen);
				return request;
			}

			//extract body based on contentLen
			size_t	bodyStart = rawRequest.find("\r\n\r\n") + 4;
			if (bodyStart + contentLen > rawRequest.size()) { //prevents a potential out-of-bounds read
				request.isValid = false;
				request.errorMessage = "Content Length mismatch.";
				return request;
			}
			request.body = rawRequest.substr(bodyStart, contentLen);

		} catch (const std::exception &e) {
			request.isValid = false;
			request.errorMessage = "Invalid Content-Length format.";
			return request;
		}
	}
	request.isValid = true; //should be redundant
	return request;
}