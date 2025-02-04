#include "../../includes/webserv.hpp"

// HttpRequest	parseHttpRequest(const std::string &rawRequest) {
// 	std::istringstream	stream(rawRequest);
// 	HttpRequest	request;
// 	std::string	line;

// 	if (std::getline(stream, line)) {
// 		std::istringstream	lineStream(line);
// 		lineStream >> request.method >> request.path >> request.httpVersion;
// 	}

// 	while (std::getline(stream, line) && line != "\r") {
// 		size_t	colonPos = line.find(':');
// 		if (colonPos != std::string::npos) {
// 			std::string	key = line.substr(0, colonPos);
// 			std::string	value = line.substr(colonPos + 1);

// 			key.erase(0, key.find_first_not_of(" \t"));
// 			key.erase(key.find_last_not_of(" \t") + 1);
// 			value.erase(0, value.find_first_not_of(" \t"));
// 			value.erase(value.find_last_not_of(" \t") + 1);

// 			request.headers[key] = value;
// 		}
// 	}

// 	if (request.headers.find("Content-Length") != request.headers.end()) {
// 		int contentLen = std::stoi(request.headers["Content-Length"]);
// 		request.body.resize(contentLen);
// 		stream.read(&request.body[0], contentLen); // read conentLen from request body MAY NEED POLL()!!!!!
// 	}
// 	return request;
// }