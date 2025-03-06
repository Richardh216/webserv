#include "../../includes/webserv.hpp"

/*
TODO

1. (done) use client_max_body_size directive from the config file
when saving the body, if not specified use default value

2. (done) headers parsed bool might be set too early, check

3. (done) check error codes when encoutering an error, create error code var and send it

4. (done) Implement client_max_body_size, we are able to get the value now, banger
4.5 (done) still needs to have a default option if it is -1, or non existant, uses -1 to set it to a default value

5. (done?) handle case where there is a body, but no content-length, or chunked-transfer-encoding
http 1.1 needs length defined, 1.0 apparetly can read until connection is closed

6 .check error handling when parsing client_max_body_size

7. split up the function into smaller chunks

*/

serverConfig &selectServer(int fd, std::vector<serverConfig>& servers, std::string hostValue) {
	struct sockaddr_in	client_address;
	socklen_t			add_len = sizeof(client_address);

	if (getsockname(fd, (struct sockaddr*)&client_address, &add_len) == -1) {
		throw std::runtime_error("Failed to get socket name");
	}

	serverConfig *first_match = nullptr;
	for (auto &server : servers) {
		if (client_address.sin_port == server.bind_addr.sin_port
			&& (client_address.sin_addr.s_addr == server.bind_addr.sin_addr.s_addr
			|| server.bind_addr.sin_addr.s_addr == INADDR_ANY)) { //INADDR_ANY: also allows 0.0.0.0, can use just 0 instead
				if (!first_match) {
					first_match = &server;
				}
				for (const auto &name : server.serverNames) {
					if (name == hostValue) {
						first_match = &server;
						return *first_match;
					}
				}
			}
	}
	if (!first_match) {
		throw std::runtime_error("No matching server configuration found");
	}
	return *first_match;
}

/*
Regarding recv() calls:
	- If the socket is non-blocking, recv() will return immediately if there's no data
*/

