#include "../../includes/webserv.hpp"

/*
TODO

1 .check error handling when parsing client_max_body_size

2. split up the function into smaller chunks

3. test more with partial requests

seems to work with requests recieved in full, need to check partial requests and it needs to be implemented

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

void timeOutCheck(int curr_nfds, std::unordered_map<int, connectionState>& connectionStates, Poller& poller) {
	const int TIMEOUT_SECONDS = 10;
	auto now = std::chrono::steady_clock::now();

	for (auto it = connectionStates.begin(); it != connectionStates.end(); ) {
		std::chrono::duration<double> elapsed = now - it->second.lastActivity;
		if (elapsed.count() > TIMEOUT_SECONDS) {
			int fd = it->first;
			std::cout << "Timeout: Closing connection for fd: " << fd << std::endl; //testing
			close(fd);
			
			// Find the correct index in poll_fds
			for (int i = 0; i < curr_nfds; ++i) {
				if (poller.poll_fds[i].fd == fd) {
					poller.removeFd(i, curr_nfds);
					break;
				}
			}

			it = connectionStates.erase(it); //remove in case of a timeout
		} else {
			++it;
		}
	}
}

HttpRequest	parseHttpRequestFromBuffer(std::string &buffer, int fd, std::vector<serverConfig>& servers) {
	HttpRequest	request;
	request.poll_fd.fd = fd;
	request.isComplete = false;
	size_t	delimLen;
	size_t	bodySep;

	//Look for the end of headers delim
	size_t	headerEnd = buffer.find("\r\n\r\n");
	bool	hasCarriageReturn = (buffer.find("\r\n") != std::string::npos);
	if (headerEnd == std::string::npos) {
		headerEnd = buffer.find("\n\n");
	}
	if (headerEnd == std::string::npos) {
		return request; //headers not complete
	}
	//Correctly identify the delimiters (might not be error-prone with: "remove trailing '\r")?
	bodySep = hasCarriageReturn ? 4 : 2; //double check
	delimLen = hasCarriageReturn ? 2 : 1;

	//create a stream from the headers
	std::istringstream headerStream(buffer.substr(0, headerEnd + bodySep));
	std::string	line;

	//parse request line
	/* We know this is wrong because it moved on from headerEnd and found the body seperator
	A valid HTTP request should always start with a non-empty request line containing
	the method, URI, and HTTP version
	*/
	if (!std::getline(headerStream, line) || line.empty()) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		request.isComplete = true; //consider it complete so connection can be closed
	}
	//remove trailin '\r' if it exists, check if uniform delimlen is still needed
	// if (!line.empty() || line.back() == '\r') {
	// 	line.pop_back();
	// }

	//split the request line into method, path ...
	std::istringstream	lineStream(line);
	if (!(lineStream >> request.method >> request.path >> request.httpVersion)) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		request.isComplete = true;
		return request;
	}

	//check HTTP method, only these 3 are valid for this project
	static const std::set<std::string>	validMethods = {"GET", "POST", "DELETE"};
	if (validMethods.find(request.method) == validMethods.end()) {
		request.isValid = false;
		request.errorCodes[405] = "Method Not Allowed";
		request.isComplete = true;
		return request;
	}

	//check HTTP version format
	if (!std::regex_match(request.httpVersion, std::regex(R"(HTTP\/\d\.\d)"))) {
		request.isValid = false;
		request.errorCodes[505] = "HTTP Version Not Supported";
		request.isComplete = true;
		return request;
	}

	//Parse Headers
	while (std::getline(headerStream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back(); //remove '\r' if present
		}
		if (line.empty()) {
			break;	//stop when an actual empty line is found
		}
		size_t	colonPos = line.find(':'); //find colon pos in each header, if not present bad request
		if (colonPos == std::string::npos) {
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			request.isComplete = true;
			return request;
		}
		//extract header key and value
		std::string	key = line.substr(0, colonPos);
		std::string	val = line.substr(colonPos + 1);
		//trim whitespaces if needed
		if (!key.empty()) {
			key.erase(0, key.find_first_not_of(" \t")); //leading whitespaces
			key.erase(key.find_last_not_of(" \t") + 1); //trailing whitespaces
		}
		if (!val.empty()) {
			val.erase(0, val.find_first_not_of(" \t"));
			val.erase(val.find_last_not_of(" \t") + 1);
		}
		//Normalize header keys
		key[0] = std::toupper(key[0]);
		size_t	j = 0;
		for (size_t i = 1; i < key.size() - 1; i++) {
			if (key[i] == '-' && i + 1 < key.size()) {
				key[i + 1] = std::toupper(key[i + 1]);
				j = i + 1;
			} else if (j != i) {
				key[i] = std::tolower(key[i]);
			}
		}
		//handle last char
		if (key.size() > 1 && j != key.size() - 1) {
			key[key.size() - 1] = std::tolower(key[key.size() - 1]);
		}
		//assign header key to val
		request.headers[key] = val;
	}
	//Headers Parsed
	request.headersParsed = true;

	//check for expected body
	bool	hasContentLength = request.headers.find("Content-Length") != request.headers.end();
	bool	hasChunkedEncoding = request.headers.find("Transfer-Encoding") != request.headers.end() &&
								request.headers["Transfer-Encoding"] == "chunked";
	//if both are provided that's an error
	if (hasContentLength && hasChunkedEncoding) {
		request.isValid = false;
		request.errorCodes[400] = "Bad Request";
		request.isComplete = true;
		return request;
	}
	//check Host Header
	std::string	hostVal;
	auto it = request.headers.find("Host");
	if (it != request.headers.end()) {
		hostVal = it->second;
	} else { //missing host header
		request.errorCodes[400] = "Bad Request";
	}

	//Get Config Data
	const size_t	DEFAULT_MAX_BODY_SIZE = 1048576; // 1MB default
	serverConfig	currentServer = selectServer(fd, servers, hostVal);
	size_t			BUFFER_SIZE;
	//check max body size
	if (currentServer.client_max_body_size == 0 && (hasContentLength || hasChunkedEncoding)) {
		request.isValid = false;
		request.errorCodes[413] = "Content Too Large";
		request.isComplete = true;
		return request;
	} else if (currentServer.client_max_body_size == -1) {
		BUFFER_SIZE = DEFAULT_MAX_BODY_SIZE; //idk if this is how it should be, but it is how it is
	} else {
		BUFFER_SIZE = currentServer.client_max_body_size;
	}

	//Handle Body based on Content-Length
	if (hasContentLength) {
		size_t	contentLen = 0;
		try {
			contentLen = std::stoull(request.headers.at("Content-Length"));
			if (contentLen == 0) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				request.isComplete = true;
				return request;
			}
			if (contentLen > BUFFER_SIZE) { //would throw error above 1MB default value, can be tweaked easily if needed
				request.isValid = false;
				request.errorCodes[413] = "Content Too Large";
				request.isComplete = true;
				return request;
			}
		} catch (const std::invalid_argument &e) { //for stoull()
			request.isValid = false;
			request.errorCodes[400] = "Bad Request";
			request.isComplete = true;
			return request;
		} catch (const std::out_of_range &e) { //for stoull()
			request.isValid = false;
			request.errorCodes[400];
			request.isComplete = true;
			return request;
		}
		//Get the start of the body
		size_t	bodyStart = headerEnd + bodySep;
		// size_t bodyStart = headerEnd + (buffer.compare(headerEnd, 4, "\r\n\r\n") == 0 ? 4 : 2); for checks
		// std::cout << "BODY START: " << bodyStart << std::endl;

		//Check for full body
		if (buffer.size() < bodyStart + contentLen) {
			return request; //not enough data yet, wait for more
		}
		//Extract the body if it's all there
		request.body = buffer.substr(bodyStart, contentLen);
		//Request is complete
		buffer.erase(0, bodyStart + contentLen);
		request.isComplete = true;
		request.isValid = true;
	}

	//Handle Chunked Transfer Encoding
	else if (hasChunkedEncoding) {
		std::string	chunkedBody;
		size_t	pos = headerEnd + bodySep; //double check if bodySep is correct
		size_t	totalBodySize = 0;
		// size_t	originalPos = pos;
		bool	lastChunk = false;

		while (true) {
			//Find the CRLF that ends the chunk size line
			size_t	lineEnd = buffer.find("\r\n", pos);
			if (lineEnd == std::string::npos) {
				lineEnd = buffer.find("\n", pos);
			}
			if (lineEnd == std::string::npos) {
				return request; //need more data for chunkSize
			}
			//Skip stray newlines before parsing chunkSize
			while (pos < buffer.size() && (buffer.compare(pos, 2, "\r\n") == 0 || buffer[pos] == '\n')) {
				pos += delimLen; //double check
			}
			//End of the chunk size line should be after current pos
			if (lineEnd <= pos) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				request.isComplete = true;
				return request;
			}
			//Extract chunkSize as a string from the buffer
			std::string	chunkSizeStr = buffer.substr(pos, lineEnd - pos);
			size_t	semicolonPos = chunkSizeStr.find(';');
			if (semicolonPos != std::string::npos) {
				chunkSizeStr = chunkSizeStr.substr(0, semicolonPos);
			}
			//Trim it
			if (!chunkSizeStr.empty()) {
				chunkSizeStr.erase(0, chunkSizeStr.find_first_not_of(" \t"));
				chunkSizeStr.erase(chunkSizeStr.find_last_not_of(" \t") + 1);
			}
			//Extract chunkSize in Hexadecimal
			size_t	chunkSize;
			try {
				chunkSize = std::stoul(chunkSizeStr, nullptr, 16);
			} catch (const std::exception &e) {
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				request.isComplete = true;
				return request;
			}
			if (chunkSize == 0) {
				lastChunk = true;
				size_t	endPos = lineEnd + delimLen;
				//Look for final CRLF
				size_t	finalEnd = buffer.find("\r\n", endPos);
				if (finalEnd == std::string::npos) {
					finalEnd = buffer.find("\n", endPos);
				}
				if (finalEnd == std::string::npos) {
					return request; //need more data for the final CRLF
				}
				//Update pos to after final CRLF
				pos = finalEnd + delimLen; //double check
				break;
			}
			if (totalBodySize + chunkSize > BUFFER_SIZE) {
				request.isValid = false;
				request.errorCodes[413] = "Content Too Large";
				request.isComplete = true;
				return request;
			}
			size_t	chunkDataStart = lineEnd + delimLen;
			//Make sure we have the entire chunk + the CRLF
			if (buffer.size() < chunkDataStart + chunkSize + delimLen) {
				return request; //need more data
			}
			//Append chunk data to chunk body
			chunkedBody.append(buffer.substr(chunkDataStart, chunkSize));
			totalBodySize += chunkSize; //keep track of body size
			//Move past chunk data
			pos = chunkDataStart + chunkSize;
			//Verify and skip CRLF
			if (buffer.compare(pos, 2, "\r\n") == 0) {
				pos += 2;
			} else if (buffer[pos] == '\n') {
				pos += 1;
			} else {
				//Invalid chunk format, missing trailing CRLF
				request.isValid = false;
				request.errorCodes[400] = "Bad Request";
				request.isComplete = true;
				return request;
			}
		}
		if (lastChunk) {
			request.body = chunkedBody;
			buffer.erase(0, pos);
			request.isValid = true;
			request.isComplete = true;
		}
	}
	//Handle case with no body, or for GET/DELETE no Content Length or Chunked Encoding
	else {
		if (request.method == "POST") {
			if (request.httpVersion == "HTTP/1.1") {
				request.isValid = false;
				request.errorCodes[411] = "Length Required";
				request.isComplete = true;
				return request;
			}
		}
		//For GET/Delete with no content info or HTTP/1.0 POST, just get headers
		buffer.erase(0, headerEnd + bodySep); //double check bodySep, might be worth to check every time if it exists
		request.isValid = true;
		request.isComplete = true;
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
	// HttpRequest request = parseHttpRequest(sv[0], servers); //old

	HttpRequest request = parseHttpRequestFromBuffer(httpRequest, sv[0], servers); //new

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
