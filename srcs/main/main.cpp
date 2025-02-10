#include "../../includes/webserv.hpp"

std::string getConfigPath(int argc, char **argv)
{
	std::string	path = "./webserv.conf"; // define a default path

	if (argc > 1)
		path = argv[1];

	return path;
}

int	main(int argc, char **argv)
{
	ConfigParser	parser;
	Sockets			server_sockets;
	Poller			poller;
	Connection		connection;

	try {
		std::string	config_path = getConfigPath(argc, argv);
		//parser.tester(config_path); //for printing (has dup check call inside)
		parser.parseConfigFile(config_path); //no prints, needs dup check called after
		parser.checkDuplicateServer();

		server_sockets.initSockets(parser.servers);
		poller.initPoll(server_sockets.server_fds);

		for (;;) {
			poller.processPoll();

			// check which fds in poll_fds are ready
			for (int i = 0; i < poller.nfds; ++i) {

				// POLLERR, POLLHUP, POLLNVAL

				if (!(poller.poll_fds[i].revents & POLLIN)) continue;

				if (connection.isServerFd(poller.poll_fds[i].fd,
						server_sockets.server_fds)) {
					connection.handleServerFd(poller.poll_fds[i].fd, poller);
				}
				else {
					//read data from the client socket (poller.poll_fds[i].fd)
					//alalize request
					//form response
					//send response
					connection.handleClientFd(poller.poll_fds[i]); // test for printing the request data
				}
			}
		}

	} catch (const std::exception& e) {
		std::cerr << "caught error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