HttpRequest	parseHttpRequest(int clientFd, std::vector<serverConfig>& servers) {
	std::string			rawRequest;
	ssize_t				bytesRead;
	HttpRequest			request;
	bool				headersRead = false;
	size_t	headerEnd;
	// size_t	delimLen; //may be worth implementing universal delim len
	
	request.poll_fd.fd = clientFd;
	char c;

	//Reading request data
	while (!headersRead) {
		bytesRead = recv(clientFd, &c, 1, 0); // Read 1 byte at a time
		if (bytesRead < 0) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			return request;
		} else if (bytesRead == 0) {
			break; //the client closed the connection
		}
		rawRequest += c; //append the data
		headerEnd = rawRequest.find("\r\n\r\n");
		if (headerEnd == std::string::npos) {
			headerEnd = rawRequest.find("\n\n"); // Support '\n' only requests
		}
		if (headerEnd != std::string::npos) {
			headersRead = true; // just read, not parsed
		}
	}

	// if no data was read, return invalid request
	if (rawRequest.empty()) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		return request;
	}

	//start parsing from rawReqeust
	std::istringstream	stream(rawRequest);
	std::string			line;

	// Read and parse the request line
	if (!std::getline(stream, line) || line.empty()) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		return request;
	}

	// Remove trailing '\r' if it exists
	if (!line.empty() && line.back() == '\r') {
		line.pop_back();
	}

	std::istringstream	lineStream(line); //split the request line into method, path ...
	if (!(lineStream >> request.method >> request.path >> request.httpVersion)) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		return request;
	}

	//check http method
	static const std::set<std::string>	validMethods = {"GET", "POST", "DELETE"}; //only these methods are allowed
	if (validMethods.find(request.method) == validMethods.end()) {
		request.isValid = false;
		request.errorCodes[405] = "Method Not Allowed";
		return request;
	}

	//check http version						//R: define a string without having to escape special characters like backslashes.
	if (!std::regex_match(request.httpVersion, std::regex(R"(HTTP\/\d\.\d)"))) { //matches strings that start with "HTTP/", then a digit, a dot, and another digit
		request.isValid = false;
		request.errorCodes[505] = "HTTP Version Not Supported";
		return request;
	}

	//read headers
	while (std::getline(stream, line)) { //line != "" makes just \n work
		if (!line.empty() && line.back() == '\r') { //remove \r if present
			line.pop_back();
		}
		if (line.empty()) {  // Stop when an actual empty line is found
			break;
		}

		size_t	colonPos = line.find(':'); //Finds the ":" separator in each header
		if (colonPos == std::string::npos) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
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
		request.errorCodes[400] = "Bad Request";
		return request;
	}

	//handle case where there is a body, but no content-length, or chunked-transfer-encoding
	//headerEnd - will be useful

	//headers parsed
	request.headersParsed = true;

	std::string	hostValue;
	auto it = request.headers.find("Host");
	if (it != request.headers.end()) {
		hostValue = it->second;
	} else { //missing host header
		request.errorCodes[400] = "Bad Request";
	}

	//get Config data
	size_t			BUFFER_SIZE; //maybe add a check if it's beyond a reasonable amount limit it, so that it won't slow everything down
	const size_t	DEFAULT_MAX_BODY_SIZE = 1048576; // 1MB default
	serverConfig	currentServer = selectServer(request.poll_fd.fd, servers, hostValue);

	if (currentServer.client_max_body_size == 0 && (hasContentLength || hasChunkedEncoding)) {
			request.isValid = false;
			request.errorCodes[413] = "Content Too Large";
			return request;
	} else if (currentServer.client_max_body_size == -1) {
		BUFFER_SIZE = DEFAULT_MAX_BODY_SIZE;
	} else {
		BUFFER_SIZE = currentServer.client_max_body_size;
	}
	std::cout << "BUFFER_SIZE: " << BUFFER_SIZE << std::endl;
	std::vector<char>	buffer(BUFFER_SIZE);

	// Handle request body
	if (hasContentLength) {
		size_t contentLen = 0;
		try {
			contentLen = std::stoull(request.headers.at("Content-Length"));
			if (contentLen == 0) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				return request;
			}
			if (contentLen > BUFFER_SIZE) { //check if contentLen can fit in BUFFER_SIZE
				request.isValid = false;
				request.errorCodes[413] = "Content Too Large";
				return request;
			}
		} catch (const std::invalid_argument& e) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			return request;
		} catch (const std::out_of_range& e) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			return request;
		}

		size_t	bodyStart = rawRequest.find("\r\n\r\n"); //start of body
		if (bodyStart == std::string::npos) {
			bodyStart = rawRequest.find("\n\n");
		}
		if (bodyStart != std::string::npos) {
			bodyStart += (rawRequest[bodyStart] == '\r') ? 4 : 2; //adjust offset
		} else {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			return request;
		}

		 // If the entire body wasn't already received with the headers, continue reading from the socket.
		while (rawRequest.size() < bodyStart + contentLen) {
			bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
			if (bytesRead <= 0) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				return request;
			}
			rawRequest.append(buffer.data(), bytesRead);
		}
		request.body = rawRequest.substr(bodyStart, contentLen); //Extracts the body from rawRequest
	}

	// Handle chunked transfer encoding
	else if (hasChunkedEncoding) {
		std::string	chunkedBody;
		size_t	pos = rawRequest.find("\r\n\r\n"); //find where body begins
		if (pos == std::string::npos) {
			pos = rawRequest.find("\n\n"); //support \n
		}
		if (pos == std::string::npos) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			return request;
		}
		pos += (rawRequest.compare(pos, 4, "\r\n\r\n") == 0) ? 4 : 2; //skip the header delim

		// Skip any extra CRLFs that may be present after the header delimiter.
		while (pos < rawRequest.size() && (rawRequest.compare(pos, 2, "\r\n") == 0 || rawRequest[pos] == '\n')) {
			pos += (rawRequest.compare(pos, 2, "\r\n") == 0) ? 2 : 1;
		}

		size_t	totalBodySize = 0;

		while (true) { //breaks when 0\r\n is found
			// Find the CRLF that ends the chunk size line.
			size_t	lineEnd = rawRequest.find("\r\n", pos);
			size_t	delimLen = 2;
			if (lineEnd == std::string::npos) {
				lineEnd = rawRequest.find("\n", pos);
				delimLen = 1; // Adjust for single newline
			}

			while (lineEnd == std::string::npos) { //if lineEnd is not found, need more data
				bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
				if (bytesRead <= 0) {
					request.isValid = false;
					request.errorCodes[400] = "Bad Request";
					return request;
				}
				rawRequest.append(buffer.data(), bytesRead);
				if ((lineEnd = rawRequest.find("\r\n", pos)) != std::string::npos) {
					delimLen = 2;
				} else if ((lineEnd = rawRequest.find("\n", pos)) != std::string::npos) {
					delimLen = 1;
				}
			}

			// Move past any stray newlines before parsing chunk size
			while (pos < rawRequest.size() && (rawRequest.compare(pos, 2, "\r\n") == 0 || rawRequest[pos] == '\n')) {
				pos += (rawRequest.compare(pos, 2, "\r\n") == 0) ? 2 : 1;
			}

			//end of the chunk size line should be after the current position
			if (lineEnd <= pos) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				return request;
			}

			//Extract the chunk size from rawRequest
			std::string	chunkSizeStr = rawRequest.substr(pos, lineEnd - pos);
			chunkSizeStr.erase(0, chunkSizeStr.find_first_not_of(" \t"));
			chunkSizeStr.erase(chunkSizeStr.find_last_not_of(" \t") + 1);
			
			//Extract the chunk size (in hexadecimal)
			size_t	chunkSize;
			try {
				chunkSize = std::stoul(chunkSizeStr, nullptr, 16); // Convert str to int, with 16 base system
			} catch (const std::exception& e) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				return request;
			}

			if (chunkSize == 0) { //the last chunk was received, end of the loop
				break;
			}

			if (totalBodySize + chunkSize > BUFFER_SIZE) { //check for client_max_body_size bounds
				request.isValid = false;
				request.errorCodes[413] = "Content Too Large";
				return request;
			}

			size_t chunkDataStart = lineEnd + delimLen; //Move past the chunk size line (and its CRLF)
			// Ensure the entire chunk and its trailing CRLF are available
			while (rawRequest.size() < chunkDataStart + chunkSize + delimLen) { //If rawRequest is too short, call recv() to read more data from the socket
				bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
				if (bytesRead <= 0) {
					request.isValid = false;
					request.errorCodes[400] = "Bad Request";
					return request;
				}
				rawRequest.append(buffer.data(), bytesRead);
			}
			chunkedBody.append(rawRequest.substr(chunkDataStart, chunkSize)); // Append the chunk data to the chunkBody

			//update body size
			totalBodySize += chunkSize;

			//advance pointer past chunked data
			pos = chunkDataStart + chunkSize;

			//advance pointer past delim
			if (pos < rawRequest.size() && rawRequest.compare(pos, 2, "\r\n") == 0) {
				pos += 2;
			} else if (pos < rawRequest.size() && rawRequest[pos] == '\n') {
				pos += 1;
			}
		}
		request.body = chunkedBody; //store full body
	}
	//in case there is a body, but no Content-Length or Chunked Transfer Encoding
	//in case it's GET or DELETE and there is no header to indicate size, it ignores the body
	if (request.method == "POST" && (!hasContentLength && !hasChunkedEncoding)) {
		if (request.httpVersion == "HTTP/1.1") { //may remove this check
			// std::cout << "VERSION 1.1 CASE" << std::endl;
			request.isValid = false;
			request.errorCodes[411] = "Length Required";
			return request;
		}
		// else if (request.httpVersion == "HTTP/1.0") { //potential, different versions behave differently, probably won't implement
		// 	std::cout << "VERSION 1.0 CASE" << std::endl;
		// }
	}
	return request;
}

