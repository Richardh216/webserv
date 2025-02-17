#include "../../includes/webserv.hpp"

HttpRequest	parseHttpRequest(int clientFd) {
	constexpr size_t	BUFFER_SIZE = 4096;
	std::vector<char>	buffer(BUFFER_SIZE);
	std::string			rawRequest;
	ssize_t				bytesRead;

	//Reading request data
	while (true) {
		bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
		if (bytesRead < 0) {
			HttpRequest request;
			request.isValid = false;
			request.errorMessage = "Error recieving data.";
			return request;
		} else if (bytesRead == 0) {
			break; //the client closed the connection
		}
		rawRequest.append(buffer.data(), bytesRead); // buffer.data() returns a pointer to its first element
		// If headers are complete and this is not a chunked request, break early.
		// if (rawRequest.find("\r\n\r\n") != std::string::npos &&
		// 	rawRequest.find("Transfer-Encoding: chunked") == std::string::npos) {
		// 		break; // End of headers
		// }
	}

	// if no data was read, return invalid request
	if (rawRequest.empty()) {
		HttpRequest request;
		request.isValid = false;
		request.errorMessage = "Empty request.";
		return request;
	}

	//start parsing from rawReqeust
	std::istringstream	stream(rawRequest);
	HttpRequest			request;
	std::string			line;

	// Read and parse the request line
	if (!std::getline(stream, line) || line.empty()) {
		request.isValid = false;
		request.errorMessage = "Invalid or empty request line.";
		return request;
	}

	std::istringstream	lineStream(line); //split the request line into method, path ...
	if (!(lineStream >> request.method >> request.path >> request.httpVersion)) {
		request.isValid = false;
		request.errorMessage = "Invalid request line.";
		return request;
	}

	//check http method
	static const std::set<std::string>	validMethods = {"GET", "POST", "DELETE"}; //only these methods are allowed
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
	while (std::getline(stream, line) && line != "\r") { //Reads each line until "\r", which is the end of headers
		size_t	colonPos = line.find(':'); //Finds the ":" separator in each header
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
		int	contentLen = std::stoi(request.headers.at("Content-Length")); //converts it to int
		if (contentLen < 0) {
			request.isValid = false;
			request.errorMessage = "Invalid Content-Length value.";
			return request;
		}

		 // If the entire body wasn't already received with the headers,
		// continue reading from the socket.
		while (rawRequest.size() < rawRequest.find("\r\n\r\n") + 4 + static_cast<size_t>(contentLen)) {
			bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
			if (bytesRead <= 0) {
				request.isValid = false;
				request.errorMessage = "Incomplete request body.";
				return request;
			}
			rawRequest.append(buffer.data(), bytesRead);
		}
		size_t	bodyStart = rawRequest.find("\r\n\r\n") + 4; //start of body
		request.body = rawRequest.substr(bodyStart, contentLen); //Extracts the body from rawRequest
	}

	// Handle chunked transfer encoding
	else if (hasChunkedEncoding) {
		std::string	chunkedBody;
		size_t	pos = rawRequest.find("\r\n\r\n"); //find where body begins
		if (pos == std::string::npos) {
			request.isValid = false;
			request.errorMessage = "Headers not complete.";
			return request;
		}
		pos += 4; //skip the header delim

		// Skip any extra CRLFs that may be present after the header delimiter.
		while (rawRequest.compare(pos, 2, "\r\n") == 0) {
			pos += 2;
		}

		while (true) { //breaks when 0\r\n is found
			// Find the CRLF that ends the chunk size line.
			size_t	lineEnd = rawRequest.find("\r\n", pos); //search for \r\n starting from pos
			while (lineEnd == std::string::npos) { //if \r\n is not found, line is incomplete, need more data
				bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
				if (bytesRead <= 0) {
					request.isValid = false;
					request.errorMessage = "Incomplete chunked transfer encoding: missing chunk size line.";
					return request;
				}
				rawRequest.append(buffer.data(), bytesRead);
				lineEnd = rawRequest.find("\r\n", pos); //re-check if \r\n is now available
			}
			//Extract the chunk size (in hexadecimal)
			std::string chunkSizeStr = rawRequest.substr(pos, lineEnd - pos); //extracts chunkSize as a string
			int	chunkSize;
			std::istringstream	iss(chunkSizeStr);
			iss >> std::hex >> chunkSize; //converts hex chunkSize into int
			if (chunkSize == 0) { //the last chunk was received, end of the loop
				break;
			}

			size_t chunkDataStart = lineEnd + 2; //Move past the chunk size line (and its CRLF)
			// Ensure the entire chunk and its trailing CRLF are available
			while (rawRequest.size() < chunkDataStart + chunkSize + 2) { //If rawRequest is too short, call recv() to read more data from the socket
				bytesRead = recv(clientFd, buffer.data(), BUFFER_SIZE, 0);
				if (bytesRead <= 0) {
					request.isValid = false;
					request.errorMessage = "Incomplete chunked transfer encoding: chunk data missing.";
					return request;
				}
				rawRequest.append(buffer.data(), bytesRead);
			}
			chunkedBody.append(rawRequest.substr(chunkDataStart, chunkSize)); // Append the chunk data to the chunkBody
			pos = chunkDataStart + chunkSize + 2; // Advance the pointer past the chunk data and the trailing CRLF
		}
		request.body = chunkedBody; //store full body
	}
	return request;
}

void testParseHttpRequest(void) {

	// std::string httpRequest = //chunked transfer encoding
	// 	"POST /api/data HTTP/1.1\r\n"
	// 	"Host: api.example.com\r\n"
	// 	"User-Agent: MyTestClient/2.0\r\n"
	// 	"Content-Type: application/json\r\n"
	// 	"Accept: application/json\r\n"
	// 	"Transfer-Encoding: chunked\r\n"
	// 	"\r\n"
	// 	"4\r\n"
	// 	"Wiki\r\n"
	// 	"5\r\n"
	// 	"pedia\r\n"
	// 	"0\r\n"
	// 	"\r\n";

		std::string httpRequest = //content-length
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
