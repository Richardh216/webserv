#include "../../includes/webserv.hpp"

int	main(int argc, char **argv)
{
	ConfigParser	parser;

	try {
		//parser.tester(argv[1]); //for printing (has dup check call inside)
		parser.parseConfigFile(argv[1]); //no prints, needs dup check called after
		parser.checkDuplicateServer();
	} catch (const std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	(void)argc;

	try {
		setupSockets(parser.servers);
	}
	catch (const std::exception& e) {
		std::cerr << "a caught error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
