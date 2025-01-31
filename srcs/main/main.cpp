#include "../../includes/webserv.hpp"

int	main(int argc, char **argv)
{
	ConfigParser	parser;

	try {
		parser.tester(argv[1]); //for printing (has dup check call inside)
		// parser.parseConfigFile(argv[1]); //no prints, needs dup check called after
		// parser.checkDuplicateServer();
	} catch (const std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	(void)argc;

	//setupSockets(parser.servers);

	return 0;
}