void testParseHttpRequest(std::vector<serverConfig>& servers) {


	// std::string httpRequest = //one for delete
	// "DELETE /users/123 HTTP/1.1\r\n"
	// "Host: example.com\r\n"
	// "User-Agent: MyClient/1.0\r\n"
	// "\r\n";

	// std::string httpRequest = //one for get
	// "GET /users/123 HTTP/1.1\r\n"
	// "Host: example.com\r\n"
	// "User-Agent: MyClient/1.0\r\n"
	// "Accept: application/json\r\n"
	// "\r\n";

	// std::string httpRequest = //chunked complex normal
	// 	"POST /api/data HTTP/1.1\r\n"
	// 	"Host: example.com\r\n"
	// 	"User-Agent: ChunkedClient/1.0\r\n"
	// 	"Content-Type: application/json\r\n"
	// 	"Transfer-Encoding: chunked\r\n"
	// 	"\r\n"
	// 	"4\r\n"
	// 	"{\"na\r\n"
	// 	"6\r\n"
	// 	"me\": \"\r\n"
	// 	"6\r\n"
	// 	"John D\r\n"
	// 	"4\r\n"
	// 	"oe\",\r\n"
	// 	"3\r\n"
	// 	"\"em\r\n"
	// 	"5\r\n"
	// 	"ail\":\r\n"
	// 	"5\r\n"
	// 	"\"john\r\n"
	// 	"6\r\n"
	// 	".doe@r\n"
	// 	"7\r\n"
	// 	"example\r\n"
	// 	"6\r\n"
	// 	".com\",\r\n"
	// 	"3\r\n"
	// 	"\"ag\r\n"
	// 	"6\r\n"
	// 	"e\": 3}\r\n"
	// 	"0\r\n"
	// 	"\r\n";

	std::string httpRequest = //chunked complex just \n
		"POST /api/data HTTP/1.1\n"
		"Host: example.com\n"
		"User-Agent: ChunkedClient/1.0\n"
		"Content-Type: application/json\n"
		"Transfer-Encoding: chunked\n"
		"\n"
		"4\n"
		"{\"na\n"
		"6\n"
		"me\": \"\n"
		"6\n"
		"John D\n"
		"4\n"
		"oe\",\n"
		"3\n"
		"\"em\n"
		"5\n"
		"ail\":\n"
		"5\n"
		"\"john\n"
		"6\n"
		".doe@r\n"
		"7\n"
		"example\n"
		"6\n"
		".com\",\n"
		"3\n"
		"\"ag\n"
		"6\n"
		"e\": 3}\n"
		"0\n"
		"\n";

	// std::string httpRequest = // just \n instead of \r\n test
	// 	"POST /api/data HTTP/1.1\n"
	// 	"Host: api.example.com\n"
	// 	"User-Agent: MyTestClient/2.0\n"
	// 	"cOnTEnt-type: application/json\n"
	// 	"Accept: application/json\n"
	// 	"Authorization: Bearer abcdef123456\n"
	// 	"cOnTEnt-lEngtH: 53\n"
	// 	"\n"
	// 	"{\"name\": \"John Doe\", \"email\": \"john.doe@example.com\"}";

	// std::string httpRequest = //content-length
	// 	"POST /api/data HTTP/1.1\r\n"
	// 	"Host: api.example.com\r\n"
	// 	"User-Agent: MyTestClient/2.0\r\n"
	// 	"cOnTEnt-type: application/json\r\n"
	// 	"Accept: application/json\r\n"
	// 	"Authorization: Bearer abcdef123456\r\n"
	// 	"cOnTEnt-lEngtH: 53\r\n"
	// 	"\r\n"
	// 	"{\"name\": \"John Doe\", \"email\": \"john.doe@example.com\"}";

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
	HttpRequest request = parseHttpRequest(sv[0], servers);

	if (!request.errorCodes.empty()) {
		std::cout << " Error Codes:\n";
		for (const auto &errorCode : request.errorCodes) {
			std::cout << "  " << errorCode.first << " -> " << errorCode.second << "\n";
		}
	}

	// Output parsed results.
	std::cout << "\npoll_fd: " << request.poll_fd.fd << "\n";
	std::cout << "\nMethod: " << request.method << "\n";
	std::cout << "Path: " << request.path << "\n";
	std::cout << "HTTP Version: " << request.httpVersion << "\n";
	for (const auto& header : request.headers) {
		std::cout << header.first << ": " << header.second << "\n";
	}
	if (!request.body.empty()) {
		std::cout << "Body: " << request.body << "\n";
	}
		
	close(sv[0]);
	close(sv[1]);
}
