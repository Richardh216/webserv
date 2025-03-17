#include "../../includes/webserv.hpp"

int	main(int argc, char **argv)
{
	Webserv			webserv;
	ConfigParser	parser;
	Sockets			server_sockets;
	Poller			poller;
	Connection		connection;

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
					if (parser.connectionStates.find(fd) == parser.connectionStates.end()) {
						parser.connectionStates[fd] = {fd, "", std::chrono::steady_clock::now(), false};
					}
					connectionState &state = parser.connectionStates[fd];
					int res = readFromFd(fd, parser, state);
					if (res == 0) {
						poller.removeFd(i, curr_nfds);
						continue;
					}
					else if (res == -1) continue;

					//try to parse HTTP request from the accumulated buffer
					HttpRequest	request = parseHttpRequestFromBuffer(state.buffer, fd, parser.servers);
					if (request.isComplete) {
						state.buffer.clear(); //clear buffer
						Response *response = new Response();
						webserv.responses.push_back(response);
						response->setFd(fd);
						//outputRequestToFile(request, "request.txt");
						response->chooseServer(request, parser.servers);
						response->formResponse(request, webserv);
						if (response->getIsFormed()) { //only enable writing if response is fully prepared
							poller.addWriteEvent(i); //moved from after parsing, not every request could be fully parsed
							poller.removeReadEvent(i);
						}
					}
					else continue; //request still incomplete, state remains in connectionStates
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
			timeOutCheck(curr_nfds, parser.connectionStates, poller); //removes fd and connectionState rn
			poller.compressFdArr();
		}

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
