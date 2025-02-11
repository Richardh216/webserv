#include "../../includes/webserv.hpp"

//keep in mind potential timeout
//potential \r\n\r\n missing
//no content length may cause issues

//have to completely rewrite handleClientFd!!!!!!
//this will be a huge change and the engine behind our webserver

//create checker for parseHttpRequest and then try to implement it through sockets

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
		request.errorMessage = "Invalid request line.";
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
		val.erase(val.find_last_not_of(" \t") + 1);

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
			if (bodyStart + contentLen > rawRequest.length()) { //prevents a potential out-of-bounds read
				request.isValid = false;
				request.errorMessage = "Content Length mismatch. Expected " + std::to_string(contentLen) +
						", but only " + std::to_string(rawRequest.length() - bodyStart) + " bytes available.";
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

int	httpRequestTester(void) {
	std::string testRequest1 =
		"GET /index.html HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"User-Agent: TestClient/1.0\r\n"
		"\r\n";

	std::string testRequest2 =
		"POST /submit HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Length: 12\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"\r\n"
		"name=JohnDoe";

	std::string testRequest3 =
		"INVALID /bad HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"\r\n";

	std::string testRequest4 =
		"GET / HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Length: -5\r\n"
		"\r\n";

	std::cout << "Testing valid GET request...\n";
	HttpRequest request1 = parseHttpRequest(testRequest1);
	if (request1.isValid) {
		std::cout << "Parsed successfully!\n";
		std::cout << "Method: " << request1.method << std::endl;
		std::cout << "path: " << request1.path << std::endl;
		std::cout << "httpVersion: " << request1.httpVersion << std::endl;
		std::cout << "body: " << request1.body << std::endl;
		std::cout << "Headers:\n";
		for (const auto& [key, value] : request1.headers) {
			std::cout << "  " << key << ": " << value << "\n";
		}
		std::cout << std::endl;
	}
	else
		std::cout << "Error: " << request1.errorMessage << "\n";

	std::cout << "\nTesting valid POST request...\n";
	HttpRequest request2 = parseHttpRequest(testRequest2);
	if (request2.isValid) {
		std::cout << "Parsed successfully!" << std::endl;
		std::cout << "Method: " << request2.method << std::endl;
		std::cout << "path: " << request2.path << std::endl;
		std::cout << "httpVersion: " << request2.httpVersion << std::endl;
		std::cout << "body: " << request2.body << std::endl;
		std::cout << "Headers:\n";
		for (const auto& [key, value] : request2.headers) {
			std::cout << "  " << key << ": " << value << "\n";
		}
		std::cout << std::endl;
	}
	else
		std::cout << "Error: " << request2.errorMessage << "\n";

	std::cout << "\nTesting invalid method...\n";
	HttpRequest request3 = parseHttpRequest(testRequest3);
	if (!request3.isValid)
		std::cout << "Error as expected: " << request3.errorMessage << "\n";
	else
		std::cout << "Unexpected success!\n";

	std::cout << "\nTesting invalid Content-Length...\n";
	HttpRequest request4 = parseHttpRequest(testRequest4);
	if (!request4.isValid)
		std::cout << "Error as expected: " << request4.errorMessage << "\n";
	else
		std::cout << "Unexpected success!\n";

	return 0;
}