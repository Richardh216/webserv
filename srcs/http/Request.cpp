#include "../../includes/webserv.hpp"

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
		if (bytesRead <= 0) break; // No more data
		rawRequest.append(buffer.data(), bytesRead); // buffer.data() returns a pointer to its first element
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
		request.isValid = false;
		request.errorMessage = "Invalid or empty request line.";
		return request;
	}

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

		//this ugly bit of code makes first letters capitalized, while keeping everything else lowercase
		key[0] = std::toupper(key[0]);
		size_t j = 0;
		for (size_t i = 1; i < key.size() - 1; i++) {
			if (key[i] == '-' && i + 1 < key.size()) {
					key[i + 1] = std::toupper(key[i + 1]);
					j = i + 1;
			} else if (j != i) {
				key[i] = std::tolower(key[i]);
			}
		}
		//handle the last character
		if (key.size() > 1 && j != key.size() - 1) {
			key[key.size() - 1] = std::tolower(key[key.size() - 1]);
		}

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
		"POST /api/data HTTP/1.1\r\n"
		"Host: api.example.com\r\n"
		"User-Agent: MyTestClient/2.0\r\n"
		"cOnTEnt-type: application/json\r\n"
		"Accept: application/json\r\n"
		"Authorization: Bearer abcdef123456\r\n"
		"cOnTEnt-lEngtH: 53\r\n"
		"\r\n"
		"{\"name\": \"John Doe\", \"email\": \"john.doe@example.com\"}";

	// std::string httpRequest =
	// 	"GET /index.html HTTP/1.1\r\n"
	// 	"Host: www.example.com\r\n"
	// 	"Connection: keep-alive\r\n"
	// 	"conteNT-lENgth: 13\r\n"
	// 	"\r\n"
	// 	"Hello, world!";  // Simple body

	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
		perror("socketpair");
		exit(EXIT_FAILURE);
	}
		
	// Write the HTTP request to one end of the socketpair.
	ssize_t n = write(sv[1], httpRequest.c_str(), httpRequest.size());
	std::cout << "Bytes written to socketpair: " << n << "\n";
		
	// Signal EOF on the writing end.
	shutdown(sv[1], SHUT_WR);
		
	// Directly call parseHttpRequest using the reading end of the socketpair.
	HttpRequest request = parseHttpRequest(sv[0]);
		
	// Output parsed results.
	std::cout << "\nMethod: " << request.method << "\n";
	std::cout << "Path: " << request.path << "\n";
	std::cout << "HTTP Version: " << request.httpVersion << "\n";
	for (const auto& header : request.headers) {
		std::cout << header.first << ": " << header.second << "\n";
	}
	std::cout << "Body: " << request.body << "\n";
		
	close(sv[0]);
	close(sv[1]);
}
