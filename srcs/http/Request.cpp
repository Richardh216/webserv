#include "../../includes/webserv.hpp"

//keep in mind potential timeout
//potential \r\n\r\n missing
//no content length may cause issues

//have to rewrite handleClientFd potentially

//create checker for parseHttpRequest and then try to implement it through sockets

/*
To do: 

1)read a request until the start of the body (after the '\r\n' line)

2)check headers

3)if there is "Transfer-Encoding: chunked" header:
	-read the first line until ';' or '\r\n' or '\n' -> this is the 'size' of the chunk data (in hex)
	-read 'size' bytes data
	-if you will encounter '0\r\n' line then this is the end of all chunks

4)if there is "Content-Length: 42" header then a request should be normal. We just need to read e.g. 42 bytes in the body section.
(A request with both "Transfer-Encoding" and "Content-Length" is malformed and should be rejected with 400 Bad Request ->
to form and send this response - this is my part, just include a bool var which I will check)

Request string could be incomplete, so you need to check everytime where did you stop and where you should continue.

General steps for both of us:
-store partial requests;
-append incoming data;
-when a full request is received, process it;
-if the request is incomplete and we wait too long, we should discard the data and close the connection;


You need to use socket fd (poller.poll_fds[i].fd) directly in your code with recv() function for reading data,
because for passing complete http request string to your function I need to check Content-Length header
for reading specific amount of bytes. Because of that when you parse a request you need to check headers
(Content-Length and Transfer-Encoding) and based on them handle body respectively

*/

