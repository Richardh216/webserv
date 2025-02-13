#include "../../includes/webserv.hpp"

std::string getConfigPath(int argc, char **argv)
{
	std::string	path = "./webserv.conf"; // define a default path

	if (argc > 1)
		path = argv[1];

	return path;
}

void	initStatusCodeInfo(std::map<int, std::string>& inf)
{
	inf[200] = "OK";

	inf[301] = "Moved Permanently";
	inf[302] = "Found";

	inf[400] = "Bad Request";
	inf[401] = "Unauthorized";
	inf[403] = "Forbidden";
	inf[404] = "Not Found";
	inf[405] = "Method Not Allowed";
	inf[408] = "Request Timeout";
	inf[413] = "Content Too Large";

	inf[500] = "Internal Server Error";
	inf[503] = "Service Unavailable";
	inf[505] = "HTTP Version Not Supported";
}
