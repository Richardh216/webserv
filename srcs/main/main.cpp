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

	webserv.config_path = getConfigPath(argc, argv);
	initStatusCodeInfo(webserv.status_code_info);
	initContentTypes(webserv.content_types);

	try {
		// parser.tester(webserv.config_path); //for printing (has checking call inside)
		parser.parseConfigFile(webserv.config_path); //no prints, needs checking called after
		parser.checkingFunction();

		testParseHttpRequest(parser.servers); //prints the tests for HTTP parsing, needs parser.servers too

		server_sockets.initSockets(parser.servers);
		poller.initPoll(server_sockets.server_fds);

		int curr_nfds;
		for (;;) {
			curr_nfds = poller.nfds;
			poller.processPoll(curr_nfds);

			// check which fds in poll_fds are ready
			for (int i = 0; i < curr_nfds; ++i) {

				bool is_server = connection.isServerFd(poller.poll_fds[i].fd,
					server_sockets.server_fds);

				if (poller.skipFd(is_server, i, curr_nfds)) continue;

				if (is_server) {
					connection.handleServerFd(poller.poll_fds[i].fd, poller);
				}
				else {
					Response response;
					HttpRequest request = parseHttpRequest(poller.poll_fds[i].fd, parser.servers); //now needs parser.servers to work
					if (request.isValid) {
						printRequest(request);
						response.chooseServer(poller.poll_fds[i].fd, request, parser.servers);
						response.formResponse(request, webserv);
						response.sendResponse(poller.poll_fds[i].fd);
					}
					poller.removeFd(i, curr_nfds);
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
