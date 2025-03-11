#include "../../includes/webserv.hpp"

void printRequest(HttpRequest& request)
{
	std::ofstream outFile("request.txt", std::ios::app);
	if (!outFile) {
		return;
	}

	outFile << "--------------------\n";
	outFile << "poll_fd: " << request.poll_fd.fd << "\n";
	outFile << "Method: " << request.method << "\n";
	outFile << "Path: " << request.path << "\n";
	outFile << "HTTP Version: " << request.httpVersion << "\n";

	for (const auto& header : request.headers) {
		outFile << header.first << ": " << header.second << "\n";
	}

	outFile << "Body: " << request.body << "\n";
	outFile << "--------------------\n\n";

	outFile.close();
}

int	main(int argc, char **argv)
{
	Webserv			webserv;
	ConfigParser	parser;
	Sockets			server_sockets;
	Poller			poller;
	Connection		connection;
	std::unordered_map<int, connectionState>	connectionStates; //fd is the key

	webserv.config_path = getConfigPath(argc, argv);
	initStatusCodeInfo(webserv.status_code_info);
	initContentTypes(webserv.content_types);

	try {
		// parser.tester(webserv.config_path); //for printing (has checking call inside)
		parser.parseConfigFile(webserv.config_path); //no prints, needs checking called after
		parser.checkingFunction();

		// testParseHttpRequest(parser.servers); //prints the tests for HTTP parsing, needs parser.servers too

		server_sockets.initSockets(parser.servers);
		poller.initPoll(server_sockets.server_fds);

		int curr_nfds;
		for (;;) {
			curr_nfds = poller.nfds;
			timeOutCheck(curr_nfds, connectionStates, poller); //removes fd and connectionState rn
			poller.processPoll(curr_nfds);

			// check which fds in poll_fds are ready
			for (int i = 0; i < curr_nfds; ++i) {
				int fd = poller.poll_fds[i].fd;
				bool is_server = connection.isServerFd(fd, server_sockets.server_fds);

				if (poller.isFdBad(i)) {
					poller.removeFd(i, curr_nfds);
					continue;
				}

				if (is_server && poller.isFdReadable(i)) {
					connection.handleServerFd(fd, poller);
				}
				//fd ready for reading
				if (!is_server && poller.isFdReadable(i)) {
					if (connectionStates.find(fd) == connectionStates.end()) {
						connectionStates[fd] = {fd, "", std::chrono::steady_clock::now(), false};
					}
					connectionState &state = connectionStates[fd];
					//maybe implement this in a seperat "read" function
					char	tmpBuf[4096];
					ssize_t	bytesRead = recv(fd, tmpBuf, sizeof(tmpBuf), 0);
					if (bytesRead > 0) {
						state.buffer.append(tmpBuf, bytesRead);
						state.lastActivity = std::chrono::steady_clock::now();
					} else if (bytesRead == 0) { //client closed connection
						state.buffer.clear(); //clear buffer
						close(fd); //remove it
						connectionStates.erase(fd);
						poller.removeFd(i, curr_nfds); // I close fd also here - is this a problem ?
						continue;
					} else { //for non-blocking mode, if no data is available bytesRead could be -1
						//we simply skip fd until the next poll
						state.lastActivity = std::chrono::steady_clock::now(); //even if recv fails
						state.isPending = true; //optional
						continue;
					}
					//try to parse HTTP request from the accumulated buffer
					HttpRequest	request = parseHttpRequestFromBuffer(state.buffer, fd, parser.servers); //new way
					// if (request.isValid) { //old way to check
					if (request.isComplete) {
						state.buffer.clear(); //clear buffer
						Response *response = new Response();
						webserv.responses.push_back(response);
						response->setFd(fd);
						printRequest(request);
						response->chooseServer(request, parser.servers);
						response->formResponse(request, webserv);
						if (response->getIsFormed()) { //only enable writing if response is fully prepared
							poller.addWriteEvent(i); //moved from after parsing, not every request could be fully parsed
						}
					}
					//else just continue
					//request still incomplete, state remains in connectionStates
				}

				if (!is_server && poller.isFdWriteable(i)) {
					Response *response = nullptr;
					auto it = webserv.responses.begin();
					for (; it != webserv.responses.end(); ++it) {
						if ((*it)->getFd() == poller.poll_fds[i].fd) {
							response = *it;
							break;
						}
					}
					bool is_sent = false;
					if (response && response->getIsFormed()) {
						is_sent = response->sendResponse();
						if (is_sent) {
							delete response;
							webserv.responses.erase(it);
							poller.removeFd(i, curr_nfds);
							poller.removeWriteEvent(i);
						}
					}
				}

			}
			poller.compressFdArr();
		}

	} catch (const std::exception& e) {
		std::cerr << "caught error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
