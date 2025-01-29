#include "../../includes/webserv.hpp"

/**
 * create an array of 'Server' objects for each
 * server from configuration file
 */

int	main(int argc, char **argv)
{
	//std::vector<Server>	servers;

	ConfigParser	parser;
	parser.tester(argv[1]);
	(void)argc;
	//(void)argv;

	return 0;
}
