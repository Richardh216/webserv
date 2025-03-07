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
					/** loop for pending requests, sockets always ready for reading, 
					 * if it matches a socket with a pendig request
					 * then call the parser again with the current state of the request, + the vector of pending lists (may not be needed if the fd it gets caled with is the one we need)
					 * if fd is inside penidng reqeusts, pick up from where it left off
					 * create timeout timer when returning
					 * 
					 * 
					 * 
					 * std::vector<HttpRequest*> pendingRequests;
					 * 
					 * int fd = poller.poll_fds[i].fd; //assume ready for reading, clients wants to send more data
					 * 
					 * find a request with the 'fd' that is now ready to recieve data
					 * 		if there is no such request just treat it as a new request, and call the parser normally to handle the fd that is now ready to read:  
					 			HttpRequest request = parseHttpRequest(poller.poll_fds[i].fd, parser.servers);
					 * 		else, if the list of pendingRequests has the fd that is now ready, send the current data we have a bout the request and call the parser again
					 * 			curr_request = found request
					 * 		else 
					 * 			timeout maybe?
					 * 
					 * 
					 * 		
					 */
					HttpRequest request = parseHttpRequest(poller.poll_fds[i].fd, parser.servers); //now needs parser.servers to work
					// if (request.isPending)// add to vec if pending request
					if (request.isValid) {
						printRequest(request);
						response.chooseServer(poller.poll_fds[i].fd, request, parser.servers);
						response.formResponse(request, webserv);
						response.sendResponse(poller.poll_fds[i].fd);
					}
					poller.removeFd(i, curr_nfds); //remove request from vec of requests too if valid
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