HttpRequest	parseHttpRequest(int clientFd) {
	constexpr size_t	BUFFER_SIZE = 4096;
	std::vector<char>	buffer(BUFFER_SIZE);
	std::string			rawRequest;
	ssize_t				bytesRead;
	struct pollfd	pfd = {clientFd, POLLIN, 0};

	// Read data from the client with polling to avoid blocking indefinitely
	while (true) {
		int	ret = poll(&pfd, 1, 5000); //5 second timeout
		if (ret == -1) { // Polling error
			HttpRequest	request;
			request.isValid = false;
			request.errorMessage = "Polling error.";
			return request;
		} else if (ret == 0) { // Timeout occurred
			HttpRequest	request;
			request.isValid = false;
			request.errorMessage = "Polling timeout.";
			return request;
		}
		bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
		std::cout << "Bytes Read: " << bytesRead << "\n"; // Debugging line ##############
		if (bytesRead <= 0) break; // No more data
		rawRequest.append(buffer.data(), bytesRead); // buffer.data() returns a pointer to its first element
		std::cout << "Raw Request: " << rawRequest << "\n"; // Debugging line ##############
		if (rawRequest.find("\r\n\r\n") != std::string::npos) break; // End of headers
	}

	if (bytesRead < 0) { // Error in receiving data
		HttpRequest request;
		request.isValid = false;
		request.errorMessage = "Failed to recieve data.";
		return request;
	}

	std::istringstream	stream(rawRequest);
	HttpRequest			request;
	std::string			line;

	// Read and parse the request line
	if (!std::getline(stream, line) || line.empty()) {
		std::cout << "Invalid or empty request line: " << line << "\n"; // Debugging line ##############
		request.isValid = false;
		request.errorMessage = "Invalid or empty request line.";
		return request;
	}

	std::istringstream	lineStream(line);
	if (!(lineStream >> request.method >> request.path >> request.httpVersion)) {
		std::cout << "Invalid request line: " << line << "\n"; // Debugging line ##################
		request.isValid = false;
		request.errorMessage = "Invalid request line.";
		return request;
	}

	std::cout << "Method: " << request.method << "\n"; // Debugging line #################
	std::cout << "Path: " << request.path << "\n"; // Debugging line #################
	std::cout << "HTTP Version: " << request.httpVersion << "\n"; // Debugging line #################

	//check http method
	static const std::set<std::string>	validMethods = {"GET", "POST", "DELETE"};
	if (validMethods.find(request.method) == validMethods.end()) {
		request.isValid = false;
		request.errorMessage = "Invalid HTTP method for the project: " + request.method;
		return request;
	}

	//check http version						//R: define a string without having to escape special characters like backslashes.
	if (!std::regex_match(request.httpVersion, std::regex(R"(HTTP\/\d\.\d)"))) { //matches strings that start with "HTTP/", then a digit, a dot, and another digit
		request.isValid = false;
		request.errorMessage = "Invalid HTTP version: " + request.httpVersion;
		return request;
	}

	//read headers
	while (std::getline(stream, line) && line != "\r") {
		std::cout << "Header line: " << line << "\n"; // Debugging line ###########
		size_t	colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			request.isValid = false;
			request.errorMessage = "Invalid header: " + line;
			return request;
		}

		// Extract header key and value
		std::string	key = line.substr(0, colonPos);
		std::string	val = line.substr(colonPos + 1);

		//trim whitespaces
		key.erase(0, key.find_first_not_of(" \t")); //trim leading whitespaces
		key.erase(key.find_last_not_of(" \t") + 1); // trim trailing whitespaces
		val.erase(0, val.find_first_not_of(" \t"));
		val.erase(val.find_last_not_of(" \t") + 1);

		std::cout << "Header: " << key << ": " << val << "\n"; // Debugging line ##########

		//check if needed
		// key[0] = std::toupper(key[0]); //this ugly bit of code makes first letters capitalized, while keeping everything else lowercase
		// size_t j = 0;
		// for (size_t i = 1; i < key.size() - 1; i++) {
		// 	if (key[i] == '-' && i + 1 < key.size()) {
		// 			key[i + 1] = std::toupper(key[i + 1]);
		// 			j = i + 1;
		// 	} else if (j != i) {
		// 		key[i] = std::tolower(key[i]);
		// 	}
		// }

		request.headers[key] = val;
	}

	// Check for Content-Length and Chunked Encoding
	bool	hasContentLength = request.headers.find("Content-Length") != request.headers.end();
	bool	hasChunkedEncoding = request.headers.find("Transfer-Encoding") != request.headers.end() && 
								request.headers["Transfer-Encoding"] == "chunked";

	if (hasContentLength && hasChunkedEncoding) {
		request.isValid = false;
		request.errorMessage = "Malformed request: both Content-Length and Transfer-Encoding present.";
		return request;
	}

	// Handle request body
	if (hasContentLength) {
		int	contentLen = std::stoi(request.headers.at("Content-Length"));
		if (contentLen < 0) {
			request.isValid = false;
			request.errorMessage = "Invalid Content-Length value.";
			return request;
		}

		// Read the body based on Content-Length
		while (rawRequest.length() < static_cast<size_t>(contentLen)) {
			int	ret = poll(&pfd, 1, 5000);
			if (ret <= 0) {
				request.isValid = false;
				request.errorMessage = "Polling timeout or error while reading body.";
				return request;
			}
			bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
			if (bytesRead <= 0) {
				request.isValid = false;
				request.errorMessage = "Incomplete request body.";
				return request;
			}
			rawRequest.append(buffer.data(), bytesRead);
		}
		request.body = rawRequest.substr(rawRequest.find("\r\n\r\n") + 4, contentLen); //Extracts the body from rawRequest
	}

	// Handle chunked transfer encoding
	if (hasChunkedEncoding) {
		std::ostringstream	bodyStream; //output string stream, allows appending to a string like writing to a file
		while (std::getline(stream, line)) {
			if (line.empty()) continue;
			int	chunkSize;
			std::istringstream	chunkSizeStream(line);
			chunkSizeStream >> std::hex >> chunkSize; //Reads chunk size as a hexadecimal value.
			if (chunkSize == 0) break; // End of chunks

			while (rawRequest.length() < static_cast<size_t>(chunkSize)) {
				int	ret = poll(&pfd, 1, 5000);
				if (ret <= 0) {
					request.isValid = false;
					request.errorMessage = "Polling timeout or error while reading chunk.";
					return request;
				}
				bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
				if (bytesRead <= 0) {
					request.isValid = false;
					request.errorMessage = "Incomplete chunked transfer encoding.";
					return request;
				}
				rawRequest.append(buffer.data(), bytesRead);
			}
			bodyStream.write(rawRequest.c_str(), chunkSize);
			stream.ignore(2); // Ignore the CRLF after chunk
		}
		request.body = bodyStream.str();
	}
	return request;
}

void testParseHttpRequest(void) {
	std::string httpRequest =
		"GET /index.html HTTP/1.1\r\n"
		"Host: www.example.com\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		"Hello, world!";  // Simple body

	int fds[2];
	pipe(fds);
	write(fds[1], httpRequest.c_str(), httpRequest.size());
	close(fds[1]);  // Close write end to signal EOF

	// Directly call parseHttpRequest without reading first
	HttpRequest request = parseHttpRequest(fds[0]);

	// Output parsed results
	std::cout << "Method: " << request.method << "\n";
	std::cout << "Path: " << request.path << "\n";
	std::cout << "HTTP Version: " << request.httpVersion << "\n";
	for (const auto& header : request.headers) {
		std::cout << header.first << ": " << header.second << "\n";
	}
	std::cout << "Body: " << request.body << "\n";

	close(fds[0]);
}